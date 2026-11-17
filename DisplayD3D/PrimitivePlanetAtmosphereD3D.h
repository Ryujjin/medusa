/*
	PrimitivePlanetAtmosphereD3D.h
	D3D11 implementation of PrimitivePlanetAtmosphere: procedural planet atmosphere shell.

	(c)2025 DarkSpace DX11 Port
*/

#ifndef PRIMITIVE_PLANET_ATMOSPHERE_D3D_H
#define PRIMITIVE_PLANET_ATMOSPHERE_D3D_H

#include "Display/PrimitivePlanetAtmosphere.h"
#include "DisplayD3D/DisplayDeviceD3D.h"
#include <d3d11.h>

class PrimitivePlanetAtmosphereD3D : public PrimitivePlanetAtmosphere
{
public:
	DECLARE_DEVICE_PRIMITIVE();

	PrimitivePlanetAtmosphereD3D();
	virtual ~PrimitivePlanetAtmosphereD3D();

	bool execute();
	void clear();
	void release();

	void setRadius( float fRadius );
	void setDensity( float fDensity );
	void setColor( const Vector3 & vColor );
	void setAnimSpeed( float fAnimSpeed );

	float getRadius() const;
	float getDensity() const;
	Vector3 getColor() const;
	float getAnimSpeed() const;

	static Reference<DevicePrimitive> create( DisplayDevice * pDisplay, float fRadius, const Vector3 & vColor, float fDensity, float fAnimSpeed );

private:
	bool generateSphereMesh( ID3D11Device * pDevice );
	bool createNoCullRasterizerState( ID3D11Device * pDevice );

	float m_fRadius;
	float m_fDensity;
	Vector3 m_vColor;
	float m_fAnimSpeed;
	int m_nVertexCount;
	int m_nIndexCount;
	bool m_bBuffersGenerated;

	D3D11BufferPtr m_pVertexBuffer;
	D3D11BufferPtr m_pIndexBuffer;
	D3D11RasterizerStatePtr m_pRasterizerStateNoCull;
};

#endif
