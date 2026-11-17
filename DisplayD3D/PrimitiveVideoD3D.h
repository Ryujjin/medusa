#pragma once

//  PrimitiveVideoD3D.h
//
//  D3D11 concrete implementation of PrimitiveVideo.
//  Manages a dynamic RGBA texture updated per-frame via updateFrame(),
//  and renders it fullscreen using VS_Video / PS_Video from Default_D3D11.fx.

#include "Display/PrimitiveVideo.h"
#include <d3d11.h>

//----------------------------------------------------------------------------

class PrimitiveVideoD3D : public PrimitiveVideo
{
public:
    DECLARE_DEVICE_PRIMITIVE();

    PrimitiveVideoD3D();
    virtual ~PrimitiveVideoD3D();

    // PrimitiveVideo interface
    bool    initialize(int width, int height) override;
    void    updateFrame(const void* pData, int stride) override;
    void    setAlpha(float fAlpha) override;
    void    setLetterbox(bool bLetterbox) override;

    // DevicePrimitive interface
    bool    execute() override;
    void    clear() override;
    void    release() override;

private:
    ID3D11Texture2D* m_pTexture;
    ID3D11ShaderResourceView* m_pSRV;

    int     m_nWidth;
    int     m_nHeight;
    float   m_fAlpha;
    bool    m_bLetterbox;
};

//----------------------------------------------------------------------------