/**
	@file DisplayEffectBlur.h
	@brief This display effect is used to blur the display, this effect works by rendering the current
	frame and the previous frame (slightly darker) into the destination.

	Shawn changed to use DirectX 11
*/

#ifndef DISPLAYEFFECTBLUR_H
#define DISPLAYEFFECTBLUR_H

#include "Display/DisplayEffect.h"
#include "ShaderD3D11.h"

#include <d3d11.h>
#include <DirectXMath.h>

//---------------------------------------------------------------------------------------------------

class DisplayEffectBlur : public DisplayEffect
{
public:
	DECLARE_WIDGET_CLASS();
	DECLARE_PROPERTY_LIST();

	//! Construction
	DisplayEffectBlur();
	virtual ~DisplayEffectBlur();

	//! DisplayEffect interface
	virtual bool				preRender(DisplayDevice* pDevice);
	virtual bool				postRender(DisplayDevice* pDevice);
	virtual void				release();

	//! Accessors
	float						blur() const;					// each update the previous frame is darkened by this amount
	// 0.0f = infinite blur, no loss from previous frame
	// 1.0f = no blur
//! Mutators
	void						setBlur(float fBlur);

protected:
	//! Data
	float						m_fBlur;						// the amount to darken our previous frame
	SizeInt						m_BufferSize;

	// DirectX 11 resources
	ID3D11Texture2D* m_pBlurRenderTarget;			// the texture our scene is actually rendered into
	ID3D11RenderTargetView* m_pBlurRTV;						// render target view for blur texture
	ID3D11ShaderResourceView* m_pBlurSRV;						// shader resource view for blur texture

	ID3D11Texture2D* m_pPreviousTexture;				// texture of our previous frame
	ID3D11RenderTargetView* m_pPreviousRTV;					// render target view for previous texture
	ID3D11ShaderResourceView* m_pPreviousSRV;					// shader resource view for previous texture

	ID3D11RenderTargetView* m_pRenderTarget;				// this is our final render target
	ID3D11DepthStencilView* m_pDepthStencil;				// depth stencil for rendering

	// State management (DX11 doesn't have state blocks)
	ID3D11RasterizerState* m_pSavedRasterizerState;
	ID3D11BlendState* m_pSavedBlendState;
	ID3D11DepthStencilState* m_pSavedDepthStencilState;
	float						m_SavedBlendFactor[4];
	UINT						m_nSavedSampleMask;
	UINT						m_nSavedStencilRef;

	Shader::Ref					m_pCombine;						// shader to render our current frame combined with our previous frame
	Shader::Ref					m_pScale;						// shader to scale/copy textures

	// Vertex buffer for full-screen quad
	ID3D11Buffer* m_pQuadVB;

	bool						m_bBlurInitialized;
	bool						m_bBlurFailed;

	// Helper methods
	bool						initializeBlur(DisplayDevice* pDevice);
	bool						createQuadVertexBuffer(DisplayDevice* pDevice);
	void						saveRenderState(DisplayDevice* pDevice);
	void						restoreRenderState(DisplayDevice* pDevice);
	void						renderQuad(DisplayDevice* pDevice);
};

//---------------------------------------------------------------------------------------------------

inline float DisplayEffectBlur::blur() const
{
	return m_fBlur;
}

inline void DisplayEffectBlur::setBlur(float fBlur)
{
	m_fBlur = fBlur;
}

#endif

//---------------------------------------------------------------------------------------------------
//EOF
