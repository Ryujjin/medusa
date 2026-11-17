/*
	NodeProceduralPlanet.cpp
	(c)2025 DarkSpace DX11 Port

	Scene node for rendering procedurally-generated planets.
*/

#define RENDER3D_DLL

#include "Render3D/NodeProceduralPlanet.h"
#include "Render3D/RenderContext.h"
#include "Display/PrimitiveMaterial.h"
#include "Display/PrimitiveTriangleList.h"
#include "Display/Types.h"
#include "Debug/Trace.h"
#include <cmath>
#include <vector>

//----------------------------------------------------------------------------

IMPLEMENT_FACTORY( NodeProceduralPlanet, BaseNode );

static const float PI_F = 3.14159265358979f;

//----------------------------------------------------------------------------

NodeProceduralPlanet::NodeProceduralPlanet()
	: m_nRings(32), m_nSegments(64), m_fRadius(1.0f)
	, m_fTerrainScale(0.1f), m_nOctaves(4)
	, m_vColor(0.3f, 0.6f, 1.0f)
{
}

NodeProceduralPlanet::~NodeProceduralPlanet()
{
}

//----------------------------------------------------------------------------

void NodeProceduralPlanet::preRender(RenderContext& context, const Matrix33& frame, const Vector3& position)
{
	// Procedural geometry is generated on-the-fly in render()
}

void NodeProceduralPlanet::render(RenderContext& context, const Matrix33& frame, const Vector3& position)
{
	DisplayDevice* pDisplay = context.display();
	if (!pDisplay)
		return;

	// Generate vertices for UV sphere
	std::vector<Vertex> vertices;

	// Generate (rings + 1) × (segments + 1) vertices
	for (int ring = 0; ring <= m_nRings; ++ring) {
		float v = (float)ring / (float)m_nRings;
		float phi = v * PI_F;
		float sinPhi = sinf(phi);
		float cosPhi = cosf(phi);

		for (int seg = 0; seg <= m_nSegments; ++seg) {
			float u = (float)seg / (float)m_nSegments;
			float theta = u * 2.0f * PI_F;
			float sinTheta = sinf(theta);
			float cosTheta = cosf(theta);

			Vector3 pos(
				m_fRadius * sinPhi * cosTheta,
				m_fRadius * cosPhi,
				m_fRadius * sinPhi * sinTheta
			);

			Vector3 normal = pos;
			normal.normalize();

			vertices.push_back(Vertex(pos, normal, u, v));
		}
	}

	// Generate triangles from vertices
	std::vector<Triangle> triangles;
	int vertexStride = m_nSegments + 1;
	int triangleCount = m_nRings * m_nSegments * 2;

	for (int ring = 0; ring < m_nRings; ++ring) {
		for (int seg = 0; seg < m_nSegments; ++seg) {
			int v0 = ring * vertexStride + seg;
			int v1 = ring * vertexStride + seg + 1;
			int v2 = (ring + 1) * vertexStride + seg;
			int v3 = (ring + 1) * vertexStride + seg + 1;

			// Two triangles per quad
			triangles.push_back(Triangle(vertices[v0], vertices[v2], vertices[v1]));
			triangles.push_back(Triangle(vertices[v1], vertices[v2], vertices[v3]));
		}
	}

	// Create and push the primitive
	if (!triangles.empty()) {
		context.push(PrimitiveMaterial::create(pDisplay, PrimitiveMaterial::NONE, false, false));
		context.pushIdentity();

		PrimitiveTriangleList::Ref pTriList = PrimitiveTriangleList::create(
			pDisplay, 
			(int)triangles.size(), 
			triangles.data()
		);
		if (pTriList.valid()) {
			context.push(pTriList);
		}
	}
}

//----------------------------------------------------------------------------

bool NodeProceduralPlanet::initialize(int rings, int segments, float radius)
{
	if (rings < 3 || segments < 3 || radius <= 0.0f)
		return false;

	m_nRings = rings;
	m_nSegments = segments;
	m_fRadius = radius;

	return true;
}

void NodeProceduralPlanet::setTerrainParams(float terrainScale, int octaves)
{
	m_fTerrainScale = terrainScale;
	m_nOctaves = (octaves > 0) ? octaves : 1;
}

void NodeProceduralPlanet::setPlanetColor(const Vector3& color)
{
	m_vColor = color;
}

//----------------------------------------------------------------------------
// EOF
