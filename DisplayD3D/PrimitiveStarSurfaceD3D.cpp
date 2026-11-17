/*
PrimitiveStarSurfaceD3D.cpp
D3D11 implementation -- renders a procedural stellar surface on a tessellated sphere.

Pipeline: VS_Tess -> HS_Main -> DS_Main -> PS_StarSphere (Default_D3D11.fx).
PS_StarSphere derives disk-space coordinates from the sphere normal and the
camera direction: r=0 at the camera-facing pole (disc centre), r=1 at the
silhouette (limb).  All surface effects (FBM turbulence, limb darkening,
chromosphere, corona, flares, CME wisps) work identically to PS_Star but
map correctly over the full 3-D sphere.

Color driven by vStarColor constant in CB0 (set by NodeRealStarField).
fTessFactor in CB0 controls hardware tessellation density (0 = max LOD from HS).

(c)2025 DarkSpace DX11 Port
*/

#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include "Math/Vector3.h"
#include "Display/Types.h"
#include "DisplayD3D/DisplayDeviceD3D.h"
#include "DisplayD3D/PrimitiveStarSurfaceD3D.h"
#include "DisplayD3D/PrimitiveFactory.h"
#include "Debug/Trace.h"

//----------------------------------------------------------------------------

IMPLEMENT_PRIMITIVE_FACTORY( PrimitiveStarSurfaceD3D );

PrimitiveStarSurfaceD3D::PrimitiveStarSurfaceD3D()
: m_nVerts(0)
, m_bCoronaShell(false)
, m_pVB(NULL)
{}

bool PrimitiveStarSurfaceD3D::initialize( const Triangle * pTriangles, int nTriangles,
   const Vector3 & starColor, bool bCoronaShell )
{
if (!pTriangles || nTriangles <= 0)
return false;

m_StarColor    = starColor;
m_bCoronaShell = bCoronaShell;
m_nVerts       = nTriangles * 3;
m_Verts.resize(m_nVerts);
for (int i = 0; i < nTriangles; ++i)
{
m_Verts[i*3+0] = pTriangles[i].v[0];
m_Verts[i*3+1] = pTriangles[i].v[1];
m_Verts[i*3+2] = pTriangles[i].v[2];
}
return true;
}

bool PrimitiveStarSurfaceD3D::execute()
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

// Push spectral colour into CB0.
DirectX::XMFLOAT4 sc(m_StarColor.x, m_StarColor.y, m_StarColor.z, 1.0f);
pDisplayD3D->m_pMatShader->setConstant("vStarColor", sc);

// Dispatch to the correct shader path.
if (m_bCoronaShell)
{
	TRACE("PrimitiveStarSurfaceD3D::execute() calling applyStarCoronaShell");
	if (!pDisplayD3D->m_pMatShader->applyStarCoronaShell(pDisplayD3D))
	{
		TRACE("PrimitiveStarSurfaceD3D::execute() applyStarCoronaShell FAILED");
		return false;
	}
}
else
{
	// Switch to VS_StarSphere + PS_StarSphere (3D sphere mesh, no tessellation).
	TRACE("PrimitiveStarSurfaceD3D::execute() calling applyStarSphere3D");
	if (!pDisplayD3D->m_pMatShader->applyStarSphere3D(pDisplayD3D))
	{
		TRACE("PrimitiveStarSurfaceD3D::execute() applyStarSphere3D FAILED");
		return false;
	}
}
TRACE(CharString().format("PrimitiveStarSurfaceD3D drawing %d verts", m_nVerts));
UINT stride = (UINT)sizeof(Vertex);
UINT offset = 0;
pContext->IASetVertexBuffers(0, 1, &m_pVB, &stride, &offset);
pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
pContext->Draw((UINT)m_nVerts, 0);

return true;
}

void PrimitiveStarSurfaceD3D::clear()
{}

void PrimitiveStarSurfaceD3D::release()
{
if (m_pVB) { m_pVB->Release(); m_pVB = NULL; }
m_Verts.clear();
m_nVerts = 0;
}

//----------------------------------------------------------------------------

bool PrimitiveStarSurfaceD3D::buildVertexBuffer( ID3D11Device * pDevice )
{
if (!pDevice || m_Verts.empty()) return false;

D3D11_BUFFER_DESC bd = {};
bd.ByteWidth  = (UINT)(m_Verts.size() * sizeof(Vertex));
bd.Usage      = D3D11_USAGE_IMMUTABLE;
bd.BindFlags  = D3D11_BIND_VERTEX_BUFFER;

D3D11_SUBRESOURCE_DATA init = {};
init.pSysMem = m_Verts.data();

HRESULT hr = pDevice->CreateBuffer(&bd, &init, &m_pVB);
if (FAILED(hr))
{
TRACE("PrimitiveStarSurfaceD3D: CreateBuffer failed.");
return false;
}
return true;
}

//----------------------------------------------------------------------------
// EOF