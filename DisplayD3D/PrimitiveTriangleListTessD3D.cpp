/*
	PrimitiveTriangleListTessD3D.cpp
	D3D11 hardware-tessellated triangle list primitive.

	Draws triangles through the tessellation pipeline:
	  VS_Tess  : transforms vertices to world space (no SV_Position)
	  HS_Main  : calculates per-patch tessellation factor from camera distance
	  DS_Main  : interpolates + reprojects to clip space
	  PS_Main  : standard pixel shader (lighting, bump, etc.)

	Falls back silently if tessellation shaders are absent (shader file too old
	or fTessFactor == 0) — applyTessellation() returns false and the primitive
	skips drawing rather than crashing.

	(c)2025 DarkSpace DX11 Port
*/

#include <d3d11.h>
#include "Debug/Trace.h"
#include "Display/Types.h"
#include "DisplayD3D/DisplayDeviceD3D.h"
#include "DisplayD3D/PrimitiveTriangleListTessD3D.h"
#include "DisplayD3D/PrimitiveFactory.h"

//------------------------------------------------------------------------------------

IMPLEMENT_PRIMITIVE_FACTORY( PrimitiveTriangleListTessD3D );

PrimitiveTriangleListTessD3D::PrimitiveTriangleListTessD3D()
	: m_VB(NULL), m_VBSize(0)
{}

bool PrimitiveTriangleListTessD3D::execute()
{
	int triangleCount = m_VBSize / 3;
	if (triangleCount <= 0 || !m_VB)
		return true;

	DisplayDeviceD3D* pDisplayD3D = (DisplayDeviceD3D*)m_pDevice;
	if (!pDisplayD3D || !pDisplayD3D->getContext())
		return false;
	if (!pDisplayD3D->m_pMatShader.valid())
		return false;

	ID3D11DeviceContext* pContext = pDisplayD3D->getContext();

	// Set vertex buffer
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	pContext->IASetVertexBuffers(0, 1, &m_VB, &stride, &offset);

	// Apply tessellation pipeline (VS_Tess/HS_Main/DS_Main + PS_Main).
	// Returns false if tessellation shaders were not compiled — skip draw.
	if (!pDisplayD3D->m_pMatShader->applyTessellation(pDisplayD3D))
		return true;  // non-fatal: tessellation simply not available

	// 3-control-point patch list required for triangle tessellation.
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);

	pContext->Draw(m_VBSize, 0);

	DisplayDevice::sm_nTrianglesRendered += triangleCount;

	// Restore standard pipeline (clear HS/DS stages).
	pDisplayD3D->m_pMatShader->clearTessellation(pDisplayD3D);

	return true;
}

void PrimitiveTriangleListTessD3D::clear()
{}

void PrimitiveTriangleListTessD3D::release()
{
	if (m_VB)
	{
		m_VB->Release();
		m_VB = NULL;
	}
	m_VBSize = 0;
}

bool PrimitiveTriangleListTessD3D::initialize( int triangleCount, const Triangle * pTriangles )
{
	RELEASEDD(m_VB);
	m_VBSize = 0;

	if (triangleCount <= 0 || !pTriangles)
		return false;

	DisplayDeviceD3D* pDisplayD3D = (DisplayDeviceD3D*)m_pDevice;
	if (!pDisplayD3D || !pDisplayD3D->getDevice())
		return false;

	ID3D11Device* pDevice = pDisplayD3D->getDevice();

	int vertexCount = triangleCount * 3;
	m_VBSize        = vertexCount;

	D3D11_BUFFER_DESC bd   = {};
	bd.Usage               = D3D11_USAGE_DEFAULT;
	bd.ByteWidth           = (UINT)(vertexCount * sizeof(Vertex));
	bd.BindFlags           = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags      = 0;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = pTriangles;

	HRESULT hr = pDevice->CreateBuffer(&bd, &initData, &m_VB);
	if (FAILED(hr))
	{
		m_VB     = NULL;
		m_VBSize = 0;
		return false;
	}

	return true;
}
