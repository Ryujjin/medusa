/*
	PrimitiveGalaxySpriteSetD3D.cpp
	D3D11 implementation — renders survey galaxy sprites from a 7×6 atlas.

	Atlas file: Bin/Textures/galaxy_sprites.png (non-fatal if absent).
	Shader:     VS_GalaxySprite + PS_GalaxySprite (Default_D3D11.fx).
	Atlas slot: t6  (alongside t5 = milkyway_panorama).

	(c)2025 DarkSpace DX11 Port
*/

#include <d3d11.h>
#include <wincodec.h>
#include <vector>
#pragma comment(lib, "WindowsCodecs.lib")
#include "DisplayD3D/DisplayDeviceD3D.h"
#include "DisplayD3D/PrimitiveGalaxySpriteSetD3D.h"
#include "DisplayD3D/PrimitiveFactory.h"
#include "Debug/Trace.h"

//----------------------------------------------------------------------------

IMPLEMENT_PRIMITIVE_FACTORY( PrimitiveGalaxySpriteSetD3D );

// Static atlas SRV -- loaded once, shared across per-frame instances.
ID3D11ShaderResourceView * PrimitiveGalaxySpriteSetD3D::m_pAtlasSRV = NULL;

PrimitiveGalaxySpriteSetD3D::PrimitiveGalaxySpriteSetD3D()
	: m_pVB(NULL)
{}

bool PrimitiveGalaxySpriteSetD3D::initialize( const SpriteVertex * pVerts, int nVerts )
{
	if (!pVerts || nVerts <= 0)
		return false;
	m_Verts.assign(pVerts, pVerts + nVerts);
	return true;
}

bool PrimitiveGalaxySpriteSetD3D::execute()
{
	DisplayDeviceD3D* pDisplayD3D = (DisplayDeviceD3D*)m_pDevice;
	if (!pDisplayD3D || !pDisplayD3D->getContext())
		return false;
	if (!pDisplayD3D->m_pMatShader.valid())
		return false;
	if (m_Verts.empty())
		return true;

	ID3D11Device*        pDevice  = pDisplayD3D->getDevice();
	ID3D11DeviceContext* pContext = pDisplayD3D->getContext();

	// Lazy-build vertex buffer on first execute.
	if (!m_pVB)
	{
		if (!buildVertexBuffer(pDevice))
			return false;
	}

	// Lazy-load atlas texture.
	if (!m_pAtlasSRV && pDevice)
		loadAtlasTexture(pDevice);

	// Switch to VS_GalaxySprite + PS_GalaxySprite (uses VERTEXL input layout).
	if (!pDisplayD3D->m_pMatShader->applyGalaxySprite(pDisplayD3D))
		return false;

	// Bind atlas to t6.
	pContext->PSSetShaderResources(6, 1, &m_pAtlasSRV);

	// Draw quad list from the pre-built vertex buffer.
	UINT stride = (UINT)sizeof(SpriteVertex);
	UINT offset = 0;
	pContext->IASetVertexBuffers(0, 1, &m_pVB, &stride, &offset);
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pContext->Draw((UINT)m_Verts.size(), 0);

	// Unbind t6.
	ID3D11ShaderResourceView* pNullSRV = NULL;
	pContext->PSSetShaderResources(6, 1, &pNullSRV);

	return true;
}

void PrimitiveGalaxySpriteSetD3D::clear()
{}

void PrimitiveGalaxySpriteSetD3D::release()
{
	if (m_pVB) { m_pVB->Release(); m_pVB = NULL; }
	m_Verts.clear();
	// m_pAtlasSRV is static -- do not release per instance.
}

//----------------------------------------------------------------------------

bool PrimitiveGalaxySpriteSetD3D::buildVertexBuffer( ID3D11Device * pDevice )
{
	if (!pDevice || m_Verts.empty()) return false;

	D3D11_BUFFER_DESC bd = {};
	bd.ByteWidth      = (UINT)(m_Verts.size() * sizeof(SpriteVertex));
	bd.Usage          = D3D11_USAGE_IMMUTABLE;
	bd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA init = {};
	init.pSysMem = m_Verts.data();

	HRESULT hr = pDevice->CreateBuffer(&bd, &init, &m_pVB);
	if (FAILED(hr))
	{
		TRACE("PrimitiveGalaxySpriteSetD3D: CreateBuffer failed.");
		return false;
	}
	return true;
}

bool PrimitiveGalaxySpriteSetD3D::loadAtlasTexture( ID3D11Device * pDevice )
{
	if (!pDevice) return false;

	// Atlas is expected at Textures/galaxy_sprites.png relative to Bin/.
	// Non-fatal if absent — sprites simply render as uniform-white quads.
	static const wchar_t * k_pPath = L"Textures/galaxy_sprites.png";

	CoInitializeEx(NULL, COINIT_MULTITHREADED);

	IWICImagingFactory * pWIC = NULL;
	HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL,
		CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, (void**)&pWIC);
	if (FAILED(hr))
	{
		TRACE("PrimitiveGalaxySpriteSetD3D: WIC unavailable, atlas skipped.");
		return false;
	}

	IWICBitmapDecoder * pDecoder = NULL;
	hr = pWIC->CreateDecoderFromFilename(k_pPath, NULL, GENERIC_READ,
		WICDecodeMetadataCacheOnDemand, &pDecoder);
	if (FAILED(hr))
	{
		TRACE("PrimitiveGalaxySpriteSetD3D: galaxy_sprites.png not found.");
		pWIC->Release();
		return false;
	}

	IWICBitmapFrameDecode * pFrame = NULL;
	hr = pDecoder->GetFrame(0, &pFrame);
	pDecoder->Release();
	if (FAILED(hr)) { pWIC->Release(); return false; }

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

	hr = pDevice->CreateShaderResourceView(pTex, NULL, &m_pAtlasSRV);
	pTex->Release();
	if (SUCCEEDED(hr))
	{
		TRACE("PrimitiveGalaxySpriteSetD3D: galaxy_sprites.png atlas loaded.");
	}
	else
	{
		TRACE("PrimitiveGalaxySpriteSetD3D: CreateShaderResourceView failed for atlas.");
	}
	return SUCCEEDED(hr);
}

//----------------------------------------------------------------------------
// EOF
