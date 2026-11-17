/*
	PrimitiveGalaxyD3D.h
	D3D11 implementation of PrimitiveGalaxy: fullscreen triangle draw via
	VS_Galaxy (SV_VertexID) + PS_Galaxy (per-pixel Milky Way band).

	(c)2025 DarkSpace DX11 Port
*/

#ifndef PRIMITIVE_GALAXY_D3D_H
#define PRIMITIVE_GALAXY_D3D_H

#include "Display/PrimitiveGalaxy.h"
#include "DisplayD3D/DisplayDeviceD3D.h"
#include <d3d11.h>

//----------------------------------------------------------------------------

class PrimitiveGalaxyD3D : public PrimitiveGalaxy
{
public:
	// Construction
	PrimitiveGalaxyD3D();

	// DevicePrimitive Interface
	bool			execute();
	void			clear();
	void			release();

	// PrimitiveGalaxy interface
	bool			initialize( const Vector3 & gnpDir, const Vector3 & galCenDir );

private:
	// Helpers
	bool			loadMilkyWayTexture( ID3D11Device * pDevice );
	bool			loadMilkyWayBump( ID3D11Device * pDevice );

	// Data
	Vector3			m_gnpDir;
	Vector3			m_galCenDir;
	// Static SRVs: loaded once for device lifetime, shared across per-frame instances
	static ID3D11ShaderResourceView * m_pMilkyWaySRV;
	static ID3D11ShaderResourceView * m_pBumpSRV;
};

//----------------------------------------------------------------------------

#endif

//----------------------------------------------------------------------------
// EOF
