/*
	NodeProceduralPlanet.h
	(c)2025 DarkSpace DX11 Port

	Scene node for rendering procedurally-generated planets.
	Generates a UV sphere and renders with Render3D node interface.

	Usage:
		NodeProceduralPlanet* planet = new NodeProceduralPlanet();
		planet->initialize(32, 64, 1.0f);  // rings, segments, radius
		planet->setTerrainParams(0.1f, 4);  // scale, octaves
		planet->setPlanetColor(Vector3(0.2f, 0.5f, 0.8f));
		scene->attach(planet);
*/

#ifndef NODE_PROCEDURAL_PLANET_H
#define NODE_PROCEDURAL_PLANET_H

#include "Render3D/BaseNode.h"
#include "Render3D/Render3dDll.h"
#include "Math/Vector3.h"

//----------------------------------------------------------------------------

class DLL NodeProceduralPlanet : public BaseNode
{
public:
	DECLARE_WIDGET_CLASS();

	NodeProceduralPlanet();
	virtual ~NodeProceduralPlanet();

	// BaseNode interface
	virtual void            render(RenderContext& context, const Matrix33& frame, const Vector3& position);
	virtual void            preRender(RenderContext& context, const Matrix33& frame, const Vector3& position);

	// Configuration
	bool                    initialize(int rings, int segments, float radius);
	void                    setTerrainParams(float terrainScale, int octaves);
	void                    setPlanetColor(const Vector3& color);

	// Accessors
	int                     getRings() const { return m_nRings; }
	int                     getSegments() const { return m_nSegments; }
	float                   getRadius() const { return m_fRadius; }
	float                   getTerrainScale() const { return m_fTerrainScale; }
	int                     getOctaves() const { return m_nOctaves; }
	Vector3                 getPlanetColor() const { return m_vColor; }

private:
	// Configuration parameters
	int                     m_nRings;
	int                     m_nSegments;
	float                   m_fRadius;
	float                   m_fTerrainScale;
	int                     m_nOctaves;
	Vector3                 m_vColor;
};

//----------------------------------------------------------------------------

#endif // NODE_PROCEDURAL_PLANET_H

//----------------------------------------------------------------------------
// EOF
