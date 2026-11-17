/*
	PrimitiveTriangleStripD3D.cpp
	(c)2005 Palestar, Richard Lyle
	Updated to DirectX 11
*/

#include "Debug/Assert.h"
#include "DisplayD3D/PrimitiveTriangleStripD3D.h"
#include "DisplayD3D/PrimitiveFactory.h"

//------------------------------------------------------------------------------------

IMPLEMENT_PRIMITIVE_FACTORY( PrimitiveTriangleStripD3D );

PrimitiveTriangleStripD3D::PrimitiveTriangleStripD3D() : m_VB( NULL ), m_Verts( 0 )
{}

bool PrimitiveTriangleStripD3D::execute()
{
	int triangleCount = m_Verts - 2;
	if ( triangleCount > 0 && m_VB )
	{
		DisplayDeviceD3D* pDisplayD3D = (DisplayDeviceD3D*)m_pDevice;
		if (!pDisplayD3D || !pDisplayD3D->getContext())
			return false;

		ID3D11DeviceContext* pContext = pDisplayD3D->getContext();

		// Set vertex buffer
		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		pContext->IASetVertexBuffers(0, 1, &m_VB, &stride, &offset);

		// Set input layout
		if (pDisplayD3D->m_pInputLayouts[DisplayDeviceD3D::VB_VERTEX].Get())
			pContext->IASetInputLayout(pDisplayD3D->m_pInputLayouts[DisplayDeviceD3D::VB_VERTEX].Get());

		// Set primitive topology
		pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		// Draw
		pContext->Draw(m_Verts, 0);

		DisplayDevice::sm_nTrianglesRendered += triangleCount;
	}
	return true;
}

void PrimitiveTriangleStripD3D::clear()
{}

void PrimitiveTriangleStripD3D::release()
{
	RELEASEDD( m_VB );
	m_Verts = 0;
}

bool PrimitiveTriangleStripD3D::initialize( int vertexCount, const Vertex * pVerticies )
{
	RELEASEDD( m_VB );

	DisplayDeviceD3D* pDisplayD3D = (DisplayDeviceD3D*)m_pDevice;
	if (!pDisplayD3D || !pDisplayD3D->getDevice())
		return false;

	ID3D11Device* pDevice = pDisplayD3D->getDevice();

	m_Verts = vertexCount;

	// Create vertex buffer
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = m_Verts * sizeof(Vertex);
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = pVerticies;

	HRESULT hr;
	try { hr = pDevice->CreateBuffer(&bufferDesc, &initData, &m_VB); }
	catch (...) { hr = E_FAIL; m_VB = NULL; LOG_ERROR("PrimitiveTriangleStripD3D", "Exception VB #1"); }
	if (FAILED(hr)) { if (m_VB) m_VB = NULL; return false; }

	TRACE( CharString().format("PrimitiveTriangleStripD3D::initialize, m_Verts = %d", m_Verts ) );
	return true;
}

//----------------------------------------------------------------------------

IMPLEMENT_PRIMITIVE_FACTORY( PrimitiveTriangleStripLD3D );

PrimitiveTriangleStripLD3D::PrimitiveTriangleStripLD3D() : m_VB( NULL ), m_Verts( 0 )
{}

bool PrimitiveTriangleStripLD3D::execute()
{
	int triangleCount = m_Verts - 2;
	if ( triangleCount > 0 && m_VB )
	{
		DisplayDeviceD3D* pDisplayD3D = (DisplayDeviceD3D*)m_pDevice;
		if (!pDisplayD3D || !pDisplayD3D->getContext())
			return false;

		ID3D11DeviceContext* pContext = pDisplayD3D->getContext();

		// Set vertex buffer
		UINT stride = sizeof(VertexL);
		UINT offset = 0;
		pContext->IASetVertexBuffers(0, 1, &m_VB, &stride, &offset);

		// Set input layout
		if (pDisplayD3D->m_pInputLayouts[DisplayDeviceD3D::VB_VERTEXL].Get())
			pContext->IASetInputLayout(pDisplayD3D->m_pInputLayouts[DisplayDeviceD3D::VB_VERTEXL].Get());

		// Set primitive topology
		pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		// Draw
		pContext->Draw(m_Verts, 0);

		DisplayDevice::sm_nTrianglesRendered += triangleCount;
	}
	return true;
}

void PrimitiveTriangleStripLD3D::clear()
{}

void PrimitiveTriangleStripLD3D::release()
{
	RELEASEDD( m_VB );
	m_Verts = 0;
}

bool PrimitiveTriangleStripLD3D::initialize( int vertexCount, const VertexL * pVerticies )
{
	RELEASEDD( m_VB );

	DisplayDeviceD3D* pDisplayD3D = (DisplayDeviceD3D*)m_pDevice;
	if (!pDisplayD3D || !pDisplayD3D->getDevice())
		return false;

	ID3D11Device* pDevice = pDisplayD3D->getDevice();

	m_Verts = vertexCount;

	// Create vertex buffer
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = m_Verts * sizeof(VertexL);
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = pVerticies;

	HRESULT hr;
	try { hr = pDevice->CreateBuffer(&bufferDesc, &initData, &m_VB); }
	catch (...) { hr = E_FAIL; m_VB = NULL; LOG_ERROR("PrimitiveTriangleStripD3D", "Exception VB #2"); }
	if (FAILED(hr)) { if (m_VB) m_VB = NULL; return false; }

	TRACE( CharString().format("PrimitiveTriangleStripLD3D::initialize, m_Verts = %d", m_Verts ) );
	return true;
}

//----------------------------------------------------------------------------

IMPLEMENT_PRIMITIVE_FACTORY( PrimitiveTriangleStripDLD3D );

PrimitiveTriangleStripDLD3D::PrimitiveTriangleStripDLD3D() : m_pVerts( NULL ), m_VertexCount( 0 )
{}

bool PrimitiveTriangleStripDLD3D::execute()
{
	int triangleCount = m_VertexCount - 2;
	if ( triangleCount > 0 )
	{
		DisplayDeviceD3D* pDisplayD3D = (DisplayDeviceD3D*)m_pDevice;
		int vbOffset = pDisplayD3D->setupDVB( 
			DisplayDeviceD3D::VB_VERTEXL, m_VertexCount, m_pVerts );
		if ( vbOffset < 0 )
			return false;

		ID3D11DeviceContext* pContext = pDisplayD3D->getContext();
		if (!pContext)
			return false;

		// Switch to VS_MainL + VertexL input layout so per-vertex color is read correctly.
		// m_pInputLayouts[VB_VERTEXL] is an uninitialized stub — always use applyVertexL().
		if (pDisplayD3D->m_pMatShader.valid() && !pDisplayD3D->m_pMatShader->released())
			pDisplayD3D->m_pMatShader->applyVertexL(pDisplayD3D);

		// Set primitive topology
		pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		// Draw
		pContext->Draw(m_VertexCount, vbOffset);

		DisplayDevice::sm_nTrianglesRendered += triangleCount;
	}
	return true;
}

void PrimitiveTriangleStripDLD3D::clear()
{}

void PrimitiveTriangleStripDLD3D::release()
{
	delete [] m_pVerts;
	m_pVerts = NULL;

	m_VertexCount = 0;
}

bool PrimitiveTriangleStripDLD3D::initialize( int vertexCount )
{
	release();

	m_pVerts = new VertexL[ vertexCount ];
	m_VertexCount = vertexCount;
	return true;
}

bool PrimitiveTriangleStripDLD3D::initialize( int vertexCount, const VertexL * pVerticies )
{
	initialize( vertexCount );
	memcpy( m_pVerts, pVerticies, sizeof(VertexL) * vertexCount );
	return true;
}

VertexL * PrimitiveTriangleStripDLD3D::lock()
{
	return m_pVerts;
}

void PrimitiveTriangleStripDLD3D::unlock()
{}

//----------------------------------------------------------------------------

IMPLEMENT_PRIMITIVE_FACTORY( PrimitiveTriangleStripDTLD3D );

PrimitiveTriangleStripDTLD3D::PrimitiveTriangleStripDTLD3D() : m_pVerts( NULL ), m_VertexCount( 0 )
{}

bool PrimitiveTriangleStripDTLD3D::execute()
{
	int triangleCount = m_VertexCount - 2;
	if ( triangleCount > 0 )
	{
		DisplayDeviceD3D* pDisplayD3D = (DisplayDeviceD3D*)m_pDevice;
		int vbOffset = pDisplayD3D->setupDVB( 
			DisplayDeviceD3D::VB_VERTEXTL, m_VertexCount, m_pVerts );
		if ( vbOffset < 0 )
			return false;

		ID3D11DeviceContext* pContext = pDisplayD3D->getContext();
		if (!pContext)
			return false;

		// Apply UI shader for screen-space vertices
		if (!pDisplayD3D->applyUIShader())
			return false;

		// Set primitive topology
		pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		// Draw
		pContext->Draw(m_VertexCount, vbOffset);

		DisplayDevice::sm_nTrianglesRendered += triangleCount;
	}
	return true;
}

void PrimitiveTriangleStripDTLD3D::clear()
{}

void PrimitiveTriangleStripDTLD3D::release()
{
	delete [] m_pVerts;
	m_pVerts = NULL;

	m_VertexCount = 0;
}

bool PrimitiveTriangleStripDTLD3D::initialize( int vertexCount )
{
	release();

	m_pVerts = new VertexTL[ vertexCount ];
	m_VertexCount = vertexCount;
	return true;
}

bool PrimitiveTriangleStripDTLD3D::initialize( int vertexCount, const VertexTL * pVerticies )
{
	initialize( vertexCount );
	memcpy( m_pVerts, pVerticies, sizeof(VertexTL) * vertexCount );
	return true;
}

VertexTL * PrimitiveTriangleStripDTLD3D::lock()
{
	return m_pVerts;
}

void PrimitiveTriangleStripDTLD3D::unlock()
{}

//------------------------------------------------------------------------------------
// EOF


