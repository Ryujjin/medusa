/**
	@file DisplayEffectBlur.cpp

	(c)2009 Palestar Inc
	@author Richard Lyle @date 9/21/2010 9:31:08 PM
	Updated to DirectX 11
*/

#include "Debug/Assert.h"
#include "DisplayEffectBlur.h"
#include "DisplayDeviceD3D.h"

//---------------------------------------------------------------------------------------------------

IMPLEMENT_FACTORY(DisplayEffectBlur, DisplayEffect);
REGISTER_FACTORY_KEY(DisplayEffectBlur, 4270906929918038098LL);

BEGIN_PROPERTY_LIST(DisplayEffectBlur, DisplayEffect)
ADD_PROPERTY(m_fBlur);
END_PROPERTY_LIST();

//---------------------------------------------------------------------------------------------------

DisplayEffectBlur::DisplayEffectBlur() :
	m_fBlur(0.85f),
	m_BufferSize(0, 0),
	m_pBlurRenderTarget(NULL),
	m_pBlurRTV(NULL),
	m_pBlurSRV(NULL),
	m_pPreviousTexture(NULL),
	m_pPreviousRTV(NULL),
	m_pPreviousSRV(NULL),
	m_pRenderTarget(NULL),
	m_pDepthStencil(NULL),
	m_pSavedRasterizerState(NULL),
	m_pSavedBlendState(NULL),
	m_pSavedDepthStencilState(NULL),
	m_nSavedSampleMask(0xFFFFFFFF),
	m_nSavedStencilRef(0),
	m_pQuadVB(NULL),
	m_bBlurInitialized(false),
	m_bBlurFailed(false)
{
	m_SavedBlendFactor[0] = m_SavedBlendFactor[1] = m_SavedBlendFactor[2] = m_SavedBlendFactor[3] = 1.0f;
}

DisplayEffectBlur::~DisplayEffectBlur()
{
	release();
}

//---------------------------------------------------------------------------------------------------

bool DisplayEffectBlur::preRender(DisplayDevice* pDevice)
{
	static int s_nCallCount = 0;
	static int s_nInitCount = 0;
	s_nCallCount++;

	if (m_bBlurFailed)
	{
		RELEASEDD(m_pRenderTarget);
		RELEASEDD(m_pDepthStencil);
		return false;
	}

	DisplayDeviceD3D* pDisplayD3D = (DisplayDeviceD3D*)pDevice;
	if (!pDisplayD3D || !pDisplayD3D->getDevice() || !pDisplayD3D->getContext())
	{
		RELEASEDD(m_pRenderTarget);
		RELEASEDD(m_pDepthStencil);
		return false;
	}

	TRACE("Blur::preRender #%d (Init:%d, Failed:%d, Initialized:%d)", 
		s_nCallCount, s_nInitCount, m_bBlurFailed ? 1 : 0, m_bBlurInitialized ? 1 : 0);

	// Initialize blur if needed
	if (!m_bBlurInitialized)
	{
		if (!initializeBlur(pDevice))
		{
			m_bBlurFailed = true;
			return false;
		}
		m_bBlurInitialized = true;
	}

	// Check if buffer size has changed
	SizeInt currentSize = pDevice->clientWindow().size();
	if (m_BufferSize != currentSize)
	{
		s_nInitCount++;
		TRACE("Blur: Buffer size changed #%d (%dx%d -> %dx%d)", 
			s_nInitCount, m_BufferSize.width, m_BufferSize.height, 
			currentSize.width, currentSize.height);
		release();
		if (!initializeBlur(pDevice))
		{
			m_bBlurFailed = true;
			return false;
		}
	}

	ID3D11DeviceContext* pContext = pDisplayD3D->getContext();

	// Save current render state
	saveRenderState(pDevice);

	// Get current render target
	pContext->OMGetRenderTargets(1, &m_pRenderTarget, &m_pDepthStencil);

	// Set our blur render target
	pContext->OMSetRenderTargets(1, &m_pBlurRTV, m_pDepthStencil);

	return true;
}

bool DisplayEffectBlur::postRender(DisplayDevice* pDevice)
{
	if (m_bBlurFailed)
	{
		// Release acquired render targets before returning
		RELEASEDD(m_pRenderTarget);
		RELEASEDD(m_pDepthStencil);
		return false;
	}

	DisplayDeviceD3D* pDisplayD3D = (DisplayDeviceD3D*)pDevice;
	if (!pDisplayD3D || !pDisplayD3D->getDevice() || !pDisplayD3D->getContext())
	{
		// Release acquired render targets before returning
		RELEASEDD(m_pRenderTarget);
		RELEASEDD(m_pDepthStencil);
		return false;
	}

	ID3D11DeviceContext* pContext = pDisplayD3D->getContext();

	// Now combine our current frame with the previous frame and render to the final target
	if (m_pCombine.valid() && m_pCombine->valid())
	{
		// Set the final render target
		pContext->OMSetRenderTargets(1, &m_pRenderTarget, NULL);

		// Set shaders
		m_pCombine->setConstant("fBlurAmount", m_fBlur);
		m_pCombine->apply(pDisplayD3D);

		// Set textures
		pContext->PSSetShaderResources(0, 1, &m_pBlurSRV);		// Current frame
		pContext->PSSetShaderResources(1, 1, &m_pPreviousSRV);	// Previous frame

		// Set sampler
		ID3D11SamplerState* pSampler = pDisplayD3D->m_pSamplerLinear.Get();
		pContext->PSSetSamplers(0, 1, &pSampler);

		// Render full-screen quad
		renderQuad(pDevice);

		// Unbind textures
		ID3D11ShaderResourceView* nullSRV[2] = { NULL, NULL };
		pContext->PSSetShaderResources(0, 2, nullSRV);
	}

	// Copy current frame to previous frame for next render
	if (m_pScale.valid() && m_pScale->valid())
	{
		// Set previous frame as render target
		pContext->OMSetRenderTargets(1, &m_pPreviousRTV, NULL);

		// Set shader
		m_pScale->apply(pDisplayD3D);

		// Set current frame as texture
		pContext->PSSetShaderResources(0, 1, &m_pBlurSRV);

		// Set sampler
		ID3D11SamplerState* pSampler = pDisplayD3D->m_pSamplerLinear.Get();
		pContext->PSSetSamplers(0, 1, &pSampler);

		// Render full-screen quad
		renderQuad(pDevice);

		// Unbind texture
		ID3D11ShaderResourceView* nullSRV = NULL;
		pContext->PSSetShaderResources(0, 1, &nullSRV);
	}

	// Restore render state
	restoreRenderState(pDevice);

	// Release saved render targets
	RELEASEDD(m_pRenderTarget);
	RELEASEDD(m_pDepthStencil);

	return true;
}

void DisplayEffectBlur::release()
{
	RELEASEDD(m_pQuadVB);

	RELEASEDD(m_pBlurSRV);
	RELEASEDD(m_pBlurRTV);
	RELEASEDD(m_pBlurRenderTarget);

	RELEASEDD(m_pPreviousSRV);
	RELEASEDD(m_pPreviousRTV);
	RELEASEDD(m_pPreviousTexture);

	RELEASEDD(m_pSavedRasterizerState);
	RELEASEDD(m_pSavedBlendState);
	RELEASEDD(m_pSavedDepthStencilState);

	m_pCombine = NULL;
	m_pScale = NULL;

	m_bBlurInitialized = false;
	m_BufferSize = SizeInt(0, 0);
}

//---------------------------------------------------------------------------------------------------

bool DisplayEffectBlur::initializeBlur(DisplayDevice* pDevice)
{
	DisplayDeviceD3D* pDisplayD3D = (DisplayDeviceD3D*)pDevice;
	if (!pDisplayD3D || !pDisplayD3D->getDevice())
		return false;

	ID3D11Device* pD3DDevice = pDisplayD3D->getDevice();
	HRESULT hr;

	// Get buffer size
	m_BufferSize = pDevice->clientWindow().size();
	if (m_BufferSize.width <= 0 || m_BufferSize.height <= 0)
		return false;

	// Create blur render target texture
	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = m_BufferSize.width;
	texDesc.Height = m_BufferSize.height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	try
	{
		hr = pD3DDevice->CreateTexture2D(&texDesc, NULL, &m_pBlurRenderTarget);
	}
	catch (...)
	{
		hr = E_FAIL;
		LOG_ERROR("DisplayEffectBlur", "Exception creating blur render target texture!");
	}
	if (FAILED(hr))
	{
		m_pBlurRenderTarget = NULL;
		if (hr == E_OUTOFMEMORY)
			LOG_ERROR("DisplayEffectBlur", "OUT OF MEMORY creating blur texture! Disabling blur effect.");
		m_bBlurFailed = true;
		return false;
	}

	// Create render target view for blur texture
	try { hr = pD3DDevice->CreateRenderTargetView(m_pBlurRenderTarget, NULL, &m_pBlurRTV); }
	catch (...) { hr = E_FAIL; LOG_ERROR("DisplayEffectBlur", "Exception creating blur RTV!"); }
	if (FAILED(hr)) { m_pBlurRTV = NULL; m_bBlurFailed = true; return false; }

	// Create shader resource view for blur texture
	try { hr = pD3DDevice->CreateShaderResourceView(m_pBlurRenderTarget, NULL, &m_pBlurSRV); }
	catch (...) { hr = E_FAIL; LOG_ERROR("DisplayEffectBlur", "Exception creating blur SRV!"); }
	if (FAILED(hr)) { m_pBlurSRV = NULL; m_bBlurFailed = true; return false; }

	// Create previous frame texture
	try
	{
		hr = pD3DDevice->CreateTexture2D(&texDesc, NULL, &m_pPreviousTexture);
	}
	catch (...)
	{
		hr = E_FAIL;
		LOG_ERROR("DisplayEffectBlur", "Exception creating previous frame texture!");
	}
	if (FAILED(hr))
	{
		m_pPreviousTexture = NULL;
		if (hr == E_OUTOFMEMORY)
			LOG_ERROR("DisplayEffectBlur", "OUT OF MEMORY creating previous frame texture! Disabling blur effect.");
		m_bBlurFailed = true;
		return false;
	}

	// Create render target view for previous texture
	try { hr = pD3DDevice->CreateRenderTargetView(m_pPreviousTexture, NULL, &m_pPreviousRTV); }
	catch (...) { hr = E_FAIL; LOG_ERROR("DisplayEffectBlur", "Exception creating previous RTV!"); }
	if (FAILED(hr)) { m_pPreviousRTV = NULL; m_bBlurFailed = true; return false; }

	// Create shader resource view for previous texture
	try { hr = pD3DDevice->CreateShaderResourceView(m_pPreviousTexture, NULL, &m_pPreviousSRV); }
	catch (...) { hr = E_FAIL; LOG_ERROR("DisplayEffectBlur", "Exception creating previous SRV!"); }
	if (FAILED(hr)) { m_pPreviousSRV = NULL; m_bBlurFailed = true; return false; }

	// Load shaders
	m_pCombine = pDisplayD3D->getShader("Shaders/BlurCombine.fx");
	if (!m_pCombine.valid() || !m_pCombine->valid())
		return false;

	m_pScale = pDisplayD3D->getShader("Shaders/BlurScale.fx");
	if (!m_pScale.valid() || !m_pScale->valid())
		return false;

	// Create vertex buffer for full-screen quad
	if (!createQuadVertexBuffer(pDevice))
		return false;

	return true;
}

bool DisplayEffectBlur::createQuadVertexBuffer(DisplayDevice* pDevice)
{
	DisplayDeviceD3D* pDisplayD3D = (DisplayDeviceD3D*)pDevice;
	if (!pDisplayD3D || !pDisplayD3D->getDevice())
		return false;

	// Full-screen quad vertices (transformed)
	struct QuadVertex
	{
		float x, y, z, w;
		float u, v;
	};

	QuadVertex vertices[] =
	{
		{ -1.0f,  1.0f, 0.5f, 1.0f, 0.0f, 0.0f },
		{  1.0f,  1.0f, 0.5f, 1.0f, 1.0f, 0.0f },
		{ -1.0f, -1.0f, 0.5f, 1.0f, 0.0f, 1.0f },
		{  1.0f, -1.0f, 0.5f, 1.0f, 1.0f, 1.0f },
	};

	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(vertices);
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = vertices;

	HRESULT hr = pDisplayD3D->getDevice()->CreateBuffer(&bd, &initData, &m_pQuadVB);
	if (FAILED(hr))
		return false;

	return true;
}

void DisplayEffectBlur::saveRenderState(DisplayDevice* pDevice)
{
	DisplayDeviceD3D* pDisplayD3D = (DisplayDeviceD3D*)pDevice;
	if (!pDisplayD3D || !pDisplayD3D->getContext())
		return;

	ID3D11DeviceContext* pContext = pDisplayD3D->getContext();

	// Save rasterizer state
	pContext->RSGetState(&m_pSavedRasterizerState);

	// Save blend state
	pContext->OMGetBlendState(&m_pSavedBlendState, m_SavedBlendFactor, &m_nSavedSampleMask);

	// Save depth stencil state
	pContext->OMGetDepthStencilState(&m_pSavedDepthStencilState, &m_nSavedStencilRef);
}

void DisplayEffectBlur::restoreRenderState(DisplayDevice* pDevice)
{
	DisplayDeviceD3D* pDisplayD3D = (DisplayDeviceD3D*)pDevice;
	if (!pDisplayD3D || !pDisplayD3D->getContext())
		return;

	ID3D11DeviceContext* pContext = pDisplayD3D->getContext();

	// Restore rasterizer state
	if (m_pSavedRasterizerState)
	{
		pContext->RSSetState(m_pSavedRasterizerState);
		RELEASEDD(m_pSavedRasterizerState);
	}

	// Restore blend state
	if (m_pSavedBlendState)
	{
		pContext->OMSetBlendState(m_pSavedBlendState, m_SavedBlendFactor, m_nSavedSampleMask);
		RELEASEDD(m_pSavedBlendState);
	}

	// Restore depth stencil state
	if (m_pSavedDepthStencilState)
	{
		pContext->OMSetDepthStencilState(m_pSavedDepthStencilState, m_nSavedStencilRef);
		RELEASEDD(m_pSavedDepthStencilState);
	}
}

void DisplayEffectBlur::renderQuad(DisplayDevice* pDevice)
{
	DisplayDeviceD3D* pDisplayD3D = (DisplayDeviceD3D*)pDevice;
	if (!pDisplayD3D || !pDisplayD3D->getContext() || !m_pQuadVB)
		return;

	ID3D11DeviceContext* pContext = pDisplayD3D->getContext();

	// Set vertex buffer
	UINT stride = sizeof(float) * 6;  // x,y,z,w,u,v
	UINT offset = 0;
	pContext->IASetVertexBuffers(0, 1, &m_pQuadVB, &stride, &offset);

	// Set primitive topology
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// Draw quad
	pContext->Draw(4, 0);
}

//---------------------------------------------------------------------------------------------------
// EOF

