/*
	PrimitiveTriangleFanD3D.cpp
	(c)2005 Palestar, Richard Lyle
	Updated to DirectX 11
*/

#include "DisplayD3D/PrimitiveTriangleFanD3D.h"
#include "DisplayD3D/PrimitiveFactory.h"

//------------------------------------------------------------------------------------

IMPLEMENT_PRIMITIVE_FACTORY( PrimitiveTriangleFanD3D );

PrimitiveTriangleFanD3D::PrimitiveTriangleFanD3D() : m_VB( NULL ), m_Verts( 0 )
{}

bool PrimitiveTriangleFanD3D::execute()
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

		// Fan → triangle list via shared index buffer
		// Fan pattern: each triangle i = {V0, V[i+1], V[i+2]} (V0 is always the center)
		pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		if (pDisplayD3D->m_pFanIndexBuffer.Get())
		{
			pContext->IASetIndexBuffer(pDisplayD3D->m_pFanIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
			pContext->DrawIndexed(triangleCount * 3, 0, 0);
		}

		DisplayDevice::sm_nTrianglesRendered += triangleCount;
	}
	return true;
}

void PrimitiveTriangleFanD3D::clear()
{}

void PrimitiveTriangleFanD3D::release()
{
	RELEASEDD( m_VB );
	m_Verts = 0;
}

bool PrimitiveTriangleFanD3D::initialize( int vertexCount, const Vertex * pVerticies )
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
	catch (...) { hr = E_FAIL; m_VB = NULL; LOG_ERROR("PrimitiveTriangleFanD3D", "Exception VB #1"); }
	if (FAILED(hr)) { if (m_VB) m_VB = NULL; return false; }

	TRACE( CharString().format("PrimitiveTriangleFanD3D::initialize, m_Verts = %d", m_Verts ) );
	return true;
}

//----------------------------------------------------------------------------

IMPLEMENT_PRIMITIVE_FACTORY( PrimitiveTriangleFanDD3D );

PrimitiveTriangleFanDD3D::PrimitiveTriangleFanDD3D() : m_pVerts( NULL ), m_VertexCount( 0 )
{}

bool PrimitiveTriangleFanDD3D::execute()
{
	int triangleCount = m_VertexCount - 2;
	if ( triangleCount > 0 )
	{
		DisplayDeviceD3D* pDisplayD3D = (DisplayDeviceD3D*)m_pDevice;
		int vbOffset = pDisplayD3D->setupDVB( 
			DisplayDeviceD3D::VB_VERTEX, m_VertexCount, m_pVerts );
		if ( vbOffset < 0 )
			return false;

		ID3D11DeviceContext* pContext = pDisplayD3D->getContext();
		if (!pContext)
			return false;

		// Set input layout
		if (pDisplayD3D->m_pInputLayouts[DisplayDeviceD3D::VB_VERTEX].Get())
			pContext->IASetInputLayout(pDisplayD3D->m_pInputLayouts[DisplayDeviceD3D::VB_VERTEX].Get());

		// Fan → triangle list via shared index buffer
		pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		if (pDisplayD3D->m_pFanIndexBuffer.Get())
		{
			pContext->IASetIndexBuffer(pDisplayD3D->m_pFanIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
			pContext->DrawIndexed(triangleCount * 3, 0, vbOffset);
		}

		DisplayDevice::sm_nTrianglesRendered += triangleCount;
	}
	return true;
}

void PrimitiveTriangleFanDD3D::clear()
{}

void PrimitiveTriangleFanDD3D::release()
{
	delete [] m_pVerts;
	m_pVerts = NULL;
	m_VertexCount = 0;
}

bool PrimitiveTriangleFanDD3D::initialize( int vertexCount )
{
	release();

	m_VertexCount = vertexCount;
	m_pVerts = new Vertex[ vertexCount ];
	return true;
}

bool PrimitiveTriangleFanDD3D::initialize( int vertexCount, const Vertex * pVerticies )
{
	initialize( vertexCount );
	memcpy( m_pVerts, pVerticies, sizeof(Vertex) * vertexCount );
	return true;
}

Vertex * PrimitiveTriangleFanDD3D::lock()
{
	return m_pVerts;
}

void PrimitiveTriangleFanDD3D::unlock()
{}

//----------------------------------------------------------------------------

IMPLEMENT_PRIMITIVE_FACTORY( PrimitiveTriangleFanLD3D );

PrimitiveTriangleFanLD3D::PrimitiveTriangleFanLD3D() : m_VB( NULL ), m_Verts( 0 )
{}

bool PrimitiveTriangleFanLD3D::execute()
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

		// Fan → triangle list via shared index buffer
		pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		if (pDisplayD3D->m_pFanIndexBuffer.Get())
		{
			pContext->IASetIndexBuffer(pDisplayD3D->m_pFanIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
			pContext->DrawIndexed(triangleCount * 3, 0, 0);
		}

		DisplayDevice::sm_nTrianglesRendered += triangleCount;
	}
	return true;
}

void PrimitiveTriangleFanLD3D::clear()
{}

void PrimitiveTriangleFanLD3D::release()
{
	RELEASEDD( m_VB );
	m_Verts = 0;
}

bool PrimitiveTriangleFanLD3D::initialize( int vertexCount, const VertexL * pVerticies )
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
	catch (...) { hr = E_FAIL; m_VB = NULL; LOG_ERROR("PrimitiveTriangleFanD3D", "Exception VB #2"); }
	if (FAILED(hr)) { if (m_VB) m_VB = NULL; return false; }

	TRACE( CharString().format("PrimitiveTriangleFanLD3D::initialize, m_Verts = %d", m_Verts ) );
	return true;
}

//----------------------------------------------------------------------------

IMPLEMENT_PRIMITIVE_FACTORY( PrimitiveTriangleFanDLD3D );

PrimitiveTriangleFanDLD3D::PrimitiveTriangleFanDLD3D() : m_VertexCount( 0 ), m_pVerts( NULL )
{}

bool PrimitiveTriangleFanDLD3D::execute()
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
		// applyVertexL() must ALWAYS be called for VertexL geometry to ensure:
		// 1. VS_MainL (colored) is active instead of VS_Main (white)
		// 2. The correct input layout is set
		// 3. Shader constants are uploaded
		// Even if m_pMatShader is null, we must set the input layout to avoid corruption.
		if (pDisplayD3D->m_pMatShader.valid() && !pDisplayD3D->m_pMatShader->released())
		{
			pDisplayD3D->m_pMatShader->applyVertexL(pDisplayD3D);
		}
		else
		{
			// Fallback: if material shader is null, try to set the input layout directly
			// to avoid white geometry and GPU state corruption.
			if (pDisplayD3D->m_pInputLayouts[DisplayDeviceD3D::VB_VERTEXL].Get())
				pContext->IASetInputLayout(pDisplayD3D->m_pInputLayouts[DisplayDeviceD3D::VB_VERTEXL].Get());
		}

		// Fan → triangle list via shared index buffer
		pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		if (pDisplayD3D->m_pFanIndexBuffer.Get())
		{
			pContext->IASetIndexBuffer(pDisplayD3D->m_pFanIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
			pContext->DrawIndexed(triangleCount * 3, 0, vbOffset);
		}

		DisplayDevice::sm_nTrianglesRendered += triangleCount;
	}
	return true;
}

void PrimitiveTriangleFanDLD3D::clear()
{}

void PrimitiveTriangleFanDLD3D::release()
{
	delete [] m_pVerts;
	m_pVerts = NULL;
	m_VertexCount = 0;
}

bool PrimitiveTriangleFanDLD3D::initialize( int vertexCount )
{
	release();

	m_pVerts = new VertexL[ vertexCount ];
	m_VertexCount = vertexCount;
	return true;
}

bool PrimitiveTriangleFanDLD3D::initialize( int vertexCount, const VertexL * pVerticies )
{
	initialize( vertexCount );
	memcpy( m_pVerts, pVerticies, sizeof(VertexL) * vertexCount );
	return true;
}

VertexL * PrimitiveTriangleFanDLD3D::lock()
{
	return m_pVerts;
}

void PrimitiveTriangleFanDLD3D::unlock()
{}

//----------------------------------------------------------------------------

IMPLEMENT_PRIMITIVE_FACTORY( PrimitiveTriangleFanDTLD3D );

PrimitiveTriangleFanDTLD3D::PrimitiveTriangleFanDTLD3D() : m_VertexCount( 0 ), m_pVerts( NULL )
{}

bool PrimitiveTriangleFanDTLD3D::execute()
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

		// Fan → triangle list via shared index buffer
		pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		if (pDisplayD3D->m_pFanIndexBuffer.Get())
		{
			pContext->IASetIndexBuffer(pDisplayD3D->m_pFanIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
			pContext->DrawIndexed(triangleCount * 3, 0, vbOffset);
		}

		DisplayDevice::sm_nTrianglesRendered += triangleCount;
	}

	return true;
}

void PrimitiveTriangleFanDTLD3D::clear()
{}

void PrimitiveTriangleFanDTLD3D::release()
{
	delete [] m_pVerts;
	m_pVerts = NULL;
	m_VertexCount = 0;
}

bool PrimitiveTriangleFanDTLD3D::initialize( int vertexCount )
{
	release();

	m_pVerts = new VertexTL[ vertexCount ];
	m_VertexCount = vertexCount;
	return true;
}

bool PrimitiveTriangleFanDTLD3D::initialize( int vertexCount, const VertexTL * pVerticies )
{
	initialize( vertexCount );
	memcpy( m_pVerts, pVerticies, sizeof(VertexTL) * vertexCount );
	return true;
}

VertexTL * PrimitiveTriangleFanDTLD3D::lock()
{
	return m_pVerts;
}

void PrimitiveTriangleFanDTLD3D::unlock()
{}

//------------------------------------------------------------------------------------
// EOF


