/*
	ProceduralPlanetD3D.cpp

	Sphere generation: UV-mapped sphere with rings × segments topology.
	North pole at (0, 1, 0), equator at y=0, south pole at (0, -1, 0).
	Normals point outward from sphere center.
*/

#define DISPLAYD3D_DLL

#include "DisplayD3D/ProceduralPlanetD3D.h"
#include "DisplayD3D/DisplayDeviceD3D.h"
#include "DisplayD3D/PrimitiveFactory.h"
#include "Debug/Trace.h"
#include <cmath>

//----------------------------------------------------------------------------

IMPLEMENT_PRIMITIVE_FACTORY(PrimitiveProceduralPlanetD3D);

static const float PI_F = 3.14159265358979f;

//----------------------------------------------------------------------------

PrimitiveProceduralPlanetD3D::PrimitiveProceduralPlanetD3D()
	: m_nRings(32), m_nSegments(64), m_fRadius(1.0f)
	, m_fTerrainScale(0.1f), m_nOctaves(4)
	, m_vColor(1.0f, 1.0f, 1.0f)
	, m_fControlPct(0.0f), m_fTechLevel(0.0f)
	, m_fAtmosphereDensity(0.5f), m_fCloudPhase(0.0f)
	, m_vCityLightColor(1.0f, 0.9f, 0.7f), m_fCityLightIntensity(0.0f)
	, m_nVerts(0), m_nIndices(0)
	, m_pVB(NULL), m_pIB(NULL)
{
}

PrimitiveProceduralPlanetD3D::~PrimitiveProceduralPlanetD3D()
{
	release();
}

//----------------------------------------------------------------------------

bool PrimitiveProceduralPlanetD3D::initialize(int rings, int segments, float radius)
{
	if (rings < 3 || segments < 3 || radius <= 0.0f)
		return false;

	m_nRings = rings;
	m_nSegments = segments;
	m_fRadius = radius;

	return generateSphere();
}

void PrimitiveProceduralPlanetD3D::setTerrainParams(float terrainScale, int octaves)
{
	m_fTerrainScale = terrainScale;
	m_nOctaves = (octaves > 0) ? octaves : 1;
}

void PrimitiveProceduralPlanetD3D::setColor(const Vector3& color)
{
	m_vColor = color;
}

void PrimitiveProceduralPlanetD3D::setPlanetParams(
	float controlPct, float techLevel,
	float atmosphereDensity, float cloudPhase,
	const Vector3& cityLightColor, float cityLightIntensity)
{
	m_fControlPct        = controlPct;
	m_fTechLevel         = techLevel;
	m_fAtmosphereDensity = atmosphereDensity;
	m_fCloudPhase        = cloudPhase;
	m_vCityLightColor    = cityLightColor;
	m_fCityLightIntensity = cityLightIntensity;
}

//----------------------------------------------------------------------------

bool PrimitiveProceduralPlanetD3D::generateSphere()
{
	m_Verts.clear();
	m_Indices.clear();

	// Generate vertices: (rings + 1) × (segments + 1) vertices
	// Ring 0 = north pole, Ring rings = south pole
	for (int ring = 0; ring <= m_nRings; ++ring) {
		float v = (float)ring / (float)m_nRings;  // 0 at north pole, 1 at south pole
		float phi = v * PI_F;                      // 0 (top) to π (bottom)

		float sinPhi = sinf(phi);
		float cosPhi = cosf(phi);

		for (int seg = 0; seg <= m_nSegments; ++seg) {
			float u = (float)seg / (float)m_nSegments;  // 0 to 1 around azimuth
			float theta = u * 2.0f * PI_F;              // 0 to 2π

			float sinTheta = sinf(theta);
			float cosTheta = cosf(theta);

			// Unit sphere point (normal direction)
			// Standard spherical coordinates: X right, Y up, Z toward viewer
			Vector3 normal(
				sinPhi * cosTheta,
				cosPhi,
				sinPhi * sinTheta
			);

			// Position = normal × radius (push out 0.5% to avoid z-fighting with art mesh)
			Vector3 position = normal * (m_fRadius * 1.005f);

			Vertex vert;
			vert.position = position;
			vert.normal   = normal;  // Already unit length
			vert.u        = u;       // Azimuth [0..1]
			vert.v        = v;       // Polar   [0..1]

			m_Verts.push_back(vert);
		}
	}

	m_nVerts = (int)m_Verts.size();

	// Generate indices: Quads become 2 triangles each
	// Winding: Counter-clockwise from above (CCW when viewed from outside)
	for (int ring = 0; ring < m_nRings; ++ring) {
		for (int seg = 0; seg < m_nSegments; ++seg) {
			// Quad vertices (in ring-major order)
			int a = ring * (m_nSegments + 1) + seg;
			int b = a + 1;
			int c = (ring + 1) * (m_nSegments + 1) + seg;
			int d = c + 1;

			// First triangle (a, b, c) - CCW from outside
			m_Indices.push_back(a);
			m_Indices.push_back(b);
			m_Indices.push_back(c);

			// Second triangle (b, d, c) - CCW from outside
			m_Indices.push_back(b);
			m_Indices.push_back(d);
			m_Indices.push_back(c);
		}
	}

	m_nIndices = (int)m_Indices.size();

	TRACE(CharString().format("ProceduralPlanetD3D: Generated sphere with %d vertices, %d triangles, %d indices",
		m_nVerts, m_nIndices / 3, m_nIndices));

	return true;
}

//----------------------------------------------------------------------------

bool PrimitiveProceduralPlanetD3D::buildVertexBuffer(ID3D11Device* pDevice)
{
	if (!pDevice || m_Verts.empty()) {
		TRACE("ProceduralPlanetD3D::buildVertexBuffer: Invalid arguments");
		return false;
	}

	D3D11_BUFFER_DESC bd = {};
	bd.ByteWidth = (UINT)(m_Verts.size() * sizeof(Vertex));
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA init = {};
	init.pSysMem = m_Verts.data();

	HRESULT hr = pDevice->CreateBuffer(&bd, &init, &m_pVB);
	if (FAILED(hr)) {
		TRACE("ProceduralPlanetD3D::buildVertexBuffer: CreateBuffer failed");
		return false;
	}

	return true;
}

bool PrimitiveProceduralPlanetD3D::buildIndexBuffer(ID3D11Device* pDevice)
{
	if (!pDevice || m_Indices.empty()) {
		TRACE("ProceduralPlanetD3D::buildIndexBuffer: Invalid arguments");
		return false;
	}

	D3D11_BUFFER_DESC bd = {};
	bd.ByteWidth = (UINT)(m_Indices.size() * sizeof(unsigned int));
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA init = {};
	init.pSysMem = m_Indices.data();

	HRESULT hr = pDevice->CreateBuffer(&bd, &init, &m_pIB);
	if (FAILED(hr)) {
		TRACE("ProceduralPlanetD3D::buildIndexBuffer: CreateBuffer failed");
		return false;
	}

	return true;
}

//----------------------------------------------------------------------------

bool PrimitiveProceduralPlanetD3D::execute()
{
	DisplayDeviceD3D* pDisplayD3D = (DisplayDeviceD3D*)m_pDevice;
	if (!pDisplayD3D || !pDisplayD3D->getContext()) {
		TRACE("ProceduralPlanetD3D::execute: No device context");
		return false;
	}

	ID3D11Device* pDevice = pDisplayD3D->getDevice();
	ID3D11DeviceContext* pContext = pDisplayD3D->getContext();
	if (!pDevice || !pContext) {
		TRACE("ProceduralPlanetD3D::execute: Invalid device/context");
		return false;
	}

	// Lazy-build vertex buffer on first execute
	if (!m_pVB) {
		if (!buildVertexBuffer(pDevice)) {
			TRACE("ProceduralPlanetD3D::execute: Failed to build VB");
			return false;
		}
	}

	// Lazy-build index buffer on first execute
	if (!m_pIB) {
		if (!buildIndexBuffer(pDevice)) {
			TRACE("ProceduralPlanetD3D::execute: Failed to build IB");
			return false;
		}
	}

	// Shared state: ADDITIVE blend + depth-test-only for all overlay passes.
	// Geometry stays bound across both draw calls.
	pContext->OMSetBlendState(pDisplayD3D->m_pBlendStateAdditive.Get(), NULL, 0xFFFFFFFF);
	pContext->OMSetDepthStencilState(pDisplayD3D->m_pDepthStencilTestOnly.Get(), 0);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	pContext->IASetVertexBuffers(0, 1, &m_pVB, &stride, &offset);
	pContext->IASetIndexBuffer(m_pIB, DXGI_FORMAT_R32_UINT, 0);
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ── Pass 1: PS_PlanetSurface  (Option-2 FBM continent/ocean/limb-darkening layer) ──
	// Renders BEFORE the rim/city-lights pass so the surface detail sits below the glow.
	if (pDisplayD3D->m_pMatShader.valid() &&
		pDisplayD3D->m_pMatShader->applyPlanetSurface(pDisplayD3D))
	{
		pContext->DrawIndexed((UINT)m_nIndices, 0, 0);
	}

	// ── Pass 2: PS_Planet  (rim lighting, dust clouds, city lights) ──
	if (pDisplayD3D->m_pMatShader.valid()) {
		// vPlanetLightColor: RGB = city light tint, A = intensity scale
		DirectX::XMFLOAT4 lightColor(
			m_vCityLightColor.x, m_vCityLightColor.y, m_vCityLightColor.z,
			m_fCityLightIntensity);
		pDisplayD3D->m_pMatShader->setConstant("vPlanetLightColor", lightColor);

		// vPlanetParams: x=controlPct, y=techLevel, z=atmosphereDensity, w=cloudPhase
		DirectX::XMFLOAT4 planetParams(
			m_fControlPct, m_fTechLevel, m_fAtmosphereDensity, m_fCloudPhase);
		pDisplayD3D->m_pMatShader->setConstant("vPlanetParams", planetParams);

		if (pDisplayD3D->m_pMatShader->applyProceduralPlanet(pDisplayD3D)) {
			pContext->DrawIndexed((UINT)m_nIndices, 0, 0);
		}
	}

	return true;
}

void PrimitiveProceduralPlanetD3D::clear()
{
	// No persistent state to clear between frames
}

void PrimitiveProceduralPlanetD3D::release()
{
	if (m_pVB) {
		m_pVB->Release();
		m_pVB = NULL;
	}
	if (m_pIB) {
		m_pIB->Release();
		m_pIB = NULL;
	}
	m_Verts.clear();
	m_Indices.clear();
	m_nVerts = 0;
	m_nIndices = 0;

	TRACE("ProceduralPlanetD3D: Released");
}

//----------------------------------------------------------------------------
// EOF

//----------------------------------------------------------------------------
// C interface for GameContext (DarkSpace layer)
// Exported so DarkSpace can create a procedural planet primitive without
// directly linking to the DisplayD3D class (same pattern as atmosphere).

extern "C" __declspec(dllexport) void * createProceduralPlanetFromGameContext(
	DisplayDevice * pDisplay, float fRadius )
{
	if ( pDisplay == NULL )
		return NULL;

	PrimitiveProceduralPlanetD3D * pPrim = new PrimitiveProceduralPlanetD3D();
	if ( !pPrim->initialize( 32, 64, fRadius ) )
	{
		delete pPrim;
		return NULL;
	}
	pPrim->setDevice( pDisplay );
	return pPrim;
}

extern "C" __declspec(dllexport) void setProceduralPlanetParams(
	void * pPrimVoid,
	float controlPct, float techLevel,
	float atmosphereDensity, float cloudPhase,
	float cityR, float cityG, float cityB, float cityIntensity )
{
	if ( pPrimVoid == NULL )
		return;
	PrimitiveProceduralPlanetD3D * pPrim = (PrimitiveProceduralPlanetD3D *)pPrimVoid;
	pPrim->setPlanetParams( controlPct, techLevel, atmosphereDensity, cloudPhase,
		Vector3( cityR, cityG, cityB ), cityIntensity );
}
