/*
	PrimitiveGalaxySpriteSet.h
	Abstract device primitive for rendering a set of galaxy billboard sprites
	from a shared 7×6 atlas texture (Bin/Textures/galaxy_sprites.png).

	Each entry is a pre-expanded billboard quad (6 vertices, 2 triangles) in
	world space. The atlas UV for each quad is baked into the vertex stream so
	the pixel shader can sample the correct sub-image for each galaxy.

	Executes VS_GalaxySprite + PS_GalaxySprite (both use VERTEXL layout).

	(c)2025 DarkSpace DX11 Port
*/

#ifndef PRIMITIVE_GALAXY_SPRITE_SET_H
#define PRIMITIVE_GALAXY_SPRITE_SET_H

#include "Display/DisplayDevice.h"
#include "MedusaDll.h"

//----------------------------------------------------------------------------

class DLL PrimitiveGalaxySpriteSet : public DevicePrimitive
{
public:
	DECLARE_DEVICE_PRIMITIVE();

	// Types
	typedef Reference<PrimitiveGalaxySpriteSet>  Ref;

	// Vertex layout matches VertexL exactly: POSITION(12) NORMAL(12) COLOR(4) TEXCOORD(8) = 36 bytes.
	// Offsets must match VERTEXL_LAYOUT in D3d10shader.cpp (0/12/24/28).
	struct SpriteVertex
	{
		float x, y, z;           // POSITION  offset  0
		float nx, ny, nz;        // NORMAL    offset 12  (unused — can be zero)
		unsigned int diffuse;    // COLOR     offset 24  (packed BGRA, same as D3DCOLOR)
		float u, v;              // TEXCOORD  offset 28
	};

	// Initialize with a flat array of pre-built quad vertices (6 per sprite).
	// pVerts must remain valid until execute() has been called once.
	virtual bool initialize( const SpriteVertex * pVerts, int nVerts ) = 0;

	// Helpers
	static Ref create( DisplayDevice * pDevice,
	                   const SpriteVertex * pVerts, int nVerts )
	{
		Ref pPrimitive;
		pDevice->create( pPrimitive );
		pPrimitive->initialize( pVerts, nVerts );
		return pPrimitive;
	}
};

//----------------------------------------------------------------------------

#endif // PRIMITIVE_GALAXY_SPRITE_SET_H

//----------------------------------------------------------------------------
// EOF
