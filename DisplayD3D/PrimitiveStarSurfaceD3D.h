/*
PrimitiveStarSurfaceD3D.h
D3D11 implementation of PrimitiveStarSurface.
Draws a tessellated UV sphere via VS_Tess + HS_Main + DS_Main + PS_StarSphere.
The sphere mesh is generated CPU-side; hardware tessellation refines it further.
No texture -- fully procedural stellar surface driven by sphere normals.

(c)2025 DarkSpace DX11 Port
*/

#ifndef PRIMITIVE_STAR_SURFACE_D3D_H
#define PRIMITIVE_STAR_SURFACE_D3D_H

#include "Display/PrimitiveStarSurface.h"
#include "DisplayD3D/DisplayDeviceD3D.h"
#include "Math/Vector3.h"
#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>

//----------------------------------------------------------------------------

class PrimitiveStarSurfaceD3D : public PrimitiveStarSurface
{
public:
// Construction / destruction
PrimitiveStarSurfaceD3D();

// DevicePrimitive interface
bool        execute();
void        clear();
void        release();

// PrimitiveStarSurface interface
bool        initialize( const Triangle * pTriangles, int nTriangles,
const Vector3 & starColor, bool bCoronaShell = false );

private:
// Helpers
bool        buildVertexBuffer( ID3D11Device * pDevice );

// Data
std::vector<Vertex>         m_Verts;       // flat vertex array (3 per triangle)
int                         m_nVerts;
Vector3                     m_StarColor;
bool                        m_bCoronaShell; // true -> use PS_CoronaShell instead of PS_StarSphere
ID3D11Buffer *              m_pVB;
};

//----------------------------------------------------------------------------

#endif // PRIMITIVE_STAR_SURFACE_D3D_H

//----------------------------------------------------------------------------
// EOF