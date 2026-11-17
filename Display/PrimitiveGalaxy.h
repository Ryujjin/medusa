/*
	PrimitiveGalaxy.h
	Abstract device primitive for procedural full-screen Milky Way band.
	Executes VS_Galaxy (SV_VertexID fullscreen triangle) + PS_Galaxy (per-pixel
	galactic-coordinate Gaussian profile).  No vertex buffer required.

	(c)2025 DarkSpace DX11 Port
*/

#ifndef PRIMITIVE_GALAXY_H
#define PRIMITIVE_GALAXY_H

#include "Math/Vector3.h"
#include "Display/DisplayDevice.h"
#include "MedusaDll.h"

//----------------------------------------------------------------------------

class DLL PrimitiveGalaxy : public DevicePrimitive
{
public:
	DECLARE_DEVICE_PRIMITIVE();

	// Types
	typedef Reference<PrimitiveGalaxy>	Ref;

	// Mutators
	// gnpDir  : world-space unit vector pointing to galactic north pole
	// galCenDir : world-space unit vector towards galactic centre (orthogonal to gnpDir)
	virtual bool			initialize( const Vector3 & gnpDir, const Vector3 & galCenDir ) = 0;

	// Helpers
	static Ref				create( DisplayDevice * pDevice,
								const Vector3 & gnpDir,
								const Vector3 & galCenDir )
	{
		Ref pPrimitive;
		pDevice->create( pPrimitive );
		pPrimitive->initialize( gnpDir, galCenDir );
		return pPrimitive;
	}

	static void				push( DisplayDevice * pDevice,
								const Vector3 & gnpDir,
								const Vector3 & galCenDir )
	{
		pDevice->push( create( pDevice, gnpDir, galCenDir ) );
	}

	// Runtime depth-effect toggle bitmask (see GalaxyDepthFlags in D3d10shader.cpp).
	// Lives here (in the exported base class) so game-side code linking only
	// against Medusa.dll can flip bits without depending on DisplayD3D.dll internals.
	static int &			depthFlags()
	{
		static int s_nDepthFlags = 0x7F;	// all depth effects on by default
		return s_nDepthFlags;
	}
};

//----------------------------------------------------------------------------

#endif

//----------------------------------------------------------------------------
// EOF
