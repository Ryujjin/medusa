/*
	PrimitiveGalaxyD3D.cpp
	D3D11 procedural Milky Way band: fullscreen triangle (SV_VertexID) drawn with
	VS_Galaxy + PS_Galaxy.  No vertex buffer required.

	(c)2025 DarkSpace DX11 Port
*/

#include <d3d11.h>
#include <wincodec.h>
#include <vector>
#pragma comment(lib, "WindowsCodecs.lib")
#include "DisplayD3D/DisplayDeviceD3D.h"
#include "DisplayD3D/PrimitiveGalaxyD3D.h"
#include "DisplayD3D/PrimitiveFactory.h"
#include "Debug/Trace.h"

//----------------------------------------------------------------------------

IMPLEMENT_PRIMITIVE_FACTORY( PrimitiveGalaxyD3D );

// Static SRV definitions -- shared across all per-frame instances.
ID3D11ShaderResourceView * PrimitiveGalaxyD3D::m_pMilkyWaySRV = NULL;
ID3D11ShaderResourceView * PrimitiveGalaxyD3D::m_pBumpSRV     = NULL;

PrimitiveGalaxyD3D::PrimitiveGalaxyD3D() :
	m_gnpDir(0, 0, 0),
	m_galCenDir(0, 0, 0)
{}

bool PrimitiveGalaxyD3D::execute()
{
	DisplayDeviceD3D* pDisplayD3D = (DisplayDeviceD3D*)m_pDevice;
	if (!pDisplayD3D || !pDisplayD3D->getContext())
		return false;
	if (!pDisplayD3D->m_pMatShader.valid())
		return false;

	ID3D11DeviceContext* pContext = pDisplayD3D->getContext();

	// Lazy-load the panorama texture the first time we execute.
	if (!m_pMilkyWaySRV && pDisplayD3D->getDevice())
		loadMilkyWayTexture(pDisplayD3D->getDevice());

	// Lazy-load the bump map the first time we execute.
	if (!m_pBumpSRV && pDisplayD3D->getDevice())
		loadMilkyWayBump(pDisplayD3D->getDevice());

	// Push galaxy direction constants into the shared constant map.
	pDisplayD3D->m_pMatShader->setConstant("vGnpDir",    m_gnpDir);
	pDisplayD3D->m_pMatShader->setConstant("vGalCenDir", m_galCenDir);
	pDisplayD3D->m_pMatShader->setConstant("nGalaxyDepthFlags", PrimitiveGalaxy::depthFlags());

	// Switch to VS_Galaxy + PS_Galaxy; re-uploads CB with the new constants.
	if (!pDisplayD3D->m_pMatShader->applyGalaxy(pDisplayD3D))
		return false;

	// Bind Milky Way panorama to t5 for PS_Galaxy (NULL = black if not loaded).
	pContext->PSSetShaderResources(5, 1, &m_pMilkyWaySRV);

	// Bind bump map to t7 for PS_Galaxy (NULL = flat if not loaded).
	pContext->PSSetShaderResources(7, 1, &m_pBumpSRV);

	// No vertex buffer: VS_Galaxy generates the triangle from SV_VertexID.
	ID3D11Buffer* pNullVB = NULL;
	UINT stride = 0, offset = 0;
	pContext->IASetVertexBuffers(0, 1, &pNullVB, &stride, &offset);
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// One triangle covers the entire screen.
	pContext->Draw(3, 0);

	// Unbind t5 and t7 so normal rendering is not affected.
	ID3D11ShaderResourceView* pNullSRV = NULL;
	pContext->PSSetShaderResources(5, 1, &pNullSRV);
	pContext->PSSetShaderResources(7, 1, &pNullSRV);

	return true;
}

void PrimitiveGalaxyD3D::clear()
{}

void PrimitiveGalaxyD3D::release()
{
	// Static SRVs are shared across instances; do not release them here.
	// They are freed when the device is destroyed.
}

bool PrimitiveGalaxyD3D::initialize( const Vector3 & gnpDir, const Vector3 & galCenDir )
{
	m_gnpDir    = gnpDir;
	m_galCenDir = galCenDir;
	return true;
}

//----------------------------------------------------------------------------

bool PrimitiveGalaxyD3D::loadMilkyWayTexture( ID3D11Device * pDevice )
{
	if (!pDevice) return false;

	// Texture is expected at Textures/milkyway_panorama.png relative to the
	// working directory (Bin/).  Non-fatal if absent — galaxy falls back to
	// procedural-only rendering.
	static const wchar_t * k_pPath = L"Textures/milkyway_panorama.png";

	// Initialise COM if not already done (no-op if already initialised).
	CoInitializeEx(NULL, COINIT_MULTITHREADED);

	IWICImagingFactory * pWIC = NULL;
	HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL,
		CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, (void**)&pWIC);
	if (FAILED(hr))
	{
		TRACE("PrimitiveGalaxyD3D: WIC factory unavailable, panorama skipped.");
		return false;
	}

	IWICBitmapDecoder * pDecoder = NULL;
	hr = pWIC->CreateDecoderFromFilename(k_pPath, NULL, GENERIC_READ,
		WICDecodeMetadataCacheOnDemand, &pDecoder);
	if (FAILED(hr))
	{
		TRACE("PrimitiveGalaxyD3D: milkyway_panorama.png not found - procedural galaxy only.");
		pWIC->Release();
		return false;
	}

	IWICBitmapFrameDecode * pFrame = NULL;
	hr = pDecoder->GetFrame(0, &pFrame);
	pDecoder->Release();
	if (FAILED(hr)) { pWIC->Release(); return false; }

	// Convert to 32-bit RGBA regardless of source format.
	IWICFormatConverter * pConv = NULL;
	hr = pWIC->CreateFormatConverter(&pConv);
	if (SUCCEEDED(hr))
		hr = pConv->Initialize(pFrame, GUID_WICPixelFormat32bppRGBA,
			WICBitmapDitherTypeNone, NULL, 0.0f, WICBitmapPaletteTypeCustom);
	pFrame->Release();
	pWIC->Release();
	if (FAILED(hr)) { if (pConv) pConv->Release(); return false; }

	UINT w = 0, h = 0;
	pConv->GetSize(&w, &h);
	if (w == 0 || h == 0) { pConv->Release(); return false; }

	std::vector<BYTE> pixels((size_t)w * h * 4);
	hr = pConv->CopyPixels(NULL, w * 4, (UINT)pixels.size(), pixels.data());
	pConv->Release();
	if (FAILED(hr)) return false;

	// Upload to an immutable GPU texture.
	D3D11_TEXTURE2D_DESC td = {};
	td.Width = w; td.Height = h;
	td.MipLevels = 1; td.ArraySize = 1;
	td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	td.SampleDesc.Count = 1;
	td.Usage = D3D11_USAGE_IMMUTABLE;
	td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA init = {};
	init.pSysMem = pixels.data();
	init.SysMemPitch = w * 4;

	ID3D11Texture2D * pTex = NULL;
	hr = pDevice->CreateTexture2D(&td, &init, &pTex);
	if (FAILED(hr)) return false;

	hr = pDevice->CreateShaderResourceView(pTex, NULL, &m_pMilkyWaySRV);
	pTex->Release();
	if (SUCCEEDED(hr))
	{
		TRACE("PrimitiveGalaxyD3D: Milky Way panorama loaded successfully.");
	}
	else
	{
		TRACE("PrimitiveGalaxyD3D: CreateShaderResourceView failed for panorama.");
	}

	return SUCCEEDED(hr);
}

//----------------------------------------------------------------------------

bool PrimitiveGalaxyD3D::loadMilkyWayBump( ID3D11Device * pDevice )
{
	if (!pDevice) return false;

	static const wchar_t * k_pPath = L"Textures/milkyway_bump.png";

	CoInitializeEx(NULL, COINIT_MULTITHREADED);

	IWICImagingFactory * pWIC = NULL;
	HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL,
		CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, (void**)&pWIC);
	if (FAILED(hr))
	{
		TRACE("PrimitiveGalaxyD3D: WIC unavailable, bump map skipped.");
		return false;
	}

	IWICBitmapDecoder * pDecoder = NULL;
	hr = pWIC->CreateDecoderFromFilename(k_pPath, NULL, GENERIC_READ,
		WICDecodeMetadataCacheOnDemand, &pDecoder);
	if (FAILED(hr))
	{
		TRACE("PrimitiveGalaxyD3D: milkyway_bump.png not found - no bump depth effect.");
		pWIC->Release();
		return false;
	}

	IWICBitmapFrameDecode * pFrame = NULL;
	hr = pDecoder->GetFrame(0, &pFrame);
	pDecoder->Release();
	if (FAILED(hr)) { pWIC->Release(); return false; }

	// Greyscale is fine; convert to 32bpp RGBA for simplicity.
	IWICFormatConverter * pConv = NULL;
	hr = pWIC->CreateFormatConverter(&pConv);
	if (SUCCEEDED(hr))
		hr = pConv->Initialize(pFrame, GUID_WICPixelFormat32bppRGBA,
			WICBitmapDitherTypeNone, NULL, 0.0f, WICBitmapPaletteTypeCustom);
	pFrame->Release();
	pWIC->Release();
	if (FAILED(hr)) { if (pConv) pConv->Release(); return false; }

	UINT w = 0, h = 0;
	pConv->GetSize(&w, &h);
	if (w == 0 || h == 0) { pConv->Release(); return false; }

	std::vector<BYTE> pixels((size_t)w * h * 4);
	hr = pConv->CopyPixels(NULL, w * 4, (UINT)pixels.size(), pixels.data());
	pConv->Release();
	if (FAILED(hr)) return false;

	D3D11_TEXTURE2D_DESC td = {};
	td.Width = w; td.Height = h;
	td.MipLevels = 1; td.ArraySize = 1;
	td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	td.SampleDesc.Count = 1;
	td.Usage = D3D11_USAGE_IMMUTABLE;
	td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA init = {};
	init.pSysMem = pixels.data();
	init.SysMemPitch = w * 4;

	ID3D11Texture2D * pTex = NULL;
	hr = pDevice->CreateTexture2D(&td, &init, &pTex);
	if (FAILED(hr)) return false;

	hr = pDevice->CreateShaderResourceView(pTex, NULL, &m_pBumpSRV);
	pTex->Release();
	if (SUCCEEDED(hr))
	{
		TRACE("PrimitiveGalaxyD3D: Milky Way bump map loaded successfully.");
	}
	else
	{
		TRACE("PrimitiveGalaxyD3D: CreateShaderResourceView failed for bump map.");
	}

	return SUCCEEDED(hr);
}

//----------------------------------------------------------------------------
// EOF
