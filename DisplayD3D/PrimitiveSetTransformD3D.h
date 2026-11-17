/*
	PrimitiveSetTransformD3D.h
	(c)2005 Palestar, Richard Lyle
	Updated to DirectX 11
*/

#ifndef PRIMITIVE_SET_TRANSFORM_D3D_H
#define PRIMITIVE_SET_TRANSFORM_D3D_H

#include "Display/PrimitiveSetTransform.h"
#include "DisplayD3D/DisplayDeviceD3D.h"
#include <DirectXMath.h>

//------------------------------------------------------------------------------------

class PrimitiveSetTransformD3D : public PrimitiveSetTransform
{
public:
	// DevicePrimitive Interface
	bool				execute();
	void				clear();
	void				release();

	// PrimitiveSetTransform interface
	const Matrix33 &	transform() const;
	const Vector3 &		transformOffset() const;
	void				setTransform( const Matrix33 &transform, const Vector3 &postTransformOffset );

	// Data	
	Matrix33			m_Transform;
	Vector3				m_PostTransformOffset;

	DirectX::XMMATRIX	m_Matrix;
};

//----------------------------------------------------------------------------



#endif

//------------------------------------------------------------------------------------
// EOF
