/*
PrimitiveVideoD3D.cpp
D3D11 concrete implementation of PrimitiveVideo.

Manages a dynamic RGBA8 texture (t8 = txVideo) that is updated each frame via
updateFrame(), then renders it fullscreen using VS_Video + PS_Video from
Default_D3D11.fx.  The same SV_VertexID fullscreen-triangle technique as
VS_Galaxy / VS_GalaxySprite is used so no vertex buffer is needed.

(c)2025 DarkSpace DX11 Port
*/

#include <d3d11.h>
#include "DisplayD3D/DisplayDeviceD3D.h"
#include "DisplayD3D/PrimitiveVideoD3D.h"
#include "DisplayD3D/PrimitiveFactory.h"
#include "Debug/Trace.h"

//----------------------------------------------------------------------------

IMPLEMENT_PRIMITIVE_FACTORY( PrimitiveVideoD3D );

//----------------------------------------------------------------------------

PrimitiveVideoD3D::PrimitiveVideoD3D() :
m_pTexture(NULL),
m_pSRV(NULL),
m_nWidth(0),
m_nHeight(0),
m_fAlpha(1.0f),
m_bLetterbox(false)
{}

PrimitiveVideoD3D::~PrimitiveVideoD3D()
{
release();
}

//----------------------------------------------------------------------------

bool PrimitiveVideoD3D::initialize( int width, int height )
{
if ( width <= 0 || height <= 0 )
{
TRACE( "PrimitiveVideoD3D::initialize -- bad size %dx%d", width, height );
return false;
}

DisplayDeviceD3D * pDisplay = (DisplayDeviceD3D *)m_pDevice;
if ( !pDisplay || !pDisplay->getDevice() )
{
TRACE( "PrimitiveVideoD3D::initialize -- no D3D device" );
return false;
}

// Release any previous texture.
if ( m_pSRV )     { m_pSRV->Release();     m_pSRV = NULL; }
if ( m_pTexture ) { m_pTexture->Release();  m_pTexture = NULL; }

ID3D11Device * pDevice = pDisplay->getDevice();

// Dynamic texture so updateFrame() can map/write every frame.
D3D11_TEXTURE2D_DESC desc = {};
desc.Width          = (UINT)width;
desc.Height         = (UINT)height;
desc.MipLevels      = 1;
desc.ArraySize      = 1;
desc.Format         = DXGI_FORMAT_R8G8B8A8_UNORM;
desc.SampleDesc.Count = 1;
desc.Usage          = D3D11_USAGE_DYNAMIC;
desc.BindFlags      = D3D11_BIND_SHADER_RESOURCE;
desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

HRESULT hr = pDevice->CreateTexture2D( &desc, NULL, &m_pTexture );
if ( FAILED(hr) )
{
TRACE( "PrimitiveVideoD3D::initialize -- CreateTexture2D failed hr=0x%08x", hr );
return false;
}

D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
srvDesc.Format              = DXGI_FORMAT_R8G8B8A8_UNORM;
srvDesc.ViewDimension       = D3D11_SRV_DIMENSION_TEXTURE2D;
srvDesc.Texture2D.MipLevels = 1;

hr = pDevice->CreateShaderResourceView( m_pTexture, &srvDesc, &m_pSRV );
if ( FAILED(hr) )
{
TRACE( "PrimitiveVideoD3D::initialize -- CreateShaderResourceView failed hr=0x%08x", hr );
m_pTexture->Release(); m_pTexture = NULL;
return false;
}

m_nWidth  = width;
m_nHeight = height;
TRACE( "PrimitiveVideoD3D::initialize -- OK %dx%d", width, height );
return true;
}

//----------------------------------------------------------------------------

void PrimitiveVideoD3D::updateFrame( const void * pData, int rowPitch )
{
if ( !m_pTexture || !pData )
return;

DisplayDeviceD3D * pDisplay = (DisplayDeviceD3D *)m_pDevice;
if ( !pDisplay || !pDisplay->getContext() )
return;

ID3D11DeviceContext * pContext = pDisplay->getContext();

D3D11_MAPPED_SUBRESOURCE mapped = {};
HRESULT hr = pContext->Map( m_pTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped );
if ( FAILED(hr) )
{
TRACE( "PrimitiveVideoD3D::updateFrame -- Map failed hr=0x%08x", hr );
return;
}

const uint8_t * pSrc = (const uint8_t *)pData;
uint8_t       * pDst = (uint8_t *)mapped.pData;
int copyPitch = min( rowPitch, (int)mapped.RowPitch );
for ( int y = 0; y < m_nHeight; ++y )
{
memcpy( pDst + y * mapped.RowPitch, pSrc + y * rowPitch, copyPitch );
}

pContext->Unmap( m_pTexture, 0 );
}

//----------------------------------------------------------------------------

void PrimitiveVideoD3D::setAlpha( float fAlpha )
{
m_fAlpha = fAlpha;
}

void PrimitiveVideoD3D::setLetterbox( bool bLetterbox )
{
m_bLetterbox = bLetterbox;
}

//----------------------------------------------------------------------------

bool PrimitiveVideoD3D::execute()
{
DisplayDeviceD3D * pDisplay = (DisplayDeviceD3D *)m_pDevice;
if ( !pDisplay || !pDisplay->getContext() )
return false;
if ( !pDisplay->m_pMatShader.valid() )
return false;
if ( !m_pSRV )
return false;

ID3D11DeviceContext * pContext = pDisplay->getContext();

// Upload vVideoParams: x=alpha, y=letterbox flag.
DirectX::XMFLOAT4 videoParams( m_fAlpha, m_bLetterbox ? 1.0f : 0.0f, 0.0f, 0.0f );
pDisplay->m_pMatShader->setConstant( "vVideoParams", videoParams );

// Bind VS_Video + PS_Video (uploads CB with updated vVideoParams).
if ( !pDisplay->m_pMatShader->applyVideo( pDisplay ) )
return false;

// Bind the video frame texture to t8.
pContext->PSSetShaderResources( 8, 1, &m_pSRV );

// Fullscreen triangle, no vertex buffer required.
ID3D11Buffer * pNullVB = NULL;
UINT stride = 0, offset = 0;
pContext->IASetVertexBuffers( 0, 1, &pNullVB, &stride, &offset );
pContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
pContext->Draw( 3, 0 );

// Unbind t8 so subsequent passes are not affected.
ID3D11ShaderResourceView * pNullSRV = NULL;
pContext->PSSetShaderResources( 8, 1, &pNullSRV );

return true;
}

//----------------------------------------------------------------------------

void PrimitiveVideoD3D::clear()
{}

void PrimitiveVideoD3D::release()
{
if ( m_pSRV )     { m_pSRV->Release();     m_pSRV = NULL; }
if ( m_pTexture ) { m_pTexture->Release();  m_pTexture = NULL; }
m_nWidth  = 0;
m_nHeight = 0;
}

//----------------------------------------------------------------------------
// EOF