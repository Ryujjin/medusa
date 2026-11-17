/*
	Replacement for 
	Shader.cpp

*/

#include "Debug/Trace.h"
#include "File/FileDisk.h"
//#include "Shader.h" REM Shawn old code, replaced with d3d10shader.h
#include "ShaderD3D11.h"
#include "DisplayDeviceD3D.h"

#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

//---------------------------------------------------------------------------------------------------

// Default constant buffer size (adjust as needed)
const UINT DEFAULT_CB_SIZE = 256;

//---------------------------------------------------------------------------------------------------

Shader::Shader() :
	m_bReleased(true),
	m_nVertexType(0),
	m_pVSBlob(NULL),
	m_pVSLBlob(NULL),
	m_pPSBlob(NULL),
	m_pVertexShader(NULL),
	m_pVertexShaderL(NULL),
	m_pPixelShader(NULL),
	m_pInputLayout(NULL),
	m_pInputLayoutL(NULL),
	m_pVSConstantBuffer(NULL),
	m_pVSGBlob(NULL),
	m_pPSGBlob(NULL),
	m_pVertexShaderGalaxy(NULL),
	m_pPixelShaderGalaxy(NULL),
	m_pVSGSBlob(NULL),
	m_pPSGSBlob(NULL),
	m_pVertexShaderGalSpr(NULL),
	m_pPixelShaderGalSpr(NULL),
	m_pVSSTBlob(NULL),
	m_pPSSTBlob(NULL),
	m_pVertexShaderStar(NULL),
	m_pPixelShaderStar(NULL),
	m_pPSSPHBlob(NULL),
	m_pPixelShaderStarSphere(NULL),
	m_pPSCSBlob(NULL),
	m_pPixelShaderCoronaShell(NULL),
	m_pVSSPH2Blob(NULL),
	m_pVertexShaderStarSphere(NULL),
	m_pVSTBlob(NULL),
	m_pHSBlob(NULL),
	m_pDSBlob(NULL),
	m_pCSBlob(NULL),
	m_pVertexShaderTess(NULL),
	m_pHullShader(NULL),
	m_pDomainShader(NULL),
	m_pComputeShader(NULL),
	m_pVertexShaderVideo(NULL),
	m_pPixelShaderVideo(NULL),
	m_pVSVIDBlob(NULL),
	m_pPSVIDBlob(NULL),
	m_pPSPPBlob(NULL),
	m_pPixelShaderProcPlanet(NULL),
	m_pPSSURFBlob(NULL),
	m_pPixelShaderPlanetSurface(NULL),
	m_pPSATMBlob(NULL),
	m_pPixelShaderAtmosphere(NULL),
	m_pPSConstantBuffer(NULL)
{
}

Shader::~Shader()
{
	release();
}

//---------------------------------------------------------------------------------------------------

bool Shader::valid() const
{
	return m_pVertexShader != NULL && m_pPixelShader != NULL;
}

bool Shader::released() const
{
	return m_bReleased;
}

//---------------------------------------------------------------------------------------------------

bool Shader::load(DisplayDeviceD3D* pDevice, const char* pShaderFile, int nVertexType /*= 0*/)
{
	if (!pDevice || !pShaderFile)
		return false;

	release();

	m_sShaderFile = pShaderFile;
	m_nVertexType = nVertexType;

	if (!compileShader(pDevice, pShaderFile))
	{
		LOG_ERROR("Shader", CharString().format("Failed to compile shader: %s", pShaderFile));
		return false;
	}

	if (!createInputLayout(pDevice))
	{
		LOG_ERROR("Shader", CharString().format("Failed to create input layout: %s", pShaderFile));
		return false;
	}

	if (!createConstantBuffers(pDevice))
	{
		LOG_ERROR("Shader", CharString().format("Failed to create constant buffers: %s", pShaderFile));
		return false;
	}

	m_bReleased = false;
	return true;
}

void Shader::release()
{
	RELEASEDD(m_pVSConstantBuffer);
	RELEASEDD(m_pPSConstantBuffer);
	RELEASEDD(m_pInputLayoutL);
	RELEASEDD(m_pInputLayout);
	RELEASEDD(m_pVertexShaderL);
	RELEASEDD(m_pVertexShader);
	RELEASEDD(m_pPixelShader);
	RELEASEDD(m_pVSLBlob);
	RELEASEDD(m_pPixelShaderGalaxy);
	RELEASEDD(m_pVertexShaderGalaxy);
	RELEASEDD(m_pPSGBlob);
	RELEASEDD(m_pVSGBlob);
	RELEASEDD(m_pPixelShaderGalSpr);
	RELEASEDD(m_pVertexShaderGalSpr);
	RELEASEDD(m_pPSGSBlob);
	RELEASEDD(m_pVSGSBlob);
	RELEASEDD(m_pPixelShaderStarSphere);
	RELEASEDD(m_pPSSPHBlob);
	RELEASEDD(m_pPixelShaderCoronaShell);
	RELEASEDD(m_pPSCSBlob);
	RELEASEDD(m_pVertexShaderStarSphere);
	RELEASEDD(m_pVSSPH2Blob);
	RELEASEDD(m_pPixelShaderStar);
	RELEASEDD(m_pVertexShaderStar);
	RELEASEDD(m_pPSSTBlob);
	RELEASEDD(m_pVSSTBlob);
	RELEASEDD(m_pComputeShader);
	RELEASEDD(m_pDomainShader);
	RELEASEDD(m_pHullShader);
	RELEASEDD(m_pVertexShaderTess);
	RELEASEDD(m_pCSBlob);
	RELEASEDD(m_pDSBlob);
	RELEASEDD(m_pHSBlob);
	RELEASEDD(m_pVSTBlob);
	RELEASEDD(m_pPixelShaderVideo);
	RELEASEDD(m_pVertexShaderVideo);
	RELEASEDD(m_pPSVIDBlob);
	RELEASEDD(m_pVSVIDBlob);
	RELEASEDD(m_pPixelShaderProcPlanet);
	RELEASEDD(m_pPSPPBlob);
	RELEASEDD(m_pPixelShaderPlanetSurface);
	RELEASEDD(m_pPSSURFBlob);
	RELEASEDD(m_pPixelShaderAtmosphere);
	RELEASEDD(m_pPSATMBlob);
	RELEASEDD(m_pVSBlob);
	RELEASEDD(m_pPSBlob);

	m_Constants.clear();
	m_bReleased = true;
}

bool Shader::apply(DisplayDeviceD3D* pDevice)
{
	if (!valid() || !pDevice || !pDevice->getContext())
		return false;

	ID3D11DeviceContext* pContext = pDevice->getContext();

	// Set shaders
	pContext->VSSetShader(m_pVertexShader, NULL, 0);
	pContext->PSSetShader(m_pPixelShader, NULL, 0);

	// Set input layout
	if (m_pInputLayout)
		pContext->IASetInputLayout(m_pInputLayout);

	// Update and set constant buffers
	updateConstantBuffers(pDevice);

	if (m_pVSConstantBuffer)
		pContext->VSSetConstantBuffers(0, 1, &m_pVSConstantBuffer);

	if (m_pPSConstantBuffer)
		pContext->PSSetConstantBuffers(0, 1, &m_pPSConstantBuffer);

	return true;
}

bool Shader::applyVertexL(DisplayDeviceD3D* pDevice)
{
	if (!pDevice || !pDevice->getContext())
		return false;
	if (!m_pVertexShaderL || !m_pInputLayoutL)
		return false;

	// Switch to the pre-lit VS and VertexL input layout.
	// Constants and PS are already bound by apply(); only VS + layout change.
	ID3D11DeviceContext* pContext = pDevice->getContext();
	pContext->VSSetShader(m_pVertexShaderL, NULL, 0);
	pContext->IASetInputLayout(m_pInputLayoutL);
	return true;
}

bool Shader::applyGalaxy(DisplayDeviceD3D* pDevice)
{
	if (!pDevice || !pDevice->getContext())
		return false;
	if (!m_pVertexShaderGalaxy || !m_pPixelShaderGalaxy)
		return false;

	ID3D11DeviceContext* pContext = pDevice->getContext();

	// Re-upload constants so vGnpDir/vGalCenDir reach the GPU.
	updateConstantBuffers(pDevice);

	// Fullscreen triangle: no input layout, SV_VertexID only.
	pContext->VSSetShader(m_pVertexShaderGalaxy, NULL, 0);
	pContext->PSSetShader(m_pPixelShaderGalaxy,  NULL, 0);
	pContext->IASetInputLayout(NULL);

	if (m_pVSConstantBuffer)
		pContext->VSSetConstantBuffers(0, 1, &m_pVSConstantBuffer);
	if (m_pPSConstantBuffer)
		pContext->PSSetConstantBuffers(0, 1, &m_pPSConstantBuffer);

	return true;
}

bool Shader::applyGalaxySprite(DisplayDeviceD3D* pDevice)
{
	if (!pDevice || !pDevice->getContext())
		return false;
	if (!m_pVertexShaderGalSpr || !m_pPixelShaderGalSpr)
		return false;
	if (!m_pInputLayoutL)
		return false;

	ID3D11DeviceContext* pContext = pDevice->getContext();

	updateConstantBuffers(pDevice);

	// Galaxy sprite billboards use the VertexL input layout (pos/normal/color/uv).
	pContext->VSSetShader(m_pVertexShaderGalSpr, NULL, 0);
	pContext->PSSetShader(m_pPixelShaderGalSpr,  NULL, 0);
	pContext->IASetInputLayout(m_pInputLayoutL);

	if (m_pVSConstantBuffer)
		pContext->VSSetConstantBuffers(0, 1, &m_pVSConstantBuffer);
	if (m_pPSConstantBuffer)
		pContext->PSSetConstantBuffers(0, 1, &m_pPSConstantBuffer);

	return true;
}

bool Shader::applyStarSurface(DisplayDeviceD3D* pDevice)
{
	if (!pDevice || !pDevice->getContext())
		return false;
	if (!m_pVertexShaderStar || !m_pPixelShaderStar)
		return false;
	if (!m_pInputLayoutL)
		return false;

	ID3D11DeviceContext* pContext = pDevice->getContext();

	// Re-upload constants so vStarColor/vStarParams reach the GPU.
	updateConstantBuffers(pDevice);

	// Star surface billboard uses the VertexL input layout (pos/normal/color/uv).
	pContext->VSSetShader(m_pVertexShaderStar, NULL, 0);
	pContext->PSSetShader(m_pPixelShaderStar,  NULL, 0);
	pContext->IASetInputLayout(m_pInputLayoutL);

	if (m_pVSConstantBuffer)
		pContext->VSSetConstantBuffers(0, 1, &m_pVSConstantBuffer);
	if (m_pPSConstantBuffer)
		pContext->PSSetConstantBuffers(0, 1, &m_pPSConstantBuffer);

	return true;
}
bool Shader::applyStarSphere(DisplayDeviceD3D* pDevice)
{
if (!pDevice || !pDevice->getContext())
return false;
// Requires tessellation pipeline + PS_StarSphere.
if (!m_pVertexShaderTess || !m_pHullShader || !m_pDomainShader || !m_pPixelShaderStarSphere)
return false;
if (!m_pInputLayout)
return false;

ID3D11DeviceContext* pContext = pDevice->getContext();
updateConstantBuffers(pDevice);

// Tessellation path: VS_Tess -> HS_Main -> DS_Main -> PS_StarSphere
pContext->VSSetShader(m_pVertexShaderTess, NULL, 0);
pContext->HSSetShader(m_pHullShader,       NULL, 0);
pContext->DSSetShader(m_pDomainShader,     NULL, 0);
pContext->PSSetShader(m_pPixelShaderStarSphere, NULL, 0);
pContext->IASetInputLayout(m_pInputLayout);

if (m_pVSConstantBuffer)
{
pContext->VSSetConstantBuffers(0, 1, &m_pVSConstantBuffer);
pContext->HSSetConstantBuffers(0, 1, &m_pVSConstantBuffer);
pContext->DSSetConstantBuffers(0, 1, &m_pVSConstantBuffer);
}
if (m_pPSConstantBuffer)
pContext->PSSetConstantBuffers(0, 1, &m_pPSConstantBuffer);

return true;
}

bool Shader::applyStarSphere3D(DisplayDeviceD3D* pDevice)
{
	if (!pDevice || !pDevice->getContext())
		return false;
	// Requires VS_StarSphere + PS_StarSphere + standard Vertex input layout.
	if (!m_pVertexShaderStarSphere || !m_pPixelShaderStarSphere)
		return false;
	if (!m_pInputLayout)
		return false;

	ID3D11DeviceContext* pContext = pDevice->getContext();
	updateConstantBuffers(pDevice);

	// Standard non-tessellated pipeline: VS_StarSphere -> PS_StarSphere
	// Clear any leftover hull/domain shaders from a previous tessellation pass.
	pContext->HSSetShader(NULL, NULL, 0);
	pContext->DSSetShader(NULL, NULL, 0);
	pContext->VSSetShader(m_pVertexShaderStarSphere, NULL, 0);
	pContext->PSSetShader(m_pPixelShaderStarSphere,  NULL, 0);
	pContext->IASetInputLayout(m_pInputLayout);

	if (m_pVSConstantBuffer)
		pContext->VSSetConstantBuffers(0, 1, &m_pVSConstantBuffer);
	if (m_pPSConstantBuffer)
		pContext->PSSetConstantBuffers(0, 1, &m_pPSConstantBuffer);

	return true;
}

bool Shader::applyStarCoronaShell(DisplayDeviceD3D* pDevice)
{
	if (!pDevice || !pDevice->getContext())
		return false;
	// Requires VS_StarSphere + PS_CoronaShell + standard Vertex input layout.
	if (!m_pVertexShaderStarSphere || !m_pPixelShaderCoronaShell)
		return false;
	if (!m_pInputLayout)
		return false;

	ID3D11DeviceContext* pContext = pDevice->getContext();
	updateConstantBuffers(pDevice);

	// Shell pipeline: VS_StarSphere -> PS_CoronaShell
	pContext->HSSetShader(NULL, NULL, 0);
	pContext->DSSetShader(NULL, NULL, 0);
	pContext->VSSetShader(m_pVertexShaderStarSphere, NULL, 0);
	pContext->PSSetShader(m_pPixelShaderCoronaShell, NULL, 0);
	pContext->IASetInputLayout(m_pInputLayout);

	if (m_pVSConstantBuffer)
		pContext->VSSetConstantBuffers(0, 1, &m_pVSConstantBuffer);
	if (m_pPSConstantBuffer)
		pContext->PSSetConstantBuffers(0, 1, &m_pPSConstantBuffer);

	return true;
}

bool Shader::applyTessellation(DisplayDeviceD3D* pDevice)
{
	if (!pDevice || !pDevice->getContext())
		return false;
	if (!m_pVertexShaderTess || !m_pHullShader || !m_pDomainShader)
		return false;

	ID3D11DeviceContext* pContext = pDevice->getContext();

	updateConstantBuffers(pDevice);

	// VS_Tess outputs world-space POSITION to HS; DS_Main reprojects to clip-space.
	pContext->VSSetShader(m_pVertexShaderTess, NULL, 0);
	pContext->HSSetShader(m_pHullShader,       NULL, 0);
	pContext->DSSetShader(m_pDomainShader,     NULL, 0);
	if (m_pInputLayout)
		pContext->IASetInputLayout(m_pInputLayout);

	// Push constants to all active stages.
	if (m_pVSConstantBuffer)
	{
		pContext->VSSetConstantBuffers(0, 1, &m_pVSConstantBuffer);
		pContext->HSSetConstantBuffers(0, 1, &m_pVSConstantBuffer);
		pContext->DSSetConstantBuffers(0, 1, &m_pVSConstantBuffer);
	}
	if (m_pPSConstantBuffer)
		pContext->PSSetConstantBuffers(0, 1, &m_pPSConstantBuffer);

	return true;
}

void Shader::clearTessellation(DisplayDeviceD3D* pDevice)
{
	if (!pDevice || !pDevice->getContext())
		return;
	ID3D11DeviceContext* pContext = pDevice->getContext();
	pContext->HSSetShader(NULL, NULL, 0);
	pContext->DSSetShader(NULL, NULL, 0);
}

bool Shader::applyCompute(DisplayDeviceD3D* pDevice)
{
	if (!pDevice || !pDevice->getContext())
		return false;
	if (!m_pComputeShader)
		return false;

	ID3D11DeviceContext* pContext = pDevice->getContext();
	updateConstantBuffers(pDevice);
	pContext->CSSetShader(m_pComputeShader, NULL, 0);
	if (m_pVSConstantBuffer)
		pContext->CSSetConstantBuffers(0, 1, &m_pVSConstantBuffer);
	return true;
}

void Shader::dispatch(DisplayDeviceD3D* pDevice, UINT x, UINT y, UINT z)
{
	if (!pDevice || !pDevice->getContext() || !m_pComputeShader)
		return;
	pDevice->getContext()->Dispatch(x, y, z);
	// Unbind compute shader so it does not interfere with graphics pipeline.
	pDevice->getContext()->CSSetShader(NULL, NULL, 0);
}

bool Shader::applyVideo(DisplayDeviceD3D* pDevice)
{
	if (!pDevice || !pDevice->getContext())
		return false;
	if (!m_pVertexShaderVideo || !m_pPixelShaderVideo)
		return false;

	ID3D11DeviceContext* pContext = pDevice->getContext();

	// Re-upload constants so vVideoParams reaches the GPU.
	updateConstantBuffers(pDevice);

	// Fullscreen triangle: no vertex buffer, SV_VertexID only.
	pContext->VSSetShader(m_pVertexShaderVideo, NULL, 0);
	pContext->PSSetShader(m_pPixelShaderVideo,  NULL, 0);
	pContext->IASetInputLayout(NULL);

	if (m_pVSConstantBuffer)
		pContext->VSSetConstantBuffers(0, 1, &m_pVSConstantBuffer);
	if (m_pPSConstantBuffer)
		pContext->PSSetConstantBuffers(0, 1, &m_pPSConstantBuffer);

	return true;
}

//---------------------------------------------------------------------------------------------------

bool Shader::applyProceduralPlanet(DisplayDeviceD3D* pDevice)
{
	if (!pDevice || !pDevice->getContext())
		return false;
	if (!m_pVertexShader || !m_pPixelShaderProcPlanet)
		return false;

	ID3D11DeviceContext* pContext = pDevice->getContext();

	// Re-upload constants so vPlanetColor, fTerrainScale, nOctaves reach the GPU.
	updateConstantBuffers(pDevice);

	// Standard vertex shader + procedural planet pixel shader + standard input layout.
	pContext->VSSetShader(m_pVertexShader, NULL, 0);
	pContext->PSSetShader(m_pPixelShaderProcPlanet, NULL, 0);
	pContext->IASetInputLayout(m_pInputLayout);

	if (m_pVSConstantBuffer)
		pContext->VSSetConstantBuffers(0, 1, &m_pVSConstantBuffer);
	if (m_pPSConstantBuffer)
		pContext->PSSetConstantBuffers(0, 1, &m_pPSConstantBuffer);

	return true;
}

//---------------------------------------------------------------------------------------------------

bool Shader::applyPlanetSurface(DisplayDeviceD3D* pDevice)
{
	if (!pDevice || !pDevice->getContext())
		return false;
	if (!m_pVertexShader || !m_pPixelShaderPlanetSurface)
		return false;

	ID3D11DeviceContext* pContext = pDevice->getContext();

	// Re-upload constants so vLightPosition / vCameraPos reach the GPU for
	// the day/night and specular calculations in PS_PlanetSurface.
	updateConstantBuffers(pDevice);

	// VS_Main + PS_PlanetSurface + standard Vertex input layout.
	pContext->VSSetShader(m_pVertexShader, NULL, 0);
	pContext->PSSetShader(m_pPixelShaderPlanetSurface, NULL, 0);
	pContext->IASetInputLayout(m_pInputLayout);

	if (m_pVSConstantBuffer)
		pContext->VSSetConstantBuffers(0, 1, &m_pVSConstantBuffer);
	if (m_pPSConstantBuffer)
		pContext->PSSetConstantBuffers(0, 1, &m_pPSConstantBuffer);

	return true;
}

//---------------------------------------------------------------------------------------------------

bool Shader::applyAtmosphere(DisplayDeviceD3D* pDevice)
{
	if (!pDevice || !pDevice->getContext())
		return false;
	if (!m_pVertexShaderL || !m_pPixelShaderAtmosphere)
		return false;
	if (!m_pInputLayoutL)
		return false;

	ID3D11DeviceContext* pContext = pDevice->getContext();

	// Mark this draw as the atmosphere shell so PS_Main's Fresnel rim block only
	// fires for this primitive, never for jump gates / shields / other alpha-blended
	// unlit meshes that happen to share the same bAlphaBlend+bEnableLighting+normal
	// heuristic. Cleared immediately after upload so it never leaks into later draws.
	setConstant("bAtmosphereShell", true);

	// Re-upload constants so vLightPosition / vCameraPos / atmosphere colour
	// params reach the GPU for the rim-glow calculation in PS_Atmosphere.
	updateConstantBuffers(pDevice);

	setConstant("bAtmosphereShell", false);

	// Clear any leftover hull/domain shaders from a previous tessellation pass.
	pContext->HSSetShader(NULL, NULL, 0);
	pContext->DSSetShader(NULL, NULL, 0);

	// VS_MainL + PS_Atmosphere + VertexL input layout.
	pContext->VSSetShader(m_pVertexShaderL, NULL, 0);
	pContext->PSSetShader(m_pPixelShaderAtmosphere, NULL, 0);
	pContext->IASetInputLayout(m_pInputLayoutL);

	if (m_pVSConstantBuffer)
		pContext->VSSetConstantBuffers(0, 1, &m_pVSConstantBuffer);
	if (m_pPSConstantBuffer)
		pContext->PSSetConstantBuffers(0, 1, &m_pPSConstantBuffer);

	return true;
}

//---------------------------------------------------------------------------------------------------
// Constant Management
//---------------------------------------------------------------------------------------------------

void Shader::setConstant(const char* pName, bool value)
{
	std::string name(pName);
	Constant& constant = m_Constants[name];
	constant.type = Constant::BOOL;
	constant.bValue = value;
}

void Shader::setConstant(const char* pName, int value)
{
	std::string name(pName);
	Constant& constant = m_Constants[name];
	constant.type = Constant::INT;
	constant.nValue = value;
}

void Shader::setConstant(const char* pName, float value)
{
	std::string name(pName);
	Constant& constant = m_Constants[name];
	constant.type = Constant::FLOAT;
	constant.fValue = value;
}

void Shader::setConstant(const char* pName, const Vector3& value)
{
	std::string name(pName);
	Constant& constant = m_Constants[name];
	constant.type = Constant::VECTOR3;
	constant.vValue = value;
}

void Shader::setConstant(const char* pName, const Matrix33& value)
{
	std::string name(pName);
	Constant& constant = m_Constants[name];
	constant.type = Constant::MATRIX33;
	constant.mValue = value;
}

void Shader::setConstant(const char* pName, const DirectX::XMMATRIX& value)
{
	std::string name(pName);
	Constant& constant = m_Constants[name];
	constant.type = Constant::XMMATRIX;
	constant.xmMatrix = value;
}

void Shader::setConstant(const char* pName, const DirectX::XMFLOAT4& value)
{
	std::string name(pName);
	Constant& constant = m_Constants[name];
	constant.type = Constant::XMFLOAT4;
	constant.xmFloat4 = value;
}

void Shader::setConstant(const char* pName, Color value)
{
	std::string name(pName);
	Constant& constant = m_Constants[name];
	constant.type = Constant::COLOR;
	constant.cValue = value;
}

void Shader::clearConstant(const char* pName)
{
    m_Constants.erase(std::string(pName));
}

//---------------------------------------------------------------------------------------------------
// Private Helper Methods
//---------------------------------------------------------------------------------------------------

// Forward declare the structure to get its size
struct ShaderConstants
{
	DirectX::XMMATRIX mWorld;
	DirectX::XMMATRIX mView;
	DirectX::XMMATRIX mProj;
	DirectX::XMFLOAT4 vCameraPos;
	DirectX::XMMATRIX mLightView;
	DirectX::XMMATRIX mLightProj;

	// Material properties
	DirectX::XMFLOAT4 vMatDiffuse;
	DirectX::XMFLOAT4 vMatSpecular;
	float fMatSpecularPower;
	DirectX::XMFLOAT3 _pad1;
	DirectX::XMFLOAT4 vMatAmbient;
	DirectX::XMFLOAT4 vMatEmissive;

	// Global ambient
	int bEnableAmbient;
	DirectX::XMFLOAT3 _pad2;
	DirectX::XMFLOAT4 vGlobalAmbient;

	// Light properties
	int nLightType;
	DirectX::XMFLOAT3 _pad3;
	DirectX::XMFLOAT4 vLightDiffuse;
	DirectX::XMFLOAT4 vLightSpecular;
	DirectX::XMFLOAT4 vLightAmbient;
	DirectX::XMFLOAT4 vLightPosition;
	DirectX::XMFLOAT4 vLightDirection;
	DirectX::XMFLOAT4 vAttenuation;

	// Shadow map
	int bEnableShadowMap;
	DirectX::XMFLOAT3 _pad4;
	DirectX::XMFLOAT4 vShadowMapSize;

	// Texture flags
	int bEnableDiffuse;
	int bEnableLightMap;
	int bEnableBumpMap;
	float fBumpDepth;

	// Lighting control
	int bEnableLighting;
	float fTime;                   // elapsed seconds — drives star twinkle animation
	int bEnableNormalMap;          // tangent-space normal map at t3
	int bAlphaBlend;               // 1 when material uses ALPHA/ALPHA_INV blend (atmosphere shells, glass, etc)
	int bAtmosphereShell;          // 1 ONLY for the planet atmosphere shell primitive -- gates the Fresnel rim so it never fires on jump gates / shields / other alpha meshes
	DirectX::XMFLOAT3 _pad5;       // keeps vGnpDir 16-byte aligned (matches HLSL implicit cbuffer packing)

	// Galaxy band constants (world-space galactic frame)
	DirectX::XMFLOAT3 vGnpDir;     // galactic north pole direction
	float             _pad6;
	DirectX::XMFLOAT3 vGalCenDir;  // galactic centre direction
	float             _pad7;

	// Star surface constants
	DirectX::XMFLOAT4 vStarColor;  // RGB = spectral color (0..1), A = intensity
	DirectX::XMFLOAT4 vStarParams; // x = radius, y = flare power, z/w unused

	// Tessellation + Compute
	float fTessFactor;             // tessellation scale, 0 = disabled
	float fDeltaTime;              // per-frame delta time in seconds (CS particle update)
	DirectX::XMFLOAT2 _pad8;

	// Video playback
	DirectX::XMFLOAT4 vVideoParams; // x = alpha (0..1), y = letterbox (0=stretch,1=16:9), z/w unused
};

// Constant buffer for UI / screen-space shaders (VB_VERTEXTL).
// Must match cbuffer UIConstants in UI_D3D11.fx exactly.
struct UIShaderConstants
{
	float g_ScreenWidth;
	float g_ScreenHeight;
	int   bEnableDiffuse;
	float _pad;
};

bool Shader::compileShader(DisplayDeviceD3D* pDevice, const char* pShaderFile)
{
	HRESULT hr;

	// Compile flags
	UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
	const char* pCacheSuffix = ".dbg";
#else
	const char* pCacheSuffix = "";
#endif

	// Shader bytecode cache paths. Separate debug/release caches via suffix so
	// debug symbols are not mixed with optimised bytecode.
	// e.g. "Shaders/Default_D3D11.fx.dbg.vs.cso"
	CharString sCacheVS  = CharString().format("%s%s.vs.cso",  pShaderFile, pCacheSuffix);
	CharString sCachePS  = CharString().format("%s%s.ps.cso",  pShaderFile, pCacheSuffix);
	CharString sCacheVSL = CharString().format("%s%s.vsl.cso", pShaderFile, pCacheSuffix);
	CharString sCacheVSG  = CharString().format("%s%s.vsg.cso",  pShaderFile, pCacheSuffix);
	CharString sCachePSG  = CharString().format("%s%s.psg.cso",  pShaderFile, pCacheSuffix);
	CharString sCacheVSGS = CharString().format("%s%s.vsgs.cso", pShaderFile, pCacheSuffix);
	CharString sCachePSGS = CharString().format("%s%s.psgs.cso", pShaderFile, pCacheSuffix);
	CharString sCacheVSST = CharString().format("%s%s.vsst.cso", pShaderFile, pCacheSuffix);
	CharString sCachePSST = CharString().format("%s%s.psst.cso", pShaderFile, pCacheSuffix);
	CharString sCacheVST  = CharString().format("%s%s.vst.cso",  pShaderFile, pCacheSuffix);
	CharString sCacheHS   = CharString().format("%s%s.hs.cso",   pShaderFile, pCacheSuffix);
	CharString sCacheDS   = CharString().format("%s%s.ds.cso",   pShaderFile, pCacheSuffix);
	CharString sCacheCS   = CharString().format("%s%s.cs.cso",   pShaderFile, pCacheSuffix);
	CharString sCachePSSPH  = CharString().format("%s%s.pssph.cso",   pShaderFile, pCacheSuffix);
	CharString sCachePSCS   = CharString().format("%s%s.pscorona.cso", pShaderFile, pCacheSuffix);
	CharString sCacheVSSPH2 = CharString().format("%s%s.vssph2.cso",  pShaderFile, pCacheSuffix);
	CharString sCacheVSVID  = CharString().format("%s%s.vsvid.cso",   pShaderFile, pCacheSuffix);
	CharString sCachePSVID  = CharString().format("%s%s.psvid.cso",   pShaderFile, pCacheSuffix);
	CharString sCachePSSURF = CharString().format("%s%s.pssurf.cso",  pShaderFile, pCacheSuffix);
	CharString sCachePSATM  = CharString().format("%s%s.psatm.cso",  pShaderFile, pCacheSuffix);
	CharString sCachePSPP   = CharString().format("%s%s.pspp.cso",   pShaderFile, pCacheSuffix);

	// Returns true if pCache exists and its write time >= pSource's write time.
	auto isCacheNewer = [](const char* pCache, const char* pSource) -> bool {
		WIN32_FILE_ATTRIBUTE_DATA cacheInfo = {}, srcInfo = {};
		if (!GetFileAttributesExA(pCache, GetFileExInfoStandard, &cacheInfo))
			return false;
		if (!GetFileAttributesExA(pSource, GetFileExInfoStandard, &srcInfo))
			return false;
		return CompareFileTime(&cacheInfo.ftLastWriteTime, &srcInfo.ftLastWriteTime) >= 0;
	};

	// Widen a narrow path for D3DReadFileToBlob / D3DWriteBlobToFile.
	auto toWide = [](const char* pStr, wchar_t* pBuf, int nBufLen) {
		MultiByteToWideChar(CP_ACP, 0, pStr, -1, pBuf, nBufLen);
	};

	wchar_t wPath[MAX_PATH];

	// Try loading pre-compiled bytecode from cache. VS, PS, PSSPH, DS, HS, and VSL must
	// all be present and newer than the source file for the cache to be considered valid.
	bool bCacheValid = isCacheNewer(sCacheVS,     pShaderFile) &&
					   isCacheNewer(sCachePS,     pShaderFile) &&
					   isCacheNewer(sCachePSSPH,  pShaderFile) &&
					   isCacheNewer(sCacheVSSPH2, pShaderFile) &&
					   isCacheNewer(sCacheDS,     pShaderFile) &&
					   isCacheNewer(sCacheHS,     pShaderFile) &&
					   isCacheNewer(sCacheVSL,    pShaderFile) &&
					   isCacheNewer(sCacheVSVID,  pShaderFile) &&
					   isCacheNewer(sCachePSVID,  pShaderFile) &&
					   isCacheNewer(sCachePSSURF, pShaderFile) &&
					   isCacheNewer(sCachePSATM,  pShaderFile) &&
					   isCacheNewer(sCachePSPP,   pShaderFile);

	if (bCacheValid)
	{
		toWide(sCacheVS, wPath, MAX_PATH);
		if (FAILED(D3DReadFileToBlob(wPath, &m_pVSBlob)))
			bCacheValid = false;
	}
	if (bCacheValid)
	{
		toWide(sCachePS, wPath, MAX_PATH);
		if (FAILED(D3DReadFileToBlob(wPath, &m_pPSBlob)))
			bCacheValid = false;
	}
	if (!bCacheValid)
	{
		// Release any partial blobs from a failed cache read.
		RELEASEDD(m_pVSBlob);
		RELEASEDD(m_pPSBlob);
	}

	if (!bCacheValid)
	{
		// Cache miss — compile from source then write blobs to disk.
		ID3DBlob* pErrorBlob = NULL;

		FileDisk file;
		if (!file.open(pShaderFile, FileDisk::READ))
		{
			LOG_ERROR("Shader", CharString().format("Failed to open shader file: %s", pShaderFile));
			return false;
		}

		dword fileSize = file.size();
		char* pFileData = new char[fileSize + 1];
		file.read(pFileData, fileSize);
		pFileData[fileSize] = 0;
		file.close();

		// Compile vertex shader
		hr = D3DCompile(
			pFileData,
			fileSize,
			pShaderFile,
			NULL,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"VS_Main",
			"vs_5_0",
			compileFlags,
			0,
			&m_pVSBlob,
			&pErrorBlob
		);

		if (FAILED(hr))
		{
			if (pErrorBlob)
			{
				LOG_ERROR("Shader", CharString().format("VS compile error in %s: %s",
					pShaderFile, (char*)pErrorBlob->GetBufferPointer()));
				RELEASEDD(pErrorBlob);
			}
			delete[] pFileData;
			return false;
		}
		if (pErrorBlob) { RELEASEDD(pErrorBlob); }

		// Compile pixel shader
		hr = D3DCompile(
			pFileData,
			fileSize,
			pShaderFile,
			NULL,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"PS_Main",
			"ps_5_0",
			compileFlags,
			0,
			&m_pPSBlob,
			&pErrorBlob
		);

		if (FAILED(hr))
		{
			if (pErrorBlob)
			{
				LOG_ERROR("Shader", CharString().format("PS compile error in %s: %s",
					pShaderFile, (char*)pErrorBlob->GetBufferPointer()));
				RELEASEDD(pErrorBlob);
			}
			delete[] pFileData;
			return false;
		}
		if (pErrorBlob) { RELEASEDD(pErrorBlob); }

		// Compile VS_MainL (pre-lit vertex shader for VertexL geometry).
		// Non-fatal: older shader files without VS_MainL simply leave m_pVSLBlob NULL.
		ID3DBlob* pVSLErrorBlob = NULL;
		hr = D3DCompile(
			pFileData,
			fileSize,
			pShaderFile,
			NULL,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"VS_MainL",
			"vs_5_0",
			compileFlags,
			0,
			&m_pVSLBlob,
			&pVSLErrorBlob
		);

		// Compile VS_Galaxy + PS_Galaxy (non-fatal, procedural galaxy).
		ID3DBlob *pVSGErr = NULL, *pPSGErr = NULL;
		hr = D3DCompile(pFileData, fileSize, pShaderFile, NULL,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS_Galaxy", "vs_5_0",
			compileFlags, 0, &m_pVSGBlob, &pVSGErr);
			if (FAILED(hr)) { RELEASEDD(m_pVSGBlob); }
		RELEASEDD(pVSGErr);
		hr = D3DCompile(pFileData, fileSize, pShaderFile, NULL,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS_Galaxy", "ps_5_0",
			compileFlags, 0, &m_pPSGBlob, &pPSGErr);
			if (FAILED(hr)) { RELEASEDD(m_pPSGBlob); }
		RELEASEDD(pPSGErr);

		// Compile VS_GalaxySprite + PS_GalaxySprite (non-fatal, survey galaxy atlas).
		ID3DBlob *pVSGSErr = NULL, *pPSGSErr = NULL;
		hr = D3DCompile(pFileData, fileSize, pShaderFile, NULL,
			D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS_GalaxySprite", "vs_5_0",
			compileFlags, 0, &m_pVSGSBlob, &pVSGSErr);
			if (FAILED(hr)) { RELEASEDD(m_pVSGSBlob); }
		RELEASEDD(pVSGSErr);
		hr = D3DCompile(pFileData, fileSize, pShaderFile, NULL,
			D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS_GalaxySprite", "ps_5_0",
			compileFlags, 0, &m_pPSGSBlob, &pPSGSErr);
			if (FAILED(hr)) { RELEASEDD(m_pPSGSBlob); }
		RELEASEDD(pPSGSErr);

		// Compile VS_Star + PS_Star (non-fatal, procedural star surface billboard).
		ID3DBlob *pVSSTErr = NULL, *pPSSTErr = NULL;
		hr = D3DCompile(pFileData, fileSize, pShaderFile, NULL,
			D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS_Star", "vs_5_0",
			compileFlags, 0, &m_pVSSTBlob, &pVSSTErr);
			if (FAILED(hr)) { RELEASEDD(m_pVSSTBlob); }
		RELEASEDD(pVSSTErr);
		hr = D3DCompile(pFileData, fileSize, pShaderFile, NULL,
			D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS_Star", "ps_5_0",
			compileFlags, 0, &m_pPSSTBlob, &pPSSTErr);
			if (FAILED(hr)) { RELEASEDD(m_pPSSTBlob); }
		RELEASEDD(pPSSTErr);

		// Compile PS_StarSphere (non-fatal, tessellated sphere variant of PS_Star).
ID3DBlob *pPSSPHErr = NULL;
hr = D3DCompile(pFileData, fileSize, pShaderFile, NULL,
D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS_StarSphere", "ps_5_0",
compileFlags, 0, &m_pPSSPHBlob, &pPSSPHErr);
if (FAILED(hr)) { RELEASEDD(m_pPSSPHBlob); }
RELEASEDD(pPSSPHErr);

		// Compile PS_CoronaShell (non-fatal, 3D geometry shell corona).
		ID3DBlob *pPSCSErr = NULL;
		hr = D3DCompile(pFileData, fileSize, pShaderFile, NULL,
			D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS_CoronaShell", "ps_5_0",
			compileFlags, 0, &m_pPSCSBlob, &pPSCSErr);
			if (FAILED(hr)) { RELEASEDD(m_pPSCSBlob); }
		RELEASEDD(pPSCSErr);

		// Compile VS_StarSphere (non-fatal, 3D sphere mesh vertex shader for star surface).
		ID3DBlob *pVSSPH2Err = NULL;
		hr = D3DCompile(pFileData, fileSize, pShaderFile, NULL,
			D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS_StarSphere", "vs_5_0",
			compileFlags, 0, &m_pVSSPH2Blob, &pVSSPH2Err);
			if (FAILED(hr)) { RELEASEDD(m_pVSSPH2Blob); }
		RELEASEDD(pVSSPH2Err);

// Compile VS_Tess + HS_Main + DS_Main (non-fatal, hardware tessellation).
		ID3DBlob *pVSTErr=NULL, *pHSErr=NULL, *pDSErr=NULL;
		hr = D3DCompile(pFileData, fileSize, pShaderFile, NULL,
			D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS_Tess", "vs_5_0",
			compileFlags, 0, &m_pVSTBlob, &pVSTErr);
			if (FAILED(hr)) { RELEASEDD(m_pVSTBlob); }
		RELEASEDD(pVSTErr);
		hr = D3DCompile(pFileData, fileSize, pShaderFile, NULL,
			D3D_COMPILE_STANDARD_FILE_INCLUDE, "HS_Main", "hs_5_0",
			compileFlags, 0, &m_pHSBlob, &pHSErr);
			if (FAILED(hr)) { RELEASEDD(m_pHSBlob); }
		RELEASEDD(pHSErr);
		hr = D3DCompile(pFileData, fileSize, pShaderFile, NULL,
			D3D_COMPILE_STANDARD_FILE_INCLUDE, "DS_Main", "ds_5_0",
			compileFlags, 0, &m_pDSBlob, &pDSErr);
			if (FAILED(hr)) { RELEASEDD(m_pDSBlob); }
		RELEASEDD(pDSErr);

		// Compile CS_Main (non-fatal, GPU particle update compute shader).
		ID3DBlob *pCSErr = NULL;
		hr = D3DCompile(pFileData, fileSize, pShaderFile, NULL,
			D3D_COMPILE_STANDARD_FILE_INCLUDE, "CS_Main", "cs_5_0",
			compileFlags, 0, &m_pCSBlob, &pCSErr);
			if (FAILED(hr)) { RELEASEDD(m_pCSBlob); }
		RELEASEDD(pCSErr);

		// Compile VS_Video + PS_Video (non-fatal, fullscreen video playback).
		ID3DBlob *pVSVIDErr = NULL, *pPSVIDErr = NULL;
		hr = D3DCompile(pFileData, fileSize, pShaderFile, NULL,
			D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS_Video", "vs_5_0",
			compileFlags, 0, &m_pVSVIDBlob, &pVSVIDErr);
			if (FAILED(hr)) { RELEASEDD(m_pVSVIDBlob); }
		RELEASEDD(pVSVIDErr);
		hr = D3DCompile(pFileData, fileSize, pShaderFile, NULL,
			D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS_Video", "ps_5_0",
			compileFlags, 0, &m_pPSVIDBlob, &pPSVIDErr);
			if (FAILED(hr)) { RELEASEDD(m_pPSVIDBlob); }
		RELEASEDD(pPSVIDErr);

		// Compile PS_ProceduralPlanet (non-fatal, procedural planet terrain shader).
		ID3DBlob *pPSPPErr = NULL;
		hr = D3DCompile(pFileData, fileSize, pShaderFile, NULL,
			D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS_ProceduralPlanet", "ps_5_0",
			compileFlags, 0, &m_pPSPPBlob, &pPSPPErr);
			if (FAILED(hr)) { RELEASEDD(m_pPSPPBlob); }
		RELEASEDD(pPSPPErr);

		// Compile PS_PlanetSurface (non-fatal, Option-2 FBM surface overlay).
		ID3DBlob *pPSSURFErr = NULL;
		hr = D3DCompile(pFileData, fileSize, pShaderFile, NULL,
			D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS_PlanetSurface", "ps_5_0",
			compileFlags, 0, &m_pPSSURFBlob, &pPSSURFErr);
			if (FAILED(hr)) { RELEASEDD(m_pPSSURFBlob); }
		RELEASEDD(pPSSURFErr);

		// Compile PS_Atmosphere (non-fatal, planet atmosphere/cloud shell overlay).
		ID3DBlob *pPSATMErr = NULL;
		hr = D3DCompile(pFileData, fileSize, pShaderFile, NULL,
			D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS_Atmosphere", "ps_5_0",
			compileFlags, 0, &m_pPSATMBlob, &pPSATMErr);
			if (FAILED(hr)) { RELEASEDD(m_pPSATMBlob); }
		RELEASEDD(pPSATMErr);

		delete[] pFileData;  // all D3DCompile calls done — safe to free
		if (FAILED(hr))
		{
			RELEASEDD(m_pVSLBlob);
			RELEASEDD(pVSLErrorBlob);
		}
		else
		{
			RELEASEDD(pVSLErrorBlob);
		}

		// Write compiled blobs to cache for subsequent launches.
		toWide(sCacheVS, wPath, MAX_PATH);
		D3DWriteBlobToFile(m_pVSBlob, wPath, TRUE);
		toWide(sCachePS, wPath, MAX_PATH);
		D3DWriteBlobToFile(m_pPSBlob, wPath, TRUE);
		if (m_pVSLBlob)
		{
			toWide(sCacheVSL, wPath, MAX_PATH);
			D3DWriteBlobToFile(m_pVSLBlob, wPath, TRUE);
		}
		toWide(sCacheVSG, wPath, MAX_PATH);
		if (m_pVSGBlob)  D3DWriteBlobToFile(m_pVSGBlob,  wPath, TRUE);
		toWide(sCachePSG, wPath, MAX_PATH);
		if (m_pPSGBlob)  D3DWriteBlobToFile(m_pPSGBlob,  wPath, TRUE);
		toWide(sCacheVSGS, wPath, MAX_PATH);
		if (m_pVSGSBlob) D3DWriteBlobToFile(m_pVSGSBlob, wPath, TRUE);
		toWide(sCachePSGS, wPath, MAX_PATH);
		if (m_pPSGSBlob) D3DWriteBlobToFile(m_pPSGSBlob, wPath, TRUE);
		toWide(sCacheVSST, wPath, MAX_PATH);
		if (m_pVSSTBlob) D3DWriteBlobToFile(m_pVSSTBlob, wPath, TRUE);
		toWide(sCachePSST, wPath, MAX_PATH);
		if (m_pPSSTBlob) D3DWriteBlobToFile(m_pPSSTBlob, wPath, TRUE);
		toWide(sCacheVST, wPath, MAX_PATH);
		if (m_pVSTBlob) D3DWriteBlobToFile(m_pVSTBlob, wPath, TRUE);
		toWide(sCacheHS, wPath, MAX_PATH);
		if (m_pHSBlob)  D3DWriteBlobToFile(m_pHSBlob,  wPath, TRUE);
		toWide(sCacheDS, wPath, MAX_PATH);
		if (m_pDSBlob)  D3DWriteBlobToFile(m_pDSBlob,  wPath, TRUE);
		toWide(sCacheCS, wPath, MAX_PATH);
		if (m_pCSBlob)  D3DWriteBlobToFile(m_pCSBlob,  wPath, TRUE);
		toWide(sCachePSSPH, wPath, MAX_PATH);
		if (m_pPSSPHBlob)  D3DWriteBlobToFile(m_pPSSPHBlob,  wPath, TRUE);
		toWide(sCachePSCS, wPath, MAX_PATH);
		if (m_pPSCSBlob)   D3DWriteBlobToFile(m_pPSCSBlob,   wPath, TRUE);
		toWide(sCacheVSSPH2, wPath, MAX_PATH);
		if (m_pVSSPH2Blob) D3DWriteBlobToFile(m_pVSSPH2Blob, wPath, TRUE);
		toWide(sCacheVSVID, wPath, MAX_PATH);
		if (m_pVSVIDBlob) D3DWriteBlobToFile(m_pVSVIDBlob, wPath, TRUE);
		toWide(sCachePSVID, wPath, MAX_PATH);
		if (m_pPSVIDBlob) D3DWriteBlobToFile(m_pPSVIDBlob, wPath, TRUE);
		toWide(sCachePSSURF, wPath, MAX_PATH);
		if (m_pPSSURFBlob) D3DWriteBlobToFile(m_pPSSURFBlob, wPath, TRUE);
		toWide(sCachePSATM, wPath, MAX_PATH);
		if (m_pPSATMBlob) D3DWriteBlobToFile(m_pPSATMBlob, wPath, TRUE);
		toWide(sCachePSPP, wPath, MAX_PATH);
		if (m_pPSPPBlob) D3DWriteBlobToFile(m_pPSPPBlob, wPath, TRUE);
	}
	else
	{
		// Cache hit — also try loading the optional VSL blob.
		toWide(sCacheVSL, wPath, MAX_PATH);
		D3DReadFileToBlob(wPath, &m_pVSLBlob);  // non-fatal if absent
		toWide(sCacheVSG, wPath, MAX_PATH);
		D3DReadFileToBlob(wPath, &m_pVSGBlob);
		toWide(sCachePSG, wPath, MAX_PATH);
		D3DReadFileToBlob(wPath, &m_pPSGBlob);
		toWide(sCacheVSGS, wPath, MAX_PATH);
		D3DReadFileToBlob(wPath, &m_pVSGSBlob);
		toWide(sCachePSGS, wPath, MAX_PATH);
		D3DReadFileToBlob(wPath, &m_pPSGSBlob);
		toWide(sCacheVSST, wPath, MAX_PATH);
		D3DReadFileToBlob(wPath, &m_pVSSTBlob);
		toWide(sCachePSST, wPath, MAX_PATH);
		D3DReadFileToBlob(wPath, &m_pPSSTBlob);
		toWide(sCacheVST, wPath, MAX_PATH);
		D3DReadFileToBlob(wPath, &m_pVSTBlob);
		toWide(sCacheHS, wPath, MAX_PATH);
		D3DReadFileToBlob(wPath, &m_pHSBlob);
		toWide(sCacheDS, wPath, MAX_PATH);
		D3DReadFileToBlob(wPath, &m_pDSBlob);
		toWide(sCacheCS, wPath, MAX_PATH);
		D3DReadFileToBlob(wPath, &m_pCSBlob);
		toWide(sCachePSSPH, wPath, MAX_PATH);
		D3DReadFileToBlob(wPath, &m_pPSSPHBlob);
		toWide(sCachePSCS, wPath, MAX_PATH);
		D3DReadFileToBlob(wPath, &m_pPSCSBlob);
		toWide(sCacheVSSPH2, wPath, MAX_PATH);
		D3DReadFileToBlob(wPath, &m_pVSSPH2Blob);
		toWide(sCacheVSVID, wPath, MAX_PATH);
		D3DReadFileToBlob(wPath, &m_pVSVIDBlob);
		toWide(sCachePSVID, wPath, MAX_PATH);
		D3DReadFileToBlob(wPath, &m_pPSVIDBlob);
		toWide(sCachePSSURF, wPath, MAX_PATH);
		D3DReadFileToBlob(wPath, &m_pPSSURFBlob);  // non-fatal if absent
		toWide(sCachePSATM, wPath, MAX_PATH);
		D3DReadFileToBlob(wPath, &m_pPSATMBlob);  // non-fatal if absent
		toWide(sCachePSPP, wPath, MAX_PATH);
		D3DReadFileToBlob(wPath, &m_pPSPPBlob);  // non-fatal if absent
	}

	// Create vertex shader from blob
	try {
		hr = pDevice->getDevice()->CreateVertexShader(
			m_pVSBlob->GetBufferPointer(),
			m_pVSBlob->GetBufferSize(),
			NULL,
			&m_pVertexShader
		);
	} catch (...) {
		hr = E_FAIL;
		LOG_ERROR("Shader", "Exception creating vertex shader!");
	}

	if (FAILED(hr))
	{
		m_pVertexShader = NULL;
		LOG_ERROR("Shader", CharString().format("CreateVertexShader failed HR=0x%08X", hr));
		return false;
	}

	// Create pixel shader from blob.
	try {
		hr = pDevice->getDevice()->CreatePixelShader(
			m_pPSBlob->GetBufferPointer(),
			m_pPSBlob->GetBufferSize(),
			NULL,
			&m_pPixelShader
		);
	} catch (...) {
		hr = E_FAIL;
		LOG_ERROR("Shader", "Exception creating pixel shader!");
	}

	if (FAILED(hr))
	{
		m_pPixelShader = NULL;
		LOG_ERROR("Shader", CharString().format("CreatePixelShader failed HR=0x%08X", hr));
		return false;
	}

	// Create VS_MainL vertex shader if blob is available (cached or compiled).
	if (m_pVSLBlob)
	{
		try {
			hr = pDevice->getDevice()->CreateVertexShader(
				m_pVSLBlob->GetBufferPointer(),
				m_pVSLBlob->GetBufferSize(),
				NULL,
				&m_pVertexShaderL
			);
		} catch (...) {
			hr = E_FAIL;
		}
		if (FAILED(hr))
		{
			m_pVertexShaderL = NULL;
			RELEASEDD(m_pVSLBlob);
		}
	}

	// Create VS_Galaxy + PS_Galaxy if blobs available (non-fatal).
	if (m_pVSGBlob)
	{
		hr = pDevice->getDevice()->CreateVertexShader(
			m_pVSGBlob->GetBufferPointer(), m_pVSGBlob->GetBufferSize(), NULL, &m_pVertexShaderGalaxy);
		if (FAILED(hr)) { m_pVertexShaderGalaxy = NULL; RELEASEDD(m_pVSGBlob); }
	}
	if (m_pPSGBlob)
	{
		hr = pDevice->getDevice()->CreatePixelShader(
			m_pPSGBlob->GetBufferPointer(), m_pPSGBlob->GetBufferSize(), NULL, &m_pPixelShaderGalaxy);
		if (FAILED(hr)) { m_pPixelShaderGalaxy = NULL; RELEASEDD(m_pPSGBlob); }
	}

	// Create VS_GalaxySprite + PS_GalaxySprite if blobs available (non-fatal).
	if (m_pVSGSBlob)
	{
		hr = pDevice->getDevice()->CreateVertexShader(
			m_pVSGSBlob->GetBufferPointer(), m_pVSGSBlob->GetBufferSize(), NULL, &m_pVertexShaderGalSpr);
		if (FAILED(hr)) { m_pVertexShaderGalSpr = NULL; RELEASEDD(m_pVSGSBlob); }
	}
	if (m_pPSGSBlob)
	{
		hr = pDevice->getDevice()->CreatePixelShader(
			m_pPSGSBlob->GetBufferPointer(), m_pPSGSBlob->GetBufferSize(), NULL, &m_pPixelShaderGalSpr);
		if (FAILED(hr)) { m_pPixelShaderGalSpr = NULL; RELEASEDD(m_pPSGSBlob); }
	}

	// Create VS_Star + PS_Star if blobs available (non-fatal, procedural star surface).
	if (m_pVSSTBlob)
	{
		hr = pDevice->getDevice()->CreateVertexShader(
			m_pVSSTBlob->GetBufferPointer(), m_pVSSTBlob->GetBufferSize(), NULL, &m_pVertexShaderStar);
		if (FAILED(hr)) { m_pVertexShaderStar = NULL; RELEASEDD(m_pVSSTBlob); }
	}
	if (m_pPSSTBlob)
	{
		hr = pDevice->getDevice()->CreatePixelShader(
			m_pPSSTBlob->GetBufferPointer(), m_pPSSTBlob->GetBufferSize(), NULL, &m_pPixelShaderStar);
		if (FAILED(hr)) { m_pPixelShaderStar = NULL; RELEASEDD(m_pPSSTBlob); }
	}

	// Create PS_StarSphere if blob available (non-fatal, sphere variant).
	if (m_pPSSPHBlob)
	{
hr = pDevice->getDevice()->CreatePixelShader(
m_pPSSPHBlob->GetBufferPointer(), m_pPSSPHBlob->GetBufferSize(), NULL, &m_pPixelShaderStarSphere);
if (FAILED(hr)) { m_pPixelShaderStarSphere = NULL; RELEASEDD(m_pPSSPHBlob); }
	}

	// Create PS_CoronaShell if blob available (non-fatal, 3D geometry shell corona).
	if (m_pPSCSBlob)
	{
		hr = pDevice->getDevice()->CreatePixelShader(
			m_pPSCSBlob->GetBufferPointer(), m_pPSCSBlob->GetBufferSize(), NULL, &m_pPixelShaderCoronaShell);
		if (FAILED(hr)) { m_pPixelShaderCoronaShell = NULL; RELEASEDD(m_pPSCSBlob); }
	}

	// Create VS_StarSphere D3D object (non-fatal, 3D sphere vertex shader).
	if (m_pVSSPH2Blob)
	{
		hr = pDevice->getDevice()->CreateVertexShader(
			m_pVSSPH2Blob->GetBufferPointer(), m_pVSSPH2Blob->GetBufferSize(), NULL, &m_pVertexShaderStarSphere);
		if (FAILED(hr)) { m_pVertexShaderStarSphere = NULL; RELEASEDD(m_pVSSPH2Blob); }
	}

// Create VS_Tess vertex shader (non-fatal, tessellation pipeline).
	if (m_pVSTBlob)
	{
		hr = pDevice->getDevice()->CreateVertexShader(
			m_pVSTBlob->GetBufferPointer(), m_pVSTBlob->GetBufferSize(), NULL, &m_pVertexShaderTess);
		if (FAILED(hr)) { m_pVertexShaderTess = NULL; RELEASEDD(m_pVSTBlob); }
	}

	// Create HS_Main hull shader (non-fatal).
	if (m_pHSBlob)
	{
		hr = pDevice->getDevice()->CreateHullShader(
			m_pHSBlob->GetBufferPointer(), m_pHSBlob->GetBufferSize(), NULL, &m_pHullShader);
		if (FAILED(hr)) { m_pHullShader = NULL; RELEASEDD(m_pHSBlob); }
	}

	// Create DS_Main domain shader (non-fatal).
	if (m_pDSBlob)
	{
		hr = pDevice->getDevice()->CreateDomainShader(
			m_pDSBlob->GetBufferPointer(), m_pDSBlob->GetBufferSize(), NULL, &m_pDomainShader);
		if (FAILED(hr)) { m_pDomainShader = NULL; RELEASEDD(m_pDSBlob); }
	}

	// Create CS_Main compute shader (non-fatal).
	if (m_pCSBlob)
	{
		hr = pDevice->getDevice()->CreateComputeShader(
			m_pCSBlob->GetBufferPointer(), m_pCSBlob->GetBufferSize(), NULL, &m_pComputeShader);
		if (FAILED(hr)) { m_pComputeShader = NULL; RELEASEDD(m_pCSBlob); }
	}

	// Create VS_Video + PS_Video (non-fatal, video playback path).
	if (m_pVSVIDBlob)
	{
		hr = pDevice->getDevice()->CreateVertexShader(
			m_pVSVIDBlob->GetBufferPointer(), m_pVSVIDBlob->GetBufferSize(), NULL, &m_pVertexShaderVideo);
		if (FAILED(hr)) { m_pVertexShaderVideo = NULL; RELEASEDD(m_pVSVIDBlob); }
	}
	if (m_pPSVIDBlob)
	{
		hr = pDevice->getDevice()->CreatePixelShader(
			m_pPSVIDBlob->GetBufferPointer(), m_pPSVIDBlob->GetBufferSize(), NULL, &m_pPixelShaderVideo);
		if (FAILED(hr)) { m_pPixelShaderVideo = NULL; RELEASEDD(m_pPSVIDBlob); }
	}

	// Create PS_ProceduralPlanet (non-fatal, procedural planet terrain).
	if (m_pPSPPBlob)
	{
		hr = pDevice->getDevice()->CreatePixelShader(
			m_pPSPPBlob->GetBufferPointer(), m_pPSPPBlob->GetBufferSize(), NULL, &m_pPixelShaderProcPlanet);
		if (FAILED(hr)) { m_pPixelShaderProcPlanet = NULL; RELEASEDD(m_pPSPPBlob); }
	}

	// Create PS_PlanetSurface (non-fatal, Option-2 FBM surface overlay).
	if (m_pPSSURFBlob)
	{
		hr = pDevice->getDevice()->CreatePixelShader(
			m_pPSSURFBlob->GetBufferPointer(), m_pPSSURFBlob->GetBufferSize(), NULL, &m_pPixelShaderPlanetSurface);
		if (FAILED(hr)) { m_pPixelShaderPlanetSurface = NULL; RELEASEDD(m_pPSSURFBlob); }
	}

	// Create PS_Atmosphere (non-fatal, planet atmosphere/cloud shell overlay).
	if (m_pPSATMBlob)
	{
		hr = pDevice->getDevice()->CreatePixelShader(
			m_pPSATMBlob->GetBufferPointer(), m_pPSATMBlob->GetBufferSize(), NULL, &m_pPixelShaderAtmosphere);
		if (FAILED(hr)) { m_pPixelShaderAtmosphere = NULL; RELEASEDD(m_pPSATMBlob); }
	}

	return true;
}

bool Shader::createInputLayout(DisplayDeviceD3D* pDevice)
{
	// Input layout for VB_VERTEXTL (screen-space VertexTL: float4 pos + COLOR + float2 uv)
	static const D3D11_INPUT_ELEMENT_DESC VERTEXTL_LAYOUT[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,     0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	// Input layout for VB_VERTEX / VB_VERTEXL (3D world: float3 pos + float3 normal + float2 uv + COLOR)
	// VB_VERTEX: Vertex struct (no COLOR field) — elements 0-2 only
	// VB_VERTEXL: VertexL struct (has COLOR at offset 32) — elements 0-3 used by VS_MainL
	static const D3D11_INPUT_ELEMENT_DESC VERTEX_LAYOUT[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	const D3D11_INPUT_ELEMENT_DESC* pLayout;
	UINT nElements;
	if (m_nVertexType == 2)  // VB_VERTEXTL
	{
		pLayout  = VERTEXTL_LAYOUT;
		nElements = ARRAYSIZE(VERTEXTL_LAYOUT);
	}
	else
	{
		pLayout  = VERTEX_LAYOUT;
		nElements = ARRAYSIZE(VERTEX_LAYOUT);
	}

	HRESULT hr;
	try {
		hr = pDevice->getDevice()->CreateInputLayout(
			pLayout,
			nElements,
			m_pVSBlob->GetBufferPointer(),
			m_pVSBlob->GetBufferSize(),
			&m_pInputLayout
		);
	} catch (...) {
		hr = E_FAIL;
		LOG_ERROR("Shader", "Exception creating input layout!");
	}

	if (FAILED(hr))
	{
		m_pInputLayout = NULL;
		LOG_ERROR("Shader", CharString().format("CreateInputLayout failed HR=0x%08X", hr));
		return false;
	}

	// Create VertexL input layout for VS_MainL: pos(0) + normal(12) + color(24) + uv(28).
	// Must be created against the VS_MainL bytecode so D3D11 validates the COLOR semantic.
	if (m_pVSLBlob)
	{
		static const D3D11_INPUT_ELEMENT_DESC VERTEXL_LAYOUT[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		HRESULT hrL;
		try {
			hrL = pDevice->getDevice()->CreateInputLayout(
				VERTEXL_LAYOUT, ARRAYSIZE(VERTEXL_LAYOUT),
				m_pVSLBlob->GetBufferPointer(),
				m_pVSLBlob->GetBufferSize(),
				&m_pInputLayoutL
			);
		} catch (...) { hrL = E_FAIL; }
		if (FAILED(hrL))
		{
			m_pInputLayoutL = NULL;
			LOG_ERROR("Shader", CharString().format("CreateInputLayout (VertexL) failed HR=0x%08X", hrL));
		}
	}

	return true;
}

bool Shader::createConstantBuffers(DisplayDeviceD3D* pDevice)
{
	HRESULT hr;

	// UI shader uses a much smaller constant buffer than the 3D shader
	const UINT cbStructSize = (m_nVertexType == 2)  // VB_VERTEXTL
		? sizeof(UIShaderConstants)
		: sizeof(ShaderConstants);

	// DX11 requires constant buffer size to be a multiple of 16 bytes
	const UINT CB_SIZE = ((cbStructSize + 15) / 16) * 16;

	D3D11_BUFFER_DESC cbDesc = {};
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.ByteWidth = CB_SIZE;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	try {
		hr = pDevice->getDevice()->CreateBuffer(&cbDesc, NULL, &m_pVSConstantBuffer);
	} catch (...) {
		hr = E_FAIL;
		LOG_ERROR("Shader", "Exception creating VS constant buffer!");
	}
	if (FAILED(hr))
	{
		m_pVSConstantBuffer = NULL;
		LOG_ERROR("Shader", CharString().format("CreateBuffer (VS CB) failed HR=0x%08X", hr));
		return false;
	}

	// Create pixel shader constant buffer
	try {
		hr = pDevice->getDevice()->CreateBuffer(&cbDesc, NULL, &m_pPSConstantBuffer);
	} catch (...) {
		hr = E_FAIL;
		LOG_ERROR("Shader", "Exception creating PS constant buffer!");
	}
	if (FAILED(hr))
	{
		m_pPSConstantBuffer = NULL;
		LOG_ERROR("Shader", CharString().format("CreateBuffer (PS CB) failed HR=0x%08X", hr));
		return false;
	}

	return true;
}

void Shader::updateConstantBuffers(DisplayDeviceD3D* pDevice)
{
	if (!pDevice || !pDevice->getContext())
		return;

	ID3D11DeviceContext* pContext = pDevice->getContext();

	// UI shader (VB_VERTEXTL) uses a separate, smaller constant buffer
	if (m_nVertexType == 2)  // VB_VERTEXTL
	{
		// CRITICAL: Use the full client/back-buffer dimensions, NOT m_Proj.m_rWindow!
		// m_Proj.m_rWindow is overwritten by each NodeWindow::setProjection() call and ends up
		// as some tiny sub-window rect (e.g., 386x41). The UI shader needs the full screen size
		// to convert pixel coordinates to NDC correctly.
		UIShaderConstants cbUI = {};
		cbUI.g_ScreenWidth  = (float)pDevice->m_ClientRectangle.width();
		cbUI.g_ScreenHeight = (float)pDevice->m_ClientRectangle.height();
		cbUI.bEnableDiffuse = 0;
		cbUI._pad = 0.0f;

		// Apply tracked UI constants
		for (auto& pair : m_Constants)
		{
			const std::string& name = pair.first;
			const Constant& c = pair.second;
			if (name == "bEnableDiffuse" && c.type == Constant::INT)
				cbUI.bEnableDiffuse = c.nValue;
			else if (name == "bEnableDiffuse" && c.type == Constant::BOOL)
				cbUI.bEnableDiffuse = c.bValue ? 1 : 0;
		}

		if (m_pVSConstantBuffer)
		{
			D3D11_MAPPED_SUBRESOURCE mapped;
			if (SUCCEEDED(pContext->Map(m_pVSConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
			{
				memcpy(mapped.pData, &cbUI, sizeof(cbUI));
				pContext->Unmap(m_pVSConstantBuffer, 0);
			}
		}
		if (m_pPSConstantBuffer)
		{
			D3D11_MAPPED_SUBRESOURCE mapped;
			if (SUCCEEDED(pContext->Map(m_pPSConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
			{
				memcpy(mapped.pData, &cbUI, sizeof(cbUI));
				pContext->Unmap(m_pPSConstantBuffer, 0);
			}
		}
		return;
	}

	// CRITICAL: Always upload constant buffer, even if m_Constants is empty!
	// The shader needs the default values to render correctly.

	// For now, we'll use a simple approach without shader reflection
	// Pack all constants into a structure that matches the shader's constant buffer layout

	// The shader expects constants in a specific order matching Default.fx
	// We'll map our tracked constants to a buffer and upload it

	ShaderConstants cbData = {};

	// Initialize with identity matrices
	cbData.mWorld = DirectX::XMMatrixIdentity();
	cbData.mView = DirectX::XMMatrixIdentity();
	cbData.mProj = DirectX::XMMatrixIdentity();
	cbData.mLightView = DirectX::XMMatrixIdentity();
	cbData.mLightProj = DirectX::XMMatrixIdentity();

	// Set default values
	cbData.vMatDiffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	cbData.vMatSpecular = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	cbData.fMatSpecularPower = 0.0f;
	cbData.vMatAmbient = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	cbData.vMatEmissive = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	cbData.bEnableAmbient = 1;
	cbData.vGlobalAmbient = DirectX::XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	cbData.nLightType = 3; // LIGHT_DIRECTIONAL
	cbData.vLightDiffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	cbData.vLightSpecular = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	cbData.vLightAmbient = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	cbData.vAttenuation = DirectX::XMFLOAT4(1.0f, 1000.0f, 1000000.0f, 0.0f);
	cbData.bEnableShadowMap = 0;
	cbData.bEnableDiffuse = 0;
	cbData.bEnableLightMap = 0;
	cbData.bEnableBumpMap = 0;
	cbData.fBumpDepth = 1.2f;
	cbData.bEnableLighting = 1;
	cbData.vLightDirection = DirectX::XMFLOAT4(0.0f, 0.0f, -1.0f, 0.0f);

	// Apply tracked constants
	for (auto& pair : m_Constants)
	{
		const std::string& name = pair.first;
		const Constant& constant = pair.second;

		if (name == "mWorld" && constant.type == Constant::XMMATRIX)
			cbData.mWorld = constant.xmMatrix;
		else if (name == "mView" && constant.type == Constant::XMMATRIX)
			cbData.mView = constant.xmMatrix;
		else if (name == "mProj" && constant.type == Constant::XMMATRIX)
			cbData.mProj = constant.xmMatrix;
		else if (name == "mLightView" && constant.type == Constant::XMMATRIX)
			cbData.mLightView = constant.xmMatrix;
		else if (name == "mLightProj" && constant.type == Constant::XMMATRIX)
			cbData.mLightProj = constant.xmMatrix;
		else if (name == "vMatDiffuse" && constant.type == Constant::COLOR)
		{
			cbData.vMatDiffuse = DirectX::XMFLOAT4(
				constant.cValue.r / 255.0f,
				constant.cValue.g / 255.0f,
				constant.cValue.b / 255.0f,
				constant.cValue.a / 255.0f
			);
		}
		else if (name == "vMatSpecular" && constant.type == Constant::COLOR)
		{
			cbData.vMatSpecular = DirectX::XMFLOAT4(
				constant.cValue.r / 255.0f,
				constant.cValue.g / 255.0f,
				constant.cValue.b / 255.0f,
				constant.cValue.a / 255.0f
			);
		}
		else if (name == "fMatSpecularPower" && constant.type == Constant::FLOAT)
			cbData.fMatSpecularPower = constant.fValue;
		else if (name == "vMatAmbient" && constant.type == Constant::COLOR)
		{
			cbData.vMatAmbient = DirectX::XMFLOAT4(
				constant.cValue.r / 255.0f,
				constant.cValue.g / 255.0f,
				constant.cValue.b / 255.0f,
				constant.cValue.a / 255.0f
			);
		}
		else if (name == "vMatEmissive" && constant.type == Constant::COLOR)
		{
			cbData.vMatEmissive = DirectX::XMFLOAT4(
				constant.cValue.r / 255.0f,
				constant.cValue.g / 255.0f,
				constant.cValue.b / 255.0f,
				constant.cValue.a / 255.0f
			);
		}
		else if (name == "bEnableAmbient" && constant.type == Constant::BOOL)
			cbData.bEnableAmbient = constant.bValue ? 1 : 0;
		else if (name == "vGlobalAmbient" && constant.type == Constant::COLOR)
		{
			cbData.vGlobalAmbient = DirectX::XMFLOAT4(
				constant.cValue.r / 255.0f,
				constant.cValue.g / 255.0f,
				constant.cValue.b / 255.0f,
				constant.cValue.a / 255.0f
			);
		}
		else if (name == "nLightType" && constant.type == Constant::INT)
			cbData.nLightType = constant.nValue;
		else if (name == "vLightDiffuse" && constant.type == Constant::COLOR)
		{
			cbData.vLightDiffuse = DirectX::XMFLOAT4(
				constant.cValue.r / 255.0f,
				constant.cValue.g / 255.0f,
				constant.cValue.b / 255.0f,
				constant.cValue.a / 255.0f
			);
		}
		else if (name == "vLightSpecular" && constant.type == Constant::COLOR)
		{
			cbData.vLightSpecular = DirectX::XMFLOAT4(
				constant.cValue.r / 255.0f,
				constant.cValue.g / 255.0f,
				constant.cValue.b / 255.0f,
				constant.cValue.a / 255.0f
			);
		}
		else if (name == "vLightAmbient" && constant.type == Constant::COLOR)
		{
			cbData.vLightAmbient = DirectX::XMFLOAT4(
				constant.cValue.r / 255.0f,
				constant.cValue.g / 255.0f,
				constant.cValue.b / 255.0f,
				constant.cValue.a / 255.0f
			);
		}
		else if (name == "vLightPosition" && constant.type == Constant::VECTOR3)
		{
			cbData.vLightPosition = DirectX::XMFLOAT4(
				constant.vValue.x, constant.vValue.y, constant.vValue.z, 1.0f
			);
		}
		else if (name == "vLightDirection" && constant.type == Constant::VECTOR3)
		{
			cbData.vLightDirection = DirectX::XMFLOAT4(
				constant.vValue.x, constant.vValue.y, constant.vValue.z, 0.0f
			);
		}
		else if (name == "vAttenuation" && constant.type == Constant::VECTOR3)
		{
			cbData.vAttenuation = DirectX::XMFLOAT4(
				constant.vValue.x, constant.vValue.y, constant.vValue.z, 0.0f
			);
		}
		else if (name == "bEnableShadowMap" && constant.type == Constant::BOOL)
			cbData.bEnableShadowMap = constant.bValue ? 1 : 0;
		else if (name == "vShadowMapSize" && constant.type == Constant::VECTOR3)
		{
			cbData.vShadowMapSize = DirectX::XMFLOAT4(
				constant.vValue.x, constant.vValue.y, constant.vValue.z, 0.0f
			);
		}
		else if (name == "vCameraPos" && constant.type == Constant::VECTOR3)
		{
			cbData.vCameraPos = DirectX::XMFLOAT4(
				constant.vValue.x, constant.vValue.y, constant.vValue.z, 1.0f
			);
		}
		else if (name == "bEnableDiffuse" && constant.type == Constant::BOOL)
			cbData.bEnableDiffuse = constant.bValue ? 1 : 0;
		else if (name == "bEnableDiffuse" && constant.type == Constant::INT)
			cbData.bEnableDiffuse = constant.nValue;
		else if (name == "bEnableLightMap" && constant.type == Constant::BOOL)
			cbData.bEnableLightMap = constant.bValue ? 1 : 0;
		else if (name == "bEnableLightMap" && constant.type == Constant::INT)
			cbData.bEnableLightMap = constant.nValue;
		else if (name == "bEnableBumpMap" && constant.type == Constant::BOOL)
			cbData.bEnableBumpMap = constant.bValue ? 1 : 0;
		else if (name == "bEnableBumpMap" && constant.type == Constant::INT)
			cbData.bEnableBumpMap = constant.nValue;
		else if (name == "bEnableLighting" && constant.type == Constant::BOOL)
			cbData.bEnableLighting = constant.bValue ? 1 : 0;
		else if (name == "fTime" && constant.type == Constant::FLOAT)
			cbData.fTime = constant.fValue;
		else if (name == "vGnpDir" && constant.type == Constant::VECTOR3)
			cbData.vGnpDir = DirectX::XMFLOAT3(constant.vValue.x, constant.vValue.y, constant.vValue.z);
		else if (name == "vGalCenDir" && constant.type == Constant::VECTOR3)
			cbData.vGalCenDir = DirectX::XMFLOAT3(constant.vValue.x, constant.vValue.y, constant.vValue.z);
		else if (name == "vStarColor" && constant.type == Constant::XMFLOAT4)
			cbData.vStarColor = constant.xmFloat4;
		else if (name == "vStarParams" && constant.type == Constant::XMFLOAT4)
			cbData.vStarParams = constant.xmFloat4;
		else if (name == "bEnableNormalMap" && constant.type == Constant::BOOL)
			cbData.bEnableNormalMap = constant.bValue ? 1 : 0;
		else if (name == "bEnableNormalMap" && constant.type == Constant::INT)
			cbData.bEnableNormalMap = constant.nValue;
		else if (name == "bAlphaBlend" && constant.type == Constant::BOOL)
			cbData.bAlphaBlend = constant.bValue ? 1 : 0;
		else if (name == "bAlphaBlend" && constant.type == Constant::INT)
			cbData.bAlphaBlend = constant.nValue;
		else if (name == "bAtmosphereShell" && constant.type == Constant::BOOL)
			cbData.bAtmosphereShell = constant.bValue ? 1 : 0;
		else if (name == "bAtmosphereShell" && constant.type == Constant::INT)
			cbData.bAtmosphereShell = constant.nValue;
		else if (name == "fTessFactor" && constant.type == Constant::FLOAT)
			cbData.fTessFactor = constant.fValue;
		else if (name == "fDeltaTime" && constant.type == Constant::FLOAT)
			cbData.fDeltaTime = constant.fValue;
		else if (name == "vVideoParams" && constant.type == Constant::XMFLOAT4)
			cbData.vVideoParams = constant.xmFloat4;
	}

	// Update vertex shader constant buffer
	if (m_pVSConstantBuffer)
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		if (SUCCEEDED(pContext->Map(m_pVSConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
		{
			memcpy(mappedResource.pData, &cbData, sizeof(ShaderConstants));
			pContext->Unmap(m_pVSConstantBuffer, 0);
		}
	}

	// Update pixel shader constant buffer (same data)
	if (m_pPSConstantBuffer)
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		if (SUCCEEDED(pContext->Map(m_pPSConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
		{
			memcpy(mappedResource.pData, &cbData, sizeof(ShaderConstants));
			pContext->Unmap(m_pPSConstantBuffer, 0);
		}
	}
}

//---------------------------------------------------------------------------------------------------
// Static Helper Methods
//---------------------------------------------------------------------------------------------------

bool Shader::applyConstant(DisplayDeviceD3D* pDevice, ID3D11Buffer* pConstantBuffer,
	const char* pName, bool value)
{
	// Implement constant buffer updates based on shader reflection
	// This is a stub - full implementation would use shader reflection
	return true;
}

bool Shader::applyConstant(DisplayDeviceD3D* pDevice, ID3D11Buffer* pConstantBuffer,
	const char* pName, int value)
{
	return true;
}

bool Shader::applyConstant(DisplayDeviceD3D* pDevice, ID3D11Buffer* pConstantBuffer,
	const char* pName, float value)
{
	return true;
}

bool Shader::applyConstant(DisplayDeviceD3D* pDevice, ID3D11Buffer* pConstantBuffer,
	const char* pName, const Vector3& value)
{
	return true;
}

bool Shader::applyConstant(DisplayDeviceD3D* pDevice, ID3D11Buffer* pConstantBuffer,
	const char* pName, const Matrix33& value)
{
	return true;
}

bool Shader::applyConstant(DisplayDeviceD3D* pDevice, ID3D11Buffer* pConstantBuffer,
	const char* pName, const DirectX::XMMATRIX& value)
{
	return true;
}

bool Shader::applyConstant(DisplayDeviceD3D* pDevice, ID3D11Buffer* pConstantBuffer,
	const char* pName, Color value)
{
	return true;
}

//---------------------------------------------------------------------------------------------------
// EOF
