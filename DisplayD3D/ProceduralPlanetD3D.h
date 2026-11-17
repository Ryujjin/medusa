/*
	ProceduralPlanetD3D.h
	(c)2025 DarkSpace DX11 Port

	GPU primitive for procedurally-generated planets with Perlin noise terrain.

	Generates a CPU UV sphere mesh and uploads to GPU vertex/index buffers.
	Terrain variation and coloring handled by PS_ProceduralPlanet shader.

	Usage:
		auto planet = PrimitiveFactory<ProceduralPlanetD3D>::create();
		planet->initialize(32, 64, 1.0f);  // rings, segments, radius
		planet->setTerrainParams(0.1f, 4);  // scale, octaves
		planet->execute();
*/

#ifndef PROCEDURAL_PLANET_D3D_H
#define PROCEDURAL_PLANET_D3D_H

#include "Display/DevicePrimitive.h"
#include "Display/Types.h"
#include "Math/Vector3.h"
#include <d3d11.h>
#include <vector>

class DisplayDeviceD3D;

//----------------------------------------------------------------------------

class PrimitiveProceduralPlanetD3D : public DevicePrimitive
{
public:
	// Construction
	PrimitiveProceduralPlanetD3D();
	virtual ~PrimitiveProceduralPlanetD3D();

	// DevicePrimitive interface
	bool        execute();
	void        clear();
	void        release();

	// Configuration
	bool        initialize(int rings, int segments, float radius);
	void        setTerrainParams(float terrainScale, int octaves);
	void        setColor(const Vector3& color);

	// Per-frame planet CB0 params (set each render, uploaded in execute())
	void        setPlanetParams(float controlPct, float techLevel,
							   float atmosphereDensity, float cloudPhase,
							   const Vector3& cityLightColor, float cityLightIntensity);

	// Accessors
	int         vertexCount() const { return m_nVerts; }
	int         triangleCount() const { return (m_nIndices > 0) ? (m_nIndices / 3) : 0; }

private:
	// Helper methods
	bool        generateSphere();
	bool        buildVertexBuffer(ID3D11Device* pDevice);
	bool        buildIndexBuffer(ID3D11Device* pDevice);

	// Configuration parameters
	int         m_nRings;
	int         m_nSegments;
	float       m_fRadius;
	float       m_fTerrainScale;
	int         m_nOctaves;
	Vector3     m_vColor;

	// Per-frame planet realism params (updated by NounPlanet::render each frame)
	float       m_fControlPct;           // 0-1 capture control percentage
	float       m_fTechLevel;            // 0-1 technology level
	float       m_fAtmosphereDensity;    // 0-1 atmosphere density
	float       m_fCloudPhase;           // 0-2pi cloud animation phase
	Vector3     m_vCityLightColor;       // RGB city light tint
	float       m_fCityLightIntensity;   // city light intensity scale

	// Vertex structure (matches input layout expectations)
	struct Vertex {
		Vector3 position;    // 12 bytes  offset 0
		Vector3 normal;      // 12 bytes  offset 12
		float   u, v;        //  8 bytes  offset 24 — matches TEXCOORD in standard input layout
	};

	// Vertex and index data (CPU-side, freed after GPU upload)
	std::vector<Vertex>         m_Verts;
	std::vector<unsigned int>   m_Indices;
	int                         m_nVerts;
	int                         m_nIndices;

	// GPU resources
	ID3D11Buffer*               m_pVB;  // Vertex buffer (immutable)
	ID3D11Buffer*               m_pIB;  // Index buffer (immutable)
};

//----------------------------------------------------------------------------

typedef DevicePrimitive::Ref PrimitiveProceduralPlanetD3DRef;

#endif // PROCEDURAL_PLANET_D3D_H

//----------------------------------------------------------------------------
// EOF
