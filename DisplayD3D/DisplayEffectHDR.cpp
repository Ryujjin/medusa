/**
	@file DisplayEffectHDR.cpp

	(c)2009 Palestar Inc
	@author Richard Lyle @date 9/5/2010 5:10:39 PM
*/

#define MEDUSA_TRACE_ON

#include "DisplayEffectHDR.h"
#include "DisplayDeviceD3D.h"
#include "PrimitiveWindowD3D.h"
#include "PrimitiveMaterialD3D.h"

#include "Standard/Settings.h"

//---------------------------------------------------------------------------------------------------

IMPLEMENT_LIGHT_FACTORY( DisplayEffectHDR, DisplayEffect );

BEGIN_PROPERTY_LIST( DisplayEffectHDR, DisplayEffect );
	ADD_PROPERTY( m_nBloomLevels );
	ADD_PROPERTY( m_nBloomSize );
END_PROPERTY_LIST();

#if defined(_DEBUG)
const char * CLIENT_SETTINGS = "ClientD";
#else
const char * CLIENT_SETTINGS = "Client";
#endif

//---------------------------------------------------------------------------------------------------

DisplayEffectHDR::DisplayEffectHDR() : 
	m_nBloomLevels( 1 ),
	// Bloom buffer downsample divisor. Was 4 (quarter-res). At quarter-res the fixed
	// 9-tap Gaussian kernel in PS_HorzBlur/PS_VertBlur covers a screen-space radius that
	// stays roughly CONSTANT in bloom-buffer pixels regardless of how large the emitting
	// object is on screen. For a distant/small planet disc, that fixed blur radius becomes
	// large relative to the planet's own visible size, producing a soft halo far wider than
	// the planet -- this does not shrink no matter how much the atmosphere/rim shell geometry
	// itself is tightened, because the spread happens in the post-process blur pass, not in
	// world-space geometry. Halving the divisor (2 = half-res instead of quarter-res) doubles
	// the bloom buffer's linear resolution, halving the blur kernel's footprint relative to
	// the full-res frame and tightening the halo around distant small objects.
	m_nBloomSize( 2 ),
	m_pBloomRenderTarget( NULL ),
	m_pBloomRenderTargetView( NULL ),
	m_pBloomRenderTargetSRV( NULL ),
	m_pOriginalRenderTarget( NULL ),
	m_pOriginalDepthStencil( NULL ),
	m_pSamplerState( NULL ),
	m_pDepthStencilState( NULL ),
	m_pRasterizerState( NULL ),
	m_pBlendState( NULL ),
	m_pBloomScaleCB( NULL ),
	m_bBloomInitialized( false ),
	m_bBloomFailed( false )
{
	for(int i=0;i<2;++i)
	{
		m_pBloomTextures[i] = NULL;
		m_pBloomRenderTargetViews[i] = NULL;
		m_pBloomShaderResourceViews[i] = NULL;
	}
}

DisplayEffectHDR::~DisplayEffectHDR()
{
	release();
}

bool DisplayEffectHDR::preRender( DisplayDevice * pDevice )
{
	static int s_nCallCount = 0;
	static int s_nInitCount = 0;
	s_nCallCount++;

	if ( m_bBloomFailed || DisplayDevice::sm_bUseFixedFunction )
		return true;
	DisplayDeviceD3D * pDeviceD3D = WidgetCast<DisplayDeviceD3D>( pDevice );
	if (! pDeviceD3D )
		return false;
	ID3D11Device * pD3DDevice = pDeviceD3D->getDevice();
	ID3D11DeviceContext * pContext = pDeviceD3D->getContext();
	if (! pD3DDevice || ! pContext )
		return false;

	TRACE("HDR::preRender #%d (Init:%d, Failed:%d, Initialized:%d)", 
		s_nCallCount, s_nInitCount, m_bBloomFailed ? 1 : 0, m_bBloomInitialized ? 1 : 0);

	// Release previous resources
	if ( m_pOriginalRenderTarget )
	{
		m_pOriginalRenderTarget->Release();
		m_pOriginalRenderTarget = NULL;
	}
	if ( m_pOriginalDepthStencil )
	{
		m_pOriginalDepthStencil->Release();
		m_pOriginalDepthStencil = NULL;
	}

	// Get the current render target to restore later
	pContext->OMGetRenderTargets( 1, &m_pOriginalRenderTarget, &m_pOriginalDepthStencil );
	if (! m_pOriginalRenderTarget )
		return false;

	// Get the render target texture to determine size
	ID3D11Resource * pResource = NULL;
	m_pOriginalRenderTarget->GetResource( &pResource );
	if (! pResource )
	{
		// Release acquired render targets before returning
		if ( m_pOriginalRenderTarget )
		{
			m_pOriginalRenderTarget->Release();
			m_pOriginalRenderTarget = NULL;
		}
		if ( m_pOriginalDepthStencil )
		{
			m_pOriginalDepthStencil->Release();
			m_pOriginalDepthStencil = NULL;
		}
		return false;
	}

	ID3D11Texture2D * pTexture = NULL;
	HRESULT hr = pResource->QueryInterface( __uuidof(ID3D11Texture2D), (void**)&pTexture );
	pResource->Release();
	if ( FAILED(hr) || !pTexture )
	{
		// Release acquired render targets before returning
		if ( m_pOriginalRenderTarget )
		{
			m_pOriginalRenderTarget->Release();
			m_pOriginalRenderTarget = NULL;
		}
		if ( m_pOriginalDepthStencil )
		{
			m_pOriginalDepthStencil->Release();
			m_pOriginalDepthStencil = NULL;
		}
		return false;
	}

	D3D11_TEXTURE2D_DESC texDesc;
	pTexture->GetDesc( &texDesc );
	pTexture->Release();

	// assume we will fail, this will get set to false if everything succeeds
	m_bBloomFailed = true;	

	if (! m_bBloomInitialized || m_RenderSize != SizeInt( texDesc.Width, texDesc.Height) )
	{
		s_nInitCount++;
		TRACE("HDR: Initializing bloom buffers #%d (Size: %dx%d -> %dx%d)", 
			s_nInitCount, m_RenderSize.width, m_RenderSize.height, texDesc.Width, texDesc.Height);

		// release our previous buffers if we have them..
		release();

		m_RenderSize = SizeInt( texDesc.Width, texDesc.Height );
		m_BloomSize = m_RenderSize / m_nBloomSize;

		if ( pDeviceD3D->pixelShaderVersion() < 2.0f || pDeviceD3D->vertexShaderVersion() < 2.0f )
		{
			// Release acquired render targets before returning
			if ( m_pOriginalRenderTarget )
			{
				m_pOriginalRenderTarget->Release();
				m_pOriginalRenderTarget = NULL;
			}
			if ( m_pOriginalDepthStencil )
			{
				m_pOriginalDepthStencil->Release();
				m_pOriginalDepthStencil = NULL;
			}
			return false;
		}

		// first, load our bloom shaders..
		if (! (m_pBrightPass = pDeviceD3D->getShader( "Shaders/PS_BrightPass.fx" )) || !m_pBrightPass->valid() )
		{
			// Release acquired render targets before returning
			if ( m_pOriginalRenderTarget )
			{
				m_pOriginalRenderTarget->Release();
				m_pOriginalRenderTarget = NULL;
			}
			if ( m_pOriginalDepthStencil )
			{
				m_pOriginalDepthStencil->Release();
				m_pOriginalDepthStencil = NULL;
			}
			return false;
		}
		if (! (m_pHorzBlur = pDeviceD3D->getShader( "Shaders/PS_HorzBlur.fx" )) || !m_pHorzBlur->valid() )
		{
			// Release acquired render targets before returning
			if ( m_pOriginalRenderTarget )
			{
				m_pOriginalRenderTarget->Release();
				m_pOriginalRenderTarget = NULL;
			}
			if ( m_pOriginalDepthStencil )
			{
				m_pOriginalDepthStencil->Release();
				m_pOriginalDepthStencil = NULL;
			}
			return false;
		}
		if (! (m_pVertBlur = pDeviceD3D->getShader( "Shaders/PS_VertBlur.fx" )) || !m_pVertBlur->valid() )
		{
			// Release acquired render targets before returning
			if ( m_pOriginalRenderTarget )
			{
				m_pOriginalRenderTarget->Release();
				m_pOriginalRenderTarget = NULL;
			}
			if ( m_pOriginalDepthStencil )
			{
				m_pOriginalDepthStencil->Release();
				m_pOriginalDepthStencil = NULL;
			}
			return false;
		}
		if (! (m_pCombine = pDeviceD3D->getShader( "Shaders/PS_Combine.fx" )) || !m_pCombine->valid() )
		{
			// Release acquired render targets before returning
			if ( m_pOriginalRenderTarget )
			{
				m_pOriginalRenderTarget->Release();
				m_pOriginalRenderTarget = NULL;
			}
			if ( m_pOriginalDepthStencil )
			{
				m_pOriginalDepthStencil->Release();
				m_pOriginalDepthStencil = NULL;
			}
			return false;
		}

		// Create texture description for HDR render target
		D3D11_TEXTURE2D_DESC rtDesc;
		ZeroMemory( &rtDesc, sizeof(rtDesc) );
		rtDesc.Width = m_RenderSize.width;
		rtDesc.Height = m_RenderSize.height;
		rtDesc.MipLevels = 1;
		rtDesc.ArraySize = 1;
		rtDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		rtDesc.SampleDesc.Count = 1;
		rtDesc.Usage = D3D11_USAGE_DEFAULT;
		rtDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

		// Create bloom render target
		try
		{
			hr = pD3DDevice->CreateTexture2D( &rtDesc, NULL, &m_pBloomRenderTarget );
		}
		catch (...)
		{
			hr = E_FAIL;
			LOG_ERROR("DisplayEffectHDR", "Exception creating bloom render target!");
		}
		if ( FAILED(hr) || !m_pBloomRenderTarget )
		{
			if (hr == E_OUTOFMEMORY)
				LOG_ERROR("DisplayEffectHDR", "OUT OF MEMORY creating HDR texture! Disabling HDR effect.");
			// Release acquired render targets before returning
			if ( m_pOriginalRenderTarget )
			{
				m_pOriginalRenderTarget->Release();
				m_pOriginalRenderTarget = NULL;
			}
			if ( m_pOriginalDepthStencil )
			{
				m_pOriginalDepthStencil->Release();
				m_pOriginalDepthStencil = NULL;
			}
			return false;
		}

		// Create RTV for bloom render target
		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
		ZeroMemory( &rtvDesc, sizeof(rtvDesc) );
		rtvDesc.Format = rtDesc.Format;
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		try { hr = pD3DDevice->CreateRenderTargetView( m_pBloomRenderTarget, &rtvDesc, &m_pBloomRenderTargetView ); }
		catch (...) { hr = E_FAIL; LOG_ERROR("DisplayEffectHDR", "Exception creating bloom RTV!"); }
		if ( FAILED(hr) || !m_pBloomRenderTargetView )
		{
			m_pBloomRenderTargetView = NULL;
			// Release acquired render targets before returning
			if ( m_pOriginalRenderTarget )
			{
				m_pOriginalRenderTarget->Release();
				m_pOriginalRenderTarget = NULL;
			}
			if ( m_pOriginalDepthStencil )
			{
				m_pOriginalDepthStencil->Release();
				m_pOriginalDepthStencil = NULL;
			}
			return false;
		}

		// Create SRV for bloom render target
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory( &srvDesc, sizeof(srvDesc) );
		srvDesc.Format = rtDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		try { hr = pD3DDevice->CreateShaderResourceView( m_pBloomRenderTarget, &srvDesc, &m_pBloomRenderTargetSRV ); }
		catch (...) { hr = E_FAIL; LOG_ERROR("DisplayEffectHDR", "Exception creating bloom SRV!"); }
		if ( FAILED(hr) || !m_pBloomRenderTargetSRV )
		{
			m_pBloomRenderTargetSRV = NULL;
			// Release acquired render targets before returning
			if ( m_pOriginalRenderTarget )
			{
				m_pOriginalRenderTarget->Release();
				m_pOriginalRenderTarget = NULL;
			}
			if ( m_pOriginalDepthStencil )
			{
				m_pOriginalDepthStencil->Release();
				m_pOriginalDepthStencil = NULL;
			}
			return false;
		}

		// Create bloom texture description
		D3D11_TEXTURE2D_DESC bloomDesc;
		ZeroMemory( &bloomDesc, sizeof(bloomDesc) );
		bloomDesc.Width = m_BloomSize.width;
		bloomDesc.Height = m_BloomSize.height;
		bloomDesc.MipLevels = 1;
		bloomDesc.ArraySize = 1;
		bloomDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		bloomDesc.SampleDesc.Count = 1;
		bloomDesc.Usage = D3D11_USAGE_DEFAULT;
		bloomDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

		// create our buffers for the bloom effect..
		for(int i=0;i<2;++i)
		{
			try
			{
				hr = pD3DDevice->CreateTexture2D( &bloomDesc, NULL, &m_pBloomTextures[i] );
			}
			catch (...)
			{
				hr = E_FAIL;
				LOG_ERROR("DisplayEffectHDR", "Exception creating bloom texture %d!", i);
			}
			if ( FAILED(hr) || !m_pBloomTextures[i] )
			{
				if (hr == E_OUTOFMEMORY)
					LOG_ERROR("DisplayEffectHDR", "OUT OF MEMORY creating bloom texture %d! Disabling HDR effect.", i);
				m_pBloomTextures[i] = NULL;
				// Release acquired render targets before returning
				if ( m_pOriginalRenderTarget )
				{
					m_pOriginalRenderTarget->Release();
					m_pOriginalRenderTarget = NULL;
				}
				if ( m_pOriginalDepthStencil )
				{
					m_pOriginalDepthStencil->Release();
					m_pOriginalDepthStencil = NULL;
				}
				return false;
			}

			// Create RTV
			try { hr = pD3DDevice->CreateRenderTargetView( m_pBloomTextures[i], &rtvDesc, &m_pBloomRenderTargetViews[i] ); }
			catch (...) { hr = E_FAIL; LOG_ERROR("DisplayEffectHDR", "Exception creating bloom RTV %d!", i); }
			if ( FAILED(hr) || !m_pBloomRenderTargetViews[i] )
			{
				m_pBloomRenderTargetViews[i] = NULL;
				// Release acquired render targets before returning
				if ( m_pOriginalRenderTarget )
				{
					m_pOriginalRenderTarget->Release();
					m_pOriginalRenderTarget = NULL;
				}
				if ( m_pOriginalDepthStencil )
				{
					m_pOriginalDepthStencil->Release();
					m_pOriginalDepthStencil = NULL;
				}
				return false;
			}

			// Create SRV
			try { hr = pD3DDevice->CreateShaderResourceView( m_pBloomTextures[i], &srvDesc, &m_pBloomShaderResourceViews[i] ); }
			catch (...) { hr = E_FAIL; LOG_ERROR("DisplayEffectHDR", "Exception creating bloom SRV %d!", i); }
			if ( FAILED(hr) || !m_pBloomShaderResourceViews[i] )
			{
				m_pBloomShaderResourceViews[i] = NULL;
				// Release acquired render targets before returning
				if ( m_pOriginalRenderTarget )
				{
					m_pOriginalRenderTarget->Release();
					m_pOriginalRenderTarget = NULL;
				}
				if ( m_pOriginalDepthStencil )
				{
					m_pOriginalDepthStencil->Release();
					m_pOriginalDepthStencil = NULL;
				}
				return false;
			}
		}

		// Create sampler state
		D3D11_SAMPLER_DESC samplerDesc;
		ZeroMemory( &samplerDesc, sizeof(samplerDesc) );
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.MaxAnisotropy = 1;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		samplerDesc.MinLOD = -FLT_MAX;
		samplerDesc.MaxLOD = FLT_MAX;
		try {
			hr = pD3DDevice->CreateSamplerState( &samplerDesc, &m_pSamplerState );
		} catch (...) {
			hr = E_FAIL;
			LOG_ERROR("DisplayEffectHDR", "Exception creating sampler state!");
		}
		if ( FAILED(hr) || !m_pSamplerState )
		{
			m_pSamplerState = NULL;
			if (hr == E_OUTOFMEMORY)
				LOG_ERROR("DisplayEffectHDR", "OUT OF MEMORY creating sampler state!");
			// Release acquired render targets before returning
			if ( m_pOriginalRenderTarget )
			{
				m_pOriginalRenderTarget->Release();
				m_pOriginalRenderTarget = NULL;
			}
			if ( m_pOriginalDepthStencil )
			{
				m_pOriginalDepthStencil->Release();
				m_pOriginalDepthStencil = NULL;
			}
			return false;
		}

		// Create depth stencil state with Z disabled
		D3D11_DEPTH_STENCIL_DESC dsDesc;
		ZeroMemory( &dsDesc, sizeof(dsDesc) );
		dsDesc.DepthEnable = FALSE;
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		dsDesc.StencilEnable = FALSE;
		hr = pD3DDevice->CreateDepthStencilState( &dsDesc, &m_pDepthStencilState );
		if ( FAILED(hr) || !m_pDepthStencilState )
		{
			// Release acquired render targets before returning
			if ( m_pOriginalRenderTarget )
			{
				m_pOriginalRenderTarget->Release();
				m_pOriginalRenderTarget = NULL;
			}
			if ( m_pOriginalDepthStencil )
			{
				m_pOriginalDepthStencil->Release();
				m_pOriginalDepthStencil = NULL;
			}
			return false;
		}

		// Create rasterizer state
		D3D11_RASTERIZER_DESC rsDesc;
		ZeroMemory( &rsDesc, sizeof(rsDesc) );
		rsDesc.FillMode = D3D11_FILL_SOLID;
		rsDesc.CullMode = D3D11_CULL_NONE;
		rsDesc.DepthClipEnable = TRUE;
		hr = pD3DDevice->CreateRasterizerState( &rsDesc, &m_pRasterizerState );
		if ( FAILED(hr) || !m_pRasterizerState )
		{
			// Release acquired render targets before returning
			if ( m_pOriginalRenderTarget )
			{
				m_pOriginalRenderTarget->Release();
				m_pOriginalRenderTarget = NULL;
			}
			if ( m_pOriginalDepthStencil )
			{
				m_pOriginalDepthStencil->Release();
				m_pOriginalDepthStencil = NULL;
			}
			return false;
		}

		// Create blend state (no blending)
		D3D11_BLEND_DESC blendDesc;
		ZeroMemory( &blendDesc, sizeof(blendDesc) );
		blendDesc.RenderTarget[0].BlendEnable = FALSE;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		hr = pD3DDevice->CreateBlendState( &blendDesc, &m_pBlendState );
		if ( FAILED(hr) || !m_pBlendState )
		{
			// Release acquired render targets before returning
			if ( m_pOriginalRenderTarget )
			{
				m_pOriginalRenderTarget->Release();
				m_pOriginalRenderTarget = NULL;
			}
			if ( m_pOriginalDepthStencil )
			{
				m_pOriginalDepthStencil->Release();
				m_pOriginalDepthStencil = NULL;
			}
			return false;
		}

		m_bBloomInitialized = true;

		// Create bloom scale constant buffer (PS b1)
		{
			D3D11_BUFFER_DESC cbDesc;
			ZeroMemory( &cbDesc, sizeof(cbDesc) );
			cbDesc.ByteWidth = 16;
			cbDesc.Usage = D3D11_USAGE_DYNAMIC;
			cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			hr = pD3DDevice->CreateBuffer( &cbDesc, NULL, &m_pBloomScaleCB );
			if ( FAILED(hr) || !m_pBloomScaleCB )
			{
				if ( m_pOriginalRenderTarget ) { m_pOriginalRenderTarget->Release(); m_pOriginalRenderTarget = NULL; }
				if ( m_pOriginalDepthStencil ) { m_pOriginalDepthStencil->Release(); m_pOriginalDepthStencil = NULL; }
				m_bBloomInitialized = false;
				return false;
			}
		}

		// give our pixel shaders the size of our bloom buffer, so they can calculate UV's correctly..
		m_pHorzBlur->setConstant( "nDiffuseWidth", m_BloomSize.width );
		m_pHorzBlur->setConstant( "nDiffuseHeight", m_BloomSize.height );
		m_pVertBlur->setConstant( "nDiffuseWidth", m_BloomSize.width );
		m_pVertBlur->setConstant( "nDiffuseHeight", m_BloomSize.height );
		m_pCombine->setConstant( "nBloomWidth", m_BloomSize.width );
		m_pCombine->setConstant( "nBloomHeight", m_BloomSize.height );

		Settings settings( CLIENT_SETTINGS );
		m_pCombine->setConstant( "fScale", settings.get( "bloomScale", 100 ) / 100.0f );
	}

	// Set the bloom render target
	pContext->OMSetRenderTargets( 1, &m_pBloomRenderTargetView, NULL );

	// Clear our render target surface to black
	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	pContext->ClearRenderTargetView( m_pBloomRenderTargetView, clearColor );

	m_bBloomFailed = false;

	return true;
}

bool DisplayEffectHDR::postRender( DisplayDevice * pDevice )
{
	if ( m_bBloomFailed || DisplayDevice::sm_bUseFixedFunction )
		return true;
	DisplayDeviceD3D * pDeviceD3D = WidgetCast<DisplayDeviceD3D>( pDevice );
	if (! pDeviceD3D )
		return false;
	ID3D11Device * pD3DDevice = pDeviceD3D->getDevice();
	ID3D11DeviceContext * pContext = pDeviceD3D->getContext();
	if (! pD3DDevice || ! pContext )
		return false;

	// assume the bloom has failed unless it reaches the end of this function and we clear this flag again
	m_bBloomFailed = true;

	// Set sampler state and other render states
	pContext->PSSetSamplers( 0, 1, &m_pSamplerState );
	pContext->PSSetSamplers( 1, 1, &m_pSamplerState );
	pContext->OMSetDepthStencilState( m_pDepthStencilState, 0 );
	pContext->RSSetState( m_pRasterizerState );
	pContext->OMSetBlendState( m_pBlendState, NULL, 0xFFFFFFFF );

	// Set viewport for bloom buffers
	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = (float)m_BloomSize.width;
	viewport.Height = (float)m_BloomSize.height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	pContext->RSSetViewports( 1, &viewport );

	// first step - render our scene at 1/4 it's original size into our first bloom buffer, using the bright pass pixel
	// shader... this will darken the image.
	if ( m_pBrightPass.valid() && m_pBrightPass->valid() )
	{
		m_pBrightPass->apply( pDeviceD3D );

		pContext->IASetInputLayout( NULL );
		pContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
		pContext->PSSetShaderResources( 0, 1, &m_pBloomRenderTargetSRV );
		pContext->OMSetRenderTargets( 1, &m_pBloomRenderTargetViews[0], NULL );
		pContext->Draw( 3, 0 );

		// Clear SRV
		ID3D11ShaderResourceView * nullSRV = NULL;
		pContext->PSSetShaderResources( 0, 1, &nullSRV );
	}
	else
	{
		// Release acquired render targets before returning
		if ( m_pOriginalRenderTarget )
		{
			m_pOriginalRenderTarget->Release();
			m_pOriginalRenderTarget = NULL;
		}
		if ( m_pOriginalDepthStencil )
		{
			m_pOriginalDepthStencil->Release();
			m_pOriginalDepthStencil = NULL;
		}
		return false;
	}

	// now blur the result N times...
	for(int i=0;i<m_nBloomLevels;++i)
	{
		// Horizontal blur pass
		if ( m_pHorzBlur.valid() && m_pHorzBlur->valid() )
		{
			m_pHorzBlur->apply( pDeviceD3D );

			pContext->IASetInputLayout( NULL );
			pContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
			pContext->OMSetRenderTargets( 1, &m_pBloomRenderTargetViews[1], NULL );
			pContext->PSSetShaderResources( 0, 1, &m_pBloomShaderResourceViews[0] );
			pContext->Draw( 3, 0 );

			// Clear SRV
			ID3D11ShaderResourceView * nullSRV = NULL;
			pContext->PSSetShaderResources( 0, 1, &nullSRV );
		}
		else
		{
			// Release acquired render targets before returning
			if ( m_pOriginalRenderTarget )
			{
				m_pOriginalRenderTarget->Release();
				m_pOriginalRenderTarget = NULL;
			}
			if ( m_pOriginalDepthStencil )
			{
				m_pOriginalDepthStencil->Release();
				m_pOriginalDepthStencil = NULL;
			}
			return false;
		}

		// Vertical blur pass
		if ( m_pVertBlur.valid() && m_pVertBlur->valid() )
		{
			m_pVertBlur->apply( pDeviceD3D );

			pContext->IASetInputLayout( NULL );
			pContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
			pContext->OMSetRenderTargets( 1, &m_pBloomRenderTargetViews[0], NULL );
			pContext->PSSetShaderResources( 0, 1, &m_pBloomShaderResourceViews[1] );
			pContext->Draw( 3, 0 );

			// Clear SRV
			ID3D11ShaderResourceView * nullSRV = NULL;
			pContext->PSSetShaderResources( 0, 1, &nullSRV );
		}
		else
		{
			// Release acquired render targets before returning
			if ( m_pOriginalRenderTarget )
			{
				m_pOriginalRenderTarget->Release();
				m_pOriginalRenderTarget = NULL;
			}
			if ( m_pOriginalDepthStencil )
			{
				m_pOriginalDepthStencil->Release();
				m_pOriginalDepthStencil = NULL;
			}
			return false;
		}
	}

	// Restore the original render target
	pContext->OMSetRenderTargets( 1, &m_pOriginalRenderTarget, m_pOriginalDepthStencil );

	// Set viewport back to original size
	viewport.Width = (float)m_RenderSize.width;
	viewport.Height = (float)m_RenderSize.height;
	pContext->RSSetViewports( 1, &viewport );

	// combine our original scene and our bloom texture..
	if ( m_pCombine.valid() && m_pCombine->valid() )
	{
		m_pCombine->apply( pDeviceD3D );

		pContext->IASetInputLayout( NULL );
		pContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
		ID3D11ShaderResourceView * srvs[2] = { m_pBloomRenderTargetSRV, m_pBloomShaderResourceViews[0] };
		pContext->PSSetShaderResources( 0, 2, srvs );
		// Update fScale from settings and bind to PS b1
		if ( m_pBloomScaleCB )
		{
			Settings settings( CLIENT_SETTINGS );
			struct { float fScale; float pad[3]; } sd;
			sd.fScale = settings.get( "bloomScale", 100 ) / 100.0f;
			sd.pad[0] = sd.pad[1] = sd.pad[2] = 0.0f;
			D3D11_MAPPED_SUBRESOURCE mr;
			if ( SUCCEEDED( pContext->Map( m_pBloomScaleCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mr ) ) )
			{
				memcpy( mr.pData, &sd, 16 );
				pContext->Unmap( m_pBloomScaleCB, 0 );
			}
			pContext->PSSetConstantBuffers( 1, 1, &m_pBloomScaleCB );
		}
		pContext->Draw( 3, 0 );

		// Clear shader resources to avoid warnings
		ID3D11ShaderResourceView * nullSRVs[2] = { NULL, NULL };
		pContext->PSSetShaderResources( 0, 2, nullSRVs );
	}
	else
	{
		// Release acquired render targets before returning
		if ( m_pOriginalRenderTarget )
		{
			m_pOriginalRenderTarget->Release();
			m_pOriginalRenderTarget = NULL;
		}
		if ( m_pOriginalDepthStencil )
		{
			m_pOriginalDepthStencil->Release();
			m_pOriginalDepthStencil = NULL;
		}
		return false;
	}

	// no problems, clear failure flag..
	m_bBloomFailed = false;

	return true;
}

void DisplayEffectHDR::release()
{
	if ( m_pOriginalRenderTarget )
	{
		m_pOriginalRenderTarget->Release();
		m_pOriginalRenderTarget = NULL;
	}
	if ( m_pOriginalDepthStencil )
	{
		m_pOriginalDepthStencil->Release();
		m_pOriginalDepthStencil = NULL;
	}
	if ( m_pBloomRenderTarget )
	{
		m_pBloomRenderTarget->Release();
		m_pBloomRenderTarget = NULL;
	}
	if ( m_pBloomRenderTargetView )
	{
		m_pBloomRenderTargetView->Release();
		m_pBloomRenderTargetView = NULL;
	}
	if ( m_pBloomRenderTargetSRV )
	{
		m_pBloomRenderTargetSRV->Release();
		m_pBloomRenderTargetSRV = NULL;
	}

	for(int i=0;i<2;++i)
	{
		if ( m_pBloomTextures[i] )
		{
			m_pBloomTextures[i]->Release();
			m_pBloomTextures[i] = NULL;
		}
		if ( m_pBloomRenderTargetViews[i] )
		{
			m_pBloomRenderTargetViews[i]->Release();
			m_pBloomRenderTargetViews[i] = NULL;
		}
		if ( m_pBloomShaderResourceViews[i] )
		{
			m_pBloomShaderResourceViews[i]->Release();
			m_pBloomShaderResourceViews[i] = NULL;
		}
	}

	if ( m_pSamplerState )
	{
		m_pSamplerState->Release();
		m_pSamplerState = NULL;
	}
	if ( m_pDepthStencilState )
	{
		m_pDepthStencilState->Release();
		m_pDepthStencilState = NULL;
	}
	if ( m_pRasterizerState )
	{
		m_pRasterizerState->Release();
		m_pRasterizerState = NULL;
	}
	if ( m_pBlendState )
	{
		m_pBlendState->Release();
		m_pBlendState = NULL;
	}

	if ( m_pBloomScaleCB )
	{
		m_pBloomScaleCB->Release();
		m_pBloomScaleCB = NULL;
	}

	m_bBloomInitialized = false;
	m_bBloomFailed = false;
}

//---------------------------------------------------------------------------------------------------
//EOF
