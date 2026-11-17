/*
PrimitivePlanetAtmosphereD3D.cpp
D3D11 implementation of procedural planet atmosphere sphere rendering.

The atmosphere sphere is a UV sphere geometry positioned at ~1.2x planet radius.
It uses PS_Atmosphere shader (procedurally generated clouds, rim glow, soft terminator).

(c)2025 DarkSpace DX11 Port - Planet Atmosphere Feature
*/

#include "PrimitivePlanetAtmosphereD3D.h"
#include "Display/PrimitivePlanetAtmosphere.h"
#include "DisplayD3D/DisplayDeviceD3D.h"
#include "DisplayD3D/ShaderD3D11.h"
#include "Debug/Assert.h"
#include "Debug/Trace.h"
#include "Math/Vector3.h"
#include "Standard/StringHash64.h"
#include "Standard/Array.h"
#include "Standard/Color.h"
#include <d3d11.h>
#include <cmath>

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Vertex structure matching VS_MainL input layout
struct VertexL
{
	Vector3 position;       // POSITION (0-11)
	Vector3 normal;         // NORMAL (12-23)
	float u, v;             // TEXCOORD (24-31)
	Color diffuse;          // COLOR (32-35)
};

//----------------------------------------------------------------------------

PrimitivePlanetAtmosphereD3D::PrimitivePlanetAtmosphereD3D()
: m_pVertexBuffer( NULL ), m_pIndexBuffer( NULL ), m_pRasterizerStateNoCull( NULL ),
  m_nVertexCount( 0 ), m_nIndexCount( 0 ),
  m_fRadius( 1.2f ), m_fDensity( 0.6f ), m_fAnimSpeed( 1.0f ),
  m_vColor( 0.8f, 0.9f, 1.0f ), m_bBuffersGenerated( false )
{
}

PrimitivePlanetAtmosphereD3D::~PrimitivePlanetAtmosphereD3D()
{
release();
}

//----------------------------------------------------------------------------

bool PrimitivePlanetAtmosphereD3D::createNoCullRasterizerState( ID3D11Device * pDevice )
{
	if ( pDevice == NULL )
		return false;

	if ( m_pRasterizerStateNoCull != NULL )
		return true;  // Already created

	D3D11_RASTERIZER_DESC rastDesc;
	rastDesc.FillMode = D3D11_FILL_SOLID;
	// Cull the FAR hemisphere of the atmosphere shell, keeping only the near (camera-facing)
	// hemisphere rasterized. The sphere mesh is authored with the standard "CCW when viewed
	// from outside" winding convention (see ProceduralPlanetD3D.cpp), but this project's
	// rasterizer convention is FrontCounterClockwise=FALSE (front faces are CW in screen
	// space -- the same convention documented for billboard winding elsewhere). Under that
	// convention the NEAR hemisphere projects as CCW = back-face, so D3D11_CULL_BACK was
	// actually culling the correct near hemisphere and leaving only the far (inside-facing)
	// hemisphere rendered. That far hemisphere pokes out past the planet's true silhouette
	// at grazing view angles, producing the lopsided crescent/half-shell halo that changes
	// shape (and can vanish) as the camera orbits the planet. D3D11_CULL_FRONT culls the far
	// hemisphere instead, so only the correct near-facing shell is drawn.
	rastDesc.CullMode = D3D11_CULL_FRONT;
	rastDesc.FrontCounterClockwise = FALSE;
	rastDesc.DepthBias = 0;
	rastDesc.DepthBiasClamp = 0.0f;
	rastDesc.SlopeScaledDepthBias = 0.0f;
	rastDesc.DepthClipEnable = TRUE;
	rastDesc.ScissorEnable = FALSE;
	rastDesc.MultisampleEnable = TRUE;
	rastDesc.AntialiasedLineEnable = FALSE;

	HRESULT hr = pDevice->CreateRasterizerState( &rastDesc, m_pRasterizerStateNoCull.GetAddressOf() );
	if ( FAILED(hr) )
	{
		TRACE("PrimitivePlanetAtmosphereD3D::createNoCullRasterizerState() - Failed to create no-cull rasterizer state (HRESULT 0x%08X)\n", hr);
		return false;
	}

	TRACE("PrimitivePlanetAtmosphereD3D::createNoCullRasterizerState() - No-cull rasterizer state created successfully\n");
	return true;
}

bool PrimitivePlanetAtmosphereD3D::generateSphereMesh( ID3D11Device * pDevice )
{
	if ( pDevice == NULL )
		return false;

	// UV sphere with proper pole handling to avoid discontinuous seams
	// Topology: 2 pole vertices + latitude rings with seam vertices
	const int RINGS = 12;          // vertical divisions (latitude)
	const int SEGMENTS = 24;       // horizontal divisions (longitude)

	// Calculate vertex and index counts
	// Vertices: north pole (1) + rings (RINGS-1) * (SEGMENTS+1 with seam) + south pole (1)
	int vertexCount = 2 + (RINGS - 1) * (SEGMENTS + 1);
	int triangleCount = SEGMENTS * 2 + (RINGS - 2) * SEGMENTS * 2;  // pole caps + middle bands
	int indexCount = triangleCount * 3;

	// Allocate vertex data
	Array<VertexL> vertices;
	vertices.allocate( vertexCount );

	// Generate vertices
	int vertexIdx = 0;
	const float PI = 3.141592653589793f;
	const float TWO_PI = 2.0f * PI;

	// North pole vertex (top)
	{
		Vector3 pos( 0.0f, m_fRadius, 0.0f );
		Vector3 normal( 0.0f, 1.0f, 0.0f );
		VertexL & vert = vertices[ vertexIdx ];
		vert.position = pos;
		vert.normal = normal;
		vert.diffuse = Color( 255, 255, 255, 255 );
		vert.u = 0.5f;  // center of pole cap
		vert.v = 0.0f;
		vertexIdx++;
	}

	// Middle rings (latitude 1 to RINGS-1)
	for ( int ring = 1; ring < RINGS; ++ring )
	{
		float lat = PI * ring / RINGS;  // 0 < lat < PI (excludes poles)
		float sinLat = sinf( lat );
		float cosLat = cosf( lat );

		for ( int seg = 0; seg <= SEGMENTS; ++seg )  // NOTE: seg <= SEGMENTS to create seam vertex
		{
			float lon = TWO_PI * (seg % SEGMENTS) / SEGMENTS;  // Wrap seg=SEGMENTS back to 0
			float sinLon = sinf( lon );
			float cosLon = cosf( lon );

			// Position on unit sphere
			Vector3 pos(
				cosLon * sinLat,
				cosLat,
				sinLon * sinLat
			);

			// Scale by radius
			pos *= m_fRadius;

			// Normal (same as position direction on unit sphere)
			Vector3 normal(
				cosLon * sinLat,
				cosLat,
				sinLon * sinLat
			);

			// UV coordinates
			float u = (float)(seg % SEGMENTS) / SEGMENTS;
			float v = (float)ring / RINGS;

			// Create vertex
			VertexL & vert = vertices[ vertexIdx ];
			vert.position = pos;
			vert.normal = normal;
			vert.diffuse = Color( 255, 255, 255, 255 );
			vert.u = u;
			vert.v = v;

			vertexIdx++;
		}
	}

	// South pole vertex (bottom)
	{
		Vector3 pos( 0.0f, -m_fRadius, 0.0f );
		Vector3 normal( 0.0f, -1.0f, 0.0f );
		VertexL & vert = vertices[ vertexIdx ];
		vert.position = pos;
		vert.normal = normal;
		vert.diffuse = Color( 255, 255, 255, 255 );
		vert.u = 0.5f;  // center of pole cap
		vert.v = 1.0f;
		vertexIdx++;
	}

	// Allocate index data
	Array<word> indices;
	indices.allocate( indexCount );

	int indexIdx = 0;

	// North pole cap triangles
	{
		int northPoleIdx = 0;
		int firstRingStart = 1;
		for ( int seg = 0; seg < SEGMENTS; ++seg )
		{
			int a = northPoleIdx;
			int b = firstRingStart + seg;
			int c = firstRingStart + (seg + 1);  // No need for modulo now, we have seam vertex

			indices[ indexIdx++ ] = a;
			indices[ indexIdx++ ] = b;
			indices[ indexIdx++ ] = c;
		}
	}

	// Middle band triangles
	for ( int ring = 1; ring < RINGS - 1; ++ring )
	{
		int ringStartIdx = 1 + (ring - 1) * (SEGMENTS + 1);
		int nextRingStartIdx = 1 + ring * (SEGMENTS + 1);

		for ( int seg = 0; seg < SEGMENTS; ++seg )
		{
			int a = ringStartIdx + seg;
			int b = ringStartIdx + (seg + 1);  // Seam vertex at index SEGMENTS
			int c = nextRingStartIdx + seg;
			int d = nextRingStartIdx + (seg + 1);

			// First triangle (a-b-c)
			indices[ indexIdx++ ] = a;
			indices[ indexIdx++ ] = b;
			indices[ indexIdx++ ] = c;

			// Second triangle (b-d-c)
			indices[ indexIdx++ ] = b;
			indices[ indexIdx++ ] = d;
			indices[ indexIdx++ ] = c;
		}
	}

	// South pole cap triangles
	{
		int southPoleIdx = 1 + (RINGS - 1) * (SEGMENTS + 1);
		int lastRingStart = 1 + (RINGS - 2) * (SEGMENTS + 1);
		for ( int seg = 0; seg < SEGMENTS; ++seg )
		{
			int a = lastRingStart + seg;
			int b = lastRingStart + (seg + 1);  // Seam vertex
			int c = southPoleIdx;

			indices[ indexIdx++ ] = a;
			indices[ indexIdx++ ] = b;
			indices[ indexIdx++ ] = c;
		}
	}

	// Create vertex buffer
	D3D11_BUFFER_DESC vbDesc;
	vbDesc.ByteWidth = vertices.size() * sizeof(VertexL);
	vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;
	vbDesc.MiscFlags = 0;
	vbDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vbData;
	vbData.pSysMem = vertices.buffer();
	vbData.SysMemPitch = 0;
	vbData.SysMemSlicePitch = 0;

	HRESULT hr = pDevice->CreateBuffer( &vbDesc, &vbData, m_pVertexBuffer.GetAddressOf() );
	if ( FAILED(hr) )
	{
		TRACE( "PrimitivePlanetAtmosphereD3D: Failed to create vertex buffer (HRESULT 0x%08X)", hr );
		return false;
	}

	// Create index buffer
	D3D11_BUFFER_DESC ibDesc;
	ibDesc.ByteWidth = indices.size() * sizeof(word);
	ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.CPUAccessFlags = 0;
	ibDesc.MiscFlags = 0;
	ibDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA ibData;
	ibData.pSysMem = indices.buffer();
	ibData.SysMemPitch = 0;
	ibData.SysMemSlicePitch = 0;

	hr = pDevice->CreateBuffer( &ibDesc, &ibData, m_pIndexBuffer.GetAddressOf() );
	if ( FAILED(hr) )
	{
		TRACE( "PrimitivePlanetAtmosphereD3D: Failed to create index buffer (HRESULT 0x%08X)", hr );
		if ( m_pVertexBuffer != NULL )
		{
			m_pVertexBuffer->Release();
			m_pVertexBuffer = NULL;
		}
		return false;
	}

	m_nVertexCount = vertices.size();
	m_nIndexCount = indices.size();
	m_bBuffersGenerated = true;

	return true;
}

bool PrimitivePlanetAtmosphereD3D::execute()
{
	// Get the DisplayDevice - m_pDevice is set by the display pipeline
	if ( m_pDevice == NULL )
		return true;  // non-fatal, silently skip rendering

	DisplayDeviceD3D * pDisplay = (DisplayDeviceD3D *)m_pDevice;
	ID3D11Device * pD3D = pDisplay->getDevice();
	ID3D11DeviceContext * pCtx = pDisplay->getContext();

	if ( !pD3D || !pCtx )
		return true;  // non-fatal

	// Generate sphere mesh on first execute (lazy initialization)
	if ( !m_bBuffersGenerated )
	{
		if ( !generateSphereMesh( pD3D ) )
		{
			TRACE("PrimitivePlanetAtmosphereD3D::execute() - Failed to generate sphere mesh\n");
			return true;  // non-fatal: atmosphere won't render but game continues
		}
		TRACE("PrimitivePlanetAtmosphereD3D::execute() - Sphere mesh generated successfully (radius=%.2f, vertices=%d, indices=%d)\n", 
			m_fRadius, m_nVertexCount, m_nIndexCount);
	}

	// Create no-cull rasterizer state on first execute
	if ( m_pRasterizerStateNoCull == NULL )
	{
		if ( !createNoCullRasterizerState( pD3D ) )
		{
			TRACE("PrimitivePlanetAtmosphereD3D::execute() - Failed to create no-cull rasterizer state\n");
			return true;  // non-fatal, will use default culling (less-than-ideal but not a crash)
		}
	}

	// Sanity check: do we have valid geometry?
	if ( m_pVertexBuffer == NULL || m_pIndexBuffer == NULL || m_nIndexCount == 0 )
		return true;

	TRACE("PrimitivePlanetAtmosphereD3D::execute() - Drawing atmosphere sphere (indices=%d)\n", m_nIndexCount);

	// Set vertex buffer and input layout (VertexL)
	UINT stride = sizeof(VertexL);
	UINT offset = 0;
	pCtx->IASetVertexBuffers( 0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset );
	pCtx->IASetIndexBuffer( m_pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0 );
	pCtx->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	// Set input layout for VertexL (if available)
	if ( pDisplay->m_pInputLayouts[DisplayDeviceD3D::VB_VERTEXL] != NULL )
	{
		pCtx->IASetInputLayout( pDisplay->m_pInputLayouts[DisplayDeviceD3D::VB_VERTEXL].Get() );
	}

	// Apply the VS_Main + PS_Atmosphere shader pair from the shared material shader
	// object (mirrors the pattern used by PrimitiveStarSurfaceD3D / applyPlanetSurface).
	if ( !pDisplay->m_pMatShader || !pDisplay->m_pMatShader->applyAtmosphere( pDisplay ) )
	{
		TRACE("PrimitivePlanetAtmosphereD3D::execute() - Failed to apply PS_Atmosphere shader\n");
		return true;  // non-fatal: fall through and return true anyway
	}

	TRACE("PrimitivePlanetAtmosphereD3D::execute() - Shader applied successfully\n");

	// Set ADDITIVE blend state (atmosphere should be additive to avoid harsh boundaries)
	// The display device has m_pBlendStateAdditive available
	if ( pDisplay->m_pBlendStateAdditive )
	{
		float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		pCtx->OMSetBlendState( pDisplay->m_pBlendStateAdditive.Get(), blendFactor, 0xFFFFFFFF );
		TRACE("PrimitivePlanetAtmosphereD3D::execute() - Additive blend state applied\n");
	}

	// Depth TEST only, no depth WRITE. The atmosphere is a translucent additive shell —
	// writing depth here would incorrectly occlude the procedural surface / other overlay
	// passes that render after it (or be occluded by them), producing a hollow halo ring
	// instead of a smooth blended shell. This matches the depth policy used by every other
	// additive overlay primitive (procedural planet surface, star corona).
	if ( pDisplay->m_pDepthStencilTestOnly )
	{
		pCtx->OMSetDepthStencilState( pDisplay->m_pDepthStencilTestOnly.Get(), 0 );
		TRACE("PrimitivePlanetAtmosphereD3D::execute() - Depth test-only state applied (no write)\n");
	}

	// Atmosphere should render across the full sphere (including back side).
	// Disable back-face culling so we render ALL vertices (including those facing away from camera).
	if ( m_pRasterizerStateNoCull )
	{
		pCtx->RSSetState( m_pRasterizerStateNoCull.Get() );
		TRACE("PrimitivePlanetAtmosphereD3D::execute() - No-cull rasterizer state applied\n");
	}

	// Issue the draw call for the sphere mesh
	pCtx->DrawIndexed( m_nIndexCount, 0, 0 );
	TRACE("PrimitivePlanetAtmosphereD3D::execute() - DrawIndexed call issued\n");

	return true;
}

void PrimitivePlanetAtmosphereD3D::clear()
{
m_nVertexCount = 0;
m_nIndexCount = 0;
m_bBuffersGenerated = false;
}

void PrimitivePlanetAtmosphereD3D::release()
{
	// Smart pointers (D3D11BufferPtr, D3D11RasterizerStatePtr) automatically release on assignment
	m_pVertexBuffer = NULL;
	m_pIndexBuffer = NULL;
	m_pRasterizerStateNoCull = NULL;
	clear();
}

//----------------------------------------------------------------------------

void PrimitivePlanetAtmosphereD3D::setRadius( float fRadius )
{
m_fRadius = fRadius > 0.0f ? fRadius : 1.2f;
}

void PrimitivePlanetAtmosphereD3D::setDensity( float fDensity )
{
m_fDensity = fDensity < 0.0f ? 0.0f : (fDensity > 1.0f ? 1.0f : fDensity);
}

void PrimitivePlanetAtmosphereD3D::setColor( const Vector3 & vColor )
{
m_vColor = vColor;
}

void PrimitivePlanetAtmosphereD3D::setAnimSpeed( float fAnimSpeed )
{
m_fAnimSpeed = fAnimSpeed < 0.0f ? 0.0f : fAnimSpeed;
}

float PrimitivePlanetAtmosphereD3D::getRadius() const
{
return m_fRadius;
}

float PrimitivePlanetAtmosphereD3D::getDensity() const
{
return m_fDensity;
}

Vector3 PrimitivePlanetAtmosphereD3D::getColor() const
{
return m_vColor;
}

float PrimitivePlanetAtmosphereD3D::getAnimSpeed() const
{
return m_fAnimSpeed;
}

//----------------------------------------------------------------------------
// Factory

Reference<DevicePrimitive> PrimitivePlanetAtmosphereD3D::create( DisplayDevice * pDisplay,
float fRadius, const Vector3 & vColor, float fDensity, float fAnimSpeed )
{
if ( pDisplay == NULL )
return NULL;

PrimitivePlanetAtmosphereD3D * pAtmosphere = new PrimitivePlanetAtmosphereD3D();
pAtmosphere->setRadius( fRadius );
pAtmosphere->setDensity( fDensity );
pAtmosphere->setColor( vColor );
pAtmosphere->setAnimSpeed( fAnimSpeed );

return Reference<DevicePrimitive>( pAtmosphere );
}

//----------------------------------------------------------------------------
// Global factory registration

namespace {
	Reference<DevicePrimitive> createPrimitivePlanetAtmosphere(
		DisplayDevice * pDisplay, float fRadius, const Vector3 & vColor,
		float fDensity, float fAnimSpeed )
	{
		return PrimitivePlanetAtmosphereD3D::create( pDisplay, fRadius, vColor, fDensity, fAnimSpeed );
	}
}

// Abstract factory implementation
Reference<DevicePrimitive> PrimitivePlanetAtmosphere::create( DisplayDevice * pDisplay,
	float fRadius, const Vector3 & vColor, float fDensity, float fAnimSpeed )
{
	return createPrimitivePlanetAtmosphere( pDisplay, fRadius, vColor, fDensity, fAnimSpeed );
}

//----------------------------------------------------------------------------
// C interface for GameContext (DarkSpace layer)
// Exported function for creating atmosphere primitives from game code.
// extern "C" prevents name mangling so the symbol can be found by DarkSpace at link time.

extern "C" __declspec(dllexport) void * createAtmosphereFromGameContext( DisplayDevice * pDisplay,
	float fRadius, const Vector3 & vColor, float fDensity, float fAnimSpeed )
{
	if ( pDisplay == NULL )
		return NULL;

	Reference<DevicePrimitive> pAtmosphere = PrimitivePlanetAtmosphereD3D::create(
		pDisplay, fRadius, vColor, fDensity, fAnimSpeed );

	if ( pAtmosphere.valid() )
		return pAtmosphere.pointer();

	return NULL;
}

//----------------------------------------------------------------------------
// DevicePrimitive registration

IMPLEMENT_DEVICE_PRIMITIVE( PrimitivePlanetAtmosphereD3D );
