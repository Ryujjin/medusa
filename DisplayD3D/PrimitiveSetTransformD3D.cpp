/*
	PrimitiveSetTransformD3D.cpp
	(c)2005 Palestar, Richard Lyle
	Updated to DirectX 11
*/

#include "DisplayD3D/PrimitiveSetTransformD3D.h"
#include "DisplayD3D/PrimitiveFactory.h"

//------------------------------------------------------------------------------------

IMPLEMENT_PRIMITIVE_FACTORY( PrimitiveSetTransformD3D );

bool PrimitiveSetTransformD3D::execute()
{
	DisplayDeviceD3D * pDeviceD3D = (DisplayDeviceD3D *)m_pDevice;
	if (!pDeviceD3D || !pDeviceD3D->m_pContext)
		return false;

	// Store the transform in the device for shader use
	pDeviceD3D->m_pCurrentTransform = this;

	// Update vertex/pixel shader globals when the transform is changed
	pDeviceD3D->updateGlobals();

	// Upload the updated constants (new mWorld/mView/mProj) to the GPU constant
	// buffer immediately.  updateGlobals() only writes to the CPU-side cache;
	// without this apply() the geometry children would draw with the stale CB
	// from the previous material's apply() — causing a one-frame transform lag
	// that shows up as orbit-ring / planet-icon misalignment when the camera moves.
	if (pDeviceD3D->m_pMatShader.valid() && !pDeviceD3D->m_pMatShader->released())
		pDeviceD3D->m_pMatShader->apply(pDeviceD3D);

	return true;
}

void PrimitiveSetTransformD3D::clear()
{}

void PrimitiveSetTransformD3D::release()
{}

//----------------------------------------------------------------------------

const Matrix33 & PrimitiveSetTransformD3D::transform() const
{ 
	return(m_Transform); 
}

const Vector3 &	PrimitiveSetTransformD3D::transformOffset() const
{ 
	return(m_PostTransformOffset); 
}

void PrimitiveSetTransformD3D::setTransform( const Matrix33 &transform, const Vector3 &postTransformOffset )
{
	m_Transform = transform;
	m_PostTransformOffset = postTransformOffset;

	// Build DirectX 11 world matrix from transform and offset.
	// Row-vector convention (HLSL: mul(pos, mWorld)): each row must be one of
	// the frame's basis vectors in world space — exactly matching DX9's layout:
	//   _11=i.x _12=i.y _13=i.z   (Row 0 = local X axis in world space)
	//   _21=j.x _22=j.y _23=j.z   (Row 1 = local Y axis in world space)
	//   _31=k.x _32=k.y _33=k.z   (Row 2 = local Z axis in world space)
	//   _41=t.x _42=t.y _43=t.z   (Row 3 = world translation)
	// Result: pos_world = frame.i*pos.x + frame.j*pos.y + frame.k*pos.z + offset
	m_Matrix = DirectX::XMMATRIX(
		m_Transform.i.x, m_Transform.i.y, m_Transform.i.z, 0.0f,
		m_Transform.j.x, m_Transform.j.y, m_Transform.j.z, 0.0f,
		m_Transform.k.x, m_Transform.k.y, m_Transform.k.z, 0.0f,
		m_PostTransformOffset.x, m_PostTransformOffset.y, m_PostTransformOffset.z, 1.0f
	);
}

//------------------------------------------------------------------------------------
// EOF
