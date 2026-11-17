/*
	PrimitiveTriangleListTess.h
	Abstract device primitive for hardware-tessellated triangle list rendering.

	Uses the tessellation pipeline: VS_Tess -> HS_Main -> DS_Main -> PS_Main.
	VS_Tess outputs world-space POSITION (not SV_Position) to the hull shader;
	DS_Main performs barycentric interpolation and reprojects to clip space.

	The tessellation factor is driven by fTessFactor in CB0 (set by the caller
	via pShader->setConstant("fTessFactor", value)).  A value of 0 or a missing
	shader gracefully falls back to a non-tessellated draw.

	Same vertex data layout as PrimitiveTriangleList (position+normal+uv).

	(c)2025 DarkSpace DX11 Port
*/

#ifndef PRIMITIVE_TRIANGLE_LIST_TESS_H
#define PRIMITIVE_TRIANGLE_LIST_TESS_H

#include "Math/Vector3.h"
#include "Display/DisplayDevice.h"
#include "Display/Types.h"
#include "MedusaDll.h"

//----------------------------------------------------------------------------

class DLL PrimitiveTriangleListTess : public DevicePrimitive
{
public:
	DECLARE_DEVICE_PRIMITIVE();

	// Types
	typedef Reference<PrimitiveTriangleListTess>	Ref;

	// Mutators
	virtual bool				initialize( int triangleCount, const Triangle * pTriangles ) = 0;

	// Helpers
	static Ref					create( DisplayDevice * pDevice, int triangleCount,
									const Triangle * pTriangles )
	{
		Ref pPrimitive;
		pDevice->create( pPrimitive );
		pPrimitive->initialize( triangleCount, pTriangles );
		return pPrimitive;
	}

	static void					push( DisplayDevice * pDevice, int triangleCount,
									const Triangle * pTriangles )
	{
		pDevice->push( create( pDevice, triangleCount, pTriangles ) );
	}
};

//----------------------------------------------------------------------------

#endif

//----------------------------------------------------------------------------
// EOF
