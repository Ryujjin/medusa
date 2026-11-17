/*
	PrimitiveMaterialD3D.h
	(c)2005 Palestar, Richard Lyle
	Updated to DirectX 11
*/

#ifndef PRIMITIVEMATERIALD3D_H
#define PRIMITIVEMATERIALD3D_H

#include "Display/PrimitiveMaterial.h"
#include "Display/PrimitiveSetTransform.h"
#include "DisplayDeviceD3D.h"
#include "ShaderD3D11.h"

//---------------------------------------------------------------------------------------------------

class PrimitiveMaterialD3D : public PrimitiveMaterial
{
public:
	DECLARE_DEVICE_PRIMITIVE();

	// Construction
	PrimitiveMaterialD3D();

	// DevicePrimitive interface
	bool				execute();
	void				clear();
	void				release();

	// PrimitiveMaterial interface - accessors
	virtual int					pass() const;
	virtual Color				diffuse() const;
	virtual Color				ambient() const;
	virtual Color				emissive() const;
	virtual Color				specular() const;
	virtual float				specularPower() const;
	virtual Blending			blending() const;
	virtual bool				doubleSided() const;
	virtual bool				lightEnabled() const;
	virtual FilterMode			filterMode() const;
	virtual const char*			shader() const;

	// PrimitiveMaterial interface - mutators
	virtual void				setPass( int nPass );
	virtual void				setMaterial( Color diffuse, Color ambient, 
									Color emissive, Color specular, float specularPower );
	virtual void				setBlending( Blending blending );
	virtual void				setDoubleSided( bool doubleSided );
	virtual void				setLightEnable( bool enable );
	virtual void				setFilterMode( FilterMode nMode );
	virtual void				setShader( const char* pShader );

	virtual int					addSurface( PrimitiveSurface* pSurface, 
									SurfaceType eType, int nIndex, int nUV, float* pParams );
	virtual void				removeSurface( int n );
	virtual void				clearSurfaces();

	virtual int					addChild( DevicePrimitive* pChild );
	virtual void				removeChild( int n );
	virtual void				clearChildren();

	virtual void				shadowPass();

	// Mutators
	void					setShader( Shader::Ref pShader );
	void					clearShaders();

	// Types
	struct Surface 
	{
		PrimitiveSurface::Ref		m_pSurface;
		SurfaceType					m_eType;
		int							m_nIndex;
		int							m_nUV;
		float						m_fParams[ PrimitiveSurface::MAX_TEXTURE_PARAMS ];
	};

	// Data
	int						m_nPass;
	Blending				m_Blending;
	Color					m_Diffuse;
	Color					m_Ambient;
	Color					m_Emissive;
	Color					m_Specular;
	float					m_SpecularPower;
	bool					m_DoubleSided;
	bool					m_LightEnable;
	FilterMode				m_nFilterMode;

	bool					m_bUpdateShaders;      // Tracks when shader needs reload
	CharString				m_sShader;
	Shader::Ref				m_pShader;

	Array< Surface >		m_Surfaces;
	Array< DevicePrimitive::Ref >
							m_Children;
	PrimitiveSetTransform::Ref	
							m_TopTransform;       // Transform optimization

	bool					m_bPushed;

private:
	// Render state management
	bool				setupRenderStates(ID3D11DeviceContext* pContext, DisplayDeviceD3D* pDeviceD3D);
	void				setupBlending(ID3D11DeviceContext* pContext, DisplayDeviceD3D* pDeviceD3D);

	// Execution paths
	bool				shouldUseSimplifiedRendering(const DisplayDeviceD3D::LightMap& lights, 
													  DisplayDeviceD3D* pDeviceD3D) const;
	bool				executeSimplifiedPath(DisplayDeviceD3D* pDeviceD3D, 
											   const DisplayDeviceD3D::LightMap& lights);
	bool				executeMultiLightPath(DisplayDeviceD3D* pDeviceD3D, 
											   ID3D11DeviceContext* pContext,
											   DisplayDeviceD3D::LightMap& lights);

	// Shader management
	Shader::Ref			selectShaderForSimplifiedPath(DisplayDeviceD3D* pDeviceD3D);
	bool				updateShaderIfNeeded(DisplayDeviceD3D* pDeviceD3D);
	void				setSimplifiedMaterialConstants(Shader::Ref shader, bool hasLights);
	void				setMultiLightMaterialConstants(Shader::Ref shader, DisplayDeviceD3D* pDeviceD3D);

	// Light rendering
	bool				renderLightPasses(DisplayDeviceD3D* pDeviceD3D, 
										   ID3D11DeviceContext* pContext,
										   DisplayDeviceD3D::LightMap& lights);
	bool				renderSingleLightPass(DisplayDeviceD3D* pDeviceD3D,
											   ID3D11DeviceContext* pContext,
											   DisplayDeviceD3D::Light& light,
											   DisplayDeviceD3D::ShadowPassList::iterator& iShadowPass,
											   int lightIndex);
	void				setLightShaderConstants(const DisplayDeviceD3D::Light& light);
	void				setupShadowMapForLight(DisplayDeviceD3D* pDeviceD3D,
												ID3D11DeviceContext* pContext,
												DisplayDeviceD3D::ShadowPassList::iterator& iShadowPass);

	// Texture and primitive execution
	bool				setupTextureStatges();
	void				clearTextureStages();
	bool				executeChildren();

	// Helpers
	static int			sortSurfaces( Surface p1, Surface p2 );
};

//---------------------------------------------------------------------------------------------------

#endif

//---------------------------------------------------------------------------------------------------
// EOF