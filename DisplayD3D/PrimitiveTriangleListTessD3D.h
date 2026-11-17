/*
	PrimitiveTriangleListTessD3D.h
	D3D11 implementation of hardware-tessellated triangle list.

	(c)2025 DarkSpace DX11 Port
*/

#ifndef PRIMITIVE_TRIANGLE_LIST_TESS_D3D_H
#define PRIMITIVE_TRIANGLE_LIST_TESS_D3D_H

#include "Display/PrimitiveTriangleListTess.h"
#include "DisplayD3D/DisplayDeviceD3D.h"

//------------------------------------------------------------------------------------

class PrimitiveTriangleListTessD3D : public PrimitiveTriangleListTess
{
public:
	// Construction
	PrimitiveTriangleListTessD3D();

	// DevicePrimitive Interface
	bool			execute();
	void			clear();
	void			release();

	// PrimitiveTriangleListTess interface
	bool			initialize( int triangleCount, const Triangle * pTriangles );

private:
	// Data
	int				m_VBSize;
	ID3D11Buffer*	m_VB;
};

//----------------------------------------------------------------------------

#endif
