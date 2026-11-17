/*
	PrimitiveMaterialD3D.cpp
	(c)2005 Palestar, Richard Lyle
	Updated to DirectX 11 - 2025

	DirectX 11 Implementation of PrimitiveMaterial
	- Shader-based rendering (no fixed-function pipeline)
	- State object management for culling, blending, depth testing
	- Multi-light rendering with shadow map support
	- Texture binding via shader resource views
*/

#include "Debug/Assert.h"
#include "Debug/Trace.h"
#include "Debug/Log.h"
#include "PrimitiveMaterialD3D.h"
#include "PrimitiveFactory.h"
#include "PrimitiveSetTransformD3D.h"

//------------------------------------------------------------------------------------
// Centralized Error Logging Helper

namespace {
	// Logs error to both TRACE and LOG_ERROR systems for comprehensive debugging
	void LogMaterialError(const char* context, const char* message)
	{
		TRACE(CharString().format("PrimitiveMaterialD3D::%s - ERROR: %s", context, message));
		LOG_ERROR("PrimitiveMaterialD3D", CharString().format("%s: %s", context, message));
	}

	void LogMaterialWarning(const char* context, const char* message)
	{
		TRACE(CharString().format("PrimitiveMaterialD3D::%s - WARNING: %s", context, message));
		LOG_WARNING("PrimitiveMaterialD3D", CharString().format("%s: %s", context, message));
	}
}

//------------------------------------------------------------------------------------

IMPLEMENT_PRIMITIVE_FACTORY( PrimitiveMaterialD3D );
IMPLEMENT_DEVICE_PRIMITIVE( PrimitiveMaterialD3D );

PrimitiveMaterialD3D::PrimitiveMaterialD3D()
{
	m_nPass = DisplayDevice::PRIMARY;
	m_Blending = NONE;
	m_DoubleSided = false;
	m_LightEnable = true;
	m_bPushed = false;
	m_nFilterMode = FILTER_ON;
	m_bUpdateShaders = false;

	// setup the material to some default values
	setMaterial( Color(255,255,255,255), Color(255,255,255), 
		Color(0,0,0), Color(255,255,255), 0.0f );
}

//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------

bool PrimitiveMaterialD3D::execute()
{
	// Input validation
	DisplayDeviceD3D* pDeviceD3D = (DisplayDeviceD3D*)m_pDevice;
	if (!pDeviceD3D)
	{
		LogMaterialError("execute", "Device context is NULL");
		return false;
	}

	ID3D11DeviceContext* pContext = pDeviceD3D->getContext();
	if (!pContext)
	{
		LogMaterialError("execute", "D3D11 device context is NULL");
		return false;
	}

	// Setup render states
	if (!setupRenderStates(pContext, pDeviceD3D))
	{
		LogMaterialError("execute", "Failed to setup render states");
		return false;
	}

	// Choose rendering path based on lighting and material properties
	DisplayDeviceD3D::LightMap& lights = pDeviceD3D->m_Lights;

	if (shouldUseSimplifiedRendering(lights, pDeviceD3D))
	{
		return executeSimplifiedPath(pDeviceD3D, lights);
	}
	else
	{
		return executeMultiLightPath(pDeviceD3D, pContext, lights);
	}
}

//------------------------------------------------------------------------------------
// Helper Methods - Extracted from execute() for clarity

bool PrimitiveMaterialD3D::setupRenderStates(ID3D11DeviceContext* pContext, DisplayDeviceD3D* pDeviceD3D)
{
	ASSERT(pContext != nullptr);
	ASSERT(pDeviceD3D != nullptr);

	// D3D11: Set rasterizer state (culling mode)
	// Double-sided materials disable backface culling for rendering both sides
	if (m_DoubleSided)
		pContext->RSSetState(pDeviceD3D->m_pRasterizerPoint.Get());  // No culling
	else
		pContext->RSSetState(pDeviceD3D->m_pRasterizerSolid.Get());   // Backface culling

	// D3D11: Set blend state for transparency
	setupBlending(pContext, pDeviceD3D);

	// D3D11: Set depth stencil state.
	// PRIMARY/opaque: full depth test + write.
	// ALPHA/ADDITIVE transparent geometry: depth test ON (occluded by solids),
	//   depth write OFF (don't block each other or later opaque draws).
	// All other secondary passes (e.g. UI): depth fully disabled.
	if (m_nPass == DisplayDevice::PRIMARY)
	{
		pContext->OMSetDepthStencilState(pDeviceD3D->m_pDepthStencilEnabled.Get(), 0);
	}
	else if (m_Blending == PrimitiveMaterial::ALPHA ||
			 m_Blending == PrimitiveMaterial::ALPHA_INV ||
			 m_Blending == PrimitiveMaterial::ADDITIVE ||
			 m_Blending == PrimitiveMaterial::ADDITIVE_INV)
	{
		pContext->OMSetDepthStencilState(pDeviceD3D->m_pDepthStencilTestOnly.Get(), 0);
	}
	else
	{
		pContext->OMSetDepthStencilState(pDeviceD3D->m_pDepthStencilDisabled.Get(), 0);
	}

	return true;
}

bool PrimitiveMaterialD3D::shouldUseSimplifiedRendering(
	const DisplayDeviceD3D::LightMap& lights, 
	DisplayDeviceD3D* pDeviceD3D) const
{
	// Use simplified rendering (single pass) when:
	// - Fixed function mode is forced
	// - Lighting is disabled
	// - No lights in scene
	// - Using additive blending (typically for effects/particles)
	return DisplayDevice::sm_bUseFixedFunction 
		|| !m_LightEnable 
		|| lights.size() == 0 
		|| m_Blending == PrimitiveMaterial::ADDITIVE;
}

bool PrimitiveMaterialD3D::executeSimplifiedPath(
	DisplayDeviceD3D* pDeviceD3D,
	const DisplayDeviceD3D::LightMap& lights)
{
	ASSERT(pDeviceD3D != nullptr);

	pDeviceD3D->m_bUsingFixedFunction = true;

	// Select and apply shader
	Shader::Ref activeShader = selectShaderForSimplifiedPath(pDeviceD3D);
	if (!activeShader.valid())
	{
		LogMaterialWarning("executeSimplifiedPath", "No valid shader available, using default");
		activeShader = pDeviceD3D->m_pDefaultShader;
	}

	if (activeShader.valid())
	{
		pDeviceD3D->m_pMatShader = activeShader;

		// Set material constants BEFORE apply() so they're included in the CB upload
		setSimplifiedMaterialConstants(activeShader, lights.size() > 0);
	}

	// Setup textures — this sets bEnableDiffuse/bEnableLightMap/bEnableBumpMap
	// Must happen BEFORE apply() so the constant buffer includes texture flags
	if (!setupTextureStatges())
	{
		LogMaterialWarning("executeSimplifiedPath", "Some texture stages failed, continuing with available textures");
	}

	// Apply shader AFTER all constants and texture flags are set,
	// so the constant buffer upload includes bEnableDiffuse etc.
	if (activeShader.valid())
	{
		if (!activeShader->apply(pDeviceD3D))
		{
			LogMaterialError("executeSimplifiedPath", "Failed to apply shader");
			return false;
		}
	}

	if (!executeChildren())
	{
		LogMaterialError("executeSimplifiedPath", "Failed to execute child primitives");
		return false;
	}

	return true;
}

Shader::Ref PrimitiveMaterialD3D::selectShaderForSimplifiedPath(DisplayDeviceD3D* pDeviceD3D)
{
	ASSERT(pDeviceD3D != nullptr);

	// Use custom shader if set and valid
	if (m_pShader.valid() && m_pShader != pDeviceD3D->m_pDefaultShader)
	{
		return m_pShader;
	}

	// Fall back to default shader
	return pDeviceD3D->m_pDefaultShader;
}

void PrimitiveMaterialD3D::setSimplifiedMaterialConstants(Shader::Ref shader, bool hasLights)
{
	ASSERT(shader.valid());

	shader->setConstant("vMatDiffuse", m_Diffuse);
	shader->setConstant("vMatAmbient", m_Ambient);
	shader->setConstant("vMatEmissive", m_Emissive);
	shader->setConstant("vMatSpecular", m_Specular);
	shader->setConstant("fMatSpecularPower", m_SpecularPower);
	shader->setConstant("bEnableLighting", m_LightEnable && hasLights);
	shader->setConstant("bAlphaBlend", m_Blending == PrimitiveMaterial::ALPHA || m_Blending == PrimitiveMaterial::ALPHA_INV);

	// Set ambient light — without this, the shader produces near-black output
	// for surfaces with low NdotL (e.g. most of a planet facing away from the default light).
	// In DX9, ambient was applied automatically by the fixed-function pipeline.
	DisplayDeviceD3D * pDeviceD3D = (DisplayDeviceD3D *)m_pDevice;
	if (pDeviceD3D)
	{
		shader->setConstant("vGlobalAmbient", pDeviceD3D->m_cAmbientLight);
		shader->setConstant("bEnableAmbient", true);
	}
}

bool PrimitiveMaterialD3D::executeMultiLightPath(
	DisplayDeviceD3D* pDeviceD3D,
	ID3D11DeviceContext* pContext,
	DisplayDeviceD3D::LightMap& lights)
{
	ASSERT(pDeviceD3D != nullptr);
	ASSERT(pContext != nullptr);

	// Update shader if needed
	if (!updateShaderIfNeeded(pDeviceD3D))
	{
		LogMaterialError("executeMultiLightPath", "Shader update failed or invalid shader");
		return false;
	}

	pDeviceD3D->m_bUsingFixedFunction = false;
	pDeviceD3D->m_pMatShader = m_pShader;

	// Set material constants
	setMultiLightMaterialConstants(m_pShader, pDeviceD3D);

	// Setup textures
	if (!setupTextureStatges())
	{
		LogMaterialError("executeMultiLightPath", "Failed to setup texture stages");
		return false;
	}

	// Render each light as a separate pass
	return renderLightPasses(pDeviceD3D, pContext, lights);
}

bool PrimitiveMaterialD3D::updateShaderIfNeeded(DisplayDeviceD3D* pDeviceD3D)
{
	ASSERT(pDeviceD3D != nullptr);

	if (m_bUpdateShaders || !m_pShader.valid() || m_pShader->released())
	{
		m_bUpdateShaders = false;

		m_pShader = NULL;
		if (m_sShader.length() > 0)
		{
			m_pShader = pDeviceD3D->getShader(m_sShader);
		}

		if (!m_pShader.valid())
		{
			m_pShader = pDeviceD3D->m_pDefaultShader;
		}
	}

	return m_pShader.valid() && !m_pShader->released();
}

void PrimitiveMaterialD3D::setMultiLightMaterialConstants(Shader::Ref shader, DisplayDeviceD3D* pDeviceD3D)
{
	ASSERT(shader.valid());
	ASSERT(pDeviceD3D != nullptr);

	shader->setConstant("vMatAmbient", m_Ambient);
	shader->setConstant("vMatDiffuse", m_Diffuse);
	shader->setConstant("vMatEmissive", m_Emissive);
	shader->setConstant("vMatSpecular", m_Specular);
	shader->setConstant("fMatSpecularPower", m_SpecularPower);
	shader->setConstant("vGlobalAmbient", pDeviceD3D->m_cAmbientLight);
	shader->setConstant("bEnableAmbient", true);
	shader->setConstant("bAlphaBlend", m_Blending == PrimitiveMaterial::ALPHA || m_Blending == PrimitiveMaterial::ALPHA_INV);
}

bool PrimitiveMaterialD3D::renderLightPasses(
	DisplayDeviceD3D* pDeviceD3D,
	ID3D11DeviceContext* pContext,
	DisplayDeviceD3D::LightMap& lights)
{
	ASSERT(pDeviceD3D != nullptr);
	ASSERT(pContext != nullptr);

	DisplayDeviceD3D::ShadowPassList::iterator iShadowPass = pDeviceD3D->m_ShadowPassList.begin();

	// D3D11: Set alpha blending for the first pass
	pContext->OMSetBlendState(pDeviceD3D->m_pBlendStateAlpha.Get(), NULL, 0xFFFFFFFF);

	// Render a pass for each light in the scene
	int nLightCount = 0;
	for (DisplayDeviceD3D::LightMap::iterator iLight = lights.begin(); 
		iLight != lights.end(); ++iLight, ++nLightCount)
	{
		// Limit to maximum lights to prevent performance issues
		if (nLightCount >= sm_nMaxLights)
		{
			LogMaterialWarning("renderLightPasses", 
				CharString().format("Exceeded max light count (%d), skipping remaining lights", sm_nMaxLights));
			break;
		}

		if (!renderSingleLightPass(pDeviceD3D, pContext, iLight->second, iShadowPass, nLightCount))
		{
			LogMaterialError("renderLightPasses", 
				CharString().format("Failed to render light pass %d", nLightCount));
			return false;
		}

		// Advance shadow pass iterator if we used a shadow map
		if (iShadowPass != pDeviceD3D->m_ShadowPassList.end())
		{
			++iShadowPass;
		}
	}

	return true;
}

bool PrimitiveMaterialD3D::renderSingleLightPass(
	DisplayDeviceD3D* pDeviceD3D,
	ID3D11DeviceContext* pContext,
	DisplayDeviceD3D::Light& light,
	DisplayDeviceD3D::ShadowPassList::iterator& iShadowPass,
	int lightIndex)
{
	ASSERT(pDeviceD3D != nullptr);
	ASSERT(pContext != nullptr);
	ASSERT(m_pShader.valid());

	// Set light parameters for shader
	setLightShaderConstants(light);

	// Handle shadow maps if available
	setupShadowMapForLight(pDeviceD3D, pContext, iShadowPass);

	// Apply the shader and upload all constants for rendering
	if (!m_pShader->apply(pDeviceD3D))
	{
		LogMaterialError("renderSingleLightPass", "Failed to apply shader");
		return false;
	}

	// Render geometry with this light
	if (!executeChildren())
	{
		LogMaterialError("renderSingleLightPass", "Failed to execute child primitives");
		return false;
	}

	// After first light, switch to additive blending and disable ambient
	if (lightIndex == 0)
	{
		pContext->OMSetBlendState(pDeviceD3D->m_pBlendStateAdditive.Get(), NULL, 0xFFFFFFFF);
		m_pShader->setConstant("bEnableAmbient", false);
	}

	return true;
}

void PrimitiveMaterialD3D::setLightShaderConstants(const DisplayDeviceD3D::Light& light)
{
	ASSERT(m_pShader.valid());

	m_pShader->setConstant("nLightType", (int)light.type);

	// Set position for point lights and spot lights
	if (light.type != DisplayDeviceD3D::Light::DIRECTIONAL)
	{
		m_pShader->setConstant("vLightPosition", light.position);
	}

	// Set direction for directional lights and spot lights
	if (light.type != DisplayDeviceD3D::Light::POINT)
	{
		m_pShader->setConstant("vLightDirection", light.direction);
	}

	// Set light colors
	m_pShader->setConstant("vLightDiffuse", light.color);
	m_pShader->setConstant("vLightSpecular", light.color);
	m_pShader->setConstant("vLightAmbient", Color(0, 0, 0));

	// Set attenuation for point lights
	if (light.type == DisplayDeviceD3D::Light::POINT)
	{
		Vector3 vAttenuation(light.attenuation0, light.attenuation1, light.attenuation2);
		m_pShader->setConstant("vAttenuation", vAttenuation);
	}
}

void PrimitiveMaterialD3D::setupShadowMapForLight(
	DisplayDeviceD3D* pDeviceD3D,
	ID3D11DeviceContext* pContext,
	DisplayDeviceD3D::ShadowPassList::iterator& iShadowPass)
{
	ASSERT(pDeviceD3D != nullptr);
	ASSERT(pContext != nullptr);
	ASSERT(m_pShader.valid());

	if (iShadowPass != pDeviceD3D->m_ShadowPassList.end())
	{
		DisplayDeviceD3D::ShadowPass& pass = *iShadowPass;

		m_pShader->setConstant("bEnableShadowMap", true);

		// TODO: Set shadow map view/projection matrices
		// m_pShader->setConstant("mLightView", pass.m_LightView);
		// m_pShader->setConstant("mLightProj", pass.m_LightProj);

		// D3D11: Bind shadow map as shader resource view in slot 7
		if (pass.m_pShadowMapSRV)
		{
			ID3D11ShaderResourceView* srv = pass.m_pShadowMapSRV.Get();
			pContext->PSSetShaderResources(7, 1, &srv);
		}
		else
		{
			LogMaterialWarning("setupShadowMapForLight", "Shadow pass has no SRV");
		}
	}
	else
	{
		m_pShader->setConstant("bEnableShadowMap", false);
	}
}


void PrimitiveMaterialD3D::clear()
{
	m_Children.release();
	m_TopTransform = NULL;
	m_bPushed = false;
}

void PrimitiveMaterialD3D::release()
{
	m_Children.release();
	m_Surfaces.release();
	m_TopTransform = NULL;
	m_bPushed = false;

	m_nPass = DisplayDevice::PRIMARY;
	m_Blending = NONE;
	m_DoubleSided = false;
	m_LightEnable = true;
	m_nFilterMode = FILTER_ON;
	m_bUpdateShaders = false;
	m_pShader = NULL;
	m_sShader = "";

	// setup the material to some default vlaues
	setMaterial( Color(255,255,255,255), Color(255,255,255), 
		Color(0,0,0), Color(255,255,255), 0.0f );
}

//------------------------------------------------------------------------------------

int PrimitiveMaterialD3D::pass() const
{
	return m_nPass;
}

Color PrimitiveMaterialD3D::diffuse() const
{
	return( m_Diffuse );
}

Color PrimitiveMaterialD3D::ambient() const
{
	return( m_Ambient );
}

Color PrimitiveMaterialD3D::emissive() const
{
	return( m_Emissive );
}

Color PrimitiveMaterialD3D::specular() const
{
	return( m_Specular );
}

float PrimitiveMaterialD3D::specularPower() const
{
	return( m_SpecularPower );
}

PrimitiveMaterial::Blending PrimitiveMaterialD3D::blending() const
{
	return( m_Blending );
}

bool PrimitiveMaterialD3D::doubleSided() const
{
	return( m_DoubleSided );
}

bool PrimitiveMaterialD3D::lightEnabled() const
{
	return( m_LightEnable );
}

PrimitiveMaterial::FilterMode PrimitiveMaterialD3D::filterMode() const
{
	return m_nFilterMode;
}

const char * PrimitiveMaterialD3D::shader() const
{
	return m_sShader;
}

//------------------------------------------------------------------------------------

void PrimitiveMaterialD3D::setPass( int nPass )
{
	m_nPass = nPass;
}

void PrimitiveMaterialD3D::setMaterial( Color diffuse, Color ambient, 
							Color emissive, Color specular, float specularPower )
{
	m_Diffuse = diffuse;
	m_Ambient = ambient;
	m_Emissive = emissive;
	m_Specular = specular;
	m_SpecularPower = specularPower;
}

void PrimitiveMaterialD3D::setBlending( Blending blending )
{
	m_Blending = blending;
}

void PrimitiveMaterialD3D::setDoubleSided( bool doubleSided )
{
	m_DoubleSided = doubleSided;
}

void PrimitiveMaterialD3D::setLightEnable( bool enable )
{
	m_LightEnable = enable;
}

void PrimitiveMaterialD3D::setFilterMode( FilterMode nMode )
{
	m_nFilterMode = nMode;
}

void PrimitiveMaterialD3D::setShader( const char * pShader )
{
	m_sShader = pShader != NULL ? pShader : "";
	m_bUpdateShaders = true;
}

int PrimitiveMaterialD3D::addSurface( PrimitiveSurface * pSurface,
								SurfaceType eType, int nIndex, int nUV, float * pParams )
{
	// Input validation
	if (!pSurface)
	{
		LogMaterialError("addSurface", "Surface pointer is NULL");
		return -1;
	}

	// Basic range check for surface type (enum values are typically 0-9)
	if (eType < 0 || eType > 20)
	{
		LogMaterialWarning("addSurface", CharString().format("Unusual surface type value: %d", eType));
	}

	Surface & surface = m_Surfaces.push();
	surface.m_pSurface	= pSurface;
	surface.m_eType		= eType;
	surface.m_nIndex	= nIndex;
	surface.m_nUV		= nUV;

	if ( pParams != NULL )
		memcpy( surface.m_fParams, pParams, sizeof(surface.m_fParams) );

	m_Surfaces.qsort( sortSurfaces );
	return m_Surfaces.size() - 1;
}

void PrimitiveMaterialD3D::removeSurface( int n )
{
	if (!m_Surfaces.isValid(n))
	{
		LogMaterialWarning("removeSurface", CharString().format("Invalid surface index: %d", n));
		return;
	}

	m_Surfaces.remove( n );
}

void PrimitiveMaterialD3D::clearSurfaces()
{
	m_Surfaces.release();
}

int PrimitiveMaterialD3D::addChild( DevicePrimitive * pPrimitive )
{
	// Input validation
	if (!pPrimitive)
	{
		LogMaterialError("addChild", "Primitive pointer is NULL");
		return -1;
	}

	// TODO: If this is a transparent material, sort incoming primitives based on depth
	// to render them back to front and fix transparency issues.

	// Optimization: Avoid redundant transforms in the child list
	if ( pPrimitive->primitiveKey() == PrimitiveSetTransform::staticPrimitiveKey() )
	{
		// Check if this transform is already the top transform
		if ( m_TopTransform == pPrimitive )
		{
			// Transform is already pushed onto this material
			return -1;
		}

		m_TopTransform = (PrimitiveSetTransform *)pPrimitive;

		// If our last primitive is also a transform, replace it instead of adding
		// This prevents accumulating many transforms for animated objects
		if ( m_Children.size() > 0 )
		{
			DevicePrimitive * pLastPrimitive = m_Children.last();
			if ( pLastPrimitive && 
				 pLastPrimitive->primitiveKey() == PrimitiveSetTransform::staticPrimitiveKey() )
			{
				m_Children[ m_Children.size() - 1 ] = pPrimitive;
				return m_Children.size() - 1;
			}
		}
	}

	m_Children.push( pPrimitive );
	return m_Children.size() - 1;
}

void PrimitiveMaterialD3D::removeChild( int n )
{
	if (!m_Children.isValid(n))
	{
		LogMaterialWarning("removeChild", CharString().format("Invalid child index: %d", n));
		return;
	}

	m_Children.remove( n );
}

void PrimitiveMaterialD3D::clearChildren()
{
	m_Children.release();
}

void PrimitiveMaterialD3D::shadowPass()
{
	// Execute all attached primitives for shadow map generation
	// Skips textures and materials as they're not needed for depth rendering
	for(int i = 0; i < m_Children.size(); i++)
	{
		DevicePrimitive * pPrimitive = m_Children[i];
		if (!pPrimitive)
			continue;

		// Skip surfaces and materials - only render geometry for shadows
		if ( pPrimitive->primitiveKey() == PrimitiveSurface::staticPrimitiveKey() ||
			pPrimitive->primitiveKey() == PrimitiveMaterial::staticPrimitiveKey() )
			continue;

		pPrimitive->setDevice( device() );
		if (!pPrimitive->execute())
		{
			LogMaterialWarning("shadowPass", "Child primitive failed to execute");
		}
	}
}

//---------------------------------------------------------------------------------------------------

void PrimitiveMaterialD3D::setShader( Shader::Ref pShader )
{
	m_pShader = pShader;
	// TODO: Shader class needs a method to get name/filename
	m_sShader = "";  // Shader name tracking
}

void PrimitiveMaterialD3D::clearShaders()
{
	m_pShader = NULL;
	m_sShader = "";
}

//---------------------------------------------------------------------------------------------------

void PrimitiveMaterialD3D::setupBlending(ID3D11DeviceContext* pContext, DisplayDeviceD3D* pDeviceD3D)
{
	if (!pContext || !pDeviceD3D)
		return;

	// D3D11: Setup blending mode using blend state objects
	switch (m_Blending)
	{
	case PrimitiveMaterial::NONE:
		pContext->OMSetBlendState(pDeviceD3D->m_pBlendStateOpaque.Get(), NULL, 0xFFFFFFFF);
		break;

	case PrimitiveMaterial::ALPHA:
	case PrimitiveMaterial::ALPHA_INV:
		// Both use alpha blending (difference handled by shader or texture setup)
		pContext->OMSetBlendState(pDeviceD3D->m_pBlendStateAlpha.Get(), NULL, 0xFFFFFFFF);
		break;

	case PrimitiveMaterial::ADDITIVE:
	case PrimitiveMaterial::ADDITIVE_INV:
		pContext->OMSetBlendState(pDeviceD3D->m_pBlendStateAdditive.Get(), NULL, 0xFFFFFFFF);
		break;
	}
}

void PrimitiveMaterialD3D::clearTextureStages()
{
	DisplayDeviceD3D * pDeviceD3D = (DisplayDeviceD3D *)m_pDevice;
	if (!pDeviceD3D)
	{
		LogMaterialError("clearTextureStages", "Device context is NULL");
		return;
	}

	ID3D11DeviceContext* pContext = pDeviceD3D->getContext();
	if (!pContext)
	{
		LogMaterialError("clearTextureStages", "D3D11 device context is NULL");
		return;
	}

	// D3D11: Unbind all shader resource views to prevent resource conflicts
	ID3D11ShaderResourceView* nullSRVs[4] = { NULL, NULL, NULL, NULL };
	pContext->PSSetShaderResources(0, 4, nullSRVs);

	// Reset texture stage counter and diffuse-bound flag
	pDeviceD3D->m_nTextureStage = 0;
	pDeviceD3D->m_bDiffuseBound = false;

	// Turn off texturing options in the ACTIVE shader (m_pMatShader), not m_pShader.
	// The simplified rendering path sets pDeviceD3D->m_pMatShader to the default shader
	// but does not set m_pShader, so using m_pShader here would leave stale flags
	// (e.g. bEnableDiffuse=1) causing the shader to sample from an unbound SRV → black.
	Shader::Ref pActiveShader = pDeviceD3D->m_pMatShader;
	if (!pActiveShader.valid())
		pActiveShader = m_pShader;

	if (pActiveShader.valid())
	{
		pActiveShader->setConstant("bEnableDiffuse", false);
		pActiveShader->setConstant("bEnableLightMap", false);
		pActiveShader->setConstant("bEnableBumpMap", false);
	}
}

bool PrimitiveMaterialD3D::setupTextureStatges()
{
	clearTextureStages();

	int nSuccessCount = 0;

	// Execute all attached surfaces to bind their textures
	for(int i = 0; i < m_Surfaces.size(); i++)
	{
		Surface & surface = m_Surfaces[i];

		// Defensive check - surface should never be null if properly added
		if (!surface.m_pSurface)
		{
			LogMaterialWarning("setupTextureStatges", 
				CharString().format("Surface %d has NULL pointer", i));
			continue;
		}

		surface.m_pSurface->setDevice( device() );
		surface.m_pSurface->set( surface.m_eType, 
			surface.m_nIndex, 
			surface.m_nUV,
			m_nFilterMode == FILTER_ON,
			surface.m_fParams );

		if (!surface.m_pSurface->execute())
		{
			LogMaterialWarning("setupTextureStatges", 
				CharString().format("Surface %d failed to execute (type=%d), skipping", i, surface.m_eType));
			continue;
		}
		else
		{
			nSuccessCount++;
		}
	}

	return true;
}

bool PrimitiveMaterialD3D::executeChildren()
{
	DisplayDeviceD3D * pDeviceD3D = (DisplayDeviceD3D *)m_pDevice;
	if (!pDeviceD3D)
	{
		LogMaterialError("executeChildren", "Device context is NULL");
		return false;
	}

	// Execute all attached primitives (geometry, transforms, etc.)
	for(int i = 0; i < m_Children.size(); i++)
	{
		DevicePrimitive * pPrimitive = m_Children[i];
		if (!pPrimitive)
		{
			// Skip null entries (should not happen in normal usage)
			LogMaterialWarning("executeChildren", CharString().format("Child %d is NULL", i));
			continue;
		}

		// If we hit a surface primitive, reset texture stages
		// This ensures proper texture binding for subsequent geometry
		if ( pPrimitive->primitiveKey() == PrimitiveSurface::staticPrimitiveKey() )
		{
			clearTextureStages();
		}

		pPrimitive->setDevice( device() );
		if (!pPrimitive->execute())
		{
			// Don't abort the entire material on a single child failure.
			// In DX9, surface/geometry failures were non-fatal. Aborting here
			// kills subsequent children (e.g. font triangle lists after a surface).
			continue;
		}
	}

	return true;
}

//----------------------------------------------------------------------------

int PrimitiveMaterialD3D::sortSurfaces( Surface p1, Surface p2 )
{
	// This array is used to sort surfaces in the order they should be executed(), lower numbers are applied first
	static int SURFACE_SORT_ORDER[] = 
	{
		1, // DIFFUSE
		2, // LIGHTMAP
		0, // BUMPMAP
		3, // DARKMAP
		4, // DETAILMAP
		5, // GLOSSMAP
		6, // NORMALMAP
		7, // PARALLAXMAP
		8, // DECALMAP
		9, // SHADERMAP
	};

	return SURFACE_SORT_ORDER[ p1.m_eType ] - SURFACE_SORT_ORDER[ p2.m_eType ];
}

//------------------------------------------------------------------------------------
// EOF

