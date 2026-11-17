/*
	PrimitiveGalaxySpriteSetD3D.h
	D3D11 implementation of PrimitiveGalaxySpriteSet.
	Loads Bin/Textures/galaxy_sprites.png as a shader resource (t6),
	then draws pre-expanded billboard quads via VS_GalaxySprite + PS_GalaxySprite.

	(c)2025 DarkSpace DX11 Port
*/

#ifndef PRIMITIVE_GALAXY_SPRITE_SET_D3D_H
#define PRIMITIVE_GALAXY_SPRITE_SET_D3D_H

#include "Display/PrimitiveGalaxySpriteSet.h"
#include "DisplayD3D/DisplayDeviceD3D.h"
#include <d3d11.h>
#include <vector>

//----------------------------------------------------------------------------

class PrimitiveGalaxySpriteSetD3D : public PrimitiveGalaxySpriteSet
{
public:
	// Construction / destruction
	PrimitiveGalaxySpriteSetD3D();

	// DevicePrimitive interface
	bool        execute();
	void        clear();
	void        release();

	// PrimitiveGalaxySpriteSet interface
	bool        initialize( const SpriteVertex * pVerts, int nVerts );

private:
	// Helpers
	bool        buildVertexBuffer( ID3D11Device * pDevice );
	bool        loadAtlasTexture( ID3D11Device * pDevice );

	// Data
	std::vector<SpriteVertex>        m_Verts;
	ID3D11Buffer *                   m_pVB;
	static ID3D11ShaderResourceView * m_pAtlasSRV;  // loaded once for device lifetime
};

//----------------------------------------------------------------------------

#endif // PRIMITIVE_GALAXY_SPRITE_SET_D3D_H

//----------------------------------------------------------------------------
// EOF
