/*
PrimitiveStarBillboardD3D.cpp
D3D11 implementation — camera-facing quad rendered with VS_Star + PS_Star.

Renders the extended corona / CME region (r > 1 disk radii) that is invisible
on the tessellated sphere (PrimitiveStarSurface) because the sphere mesh only
covers r <= 1.  Both primitives are submitted ADDITIVE so they composite
naturally: the sphere paints the photosphere disc; this billboard paints
the outer corona, flares, and CME filaments around it.

UV [0,1] across the quad → PS_Star maps to disk coords via:
    uv_disk = uv * 5.0 - 2.5      (range -2.5 .. +2.5 disk radii)

Vertex layout: VertexL  (pos float3, normal float3, diffuse Color, u float, v float)
Input layout bound by applyStarSurface(): m_pInputLayoutL

(c)2025 DarkSpace DX11 Port
*/

#include <d3d11.h>
#include <DirectXMath.h>
#include "Math/Vector3.h"
#include "DisplayD3D/DisplayDeviceD3D.h"
#include "DisplayD3D/PrimitiveStarBillboardD3D.h"
#include "DisplayD3D/PrimitiveFactory.h"
#include "Debug/Trace.h"
#include "Standard/CharString.h"

//----------------------------------------------------------------------------

IMPLEMENT_PRIMITIVE_FACTORY( PrimitiveStarBillboardD3D );

PrimitiveStarBillboardD3D::PrimitiveStarBillboardD3D()
: m_pVB(NULL)
{}

bool PrimitiveStarBillboardD3D::initializeMesh( const Vertex * pVerts, int nVerts,
                                                const Vector3 & starColor )
{
    m_StarColor = starColor;
    m_Verts.assign( pVerts, pVerts + nVerts );
    // Release any old VB so execute() rebuilds it on next call.
    if ( m_pVB ) { m_pVB->Release(); m_pVB = NULL; }
    return true;
}

bool PrimitiveStarBillboardD3D::initialize( const Vector3 & center,
                                            const Vector3 & right,
                                            const Vector3 & up,
                                            float           halfSize,
                                            const Vector3 & starColor )
{
    m_StarColor = starColor;
    m_Verts.clear();

    // Four corners of the billboard quad in world space.
    // Normal = zero (unused by PS_Star).
    // Color  = white fully opaque (0xFFFFFFFF = ARGB white).
    // UV     = [0,1] per corner — PS_Star remaps to ±2.5 disk coords.
    const unsigned white = 0xFFFFFFFF;

    Vector3 tl = center - right*halfSize + up*halfSize;
    Vector3 tr = center + right*halfSize + up*halfSize;
    Vector3 bl = center - right*halfSize - up*halfSize;
    Vector3 br = center + right*halfSize - up*halfSize;

    // Two CW triangles (viewed from camera, FrontCounterClockwise=FALSE)
    auto vl = [&](const Vector3& p, float u, float v) -> VL {
        VL vert;
        vert.px=p.x; vert.py=p.y; vert.pz=p.z;
        vert.nx=0.0f; vert.ny=0.0f; vert.nz=0.0f;
        vert.diffuse=white;
        vert.u=u; vert.v=v;
        return vert;
    };

    m_Verts.resize(6);
    m_Verts[0] = vl(tl, 0.0f, 0.0f);
    m_Verts[1] = vl(tr, 1.0f, 0.0f);
    m_Verts[2] = vl(br, 1.0f, 1.0f);
    m_Verts[3] = vl(tl, 0.0f, 0.0f);
    m_Verts[4] = vl(br, 1.0f, 1.0f);
    m_Verts[5] = vl(bl, 0.0f, 1.0f);

    return true;
}

bool PrimitiveStarBillboardD3D::execute()
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
        {
            TRACE("PrimitiveStarBillboardD3D::execute() buildVertexBuffer FAILED");
            return false;
        }
    }

    // Push spectral colour into CB0 so PS_Star can tint the surface.
    DirectX::XMFLOAT4 sc(m_StarColor.x, m_StarColor.y, m_StarColor.z, 1.0f);
    pDisplayD3D->m_pMatShader->setConstant("vStarColor", sc);

    // Bind VS_Star + PS_Star + VertexL input layout.
    TRACE("PrimitiveStarBillboardD3D::execute() calling applyStarSurface");
    if (!pDisplayD3D->m_pMatShader->applyStarSurface(pDisplayD3D))
    {
        TRACE("PrimitiveStarBillboardD3D::execute() applyStarSurface FAILED");
        return false;
    }

    UINT stride = (UINT)sizeof(VL);
    UINT offset = 0;
    pContext->IASetVertexBuffers(0, 1, &m_pVB, &stride, &offset);
    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pContext->Draw((UINT)m_Verts.size(), 0);

    TRACE(CharString().format("PrimitiveStarBillboardD3D: drew %d verts", (int)m_Verts.size()));
    return true;
}

void PrimitiveStarBillboardD3D::clear()
{}

void PrimitiveStarBillboardD3D::release()
{
    if (m_pVB) { m_pVB->Release(); m_pVB = NULL; }
    m_Verts.clear();
}

bool PrimitiveStarBillboardD3D::buildVertexBuffer( ID3D11Device * pDevice )
{
    if (!pDevice || m_Verts.empty()) return false;

    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth  = (UINT)(m_Verts.size() * sizeof(VL));
    bd.Usage      = D3D11_USAGE_IMMUTABLE;
    bd.BindFlags  = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA init = {};
    init.pSysMem = m_Verts.data();

    HRESULT hr = pDevice->CreateBuffer(&bd, &init, &m_pVB);
    if (FAILED(hr))
    {
        TRACE("PrimitiveStarBillboardD3D: CreateBuffer FAILED");
        return false;
    }
    return true;
}

// EOF
