/*
	PrimitiveLineStripD3D.cpp
	(c)2005 Palestar, Richard Lyle
	Updated to DirectX 11
*/

#include "Debug/Assert.h"
#include "DisplayD3D/PrimitiveLineStripD3D.h"
#include "DisplayD3D/PrimitiveFactory.h"

//------------------------------------------------------------------------------------

IMPLEMENT_PRIMITIVE_FACTORY( PrimitiveLineStripD3D );

PrimitiveLineStripD3D::PrimitiveLineStripD3D() : m_VB( NULL ), m_Verts( 0 )
{}

bool PrimitiveLineStripD3D::execute()
{
	int lineCount = m_Verts - 1;
	if ( lineCount > 0 && m_VB != NULL )
	{
		DisplayDeviceD3D* pDevice = (DisplayDeviceD3D*)m_pDevice;
		ID3D11DeviceContext* pContext = pDevice->getContext();
		if (!pContext)
			return false;

		// Set the vertex buffer
		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		pContext->IASetVertexBuffers(0, 1, &m_VB, &stride, &offset);

		// Set primitive topology
		pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

		// Draw the line strip
		pContext->Draw(m_Verts, 0);

		DisplayDevice::sm_nLinesRendered += lineCount;
	}

	return true;
}

void PrimitiveLineStripD3D::clear()
{}

void PrimitiveLineStripD3D::release()
{
	RELEASEDD( m_VB );
	m_Verts = 0;
}

bool PrimitiveLineStripD3D::initialize( int vertexCount, const Vertex * pVerticies )
{
	DisplayDeviceD3D* pDevice = (DisplayDeviceD3D*)m_pDevice;
	ID3D11Device* pD3DDevice = pDevice->getDevice();
	if (!pD3DDevice)
		return false;

	m_Verts = vertexCount;
	// release any previous vertex buffer
	RELEASEDD( m_VB );

	// Create vertex buffer
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = vertexCount * sizeof(Vertex);
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = pVerticies;

	HRESULT hr;
	try { hr = pD3DDevice->CreateBuffer(&bd, &initData, &m_VB); }
	catch (...) { hr = E_FAIL; m_VB = NULL; LOG_ERROR("PrimitiveLineStripD3D", "Exception VB Vertex"); }
	if (FAILED(hr)) { if (m_VB) m_VB = NULL; return false; }

	return true;
}

//----------------------------------------------------------------------------

IMPLEMENT_PRIMITIVE_FACTORY( PrimitiveLineStripLD3D );

PrimitiveLineStripLD3D::PrimitiveLineStripLD3D() : m_VB( NULL ), m_Verts( 0 )
{}

bool PrimitiveLineStripLD3D::execute()
{
	int lineCount = m_Verts - 1;
	if ( lineCount > 0 && m_VB != NULL )
	{
		DisplayDeviceD3D* pDevice = (DisplayDeviceD3D*)m_pDevice;
		ID3D11DeviceContext* pContext = pDevice->getContext();
		if (!pContext)
			return false;

		// Set the vertex buffer
		UINT stride = sizeof(VertexL);
		UINT offset = 0;
		pContext->IASetVertexBuffers(0, 1, &m_VB, &stride, &offset);

		// Set primitive topology
		pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

		// Draw the line strip
		pContext->Draw(m_Verts, 0);

		DisplayDevice::sm_nLinesRendered += lineCount;
	}

	return true;
}

void PrimitiveLineStripLD3D::clear()
{}

void PrimitiveLineStripLD3D::release()
{
	RELEASEDD( m_VB );
	m_Verts = 0;
}

bool PrimitiveLineStripLD3D::initialize( int vertexCount, const VertexL * pVerticies )
{
	DisplayDeviceD3D* pDevice = (DisplayDeviceD3D*)m_pDevice;
	ID3D11Device* pD3DDevice = pDevice->getDevice();
	if (!pD3DDevice)
		return false;

	m_Verts = vertexCount;
	// release any previous vertex buffer
	RELEASEDD( m_VB );

	// Create vertex buffer
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = vertexCount * sizeof(VertexL);
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = pVerticies;

	HRESULT hr;
	try { hr = pD3DDevice->CreateBuffer(&bd, &initData, &m_VB); }
	catch (...) { hr = E_FAIL; m_VB = NULL; LOG_ERROR("PrimitiveLineStripD3D", "Exception VB VertexL"); }
	if (FAILED(hr)) { if (m_VB) m_VB = NULL; return false; }

	return true;
}

//----------------------------------------------------------------------------

IMPLEMENT_PRIMITIVE_FACTORY( PrimitiveLineStripDLD3D );

PrimitiveLineStripDLD3D::PrimitiveLineStripDLD3D() : m_pVerts( NULL ), m_VertexCount( 0 )
{}

bool PrimitiveLineStripDLD3D::execute()
{
	int lineCount = m_VertexCount - 1;
	if ( lineCount > 0 )
	{
		DisplayDeviceD3D* pDevice = (DisplayDeviceD3D*)m_pDevice;
		ID3D11DeviceContext* pContext = pDevice->getContext();
		if (!pContext)
			return false;

		int vbOffset = pDevice->setupDVB( 
			DisplayDeviceD3D::VB_VERTEXL, m_VertexCount, m_pVerts );
		if ( vbOffset < 0 )
			return false;

		// For VertexL geometry, we MUST:
		// 1. Use VS_MainL (which passes through per-vertex COLOR)
		// 2. Set the correct input layout for VertexL (COLOR semantic)
		// 3. Apply the shader so constants are uploaded
		//
		// applyVertexL() on m_pMatShader activates VS_MainL and binds the correct layout.
		// This MUST be called every time VertexL geometry is rendered, or geometry will
		// render with the wrong shader (white) and/or wrong input layout.
		// Even if m_pMatShader is briefly null, we STILL call it to ensure consistency.
		if (pDevice->m_pMatShader.valid() && !pDevice->m_pMatShader->released())
		{
			pDevice->m_pMatShader->applyVertexL(pDevice);
		}
		else
		{
			// Fallback: if material shader is null, try to set the input layout directly
			// to avoid white geometry and GPU state corruption.
			// This indicates a material-pushing bug, but we guard against the symptom.
			if (pDevice->m_pInputLayouts[DisplayDeviceD3D::VB_VERTEXL].Get())
				pContext->IASetInputLayout(pDevice->m_pInputLayouts[DisplayDeviceD3D::VB_VERTEXL].Get());
		}

		// Set primitive topology
		pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

		// Draw the line strip
		pContext->Draw(m_VertexCount, vbOffset);

		DisplayDevice::sm_nLinesRendered += lineCount;
	}

	return true;
}

void PrimitiveLineStripDLD3D::clear()
{}

void PrimitiveLineStripDLD3D::release()
{
	delete [] m_pVerts;
	m_pVerts = NULL;

	m_VertexCount = 0;
}

bool PrimitiveLineStripDLD3D::initialize( int vertexCount )
{
	release();

	m_VertexCount = vertexCount;
	m_pVerts = new VertexL[ vertexCount ];
	return true;
}

bool PrimitiveLineStripDLD3D::initialize( int vertexCount, const VertexL * pVerticies )
{
	// allocate the buffer
	initialize( vertexCount );
	// copy in the new veritices
	memcpy( m_pVerts, pVerticies, sizeof(VertexL) * vertexCount );	
	return true;
}

VertexL * PrimitiveLineStripDLD3D::lock()
{
	return m_pVerts;
}

void PrimitiveLineStripDLD3D::unlock()
{}

//----------------------------------------------------------------------------

IMPLEMENT_PRIMITIVE_FACTORY( PrimitiveLineStripTLD3D );

PrimitiveLineStripTLD3D::PrimitiveLineStripTLD3D() : m_VB( NULL ), m_Verts( 0 )
{}

bool PrimitiveLineStripTLD3D::execute()
{
	int lineCount = m_Verts - 1;
	if ( lineCount > 0 && m_VB != NULL )
	{
		DisplayDeviceD3D* pDevice = (DisplayDeviceD3D*)m_pDevice;
		ID3D11DeviceContext* pContext = pDevice->getContext();
		if (!pContext)
			return false;

		// Apply UI shader for screen-space vertices
		if (!pDevice->applyUIShader())
			return false;

		// Set the vertex buffer
		UINT stride = sizeof(VertexTL);
		UINT offset = 0;
		pContext->IASetVertexBuffers(0, 1, &m_VB, &stride, &offset);

		// Set primitive topology
		pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

		// Draw the line strip
		pContext->Draw(m_Verts, 0);

		DisplayDevice::sm_nLinesRendered += lineCount;
	}

	return true;
}

void PrimitiveLineStripTLD3D::clear()
{}

void PrimitiveLineStripTLD3D::release()
{
	RELEASEDD( m_VB );
	m_Verts = 0;
}

bool PrimitiveLineStripTLD3D::initialize( int vertexCount, const VertexTL * pVerticies )
{
	DisplayDeviceD3D* pDevice = (DisplayDeviceD3D*)m_pDevice;
	ID3D11Device* pD3DDevice = pDevice->getDevice();
	if (!pD3DDevice)
		return false;

	m_Verts = vertexCount;
	// release any previous vertex buffer
	RELEASEDD( m_VB );

	// Create vertex buffer
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = vertexCount * sizeof(VertexTL);
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = pVerticies;

	HRESULT hr;
	try { hr = pD3DDevice->CreateBuffer(&bd, &initData, &m_VB); }
	catch (...) { hr = E_FAIL; m_VB = NULL; LOG_ERROR("PrimitiveLineStripD3D", "Exception VB VertexTL"); }
	if (FAILED(hr)) { if (m_VB) m_VB = NULL; return false; }

	return true;
}

//----------------------------------------------------------------------------

IMPLEMENT_PRIMITIVE_FACTORY( PrimitiveLineStripDTLD3D );

PrimitiveLineStripDTLD3D::PrimitiveLineStripDTLD3D() : m_pVerts( NULL ), m_VertexCount( 0 )
{}

bool PrimitiveLineStripDTLD3D::execute()
{
	int lineCount = m_VertexCount - 1;
	if ( lineCount > 0 )
	{
		DisplayDeviceD3D* pDevice = (DisplayDeviceD3D*)m_pDevice;
		ID3D11DeviceContext* pContext = pDevice->getContext();
		if (!pContext)
			return false;

		int vbOffset = pDevice->setupDVB( 
			DisplayDeviceD3D::VB_VERTEXTL, m_VertexCount, m_pVerts );
		if ( vbOffset < 0 )
			return false;

		// Apply UI shader for screen-space vertices
		if (!pDevice->applyUIShader())
			return false;

		// Set primitive topology
		pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

		// Draw the line strip
		pContext->Draw(m_VertexCount, vbOffset);

		DisplayDevice::sm_nLinesRendered += lineCount;
	}

	return true;
}

void PrimitiveLineStripDTLD3D::clear()
{}

void PrimitiveLineStripDTLD3D::release()
{
	delete [] m_pVerts;
	m_pVerts = NULL;

	m_VertexCount = 0;
}

bool PrimitiveLineStripDTLD3D::initialize( int vertexCount )
{
	release();

	m_VertexCount = vertexCount;
	m_pVerts = new VertexTL[ vertexCount ];
	return true;
}

bool PrimitiveLineStripDTLD3D::initialize( int vertexCount, const VertexTL * pVerticies )
{
	initialize( vertexCount );
	memcpy( m_pVerts, pVerticies, sizeof(VertexTL) * vertexCount );	
	return true;
}

VertexTL * PrimitiveLineStripDTLD3D::lock()
{
	return m_pVerts;
}

void PrimitiveLineStripDTLD3D::unlock()
{}

//------------------------------------------------------------------------------------
// EOF

//------------------------------------------------------------------------------------
// EOF


