/*
PrimitiveStarBillboardD3D.h
D3D11 implementation of PrimitiveStarBillboard.

Generates a camera-facing quad (two triangles, 6 VertexL vertices) in world
space and renders it through VS_Star + PS_Star so that the procedural corona,
K-corona halo, spike arms, solar flares, and CME filaments are visible around
the stellar disc.

UV layout: (0,0) top-left → (1,1) bottom-right, mapped to (-2.5,-2.5)→(+2.5,+2.5)
in PS_Star disk coords by:  uv = v.uv * 5.0 - 2.5

Vertex layout: VertexL (POSITION float3, NORMAL float3, COLOR DWORD, TEXCOORD float2)
Input layout: m_pInputLayoutL  (bound by applyStarSurface)

(c)2025 DarkSpace DX11 Port
*/

#ifndef PRIMITIVE_STAR_BILLBOARD_D3D_H
#define PRIMITIVE_STAR_BILLBOARD_D3D_H

#include "Display/PrimitiveStarBillboard.h"
#include "DisplayD3D/DisplayDeviceD3D.h"
#include "Math/Vector3.h"
#include <d3d11.h>
#include <vector>

//----------------------------------------------------------------------------

class PrimitiveStarBillboardD3D : public PrimitiveStarBillboard
{
public:
    PrimitiveStarBillboardD3D();

    // DevicePrimitive interface
    bool  execute();
    void  clear();
    void  release();

    // PrimitiveStarBillboard interface — camera-facing quad (original billboard)
    bool  initialize( const Vector3 & center,
                      const Vector3 & right,
                      const Vector3 & up,
                      float           halfSize,
                      const Vector3 & starColor );

    // Atmosphere sphere interface — arbitrary VertexL triangle mesh.
    // Caller builds the mesh (positions + camera-projected UVs) and passes
    // ownership of the array; initialize copies data into m_Verts.
    bool  initializeMesh( const Vertex * pVerts,
                          int            nVerts,
                          const Vector3 & starColor );

public:
    // VertexL layout: pos(12) + normal(12) + color(4) + uv(8) = 36 bytes
    // Aliased from PrimitiveStarBillboard::Vertex — kept here for internal use.
    typedef PrimitiveStarBillboard::Vertex VL;

private:
    bool  buildVertexBuffer( ID3D11Device * pDevice );

    std::vector<VL>  m_Verts;
    Vector3          m_StarColor;
    ID3D11Buffer *   m_pVB;
};

//----------------------------------------------------------------------------

#endif // PRIMITIVE_STAR_BILLBOARD_D3D_H

// EOF
