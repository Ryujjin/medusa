/**
	@file DisplayEffectHDR.h
	@brief HDR with bloom effect..

	(c)2009 Palestar Inc
	@author Richard Lyle @date 9/5/2010 5:10:22 PM
*/

#ifndef DISPLAYEFFECTHDR_H
#define DISPLAYEFFECTHDR_H

#include "Display/DisplayEffect.h"
#include "ShaderD3D11.h"// Shawn added for d3d10
//#include "Shader.h"

#include <d3d11.h>
#include <directxmath.h>

//---------------------------------------------------------------------------------------------------

class DisplayEffectHDR : public DisplayEffect
{
public:
	DECLARE_WIDGET_CLASS();
	DECLARE_PROPERTY_LIST();

	//! Construction
	DisplayEffectHDR();
	virtual ~DisplayEffectHDR();

	//! DisplayEffect interface
	virtual bool				preRender( DisplayDevice * pDevice );		
	virtual bool				postRender( DisplayDevice * pDevice );		
	virtual void				release();

	//! Accessors
	int							bloomLevels() const;				// how many steps for the bloom process
	//! Mutators
	void						setBloomLevels( int nBloomLevels );

protected:
	//! Data
	int							m_nBloomLevels;					// how much bloom to apply
	int							m_nBloomSize;					// divide the frame buffer size by this amount

	SizeInt						m_RenderSize;
	SizeInt						m_BloomSize;

	// D3D11 resources
	ID3D11Texture2D *			m_pBloomRenderTarget;			// the texture our scene is actually rendered into..
	ID3D11RenderTargetView *	m_pBloomRenderTargetView;		// RTV for the bloom render target
	ID3D11ShaderResourceView *	m_pBloomRenderTargetSRV;		// SRV for reading from bloom render target

	ID3D11Texture2D *			m_pBloomTextures[ 2 ];			// bloom is rendered back and forth between these two buffers to produce final image
	ID3D11RenderTargetView *	m_pBloomRenderTargetViews[ 2 ];	// RTVs for bloom textures
	ID3D11ShaderResourceView *	m_pBloomShaderResourceViews[ 2 ];// SRVs for reading from bloom textures

	ID3D11RenderTargetView *	m_pOriginalRenderTarget;		// this is our final render target
	ID3D11DepthStencilView *	m_pOriginalDepthStencil;		// original depth stencil

	ID3D11SamplerState *		m_pSamplerState;				// sampler state for texture filtering
	ID3D11DepthStencilState *	m_pDepthStencilState;			// depth stencil state with Z disabled
	ID3D11RasterizerState *		m_pRasterizerState;				// rasterizer state
	ID3D11BlendState *			m_pBlendState;					// blend state
	ID3D11Buffer *					m_pBloomScaleCB;				// 16-byte dynamic CB for fScale at PS b1

	Shader::Ref					m_pBrightPass;
	Shader::Ref					m_pHorzBlur;
	Shader::Ref					m_pVertBlur;
	Shader::Ref					m_pCombine;

	bool						m_bBloomInitialized;
	bool						m_bBloomFailed;
};

//---------------------------------------------------------------------------------------------------

inline int DisplayEffectHDR::bloomLevels() const
{
	return m_nBloomLevels;
}

inline void DisplayEffectHDR::setBloomLevels( int nBloomLevels )
{
	m_nBloomLevels = nBloomLevels;
}

#endif

//---------------------------------------------------------------------------------------------------
//EOF
