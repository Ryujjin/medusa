/*
	PrimitiveTriangleListD3D.cpp
	(c)2005 Palestar, Richard Lyle
	Updated to DirectX 11
*/

#define USE_DRAW_INDEXED_PRIMITIVE		// undefine to use the slower DrawPrimitiveVB

#include <d3d11.h>
#include "Debug/Trace.h"
#include "DisplayD3D/DisplayDeviceD3D.h"
#include "DisplayD3D/PrimitiveTriangleListD3D.h"
#include "DisplayD3D/PrimitiveFactory.h"

// CRITICAL: Force TRACE output to Visual Studio Output window
#undef TRACE  
#define TRACE(msg) do { \
	OutputDebugStringA("[TRIANGLES] "); \
	OutputDebugStringA((const char*)(CharString() = msg)); \
	OutputDebugStringA("\n"); \
} while(0)

//------------------------------------------------------------------------------------

IMPLEMENT_PRIMITIVE_FACTORY( PrimitiveTriangleListD3D );

PrimitiveTriangleListD3D::PrimitiveTriangleListD3D() : m_VB( NULL ), m_VBSize( 0 )
{}

bool PrimitiveTriangleListD3D::execute()
{
	int triangleCount = m_VBSize / 3;
	if ( triangleCount > 0 && m_VB )
	{
		DisplayDeviceD3D* pDisplayD3D = (DisplayDeviceD3D*)m_pDevice;
		if (!pDisplayD3D || !pDisplayD3D->getContext())
			return false;

		ID3D11DeviceContext* pContext = pDisplayD3D->getContext();

		TRACE(CharString().format("PrimitiveTriangleListD3D::execute() - Drawing %d triangles (%d vertices)", 
			triangleCount, m_VBSize));

		// Set vertex buffer
		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		pContext->IASetVertexBuffers(0, 1, &m_VB, &stride, &offset);

		// Set input layout for Vertex type
		if (pDisplayD3D->m_pInputLayouts[DisplayDeviceD3D::VB_VERTEX].Get())
			pContext->IASetInputLayout(pDisplayD3D->m_pInputLayouts[DisplayDeviceD3D::VB_VERTEX].Get());

		// Set primitive topology
		pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Draw
		pContext->Draw(m_VBSize, 0);

		DisplayDevice::sm_nTrianglesRendered += triangleCount;
	}
	else
	{
		if (triangleCount > 0)
			TRACE("PrimitiveTriangleListD3D::execute() - WARNING: No vertex buffer!");
	}

	return true;
}

void PrimitiveTriangleListD3D::clear()
{}

void PrimitiveTriangleListD3D::release()
{
	if (m_VB) {
		m_VB->Release();
		m_VB = NULL;
	}
}

bool PrimitiveTriangleListD3D::initialize( int triangleCount, const Triangle * pTriangles )
{
	RELEASEDD(m_VB);

	DisplayDeviceD3D* pDisplayD3D = (DisplayDeviceD3D*)m_pDevice;
	if (!pDisplayD3D || !pDisplayD3D->getDevice())
		return false;

	ID3D11Device* pDevice = pDisplayD3D->getDevice();

	m_VBSize = triangleCount * 3;

	// Create vertex buffer
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = m_VBSize * sizeof(Vertex);
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = pTriangles;

	HRESULT hr;
	try { hr = pDevice->CreateBuffer(&bufferDesc, &initData, &m_VB); }
	catch (...) { hr = E_FAIL; m_VB = NULL; LOG_ERROR("PrimitiveTriangleListD3D", "Exception creating VB"); }
	if (FAILED(hr) && m_VB != NULL) m_VB = NULL;
	return SUCCEEDED(hr);
}

//----------------------------------------------------------------------------

IMPLEMENT_PRIMITIVE_FACTORY( PrimitiveTriangleListDD3D );

PrimitiveTriangleListDD3D::PrimitiveTriangleListDD3D() : m_pTriangles( NULL ), m_TriangleCount( 0 )
{}

bool PrimitiveTriangleListDD3D::execute()
{
	if (m_TriangleCount > 0)
	{
		DisplayDeviceD3D* pDisplayD3D = (DisplayDeviceD3D*)m_pDevice;
		if (!pDisplayD3D || !pDisplayD3D->getContext())
			return false;

		int vbOffset = pDisplayD3D->setupDVB(
			DisplayDeviceD3D::VB_VERTEX, m_TriangleCount * 3, m_pTriangles);
		if (vbOffset < 0)
			return false;

		ID3D11DeviceContext* pContext = pDisplayD3D->getContext();

		// Set input layout for Vertex type
		if (pDisplayD3D->m_pInputLayouts[DisplayDeviceD3D::VB_VERTEX].Get())
			pContext->IASetInputLayout(pDisplayD3D->m_pInputLayouts[DisplayDeviceD3D::VB_VERTEX].Get());

		// Set primitive topology
		pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Draw
		pContext->Draw(m_TriangleCount * 3, vbOffset);

		DisplayDevice::sm_nTrianglesRendered += m_TriangleCount;
	}

	return true;
}

void PrimitiveTriangleListDD3D::clear()
{}

void PrimitiveTriangleListDD3D::release()
{
	delete [] m_pTriangles;
	m_pTriangles = NULL;

	m_TriangleCount = 0;
}

bool PrimitiveTriangleListDD3D::initialize( int triangleCount )
{
	release();

	m_pTriangles = new Triangle[ triangleCount ];
	m_TriangleCount = triangleCount;
	return true;
}

bool PrimitiveTriangleListDD3D::initialize( int triangleCount, const Triangle * pTriangles )
{
	initialize( triangleCount );
	memcpy( m_pTriangles, pTriangles, sizeof(Triangle) * triangleCount );
	return true;
}

Triangle * PrimitiveTriangleListDD3D::lock()
{
	return m_pTriangles;
}

void PrimitiveTriangleListDD3D::unlock()
{}

//----------------------------------------------------------------------------

IMPLEMENT_PRIMITIVE_FACTORY( PrimitiveTriangleListID3D );

PrimitiveTriangleListID3D::PrimitiveTriangleListID3D() : m_VB( NULL ), m_VBI( NULL ), m_SystemVB( NULL )
{}

bool PrimitiveTriangleListID3D::execute()
{
	if ( m_Triangles > 0 && m_VB && m_VBI )
	{
		DisplayDeviceD3D* pDisplayD3D = (DisplayDeviceD3D *)m_pDevice;
		if (!pDisplayD3D || !pDisplayD3D->getContext())
			return false;

		ID3D11DeviceContext* pContext = pDisplayD3D->getContext();

		// Set vertex buffer
		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		pContext->IASetVertexBuffers(0, 1, &m_VB, &stride, &offset);

		// Set index buffer
		pContext->IASetIndexBuffer(m_VBI, DXGI_FORMAT_R16_UINT, 0);

		// Set input layout for Vertex type
		if (pDisplayD3D->m_pInputLayouts[DisplayDeviceD3D::VB_VERTEX].Get())
			pContext->IASetInputLayout(pDisplayD3D->m_pInputLayouts[DisplayDeviceD3D::VB_VERTEX].Get());

		// Set primitive topology
		pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Draw indexed
		pContext->DrawIndexed(m_Triangles * 3, 0, 0);

		DisplayDevice::sm_nTrianglesRendered += m_Triangles;
	}
	return true;
}

void PrimitiveTriangleListID3D::clear()
{}

void PrimitiveTriangleListID3D::release()
{
	if (m_VB) {
		m_VB->Release();
		m_VB = NULL;
	}
	if (m_SystemVB) {
		m_SystemVB->Release();
		m_SystemVB = NULL;
	}
	if (m_VBI) {
		m_VBI->Release();
		m_VBI = NULL;
	}
}

bool PrimitiveTriangleListID3D::initialize( int triangleCount, const Triangle * pTriangles )
{
	release();

	m_Triangles = triangleCount;

	// create the vertex array
	Array< Vertex > verts;
	// allocate the index array
	Array< word > vbi( m_Triangles * 3 );
	ASSERT( vbi.size() < 65536 );

	int index = 0;
	for(int i=0;i<triangleCount;i++)
		for(int j=0;j<3;j++)
		{
			const Vertex & vert = pTriangles[i].v[j];

			vbi[ index ] = 0xffff;

			// check the verts array for a vertex that matches
			for(int k=0;k<verts.size();k++)
				if ( (verts[k].position - vert.position).magnitude2() < 0.001f &&
					(verts[k].normal - vert.normal).magnitude2() < 0.001f &&
					fabs(verts[k].u - vert.u) < 0.001f &&
					fabs(verts[k].v - vert.v) < 0.001f )
				{
					// found matching vert
					vbi[ index ] = k;
					break;
				}

			if ( vbi[ index ] == 0xffff )
			{
				// add new unique vertex
				verts.push( vert );
				vbi[ index ] = verts.size() - 1;
			}

			// next vertex
			index++;
		}
	ASSERT( index == vbi.size() );

	m_Verts = verts.size();

	ID3D11Device* d3dDevice = ((DisplayDeviceD3D *)m_pDevice)->getDevice();

	// Create vertex buffer
	D3D11_BUFFER_DESC vbDesc = {};
	vbDesc.Usage = D3D11_USAGE_DEFAULT;
	vbDesc.ByteWidth = m_Verts * sizeof(Vertex);
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA vbInitData = {};
	vbInitData.pSysMem = verts.buffer();

	HRESULT hr;
	try { hr = d3dDevice->CreateBuffer(&vbDesc, &vbInitData, &m_VB); }
	catch (...) { hr = E_FAIL; m_VB = NULL; LOG_ERROR("PrimitiveTriangleListD3D", "Exception VB #1"); }
	if (FAILED(hr)) { if (m_VB) m_VB = NULL; return false; }

	// Create index buffer
	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.Usage = D3D11_USAGE_DEFAULT;
	ibDesc.ByteWidth = vbi.size() * sizeof(word);
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA ibInitData = {};
	ibInitData.pSysMem = vbi.buffer();

	// Reuse hr from above
	try { hr = d3dDevice->CreateBuffer(&ibDesc, &ibInitData, &m_VBI); }
	catch (...) { hr = E_FAIL; m_VBI = NULL; LOG_ERROR("PrimitiveTriangleListD3D", "Exception IB #1"); }
	if (FAILED(hr))
	{
		if (m_VBI) m_VBI = NULL;
		RELEASEDD(m_VB);
		return false;
	}

	return true;
}

bool PrimitiveTriangleListID3D::initialize( int triangleCount, const word * pTriangles,
								int vertexCount, const Vertex * pVerticies )
{
	release();

	ID3D11Device* d3dDevice = ((DisplayDeviceD3D *)m_pDevice)->getDevice();

	m_Triangles = triangleCount;
	m_Verts = vertexCount;

	// Create vertex buffer
	D3D11_BUFFER_DESC vbDesc = {};
	vbDesc.Usage = D3D11_USAGE_DEFAULT;
	vbDesc.ByteWidth = m_Verts * sizeof(Vertex);
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA vbInitData = {};
	vbInitData.pSysMem = pVerticies;

	HRESULT hr;
	try { hr = d3dDevice->CreateBuffer(&vbDesc, &vbInitData, &m_VB); }
	catch (...) { hr = E_FAIL; m_VB = NULL; LOG_ERROR("PrimitiveTriangleListD3D", "Exception VB #2"); }
	if (FAILED(hr)) { if (m_VB) m_VB = NULL; return false; }

	// Create index buffer
	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.Usage = D3D11_USAGE_DEFAULT;
	ibDesc.ByteWidth = m_Triangles * 3 * sizeof(word);
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA ibInitData = {};
	ibInitData.pSysMem = pTriangles;

	if (FAILED(d3dDevice->CreateBuffer(&ibDesc, &ibInitData, &m_VBI)))
	{
		RELEASEDD(m_VB);
		return false;
	}

	return true;
}

bool PrimitiveTriangleListID3D::initialize( int triangleCount, const word * pTriangles,
								PrimitiveTriangleListI * pVerts )
{
	ID3D11Device* d3dDevice = ((DisplayDeviceD3D *)m_pDevice)->getDevice();

	release();

	m_Triangles = triangleCount;
	m_Verts = ((PrimitiveTriangleListID3D *)pVerts)->m_Verts;

	// Share vertex buffer from pVerts
	m_VB = ((PrimitiveTriangleListID3D *)pVerts)->m_VB;
	if (m_VB)
		m_VB->AddRef();

	// Create index buffer
	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.Usage = D3D11_USAGE_DEFAULT;
	ibDesc.ByteWidth = m_Triangles * 3 * sizeof(word);
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA ibInitData = {};
	ibInitData.pSysMem = pTriangles;

	if (FAILED(d3dDevice->CreateBuffer(&ibDesc, &ibInitData, &m_VBI)))
	{
		RELEASEDD(m_VB);
		return false;
	}

	return true;
}

bool PrimitiveTriangleListID3D::createOptimizedVB( ID3D11Device* d3dDevice, int triangleCount, const word * pTriangles )
{
	ASSERT( m_SystemVB );

	m_Triangles = triangleCount;

	// Create optimized vertex buffer from system memory buffer
	D3D11_BUFFER_DESC vbDesc = {};
	vbDesc.Usage = D3D11_USAGE_DEFAULT;
	vbDesc.ByteWidth = m_Verts * sizeof(Vertex);
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;

	// Copy data from system VB
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	DisplayDeviceD3D* pDisplayD3D = (DisplayDeviceD3D*)m_pDevice;
	if (FAILED(pDisplayD3D->getContext()->Map(m_SystemVB, 0, D3D11_MAP_READ, 0, &mappedResource)))
		return false;

	D3D11_SUBRESOURCE_DATA vbInitData = {};
	vbInitData.pSysMem = mappedResource.pData;

	HRESULT hr = d3dDevice->CreateBuffer(&vbDesc, &vbInitData, &m_VB);

	pDisplayD3D->getContext()->Unmap(m_SystemVB, 0);

	return SUCCEEDED(hr);
}

//----------------------------------------------------------------------------

IMPLEMENT_PRIMITIVE_FACTORY( PrimitiveTriangleListLD3D );

PrimitiveTriangleListLD3D::PrimitiveTriangleListLD3D() : m_VB( NULL ), m_VBSize( 0 )
{}

bool PrimitiveTriangleListLD3D::execute()
{
	int triangleCount = m_VBSize / 3;
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

		// Set input layout for VertexL type
		if (pDisplayD3D->m_pInputLayouts[DisplayDeviceD3D::VB_VERTEXL].Get())
			pContext->IASetInputLayout(pDisplayD3D->m_pInputLayouts[DisplayDeviceD3D::VB_VERTEXL].Get());

		// Switch to VS_MainL so per-vertex color (diffuse + alpha) reaches the shader.
		// apply() binds VS_Main; applyVertexL() patches just the VS and input layout.
		if (pDisplayD3D->m_pMatShader.valid())
			pDisplayD3D->m_pMatShader->applyVertexL(pDisplayD3D);

		// Set primitive topology
		pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Draw
		pContext->Draw(m_VBSize, 0);

		DisplayDevice::sm_nTrianglesRendered += triangleCount;
	}
	return true;
}

void PrimitiveTriangleListLD3D::clear()
{}

void PrimitiveTriangleListLD3D::release()
{
	if (m_VB) {
		m_VB->Release();
		m_VB = NULL;
	}
	m_VBSize = 0;
}

bool PrimitiveTriangleListLD3D::initialize( int triangleCount, const TriangleL * pTriangles )
{
	ASSERT( triangleCount > 0 );

	release();

	DisplayDeviceD3D* pDisplayD3D = (DisplayDeviceD3D*)m_pDevice;
	if (!pDisplayD3D || !pDisplayD3D->getDevice())
		return false;

	ID3D11Device* pDevice = pDisplayD3D->getDevice();

	m_VBSize = triangleCount * 3;

	// Create vertex buffer
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = m_VBSize * sizeof(VertexL);
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = pTriangles;

	HRESULT hr = pDevice->CreateBuffer(&bufferDesc, &initData, &m_VB);
	return SUCCEEDED(hr);
}

//----------------------------------------------------------------------------

IMPLEMENT_PRIMITIVE_FACTORY( PrimitiveTriangleListDLD3D );

PrimitiveTriangleListDLD3D::PrimitiveTriangleListDLD3D() : m_pTriangles( NULL ), m_TriangleCount( 0 )
{}

bool PrimitiveTriangleListDLD3D::execute()
{
	if ( m_TriangleCount > 0 )
	{
		DisplayDeviceD3D* pDisplayD3D = (DisplayDeviceD3D*)m_pDevice;
		if (!pDisplayD3D || !pDisplayD3D->getContext())
			return false;

		int vbOffset = pDisplayD3D->setupDVB( 
			DisplayDeviceD3D::VB_VERTEXL, m_TriangleCount * 3, m_pTriangles );
		if ( vbOffset < 0 )
			return false;

		ID3D11DeviceContext* pContext = pDisplayD3D->getContext();

		// Set input layout for VertexL type
		if (pDisplayD3D->m_pInputLayouts[DisplayDeviceD3D::VB_VERTEXL].Get())
			pContext->IASetInputLayout(pDisplayD3D->m_pInputLayouts[DisplayDeviceD3D::VB_VERTEXL].Get());

		// Switch to VS_MainL so per-vertex color (diffuse + alpha) reaches the shader.
		// apply() binds VS_Main; applyVertexL() patches just the VS and input layout.
		if (pDisplayD3D->m_pMatShader.valid())
			pDisplayD3D->m_pMatShader->applyVertexL(pDisplayD3D);

		// Set primitive topology
		pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Draw with vbOffset from setupDVB
		pContext->Draw(m_TriangleCount * 3, vbOffset);

		DisplayDevice::sm_nTrianglesRendered += m_TriangleCount;
	}
	return true;
}

void PrimitiveTriangleListDLD3D::clear()
{}

void PrimitiveTriangleListDLD3D::release()
{
	delete [] m_pTriangles;
	m_pTriangles = NULL;

	m_TriangleCount = 0;
}

bool PrimitiveTriangleListDLD3D::initialize( int triangleCount )
{
	release();

	m_pTriangles = new TriangleL[ triangleCount ];
	m_TriangleCount = triangleCount;
	return true;
}

bool PrimitiveTriangleListDLD3D::initialize( int triangleCount, const TriangleL * pTriangles )
{
	initialize( triangleCount );
	memcpy( m_pTriangles, pTriangles, sizeof(TriangleL) * triangleCount );
	return true;
}

TriangleL * PrimitiveTriangleListDLD3D::lock()
{
	return m_pTriangles;
}

void PrimitiveTriangleListDLD3D::unlock()
{}

//----------------------------------------------------------------------------

IMPLEMENT_PRIMITIVE_FACTORY( PrimitiveTriangleListTLD3D );

PrimitiveTriangleListTLD3D::PrimitiveTriangleListTLD3D() : m_VB( NULL )
{}

bool PrimitiveTriangleListTLD3D::execute()
{
	int triangleCount = m_VBSize / 3;
	if ( triangleCount > 0 && m_VB )
	{
		DisplayDeviceD3D* pDisplayD3D = (DisplayDeviceD3D*)m_pDevice;
		if (!pDisplayD3D || !pDisplayD3D->getContext())
			return false;

		ID3D11DeviceContext* pContext = pDisplayD3D->getContext();

		// Apply UI shader for screen-space vertices
		if (!pDisplayD3D->applyUIShader())
			return false;

		// Set vertex buffer
		UINT stride = sizeof(VertexTL);
		UINT offset = 0;
		pContext->IASetVertexBuffers(0, 1, &m_VB, &stride, &offset);

		// Set primitive topology
		pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Draw
		pContext->Draw(m_VBSize, 0);

		DisplayDevice::sm_nTrianglesRendered += triangleCount;
	}
	return true;
}

void PrimitiveTriangleListTLD3D::clear()
{}

void PrimitiveTriangleListTLD3D::release()
{
	if (m_VB) {
		m_VB->Release();
		m_VB = NULL;
	}
}

bool PrimitiveTriangleListTLD3D::initialize( int triangleCount, const TriangleTL * pTriangles )
{
	release();

	DisplayDeviceD3D* pDisplayD3D = (DisplayDeviceD3D*)m_pDevice;
	if (!pDisplayD3D || !pDisplayD3D->getDevice())
		return false;

	ID3D11Device* pDevice = pDisplayD3D->getDevice();

	m_VBSize = triangleCount * 3;

	// Create vertex buffer
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = m_VBSize * sizeof(VertexTL);
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = pTriangles;

	HRESULT hr = pDevice->CreateBuffer(&bufferDesc, &initData, &m_VB);
	return SUCCEEDED(hr);
}

//----------------------------------------------------------------------------

IMPLEMENT_PRIMITIVE_FACTORY( PrimitiveTriangleListDTLD3D );

PrimitiveTriangleListDTLD3D::PrimitiveTriangleListDTLD3D() : m_pTriangles( NULL ), m_TriangleCount( 0 )
{}

bool PrimitiveTriangleListDTLD3D::execute()
{
	if ( m_TriangleCount > 0 )
	{
		DisplayDeviceD3D* pDisplayD3D = (DisplayDeviceD3D*)m_pDevice;
		if (!pDisplayD3D || !pDisplayD3D->getContext())
			return false;

		int vbOffset = pDisplayD3D->setupDVB( 
			DisplayDeviceD3D::VB_VERTEXTL, m_TriangleCount * 3, m_pTriangles );
		if ( vbOffset < 0 )
			return false;

		ID3D11DeviceContext* pContext = pDisplayD3D->getContext();

		// Apply UI shader for screen-space vertices
		if (!pDisplayD3D->applyUIShader())
			return false;

		// Set primitive topology
		pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Draw with vbOffset from setupDVB
		pContext->Draw(m_TriangleCount * 3, vbOffset);

		DisplayDevice::sm_nTrianglesRendered += m_TriangleCount;
	}
	return true;
}

void PrimitiveTriangleListDTLD3D::clear()
{}

void PrimitiveTriangleListDTLD3D::release()
{
	delete [] m_pTriangles;
	m_pTriangles = NULL;

	m_TriangleCount = 0;
}

bool PrimitiveTriangleListDTLD3D::initialize( int triangleCount )
{
	release();

	m_pTriangles = new TriangleTL[ triangleCount ];
	m_TriangleCount = triangleCount;
	return true;
}

bool PrimitiveTriangleListDTLD3D::initialize( int triangleCount, const TriangleTL * pTriangles )
{
	initialize( triangleCount );
	memcpy( m_pTriangles, pTriangles, sizeof(TriangleTL) * triangleCount );
	return true;
}

TriangleTL * PrimitiveTriangleListDTLD3D::lock()
{
	return m_pTriangles;
}

void PrimitiveTriangleListDTLD3D::unlock()
{}

//------------------------------------------------------------------------------------
// EOF


