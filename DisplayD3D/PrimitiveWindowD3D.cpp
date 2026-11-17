/*
	PrimitiveWindowD3D.cpp
	(c)2005 Palestar, Richard Lyle
	Updated to DirectX 11
*/

#include "DisplayD3D/PrimitiveWindowD3D.h"
#include "DisplayD3D/PrimitiveFactory.h"
#include <stdio.h>   // for sprintf_s in diagnostics

//------------------------------------------------------------------------------------

// CRITICAL FIX: Override primitiveKey() to return BASE CLASS key!
const PrimitiveKey& PrimitiveWindowD3D::staticPrimitiveKey()
{
	static PrimitiveKey KEY(StringHash64("PrimitiveWindowD3D"));
	return KEY;
}
const PrimitiveKey& PrimitiveWindowD3D::primitiveKey()
{
	return PrimitiveWindow::staticPrimitiveKey();  // Return BASE CLASS key!
}

IMPLEMENT_PRIMITIVE_FACTORY( PrimitiveWindowD3D );

PrimitiveWindowD3D::PrimitiveWindowD3D()
{}

//----------------------------------------------------------------------------

bool PrimitiveWindowD3D::execute()
{
	DisplayDeviceD3D* pDevice = (DisplayDeviceD3D*)m_pDevice;
	ID3D11DeviceContext* pContext = pDevice->getContext();
	if (!pContext)
		return false;

	int vbOffset = pDevice->setupDVB( 
		DisplayDeviceD3D::VB_VERTEXTL, 4, m_Quad );
	if ( vbOffset < 0 )
		return false;

	// Apply UI shader for screen-space vertices.
	// If the shader is not loaded (e.g. missing .fx file), skip the draw entirely.
	// Drawing with the 3D Default shader active produces garbage geometry from
	// screen-space VertexTL data passing through world/view/projection transforms.
	if (!pDevice->applyUIShader())
		return false;

	// Set primitive topology for triangle strip (quad = 2 triangles, 4 vertices)
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// Draw 4 vertices as triangle strip (use vbOffset from dynamic VB)
	pContext->Draw(4, vbOffset);

	DisplayDevice::sm_nTrianglesRendered += 2;
	return true;
}

void PrimitiveWindowD3D::clear()
{}

void PrimitiveWindowD3D::release()
{}

//----------------------------------------------------------------------------

#undef RGB

bool PrimitiveWindowD3D::initialize( const RectInt & window, const RectFloat & uv, Color diffuse )
{
	// initialize the D3DTLVERTEX array in zig-zag order for TRIANGLE_STRIP
	// Order: TL, TR, BL, BR (not fan order TL,TR,BR,BL)
	m_Quad[ 0 ].position.x = window.left;		m_Quad[ 0 ].position.y = window.top;
	m_Quad[ 1 ].position.x = window.right;		m_Quad[ 1 ].position.y = window.top;
	m_Quad[ 2 ].position.x = window.left;		m_Quad[ 2 ].position.y = window.bottom;
	m_Quad[ 3 ].position.x = window.right;		m_Quad[ 3 ].position.y = window.bottom;
	m_Quad[ 0 ].position.z = m_Quad[ 1 ].position.z = m_Quad[ 2 ].position.z = m_Quad[ 3 ].position.z = 0.0f;

	m_Quad[ 0 ].u = uv.left;			m_Quad[ 0 ].v = uv.top;
	m_Quad[ 1 ].u = uv.right;			m_Quad[ 1 ].v = uv.top;
	m_Quad[ 2 ].u = uv.left;			m_Quad[ 2 ].v = uv.bottom;
	m_Quad[ 3 ].u = uv.right;			m_Quad[ 3 ].v = uv.bottom;

	m_Quad[ 0 ].w = m_Quad[ 1 ].w = m_Quad[ 2 ].w = m_Quad[ 3 ].w = 1.0f;
	m_Quad[ 0 ].diffuse = m_Quad[ 1 ].diffuse = m_Quad[ 2 ].diffuse = m_Quad[ 3 ].diffuse = diffuse;
	return true;
}

//------------------------------------------------------------------------------------
// EOF

