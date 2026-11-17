/*
	DisplayDeviceD3D.cpp
	(c)2005 Palestar, Richard Lyle
	Updated to DirectX 11  by Shawn Gonzales and  AI
*/

#define MEDUSA_TRACE_ON

#include "Debug/Trace.h"
#include "Debug/Assert.h"
#include "Debug/Profile.h"
#include "File/FileDisk.h"
#include "File/Path.h"
#include "Math/Constants.h"
#include "Math/Plane.h"
#include "Math/SphericalHull.h"
#include "Standard/Limits.h"
#include "Draw/ImageCodec.h"

#include "Display/Types.h"
#include "DisplayDeviceD3D.h"
#include "PrimitiveFactory.h"
#include "PrimitiveSurfaceD3D.h"
#include "PrimitiveMaterialD3D.h"
#include "PrimitiveWindowD3D.h"
#include "PrimitiveSetTransformD3D.h"
#include "PrimitiveStarSurfaceD3D.h"
#include "PrimitiveGalaxyD3D.h"
#include "PrimitiveGalaxySpriteSetD3D.h"

#include "DisplayEffectHDR.h"
#include "DisplayEffectBlur.h"

#include <math.h>

using namespace DirectX;

// Route TRACE to the VS Output window in debug builds only
#ifdef _DEBUG
#undef TRACE  
#define TRACE(msg) do { \
	OutputDebugStringA("[TRACE] "); \
	OutputDebugStringA((const char*)(CharString() = msg)); \
	OutputDebugStringA("\n"); \
} while(0)
#endif

// Diagnostic counter: tracks push() calls between endScene() calls
static int s_nPushCalls = 0;

//---------------------------------------------------------------------------------------------------

DXGIFactoryPtr DisplayDeviceD3D::sm_pDXGIFactory;  // Now RAII wrapper, automatically null
DisplayDeviceD3D::ModeList		DisplayDeviceD3D::sm_ModeList;
DisplayDeviceD3D::DeviceList	DisplayDeviceD3D::sm_DeviceList;

//---------------------------------------------------------------------------------------------------

const float DEFAULT_SHADOW_RADIUS = 1500.0f;
const int	DEFAULT_SHADOW_MAP_SIZE = 2048;

//---------------------------------------------------------------------------------------------------

static const int VERTEX_SIZES[DisplayDeviceD3D::VB_COUNT] =
{
	sizeof(Vertex),
	sizeof(VertexL),
	sizeof(VertexTL)
};

// Input layout descriptions for each vertex type
static const D3D11_INPUT_ELEMENT_DESC VERTEX_LAYOUT[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

static const D3D11_INPUT_ELEMENT_DESC VERTEXL_LAYOUT[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

static const D3D11_INPUT_ELEMENT_DESC VERTEXTL_LAYOUT[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

//---------------------------------------------------------------------------------------------------

IMPLEMENT_FACTORY(DisplayDeviceD3D, DisplayDevice);

DisplayDeviceD3D::DisplayDeviceD3D() :
	m_dwLockingThread(0),
	m_nLockCount(0),
	m_bBeginScene(false),
	m_nTextureStage(0),
	m_bDiffuseBound(false),
	m_TextureP2(false),
	m_TextureSquare(false),
	m_TextureMaxSize(16384, 16384),
	m_TextureMinSize(1, 1),
	m_fPixelShaderVersion(5.0f),
	m_fVertexShaderVersion(5.0f),
	m_eFilterMode(FILTER_ANISOTROPIC),
	m_HWND(NULL),
	m_bHardware(true),
	m_bWindowed(false),
	m_eFSAA(FSAA_NONE),
	m_ClientRectangle(0, 0, -1, -1),
	m_bMinimized(false),
	// D3D11 resources now automatically initialized to null by RAII wrappers
	m_cAmbientLight(BLACK),
	m_NextLightId(0),
	// Shader resources automatically initialized to null by RAII wrappers
	m_bShadowMapReady(false),
	m_bShadowMapSupported(true),
	m_bShadowPass(false),
	m_nMaxShadowLights(0),
	m_nShadowMapPass(0),
	m_bUsingFixedFunction(false),
	m_vShadowFocus(0, 0, 1.0f),
	m_fShadowRadius(DEFAULT_SHADOW_RADIUS),
	m_szShadowMap(DEFAULT_SHADOW_MAP_SIZE, DEFAULT_SHADOW_MAP_SIZE),
	m_bShadowMapActive(false),
	m_ShadowLightView(DirectX::XMMatrixIdentity()),
	m_ShadowLightProj(DirectX::XMMatrixIdentity())
	// Render states automatically initialized to null by RAII wrappers
{
	TRACE("DisplayDeviceD3D created!");

	// Ensure all primitive factories are initialized before use
	// This fixes static initialization order issues between DLLs
	BasePrimitiveFactory::initializeFactories();

	lock();

	for (int vb = 0; vb < VB_COUNT; vb++)
	{
		// m_VB[vb] and m_pInputLayouts[vb] auto-initialized by RAII
		m_VBOffset[vb] = 0;
	}

	enumerateModes();

	// Register supported effects
	registerEffect("HDR", DisplayEffectHDR::staticFactory());
	registerEffect("BLUR", DisplayEffectBlur::staticFactory());

	unlock();
}

DisplayDeviceD3D::~DisplayDeviceD3D()
{
	TRACE("DisplayDeviceD3D destroyed!");
	release();
}

//------------------------------------------------------------------------------------------------
// Accessors
//------------------------------------------------------------------------------------------------

bool DisplayDeviceD3D::isLocked() const
{
	return ::GetCurrentThreadId() == m_dwLockingThread;
}

int DisplayDeviceD3D::modeCount() const
{
	return sm_ModeList.size();
}

const DisplayDevice::Mode* DisplayDeviceD3D::mode(int n) const
{
	return &sm_ModeList[n];
}

ColorFormat* DisplayDeviceD3D::primaryFormat() const
{
	return m_pFormat;
}

int DisplayDeviceD3D::surfaceFormatCount() const
{
	return m_TextureFormats.size();
}

ColorFormat::Format DisplayDeviceD3D::surfaceFormat(int n) const
{
	return m_TextureFormats[n];
}

const DisplayDevice::Mode* DisplayDeviceD3D::activeMode() const
{
	return &m_Mode;
}

bool DisplayDeviceD3D::windowed() const
{
	return m_bWindowed;
}

RectInt DisplayDeviceD3D::clientWindow() const
{
	return m_ClientRectangle;
}

RectInt DisplayDeviceD3D::renderWindow() const
{
	D3D11_VIEWPORT vp;
	UINT numViewports = 1;
	m_pContext->RSGetViewports(&numViewports, &vp);

	return RectInt(PointInt((int)vp.TopLeftX, (int)vp.TopLeftY),
		SizeInt((int)vp.Width, (int)vp.Height));
}

bool DisplayDeviceD3D::textureP2() const
{
	return false; // DX11 doesn't require power-of-2 textures
}

bool DisplayDeviceD3D::textureSquare() const
{
	return false; // DX11 doesn't require square textures
}

SizeInt DisplayDeviceD3D::textureMaxSize() const
{
	return m_TextureMaxSize;
}

SizeInt DisplayDeviceD3D::textureMinSize() const
{
	return m_TextureMinSize;
}

float DisplayDeviceD3D::pixelShaderVersion() const
{
	return m_fPixelShaderVersion;
}

float DisplayDeviceD3D::vertexShaderVersion() const
{
	return m_fVertexShaderVersion;
}

DisplayDevice::FilterMode DisplayDeviceD3D::filterMode() const
{
	return m_eFilterMode;
}

dword DisplayDeviceD3D::totalVideoMemory() const
{
	DXGIDevicePtr pDXGIDevice;
	DXGIAdapterPtr pAdapter;
	DXGI_ADAPTER_DESC adapterDesc;

	if (m_pDevice && SUCCEEDED(m_pDevice->QueryInterface(__uuidof(IDXGIDevice), 
		(void**)pDXGIDevice.GetAddressOf())))
	{
		if (SUCCEEDED(pDXGIDevice->GetAdapter(pAdapter.GetAddressOf())))
		{
			pAdapter->GetDesc(&adapterDesc);
			// RAII handles cleanup automatically
			return (dword)(adapterDesc.DedicatedVideoMemory / (1024 * 1024)); // Convert to MB
		}
	}

	return 0;
}

dword DisplayDeviceD3D::freeVideoMemory() const
{
	// DX11 doesn't provide direct memory query like DX9
	// Return total as approximation
	return totalVideoMemory();
}

//------------------------------------------------------------------------------------------------
// Mutators
//------------------------------------------------------------------------------------------------

void DisplayDeviceD3D::lock()
{
	m_Lock.lock();
	if (m_nLockCount == 0)
	{
		m_dwLockingThread = ::GetCurrentThreadId();
		// Drain any material surfaces queued by the Broker background thread.
		// Called on the render thread every frame so uploads overlap with disk I/O,
		// matching the DX9 behaviour where Material::read() called createDeviceSurfaces()
		// directly.  We use a callback to avoid a circular dependency on Render3D.
		if (sm_pfnFlushPendingMaterials)
			sm_pfnFlushPendingMaterials(this);
	}
	++m_nLockCount;
}

void DisplayDeviceD3D::unlock()
{
	if (--m_nLockCount <= 0)
		m_dwLockingThread = 0;
	m_Lock.unlock();
}

bool DisplayDeviceD3D::initialize(void* hWnd, const Mode* pMode, bool bWindowed, bool bHardware, FSAA eFSAA)
{
	if (pMode == NULL && !bWindowed)
		return false;

	lock();
	release();

	m_HWND = static_cast<HWND>(hWnd);
	m_bHardware = bHardware;
	m_eFSAA = eFSAA;

	if (pMode != NULL)
	{
		m_Mode = *pMode;
		m_pFormat = ColorFormat::allocateFormat(pMode->colorFormat);
	}
	else
	{
		if (!bWindowed)
		{
			TRACE("Mode required for non-windowed modes!");
			unlock();
			return false;
		}
		m_pFormat = ColorFormat::allocateFormat(ColorFormat::RGB888);
	}

	m_bWindowed = bWindowed;

	m_ClientPlacement.length = sizeof(m_ClientPlacement);
	GetWindowPlacement(m_HWND, &m_ClientPlacement);

	if (!initializeDirectX())
	{
		unlock();
		return false;
	}

	sm_DeviceList.push(this);
	unlock();

	return true;
}

bool DisplayDeviceD3D::setMode(const Mode* pMode, bool bWindowed)
{
	if (pMode == NULL && !bWindowed)
		return false;

	if (pMode != NULL)
		m_Mode = *pMode;
	m_bWindowed = bWindowed;

	if (!resetDirectX())
		return false;

	if (m_bWindowed)
	{
		int cx = m_Mode.screenSize.width + (GetSystemMetrics(SM_CXSIZEFRAME) * 2);
		int cy = m_Mode.screenSize.height + (GetSystemMetrics(SM_CYSIZEFRAME) * 2)
			+ GetSystemMetrics(SM_CYCAPTION);

		::SetWindowPos(m_HWND, HWND_NOTOPMOST, -1, -1, cx, cy, SWP_NOMOVE | SWP_NOZORDER);
	}

	return true;
}

void DisplayDeviceD3D::setFilterMode(FilterMode eMode)
{
	m_eFilterMode = eMode;
}

void DisplayDeviceD3D::release()
{
	lock();

	sm_DeviceList.removeSearch(this);
	abortScene();
	m_TextureFormats.release();
	m_pFormat = NULL;

	freeDirectX();
	unlock();
}

void DisplayDeviceD3D::flushVideoMemory() const
{
	// DX11 handles memory management automatically
}

void DisplayDeviceD3D::clear(Color nColor)
{
	if (!m_pContext || !activeRTV())
		return;

	float clearColor[4] = {
		nColor.r / 255.0f,
		nColor.g / 255.0f,
		nColor.b / 255.0f,
		nColor.a / 255.0f
	};

	m_pContext->ClearRenderTargetView(activeRTV(), clearColor);
}

void DisplayDeviceD3D::clearZ(float fDepth)
{
	if (!m_pContext || !m_pDepthStencilView)
		return;

	m_pContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH, fDepth, 0);
}

void DisplayDeviceD3D::setAmbient(Color nColor)
{
	m_cAmbientLight = nColor;
}

int DisplayDeviceD3D::addDirectionalLight(int nPriority, Color nColor, const Vector3& vDirection)
{
	if (m_bShadowPass)
		return -1;

	Light light;
	light.type = Light::DIRECTIONAL;
	light.color = nColor;
	light.direction = vDirection;
	light.position = Vector3::ZERO;
	light.radius = 0.0f;
	light.attenuation0 = 1.0f;
	light.attenuation1 = 0.0f;
	light.attenuation2 = 0.0f;

	m_Lights.insert(std::pair<int, Light>(nPriority, light));
	return m_Lights.size();
}

int DisplayDeviceD3D::addPointLight(int nPriority, Color nColor, const Vector3& vPosition, float fRadius)
{
	if (m_bShadowPass)
		return -1;

	Light light;
	light.type = Light::POINT;
	light.color = nColor;
	light.direction = Vector3::ZERO;
	light.position = vPosition;
	light.radius = fRadius;
	light.attenuation0 = 1.0f;
	light.attenuation1 = fRadius > 0.0f ? 1.0f / fRadius : 0.0f;
	light.attenuation2 = 0.0f;

	m_Lights.insert(std::pair<int, Light>(nPriority, light));
	return m_Lights.size();
}

void DisplayDeviceD3D::clearLights()
{
	m_Lights.clear();
	m_NextLightId = 0;
}

void DisplayDeviceD3D::setFillMode(FillMode nMode)
{
	if (!m_pContext)
		return;

	switch (nMode)
	{
	case FILL_SOLID:
		m_pContext->RSSetState(m_pRasterizerSolid.Get());
		break;
	case FILL_WIREFRAME:
		m_pContext->RSSetState(m_pRasterizerWireframe.Get());
		break;
	case FILL_POINT:
		m_pContext->RSSetState(m_pRasterizerPoint.Get());
		break;
	}
}

void DisplayDeviceD3D::setFog(FogMode nMode, float fBegin, float fEnd, Color nColor)
{
	// Fog must be implemented in shaders for DX11
	// Store fog parameters in constant buffer and pass to shaders
}

void DisplayDeviceD3D::disableFog()
{
	// Fog must be implemented in shaders for DX11
}

void DisplayDeviceD3D::setProjection(const Matrix33& mFrame, const Vector3& vPosition,
	const RectInt& rWindow, float fFOV, float fFront, float fBack)
{
	m_Proj.m_mFrame = mFrame;
	m_Proj.m_vPosition = vPosition;
	m_Proj.m_rWindow = rWindow;
	m_Proj.m_fFOV = fFOV;
	m_Proj.m_fFront = fFront;
	m_Proj.m_fBack = fBack;

	updateProjection();
}

void DisplayDeviceD3D::setShadowPass(int a_nMaxLights, const Vector3& a_vFocus,
	float a_fShadowRadius, const SizeInt& a_szShadowMap)
{
	m_nMaxShadowLights = a_nMaxLights;
	m_vShadowFocus = a_vFocus;
	m_fShadowRadius = a_fShadowRadius;

	if (m_szShadowMap != a_szShadowMap)
	{
		m_szShadowMap = a_szShadowMap;
		m_ShadowPassList.clear();
		m_bShadowMapReady = false;
	}
}

//---------------------------------------------------------------------------------------------------
// Scene Management
//---------------------------------------------------------------------------------------------------

bool DisplayDeviceD3D::beginScene()
{
	// Reset dynamic VB offsets at frame start so every frame begins at offset 0
	// (WRITE_DISCARD). Without this, a mid-frame wrap triggers WRITE_DISCARD and
	// invalidates the buffer, corrupting earlier draws in the same frame.
	resetDVB();

	// Clear lights pushed by the previous frame's NodeLight::render() calls.
	// Without this, m_Lights (populated via addDirectionalLight()/addPointLight())
	// grows unbounded every frame -- and updateGlobals() would keep reading whatever
	// stale entry happens to sort first instead of the current frame's actual light(s).
	clearLights();

	// CRITICAL: DON'T call ClearState() - it unbinds render targets!
	// Just flush pending GPU commands
	if (m_pContext)
	{
		m_pContext->Flush();
	}

	m_bFirstShadowPass = true;
	m_nShadowMapPass = 0;
	m_bShadowMapActive = false;
	m_pCurrentTransform = NULL;
	m_pCurrentMaterial = NULL;

	// CRITICAL: Ensure render target is bound for this frame
	if (activeRTV() && m_pDepthStencilView)
	{
		ID3D11RenderTargetView* pRTV = activeRTV();
		m_pContext->OMSetRenderTargets(1, &pRTV, m_pDepthStencilView.Get());

		// Rebind default states that may have been cleared
		if (m_pRasterizerSolid)
			m_pContext->RSSetState(m_pRasterizerSolid.Get());
		if (m_pDepthStencilEnabled)
			m_pContext->OMSetDepthStencilState(m_pDepthStencilEnabled.Get(), 0);

		// Rebind samplers
		ID3D11SamplerState* defaultSamplers[4] = { 
			m_pSamplerLinear.Get(), m_pSamplerLinear.Get(), 
			m_pSamplerLinear.Get(), m_pSamplerAnisotropic.Get()
		};
		m_pContext->PSSetSamplers(0, 4, defaultSamplers);
	}

	TRACE("DisplayDeviceD3D::beginScene() - Scene started with render target bound");
	return true;
}

bool DisplayDeviceD3D::beginShadowPass(Transform& a_LightTransform)
{
	if (!m_bShadowMapSupported)
		return false;
	if (m_bShadowPass)
		return false;

	if (m_bFirstShadowPass)
	{
		m_bFirstShadowPass = false;
		m_ShadowPassList.resize(Min<int>(m_nMaxShadowLights, m_Lights.size()));
		m_iCurrentShadowPass = m_ShadowPassList.begin();
		m_iCurrentShadowLight = m_Lights.begin();
	}

	if (m_iCurrentShadowLight == m_Lights.end())
		return false;
	if (m_iCurrentShadowPass == m_ShadowPassList.end())
		return false;

	if (!readyShadowMap())
		return false;

	Light& light = m_iCurrentShadowLight->second;

	Vector3 vLightViewPosition(Vector3::ZERO);
	Vector3 vLightViewDirection(Vector3::ZERO);

	float fMidZ = (m_Proj.m_fBack - m_Proj.m_fFront) * 0.5f;
	Vector3 vWorldFocus(m_Proj.m_vPosition + (m_Proj.m_mFrame % m_vShadowFocus));

	float fShadowScale = 0.0f;
	bool bOrthoProj = false;

	if (light.type == Light::DIRECTIONAL)
	{
		vLightViewDirection = light.direction;
		vLightViewPosition = vWorldFocus - (vLightViewDirection * fMidZ);
		fShadowScale = light.color.r * light.color.g * light.color.b / (255.0f * 255.0f * 255.0f);
		bOrthoProj = true;
	}
	else if (light.type == Light::POINT)
	{
		vLightViewPosition = light.position;
		vLightViewDirection = vWorldFocus - vLightViewPosition;
		float fDistance = vLightViewDirection.magnitude();
		if (fDistance > 0.0f)
			vLightViewDirection *= 1.0f / fDistance;

		if (fDistance < light.radius)
		{
			if (fDistance > 0.0f)
				fShadowScale = 1.0f - (fDistance / light.radius);
			else
				fShadowScale = 1.0f;
		}
	}

	if (fShadowScale <= 0.05f)
		return false;

	SphericalHull sphere(m_vShadowFocus, m_fShadowRadius);
	Vector3 vClippedLightPos(m_Proj.m_mFrame * (vLightViewPosition - m_Proj.m_vPosition));
	sphere.intersect(vClippedLightPos, m_vShadowFocus - vClippedLightPos, vClippedLightPos);
	vLightViewPosition = m_Proj.m_vPosition + (m_Proj.m_mFrame % vClippedLightPos);

	ShadowPass& pass = *m_iCurrentShadowPass;
	pass.m_LightTransform.m_mFrame = Matrix33(vLightViewDirection);
	pass.m_LightTransform.m_vTranslate = vLightViewPosition;
	pass.m_bOrthoProj = bOrthoProj;
	pass.m_Primitives.release();

	// Build light-space view matrix from computed position and look-at target.
	{
		XMVECTOR xEye = XMVectorSet(vLightViewPosition.x, vLightViewPosition.y, vLightViewPosition.z, 1.0f);
		XMVECTOR xAt  = XMVectorSet(vWorldFocus.x, vWorldFocus.y, vWorldFocus.z, 1.0f);

		// Derive an up vector from the light frame built above.
		Matrix33 lightFrame(vLightViewDirection);
		XMVECTOR xUp = XMVectorSet(lightFrame.j.x, lightFrame.j.y, lightFrame.j.z, 0.0f);

		// Guard against degenerate case where up is parallel to direction.
		// Compute dot product manually: lightFrame.j · vLightViewDirection
		float dotProduct = lightFrame.j.x * vLightViewDirection.x + 
		                   lightFrame.j.y * vLightViewDirection.y + 
		                   lightFrame.j.z * vLightViewDirection.z;
		if (fabsf(dotProduct) > 0.9f)
			xUp = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

		pass.m_LightView = XMMatrixLookAtLH(xEye, xAt, xUp);
	}

	// Build light-space projection matrix.
	{
		float nearZ = 1.0f;
		float farZ  = m_fShadowRadius * 4.0f;
		if (bOrthoProj)
		{
			float size = m_fShadowRadius * 2.0f;
			pass.m_LightProj = XMMatrixOrthographicLH(size, size, nearZ, farZ);
		}
		else
		{
			pass.m_LightProj = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.0f, nearZ, farZ);
		}
	}

	a_LightTransform = pass.m_LightTransform;
	m_bShadowPass = true;
	m_pCurrentTransform = NULL;
	m_pCurrentMaterial = NULL;

	return true;
}

bool DisplayDeviceD3D::endShadowPass()
{
	if (!m_bShadowPass)
		return false;

	m_bShadowPass = false;

	ShadowPass& pass = *m_iCurrentShadowPass;

	if (pass.m_pShadowMapRTV.Get() && m_pShadowMapDSV.Get() && m_pShadowMapShader.valid())
	{
		// Save the current render target and depth stencil.
		ID3D11RenderTargetView* pOldRTV = NULL;
		ID3D11DepthStencilView* pOldDSV = NULL;
		m_pContext->OMGetRenderTargets(1, &pOldRTV, &pOldDSV);

		// Save the current viewport.
		UINT nViewports = 1;
		D3D11_VIEWPORT oldViewport = {};
		m_pContext->RSGetViewports(&nViewports, &oldViewport);

		// Bind the shadow map colour RT + shared depth stencil.
		ID3D11RenderTargetView* pShadowRTV = pass.m_pShadowMapRTV.Get();
		m_pContext->OMSetRenderTargets(1, &pShadowRTV, m_pShadowMapDSV.Get());

		// Clear to 1.0 (max depth) and clear the DSV.
		float clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		m_pContext->ClearRenderTargetView(pShadowRTV, clearColor);
		m_pContext->ClearDepthStencilView(m_pShadowMapDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

		// Set the viewport to shadow map dimensions.
		D3D11_VIEWPORT shadowViewport = {};
		shadowViewport.Width    = (float)m_szShadowMap.width;
		shadowViewport.Height   = (float)m_szShadowMap.height;
		shadowViewport.MinDepth = 0.0f;
		shadowViewport.MaxDepth = 1.0f;
		m_pContext->RSSetViewports(1, &shadowViewport);

		// Depth testing on, solid rasterizer (no double-sided culling needed for depth pass).
		if (m_pRasterizerSolid)
			m_pContext->RSSetState(m_pRasterizerSolid.Get());
		if (m_pDepthStencilEnabled)
			m_pContext->OMSetDepthStencilState(m_pDepthStencilEnabled.Get(), 0);

		// Switch active shader to the depth-only shadow map shader.
		Shader::Ref pOldShader = m_pMatShader;
		m_pMatShader = m_pShadowMapShader;

		// Prime the shader with the light matrices; identity world until the first transform.
		m_pMatShader->setConstant("mWorld", XMMatrixIdentity());
		m_pMatShader->setConstant("mView",  pass.m_LightView);
		m_pMatShader->setConstant("mProj",  pass.m_LightProj);
		m_pMatShader->apply(this);   // binds VS/PS/layout and uploads CB

		// Execute each primitive collected during the shadow geometry pass.
		// Transforms update mWorld; geometry primitives issue the draw calls.
		for (int i = 0; i < pass.m_Primitives.size(); ++i)
		{
			DevicePrimitive* pPrim = pass.m_Primitives[i];
			if (!pPrim)
				continue;
			pPrim->setDevice(this);

			if (pPrim->primitiveKey() == PrimitiveSetTransform::staticPrimitiveKey())
			{
				// Update the world matrix in the shadow shader and re-apply.
				PrimitiveSetTransformD3D* pT = (PrimitiveSetTransformD3D*)pPrim;
				m_pMatShader->setConstant("mWorld", pT->m_Matrix);
				m_pMatShader->setConstant("mView",  pass.m_LightView);
				m_pMatShader->setConstant("mProj",  pass.m_LightProj);
				m_pMatShader->apply(this);
			}
			else
			{
				// Geometry primitive: the CB is already current, just draw.
				pPrim->execute();
			}
		}

		// Store the completed light matrices so the main pass can use them.
		m_bShadowMapActive = true;
		m_ShadowLightView  = pass.m_LightView;
		m_ShadowLightProj  = pass.m_LightProj;

		// Restore original render target, depth stencil, and viewport.
		m_pContext->OMSetRenderTargets(1, &pOldRTV, pOldDSV);
		if (pOldRTV) pOldRTV->Release();
		if (pOldDSV) pOldDSV->Release();
		m_pContext->RSSetViewports(1, &oldViewport);

		// Restore the previous active shader.
		m_pMatShader = pOldShader;
	}

	++m_iCurrentShadowPass;
	++m_iCurrentShadowLight;
	++m_nShadowMapPass;

	return true;
}

bool DisplayDeviceD3D::endScene()
{
	int nMaterialCount = m_Stack[0].size() + m_Stack[1].size() + m_Stack[2].size();
	TRACE(CharString().format("DisplayDeviceD3D::endScene() - ENTRY, %d materials, %d push() calls", 
		nMaterialCount, s_nPushCalls));
	s_nPushCalls = 0;

	if (m_bMinimized || !m_pDevice)
	{
		TRACE("DisplayDeviceD3D::endScene() - Aborting: minimized or no device");
		abortScene();
		return false;
	}

	// If a shadow pass rendered this frame, bind its depth map to t4 so the Default
	// shader can perform shadow comparisons during the main material pass.
	if (m_bShadowMapActive && !m_ShadowPassList.empty())
	{
		ID3D11ShaderResourceView* pShadowSRV = m_ShadowPassList.front().m_pShadowMapSRV.Get();
		if (pShadowSRV)
			m_pContext->PSSetShaderResources(4, 1, &pShadowSRV);
	}

	// Execute primitives from all passes
	for (int i = 0; i < PASS_COUNT; ++i)
	{
		Array<PrimitiveMaterial::Ref>& materials = m_Stack[i];
		for (int j = 0; j < materials.size(); ++j)
		{
			PrimitiveMaterial* pMaterial = materials[j];
			pMaterial->execute();
			pMaterial->clear();
		}
		materials.release();
	}

	// Unbind shadow map SRV so it can be written again next frame.
	if (m_bShadowMapActive)
	{
		ID3D11ShaderResourceView* pNullSRV = NULL;
		m_pContext->PSSetShaderResources(4, 1, &pNullSRV);
		m_bShadowMapActive = false;
	}

	// If MSAA is active, resolve the off-screen MSAA RT into the 1× back buffer before
	// post-process effects run.  After this point m_pRenderTargetView (1×) is the active RT.
	if (m_pMSAAColorTex && m_pMSAAColorRTV && m_pBackBuffer)
	{
		// Unbind MSAA RT so it can be used as a resolve source
		ID3D11RenderTargetView* pNullRTV = NULL;
		m_pContext->OMSetRenderTargets(1, &pNullRTV, NULL);

		D3D11_TEXTURE2D_DESC desc;
		m_pBackBuffer->GetDesc(&desc);
		m_pContext->ResolveSubresource(m_pBackBuffer.Get(), 0,
			m_pMSAAColorTex.Get(), 0, desc.Format);

		// Bind 1× back buffer for effects (no depth needed for post-process)
		ID3D11RenderTargetView* pRTV = m_pRenderTargetView.Get();
		m_pContext->OMSetRenderTargets(1, &pRTV, NULL);
	}

	// Process effects
	for (EffectList::reverse_iterator iEffect = m_EffectList.rbegin();
		iEffect != m_EffectList.rend(); ++iEffect)
	{
		DisplayEffect* pEffect = *iEffect;
		if (!pEffect->postRender(this))
			TRACE("ERROR: Effect postRender() returned false!");
	}
	m_EffectList.clear();

	TRACE("DisplayDeviceD3D::endScene() - SUCCESS");

	return true;
}

void DisplayDeviceD3D::abortScene()
{
	for (int i = 0; i < PASS_COUNT; i++)
	{
		Array<PrimitiveMaterial::Ref>& materials = m_Stack[i];
		for (int j = 0; j < materials.size(); j++)
			materials[j]->clear();
		materials.release();
	}
}

void DisplayDeviceD3D::present()
{
	PROFILE_START("DisplayDeviceD3D::present");

	if (m_pSwapChain)
	{
		// Frame counter and first-present diagnostic
		static int s_nPresentCount = 0;
		s_nPresentCount++;
		if (s_nPresentCount <= 5)
		{
			DXGI_SWAP_CHAIN_DESC scDesc = {};
			m_pSwapChain->GetDesc(&scDesc);
			char buf[256];
			sprintf_s(buf, "[TRACE] present() frame #%d: SwapChain %dx%d, Format=%d\n",
				s_nPresentCount, scDesc.BufferDesc.Width, scDesc.BufferDesc.Height, scDesc.BufferDesc.Format);
			OutputDebugStringA(buf);
		}
		HRESULT hr = m_pSwapChain->Present(0, 0);
		if (FAILED(hr))
		{
			char buf[128];
			sprintf_s(buf, "[TRACE] present() FAILED! HR=0x%08X\n", hr);
			OutputDebugStringA(buf);
		}
	}

	updateClientArea(true);

	// CRITICAL: Clean up per-frame resources to prevent memory leaks
	if (m_pContext)
	{
		// Unbind all shader resources to allow them to be released
		ID3D11ShaderResourceView* nullSRVs[8] = { NULL };
		m_pContext->PSSetShaderResources(0, 8, nullSRVs);
		m_pContext->VSSetShaderResources(0, 8, nullSRVs);
	}

	PROFILE_END();
}

DevicePrimitive* DisplayDeviceD3D::create(const PrimitiveKey& key)
{
	// Ensure factories are initialized before lookup to handle static initialization order issues
	BasePrimitiveFactory::initializeFactories();

	BasePrimitiveFactory* pFactory = BasePrimitiveFactory::findFactory(key);
	if (pFactory == NULL)
	{
		TRACE(CharString().format("DisplayDeviceD3D::create() - FAILED: No factory for key=%llu", key));
		throw PrimitiveFailure();
	}

	DevicePrimitive* pPrimitive = pFactory->create();
	if (pPrimitive == NULL)
	{
		TRACE(CharString().format("DisplayDeviceD3D::create() - FAILED: Factory returned NULL for key=%llu", key));
		throw PrimitiveFailure();
	}

	pPrimitive->setDevice(this);
	return pPrimitive;
}

bool DisplayDeviceD3D::push(DevicePrimitive* pPrimitive)
{
	s_nPushCalls++;

	if (pPrimitive == NULL)
	{
		TRACE("DisplayDeviceD3D::push() - NULL primitive!");
		return false;
	}

	pPrimitive->setDevice(this);

	// Check if this primitive is a material using either the abstract base class key
	// (PrimitiveMaterial) or the concrete D3D implementation key (PrimitiveMaterialD3D).
	// The factory maps the abstract key -> D3D factory, so created primitives carry the
	// concrete D3D key, not the abstract key that push() callers use to identify materials.
	bool const isPrimitiveMaterial = 
		pPrimitive->primitiveKey() == PrimitiveMaterial::staticPrimitiveKey() ||
		pPrimitive->primitiveKey() == PrimitiveMaterialD3D::staticPrimitiveKey();

	if (isPrimitiveMaterial)
	{
		m_pCurrentMaterial = (PrimitiveMaterial*)pPrimitive;

		if (!m_bShadowPass)
		{
			PrimitiveMaterialD3D* pMaterial = (PrimitiveMaterialD3D*)pPrimitive;
			if (!pMaterial->m_bPushed)
			{
				m_Stack[pMaterial->pass()].push(pMaterial);
				pMaterial->m_bPushed = true;
			}

			if (m_pCurrentTransform.valid())
				pMaterial->addChild(m_pCurrentTransform);
		}
		else
		{
			ShadowPass& pass = *m_iCurrentShadowPass;
			if (m_pCurrentTransform.valid() &&
				(pass.m_Primitives.size() == 0 || pass.m_Primitives.last() != m_pCurrentTransform))
				pass.m_Primitives.push((DevicePrimitive*)m_pCurrentTransform);
		}
	}
	else
	{
		if (pPrimitive->primitiveKey() == PrimitiveSetTransform::staticPrimitiveKey())
		{
			m_pCurrentTransform = (PrimitiveSetTransform*)pPrimitive;
		}

		if (!m_bShadowPass)
		{
			if (m_pCurrentMaterial.valid())
			{
				m_pCurrentMaterial->addChild(pPrimitive);
			}
		}
		else
		{
			ShadowPass& pass = *m_iCurrentShadowPass;
			if (!m_pCurrentMaterial.valid() || m_pCurrentMaterial->pass() == PRIMARY)
				pass.m_Primitives.push(pPrimitive);
		}
	}

	return true;
}

DisplayEffect::Ref DisplayDeviceD3D::createEffect(const char* pName)
{
	EffectMap::iterator iFactory = m_EffectMap.find(pName);
	if (iFactory == m_EffectMap.end())
		return NULL;

	Factory* pFactory = iFactory->second;
	if (!pFactory)
		return NULL;

	Widget::Ref pUncasted = pFactory->createWidget();
	if (!pUncasted)
		return NULL;

	DisplayEffect::Ref pEffect = WidgetCast<DisplayEffect>(pUncasted);
	if (!pEffect.valid())
		return NULL;

	m_CreatedEffects.push_back(pEffect.pointer());
	return pEffect;
}

bool DisplayDeviceD3D::push(DisplayEffect* pEffect)
{
	m_EffectList.push_back(pEffect);
	return true;
}

bool DisplayDeviceD3D::capture(const char* pFilename)
{
	// DX11 screen capture implementation
	// Would require getting back buffer texture and saving to file
	return false;
}

Shader::Ref DisplayDeviceD3D::getShader(const char* pShaderName, int nVertexType /*= 0*/)
{
	ShaderMap::iterator iShader = m_ShaderMap.find(pShaderName);
	if (iShader != m_ShaderMap.end())
		return iShader->second;

	Shader::Ref pShader = new Shader();
	m_ShaderMap[pShaderName] = pShader;
	pShader->load(this, pShaderName, nVertexType);

	return pShader;
}

bool DisplayDeviceD3D::releaseShader(const char* pShader)
{
	ShaderMap::iterator iShader = m_ShaderMap.find(pShader);
	if (iShader == m_ShaderMap.end())
		return false;

	iShader->second->release();
	m_ShaderMap.erase(iShader);
	return true;
}

void DisplayDeviceD3D::releaseShaders()
{
	for (ShaderMap::iterator iShader = m_ShaderMap.begin();
		iShader != m_ShaderMap.end(); ++iShader)
	{
		iShader->second->release();
	}
	m_ShaderMap.clear();
}

bool DisplayDeviceD3D::applyUIShader()
{
	Shader::Ref pUIShader = m_pUIShader;
	if (!pUIShader.valid() || !pUIShader->valid())
		return false;

	ID3D11DeviceContext* pContext = getContext();
	if (!pContext)
		return false;

	// CRITICAL: Set viewport to full client rect for UI rendering.
	// Each NodeWindow::setProjection() sets the viewport to its sub-window rect.
	// But VB_VERTEXTL vertices are in SCREEN-SPACE pixels (0..fullWidth, 0..fullHeight)
	// and the UI shader converts them to NDC using the full screen dimensions.
	// The viewport must match or the rasterizer clips to the wrong area.
	D3D11_VIEWPORT vp;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width = (float)m_ClientRectangle.width();
	vp.Height = (float)m_ClientRectangle.height();
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	pContext->RSSetViewports(1, &vp);

	// Bind the VB_VERTEXTL input layout so the GPU correctly parses VertexTL vertex data
	// (position float4, color R8G8B8A8, texcoord float2). Without this, whatever 3D layout
	// was last set by a PrimitiveMaterial draw remains active and misreads the color bytes → black.
	if (m_pInputLayouts[VB_VERTEXTL].Get())
		pContext->IASetInputLayout(m_pInputLayouts[VB_VERTEXTL].Get());

	// Disable depth testing for UI — screen-space elements should always be visible.
	if (m_pDepthStencilDisabled)
		pContext->OMSetDepthStencilState(m_pDepthStencilDisabled.Get(), 0);

	// NOTE: Do NOT override blend state here.
	// setupBlending() in PrimitiveMaterialD3D::setupRenderStates() already set the correct
	// blend state for this material (NONE→opaque, ALPHA→alpha, ADDITIVE→additive).
	// Overriding to alpha blend here causes NONE-blend buttons to become transparent when
	// their vertex Color has alpha=0 (common in DX9 code that ignored alpha for opaque quads).
	// Font materials use PrimitiveMaterial::ALPHA blend, so setupBlending() already sets
	// m_pBlendStateAlpha for them — no override needed here.

	// Detect whether a texture was already bound to slot 0 by the parent material's
	// setupTextureStages() call so the UI shader samples it correctly.
	ID3D11ShaderResourceView* pSRV = nullptr;
	pContext->PSGetShaderResources(0, 1, &pSRV);
	bool bHasTexture = (pSRV != nullptr);
	if (pSRV)
		pSRV->Release();

	pUIShader->setConstant("bEnableDiffuse", bHasTexture ? 1 : 0);
	pUIShader->apply(this);
	return true;
}

void DisplayDeviceD3D::registerEffect(const char* pName, Factory* pFactory)
{
	m_EffectMap[pName] = pFactory;
}

//----------------------------------------------------------------------------
// Dynamic Vertex Buffer Management
//----------------------------------------------------------------------------

int DisplayDeviceD3D::allocateDVB(int vertexType, int vertexCount, ID3D11Buffer** ppBuffer)
{
	if (vertexCount > DYNAMIC_VB_SIZE || vertexType < 0 || vertexType >= VB_COUNT)
	{
		TRACE(CharString().format("DisplayDeviceD3D::allocateDVB, vertexType = %d, vertexCount = %d",
			vertexType, vertexCount));
		return -1;
	}

	if (!m_VB[vertexType])
	{
		TRACE(CharString().format("DisplayDeviceD3D::allocateDVB, creating VB, vertexType = %d", vertexType));

		D3D11_BUFFER_DESC bd = {};
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = DYNAMIC_VB_SIZE * VERTEX_SIZES[vertexType];
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.MiscFlags = 0;

		if (FAILED(m_pDevice->CreateBuffer(&bd, NULL, m_VB[vertexType].GetAddressOf())))
			return -1;

		m_VBOffset[vertexType] = 0;
	}

	int space = DYNAMIC_VB_SIZE - m_VBOffset[vertexType];
	if (space < vertexCount)
		return -1;

	int offset = m_VBOffset[vertexType];
	*ppBuffer = m_VB[vertexType].Get();  // Return raw pointer
	m_VBOffset[vertexType] += vertexCount;

	return offset;
}

int DisplayDeviceD3D::setupDVB(int vertexType, int vertexCount, void* pVerts)
{
	ASSERT(vertexType >= 0 && vertexType < VB_COUNT);

	const dword vertSize = VERTEX_SIZES[vertexType];
	const dword vertBytes = vertexCount * vertSize;

	ID3D11Buffer* pVB = NULL;
	int vbOffset = allocateDVB(vertexType, vertexCount, &pVB);
	if (vbOffset < 0)
		return -1;

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	D3D11_MAP mapType = vbOffset > 0 ? D3D11_MAP_WRITE_NO_OVERWRITE : D3D11_MAP_WRITE_DISCARD;

	if (FAILED(m_pContext->Map(pVB, 0, mapType, 0, &mappedResource)))
		return -1;

	byte* pBuffer = (byte*)mappedResource.pData + (vbOffset * vertSize);
	memcpy(pBuffer, pVerts, vertBytes);

	m_pContext->Unmap(pVB, 0);

	// Bind vertex buffer from start (offset 0). The returned vbOffset is a VERTEX index
	// that callers pass to Draw(count, vbOffset). IASetVertexBuffers must NOT also offset,
	// or the offset is double-counted. PrimitiveWindowD3D::execute uses Draw(4, 0) which is
	// also correct with this binding since its vbOffset from setupDVB would be redundant.
	UINT stride = vertSize;
	UINT offset = 0;
	m_pContext->IASetVertexBuffers(0, 1, &pVB, &stride, &offset);

	return vbOffset;
}

void DisplayDeviceD3D::resetDVB()
{
	for (int vb = 0; vb < VB_COUNT; vb++)
		m_VBOffset[vb] = 0;
}

void DisplayDeviceD3D::releaseDVB()
{
	for (int vb = 0; vb < VB_COUNT; vb++)
	{
		m_VB[vb].Reset();  // RAII wrapper handles cleanup
		m_VBOffset[vb] = 0;
	}
}

DWORD DisplayDeviceD3D::getD3DFilterMode() const
{
	// Return appropriate filter mode for samplers
	switch (m_eFilterMode)
	{
	case FILTER_POINT:
		return D3D11_FILTER_MIN_MAG_MIP_POINT;
	case FILTER_LINEAR:
		return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	case FILTER_ANISOTROPIC:
		return D3D11_FILTER_ANISOTROPIC;
	default:
		return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	}
}

//----------------------------------------------------------------------------
// DirectX Initialization
//----------------------------------------------------------------------------

bool DisplayDeviceD3D::initializeDirectX()
{
	HRESULT hr;

	// Create DXGI Factory with thread-safe initialization
	if (!sm_pDXGIFactory)
	{
		hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)sm_pDXGIFactory.GetAddressOf());
		if (FAILED(hr))
		{
			TRACE("Failed to create DXGI Factory!");
			return false;
		}
	}

	// Create Device and Context
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};

	UINT createDeviceFlags = 0;
#if defined(_DEBUG) && defined(D3D11_ENABLE_DEBUG_LAYER)
	// NOTE: The D3D11 debug layer consumes ~500MB+ of virtual address space.
	// In a 32-bit (Win32) process with a 2GB limit, this leaves insufficient
	// memory for textures, staging buffers, and font glyph sheets.
	// Only enable when explicitly needed by defining D3D11_ENABLE_DEBUG_LAYER.
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevel;

	// Use temporary raw pointers for D3D11CreateDevice (it uses different convention)
	ID3D11Device* pTempDevice = nullptr;
	ID3D11DeviceContext* pTempContext = nullptr;

	hr = D3D11CreateDevice(
		NULL,
		m_bHardware ? D3D_DRIVER_TYPE_HARDWARE : D3D_DRIVER_TYPE_WARP,
		NULL,
		createDeviceFlags,
		featureLevels,
		ARRAYSIZE(featureLevels),
		D3D11_SDK_VERSION,
		&pTempDevice,
		&featureLevel,
		&pTempContext
	);

	if (FAILED(hr))
	{
		TRACE("Failed to create D3D11 device!");
		return false;
	}

	// Transfer ownership to RAII wrappers
	m_pDevice.Reset(pTempDevice);
	m_pContext.Reset(pTempContext);

	TRACE(CharString().format("Created D3D11 device with feature level: %x", featureLevel));

	// Log system info using RAII wrappers
	DXGIDevicePtr pDXGIDevice;
	DXGIAdapterPtr pAdapter;
	DXGI_ADAPTER_DESC adapterDesc;

	if (SUCCEEDED(m_pDevice->QueryInterface(__uuidof(IDXGIDevice), 
		(void**)pDXGIDevice.GetAddressOf())))
	{
		if (SUCCEEDED(pDXGIDevice->GetAdapter(pAdapter.GetAddressOf())))
		{
			pAdapter->GetDesc(&adapterDesc);
			LOG_STATUS("SysInfo", CharString().format("Display Device Name: %S", adapterDesc.Description));
			LOG_STATUS("SysInfo", CharString().format("Video Memory: %d MB",
				(adapterDesc.DedicatedVideoMemory / (1024 * 1024))));
			// RAII handles cleanup automatically
		}
	}

	// Create swap chain
	DXGI_SWAP_CHAIN_DESC scDesc;
	if (!getSwapChainDesc(scDesc))
	{
		TRACE("Failed to get swap chain description!");
		return false;  // RAII will cleanup device/context
	}

	hr = sm_pDXGIFactory->CreateSwapChain(m_pDevice.Get(), &scDesc, 
		m_pSwapChain.GetAddressOf());
	if (FAILED(hr))
	{
		TRACE("Failed to create swap chain!");
		return false;  // RAII will cleanup
	}

	// Disable Alt+Enter for fullscreen switching
	sm_pDXGIFactory->MakeWindowAssociation(m_HWND, DXGI_MWA_NO_ALT_ENTER);

	// Create render targets and depth stencil
	if (!updateBuffers())
	{
		TRACE("Failed to create render targets!");
		return false;  // RAII will cleanup
	}

	// Create render states
	if (!createStates())
	{
		TRACE("Failed to create render states!");
		return false;  // RAII will cleanup
	}

	// Enumerate texture formats
	enumerateTextures();

	// Update window
	updateWindow();
	updateClientArea(false);

	// Set default shader version
	m_fPixelShaderVersion = 5.0f;
	m_fVertexShaderVersion = 5.0f;

	// Load default shader - CRITICAL for D3D11 rendering!
	// Try D3D11 version first, then fall back to D3D9 FX (which won't work but for compatibility)
	TRACE("========================================");
	TRACE("DisplayDeviceD3D::initialize() - Loading default shader...");
	TRACE("========================================");

	m_pDefaultShader = getShader("Shaders/Default_D3D11.fx");
	if (!m_pDefaultShader.valid() || !m_pDefaultShader->valid())
	{
		// Try alternate paths
		TRACE("WARNING: Shaders/Default_D3D11.fx not found, trying Default_D3D11.fx");
		m_pDefaultShader = getShader("Default_D3D11.fx");
	}
	if (!m_pDefaultShader.valid() || !m_pDefaultShader->valid())
	{
		// Try old D3D9 version (won't work but try anyway)
		LOG_WARNING("DisplayDeviceD3D", "D3D11 shader not found, trying D3D9 version (will likely fail)");
		TRACE("ERROR: D3D11 shader not found, trying Shaders/Default.fx");
		m_pDefaultShader = getShader("Shaders/Default.fx");
	}

	if (!m_pDefaultShader.valid() || !m_pDefaultShader->valid())
	{
		LOG_ERROR("DisplayDeviceD3D", "CRITICAL: Failed to load default shader! Rendering will not work!");
		LOG_ERROR("DisplayDeviceD3D", "Looked for: Shaders/Default_D3D11.fx, Default_D3D11.fx, Shaders/Default.fx");
		TRACE("========================================");
		TRACE("CRITICAL ERROR: NO SHADER LOADED!");
		TRACE("========================================");
		// Don't fail initialization, but warn heavily
	}
	else
	{
		TRACE("========================================");
		TRACE("SUCCESS: Default shader loaded successfully!");
		TRACE("========================================");
	}

	// Load UI shader for VB_VERTEXTL (screen-space) primitives (PrimitiveWindow, fonts, etc.)
	TRACE("DisplayDeviceD3D::initialize() - Loading UI shader (VB_VERTEXTL)...");
	m_pUIShader = getShader("Shaders/UI_D3D11.fx", 2);  // 2 = VB_VERTEXTL
	if (!m_pUIShader.valid() || !m_pUIShader->valid())
		m_pUIShader = getShader("UI_D3D11.fx", 2);
	if (!m_pUIShader.valid() || !m_pUIShader->valid())
		LOG_ERROR("DisplayDeviceD3D", "WARNING: Failed to load UI shader (Shaders/UI_D3D11.fx) — 2D UI will not render");

	return true;
}

bool DisplayDeviceD3D::resetDirectX()
{
	// Release current resources - RAII handles this automatically
	m_pMSAAColorRTV.Reset();
	m_pMSAAColorTex.Reset();
	m_pRenderTargetView.Reset();
	m_pDepthStencilView.Reset();
	m_pDepthStencil.Reset();
	m_pBackBuffer.Reset();

	releaseDVB();

	// Release shadow resources
	m_ShadowPassList.clear();  // RAII in ShadowPass struct handles cleanup
	m_bShadowMapReady = false;
	m_pShadowMapDSV.Reset();

	// Exit exclusive fullscreen before resizing buffers (safe to call even if already windowed)
	if (m_pSwapChain)
	{
		BOOL bFullscreen = FALSE;
		IDXGIOutput* pOutput = NULL;
		m_pSwapChain->GetFullscreenState(&bFullscreen, &pOutput);
		if (pOutput) pOutput->Release();
		if (bFullscreen)
			m_pSwapChain->SetFullscreenState(FALSE, NULL);
	}

	// Resize swap chain
	DXGI_SWAP_CHAIN_DESC scDesc;
	if (!getSwapChainDesc(scDesc))
		return false;

	HRESULT hr = m_pSwapChain->ResizeBuffers(
		scDesc.BufferCount,
		scDesc.BufferDesc.Width,
		scDesc.BufferDesc.Height,
		scDesc.BufferDesc.Format,
		scDesc.Flags
	);

	if (FAILED(hr))
	{
		TRACE("Failed to resize swap chain buffers!");
		return false;
	}

	// Recreate render targets
	if (!updateBuffers())
	{
		TRACE("Failed to recreate render targets!");
		return false;
	}

	updateWindow();
	return true;
}

void DisplayDeviceD3D::freeDirectX()
{
	releaseDVB();
	releaseStates();
	releaseShaders();

	// Release shadow resources - RAII handles cleanup
	m_ShadowPassList.clear();
	m_bShadowMapReady = false;
	m_pShadowMapDSV.Reset();

	// Release effects
	// IMPORTANT: Call release() while weak refs are still valid
	for (WeakEffectList::iterator iEffect = m_CreatedEffects.begin();
		iEffect != m_CreatedEffects.end(); ++iEffect)
	{
		DisplayEffect* pEffect = *iEffect;
		if (pEffect != NULL)
			pEffect->release();
	}
	// Clear the list to remove weak refs
	m_CreatedEffects.clear();

	// Release shader resources - RAII handles cleanup
	m_pVS.Reset();
	m_pPS.Reset();
	m_pVSConstantBuffer.Reset();
	m_pPSConstantBuffer.Reset();

	// Release input layouts - RAII handles cleanup
	for (int i = 0; i < VB_COUNT; ++i)
		m_pInputLayouts[i].Reset();

	// Release render targets - RAII handles cleanup
	m_pMSAAColorRTV.Reset();
	m_pMSAAColorTex.Reset();
	m_pRenderTargetView.Reset();
	m_pDepthStencilView.Reset();
	m_pDepthStencil.Reset();
	m_pBackBuffer.Reset();

	// Release device resources - RAII handles cleanup
	m_pSwapChain.Reset();

	if (m_pContext)
	{
		m_pContext->ClearState();
		m_pContext->Flush();
		m_pContext.Reset();
	}

	m_pDevice.Reset();

	// All resources automatically cleaned up by RAII destructors!
}

bool DisplayDeviceD3D::readyShadowMap()
{
	if (m_bShadowMapReady)
		return true;
	if (!m_bShadowMapSupported)
		return false;
	if (m_bMinimized || !m_pDevice)
		return false;

	// Release existing shadow maps - RAII handles cleanup automatically
	for (ShadowPassList::iterator iPass = m_ShadowPassList.begin();
		iPass != m_ShadowPassList.end(); ++iPass)
	{
		(*iPass).m_pShadowMap.Reset();
		(*iPass).m_pShadowMapRTV.Reset();
		(*iPass).m_pShadowMapSRV.Reset();
	}
	m_pShadowMapDSV.Reset();

	m_bShadowMapSupported = false;

	// Check shader support
	if (pixelShaderVersion() < 4.0f || vertexShaderVersion() < 4.0f)
		return false;

	// Load shadow map shader
	if (!(m_pShadowMapShader = getShader("Shaders/ShadowMap.fx")))
		return false;
	if (!m_pShadowMapShader->valid())
		return false;

	m_bShadowMapSupported = true;

	// Create shared depth-stencil buffer used while rendering into shadow maps
	{
		UINT w = (UINT)m_szShadowMap.width;
		UINT h = (UINT)m_szShadowMap.height;

		D3D11_TEXTURE2D_DESC dsDesc = {};
		dsDesc.Width            = w;
		dsDesc.Height           = h;
		dsDesc.MipLevels        = 1;
		dsDesc.ArraySize        = 1;
		dsDesc.Format           = DXGI_FORMAT_D32_FLOAT;
		dsDesc.SampleDesc.Count = 1;
		dsDesc.Usage            = D3D11_USAGE_DEFAULT;
		dsDesc.BindFlags        = D3D11_BIND_DEPTH_STENCIL;

		ID3D11Texture2D* pDSTex = NULL;
		HRESULT hr = m_pDevice->CreateTexture2D(&dsDesc, NULL, &pDSTex);
		if (FAILED(hr) || !pDSTex)
		{
			LOG_ERROR("DisplayDeviceD3D", CharString().format("readyShadowMap: CreateTexture2D (DSV) failed HR=0x%08X", hr));
			return false;
		}

		hr = m_pDevice->CreateDepthStencilView(pDSTex, NULL, m_pShadowMapDSV.GetAddressOf());
		pDSTex->Release();
		if (FAILED(hr))
		{
			LOG_ERROR("DisplayDeviceD3D", CharString().format("readyShadowMap: CreateDepthStencilView failed HR=0x%08X", hr));
			return false;
		}
	}

	// Create per-pass R32F shadow map textures (colour RT + SRV)
	for (ShadowPassList::iterator iPass = m_ShadowPassList.begin();
		iPass != m_ShadowPassList.end(); ++iPass)
	{
		UINT w = (UINT)m_szShadowMap.width;
		UINT h = (UINT)m_szShadowMap.height;

		D3D11_TEXTURE2D_DESC smDesc = {};
		smDesc.Width            = w;
		smDesc.Height           = h;
		smDesc.MipLevels        = 1;
		smDesc.ArraySize        = 1;
		smDesc.Format           = DXGI_FORMAT_R32_FLOAT;
		smDesc.SampleDesc.Count = 1;
		smDesc.Usage            = D3D11_USAGE_DEFAULT;
		smDesc.BindFlags        = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

		HRESULT hr = m_pDevice->CreateTexture2D(&smDesc, NULL, (*iPass).m_pShadowMap.GetAddressOf());
		if (FAILED(hr))
		{
			LOG_ERROR("DisplayDeviceD3D", CharString().format("readyShadowMap: CreateTexture2D (SM) failed HR=0x%08X", hr));
			return false;
		}

		hr = m_pDevice->CreateRenderTargetView((*iPass).m_pShadowMap.Get(), NULL, (*iPass).m_pShadowMapRTV.GetAddressOf());
		if (FAILED(hr))
		{
			LOG_ERROR("DisplayDeviceD3D", CharString().format("readyShadowMap: CreateRenderTargetView failed HR=0x%08X", hr));
			return false;
		}

		hr = m_pDevice->CreateShaderResourceView((*iPass).m_pShadowMap.Get(), NULL, (*iPass).m_pShadowMapSRV.GetAddressOf());
		if (FAILED(hr))
		{
			LOG_ERROR("DisplayDeviceD3D", CharString().format("readyShadowMap: CreateShaderResourceView failed HR=0x%08X", hr));
			return false;
		}
	}

	m_bShadowMapReady = true;
	return true;
}

bool DisplayDeviceD3D::createStates()
{
	HRESULT hr;

	// Create rasterizer states with RAII wrappers
	D3D11_RASTERIZER_DESC rastDesc = {};
	rastDesc.FillMode = D3D11_FILL_SOLID;
	rastDesc.CullMode = D3D11_CULL_BACK;
	rastDesc.FrontCounterClockwise = FALSE;
	rastDesc.DepthClipEnable = TRUE;
	rastDesc.ScissorEnable = FALSE;
	rastDesc.MultisampleEnable = (m_eFSAA != FSAA_NONE) ? TRUE : FALSE;
	rastDesc.AntialiasedLineEnable = (m_eFSAA != FSAA_NONE) ? TRUE : FALSE;

	hr = m_pDevice->CreateRasterizerState(&rastDesc, m_pRasterizerSolid.GetAddressOf());
	if (FAILED(hr)) {
		releaseStates(); // RAII cleanup on failure
		return false;
	}

	rastDesc.FillMode = D3D11_FILL_WIREFRAME;
	hr = m_pDevice->CreateRasterizerState(&rastDesc, m_pRasterizerWireframe.GetAddressOf());
	if (FAILED(hr)) {
		releaseStates();
		return false;
	}

	rastDesc.FillMode = D3D11_FILL_SOLID;
	rastDesc.CullMode = D3D11_CULL_NONE;
	hr = m_pDevice->CreateRasterizerState(&rastDesc, m_pRasterizerPoint.GetAddressOf());
	if (FAILED(hr)) {
		releaseStates();
		return false;
	}

	// Create blend states with RAII wrappers
	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.RenderTarget[0].BlendEnable = FALSE;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	hr = m_pDevice->CreateBlendState(&blendDesc, m_pBlendStateOpaque.GetAddressOf());
	if (FAILED(hr)) {
		releaseStates();
		return false;
	}

	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

	hr = m_pDevice->CreateBlendState(&blendDesc, m_pBlendStateAlpha.GetAddressOf());
	if (FAILED(hr)) {
		releaseStates();
		return false;
	}

	// Additive: SrcAlpha * Src + One * Dest (standard DX9 additive equivalent)
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;

	hr = m_pDevice->CreateBlendState(&blendDesc, m_pBlendStateAdditive.GetAddressOf());
	if (FAILED(hr)) {
		releaseStates();
		return false;
	}

	// Create depth stencil states with RAII wrappers
	D3D11_DEPTH_STENCIL_DESC dsDesc = {};
	dsDesc.DepthEnable = TRUE;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	dsDesc.StencilEnable = FALSE;

	hr = m_pDevice->CreateDepthStencilState(&dsDesc, m_pDepthStencilEnabled.GetAddressOf());
	if (FAILED(hr)) {
		releaseStates();
		return false;
	}

	dsDesc.DepthEnable = FALSE;
	hr = m_pDevice->CreateDepthStencilState(&dsDesc, m_pDepthStencilDisabled.GetAddressOf());
	if (FAILED(hr)) {
		releaseStates();
		return false;
	}

	// Transparent / additive geometry: depth test ON (be occluded by solid geometry)
	// but depth write OFF (don't occlude each other or later solid draws).
	dsDesc.DepthEnable = TRUE;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	hr = m_pDevice->CreateDepthStencilState(&dsDesc, m_pDepthStencilTestOnly.GetAddressOf());
	if (FAILED(hr)) {
		releaseStates();
		return false;
	}

	// Create sampler states with RAII wrappers
	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.MaxAnisotropy = 1;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

	try {
		hr = m_pDevice->CreateSamplerState(&sampDesc, m_pSamplerLinear.GetAddressOf());
	} catch (...) {
		hr = E_FAIL;
		LOG_ERROR("DisplayDeviceD3D", "Exception creating linear sampler!");
	}
	if (FAILED(hr)) {
		if (hr == E_OUTOFMEMORY)
			LOG_ERROR("DisplayDeviceD3D", "OUT OF MEMORY creating linear sampler!");
		releaseStates();
		return false;
	}

	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	try {
		hr = m_pDevice->CreateSamplerState(&sampDesc, m_pSamplerPoint.GetAddressOf());
	} catch (...) {
		hr = E_FAIL;
		LOG_ERROR("DisplayDeviceD3D", "Exception creating point sampler!");
	}
	if (FAILED(hr)) {
		if (hr == E_OUTOFMEMORY)
			LOG_ERROR("DisplayDeviceD3D", "OUT OF MEMORY creating point sampler!");
		releaseStates();
		return false;
	}

	sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	sampDesc.MaxAnisotropy = 16;
	try {
		hr = m_pDevice->CreateSamplerState(&sampDesc, m_pSamplerAnisotropic.GetAddressOf());
	} catch (...) {
		hr = E_FAIL;
		LOG_ERROR("DisplayDeviceD3D", "Exception creating anisotropic sampler!");
	}
	if (FAILED(hr)) {
		if (hr == E_OUTOFMEMORY)
			LOG_ERROR("DisplayDeviceD3D", "OUT OF MEMORY creating anisotropic sampler!");
		releaseStates();
		return false;
	}

	// Set default states using .Get() to retrieve raw pointers
	m_pContext->RSSetState(m_pRasterizerSolid.Get());
	m_pContext->OMSetBlendState(m_pBlendStateOpaque.Get(), NULL, 0xFFFFFFFF);
	m_pContext->OMSetDepthStencilState(m_pDepthStencilEnabled.Get(), 0);

	// D3D11 CRITICAL: Bind default samplers to pixel shader slots
	// Without samplers, textures cannot be read by shaders!
	ID3D11SamplerState* defaultSamplers[4] = { 
		m_pSamplerLinear.Get(),      // Slot 0 - default for most textures
		m_pSamplerLinear.Get(),      // Slot 1 - lightmaps
		m_pSamplerLinear.Get(),      // Slot 2 - bumpmaps  
		m_pSamplerAnisotropic.Get()  // Slot 3 - detail textures
	};
	m_pContext->PSSetSamplers(0, 4, defaultSamplers);
	TRACE("DisplayDeviceD3D::initialize() - Default samplers bound to PS slots 0-3");

	// Create shared fan index buffer — converts triangle fan to triangle list.
	// Pattern: for each triangle i → indices {0, i+1, i+2} (center vertex is always 0).
	const int MAX_FAN_VERTS = 1024;
	const int maxTriangles = MAX_FAN_VERTS - 2;
	const int indexCount = maxTriangles * 3;
	WORD fanIndices[maxTriangles * 3];
	for (int i = 0; i < maxTriangles; i++)
	{
		fanIndices[i * 3 + 0] = 0;
		fanIndices[i * 3 + 1] = (WORD)(i + 1);
		fanIndices[i * 3 + 2] = (WORD)(i + 2);
	}

	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
	ibDesc.ByteWidth = sizeof(WORD) * indexCount;
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA ibData = {};
	ibData.pSysMem = fanIndices;

	hr = m_pDevice->CreateBuffer(&ibDesc, &ibData, m_pFanIndexBuffer.GetAddressOf());
	if (FAILED(hr))
		TRACE("DisplayDeviceD3D::createStates() - WARNING: Fan index buffer creation failed!");

	return true;
}

void DisplayDeviceD3D::releaseStates()
{
	// RAII wrappers handle cleanup automatically
	m_pRasterizerSolid.Reset();
	m_pRasterizerWireframe.Reset();
	m_pRasterizerPoint.Reset();
	m_pBlendStateOpaque.Reset();
	m_pBlendStateAlpha.Reset();
	m_pBlendStateAdditive.Reset();
	m_pDepthStencilEnabled.Reset();
	m_pDepthStencilDisabled.Reset();
	m_pSamplerLinear.Reset();
	m_pSamplerPoint.Reset();
	m_pSamplerAnisotropic.Reset();
	m_pFanIndexBuffer.Reset();
}

bool DisplayDeviceD3D::validateRenderTarget()
{
	if (!activeRTV() || !m_pDepthStencilView)
		return false;

	ID3D11RenderTargetView* pRTV = activeRTV();
	m_pContext->OMSetRenderTargets(1, &pRTV, m_pDepthStencilView.Get());
	return true;
}

bool DisplayDeviceD3D::getSampleMode(DXGI_SWAP_CHAIN_DESC& scDesc)
{
	// Check MSAA support
	if (m_eFSAA != FSAA_NONE)
	{
		UINT quality = 0;
		int sampleCount = (int)m_eFSAA;

		if (SUCCEEDED(m_pDevice->CheckMultisampleQualityLevels(
			scDesc.BufferDesc.Format, sampleCount, &quality)))
		{
			if (quality > 0)
			{
				scDesc.SampleDesc.Count = sampleCount;
				scDesc.SampleDesc.Quality = quality - 1;
				return true;
			}
		}
	}

	scDesc.SampleDesc.Count = 1;
	scDesc.SampleDesc.Quality = 0;
	return false;
}

bool DisplayDeviceD3D::getSwapChainDesc(DXGI_SWAP_CHAIN_DESC& scDesc)
{
	ZeroMemory(&scDesc, sizeof(scDesc));

	if (m_bWindowed)
	{
		scDesc.BufferDesc.Width = m_Mode.screenSize.width;
		scDesc.BufferDesc.Height = m_Mode.screenSize.height;
		scDesc.BufferDesc.RefreshRate.Numerator = 0;
		scDesc.BufferDesc.RefreshRate.Denominator = 1;
		scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		scDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		scDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		scDesc.Windowed = TRUE;
	}
	else
	{
		scDesc.BufferDesc.Width = m_Mode.screenSize.width;
		scDesc.BufferDesc.Height = m_Mode.screenSize.height;
		scDesc.BufferDesc.RefreshRate.Numerator = 60;
		scDesc.BufferDesc.RefreshRate.Denominator = 1;
		scDesc.BufferDesc.Format = findFormat(m_Mode.colorFormat);
		scDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		scDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		scDesc.Windowed = TRUE;  // borderless windowed — avoids DXGI exclusive fullscreen
	}

	// Swap chain is always 1× — MSAA is done on a separate off-screen RT (Path B)
	scDesc.SampleDesc.Count = 1;
	scDesc.SampleDesc.Quality = 0;

	scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.BufferCount = 1;
	scDesc.OutputWindow = m_HWND;
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	scDesc.Flags = 0;

	return true;
}

bool DisplayDeviceD3D::updateBuffers()
{
	HRESULT hr;

	// Get back buffer from swap chain using RAII wrapper
	hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), 
		(void**)m_pBackBuffer.GetAddressOf());
	if (FAILED(hr))
	{
		TRACE("Failed to get back buffer!");
		return false;
	}

	// Create render target view using RAII wrapper
	try { 
		hr = m_pDevice->CreateRenderTargetView(m_pBackBuffer.Get(), NULL, 
			m_pRenderTargetView.GetAddressOf()); 
	}
	catch (...) { 
		hr = E_FAIL; 
		LOG_ERROR("DisplayDeviceD3D", "Exception creating render target view!"); 
	}
	if (FAILED(hr))
	{
		if (hr == E_OUTOFMEMORY)
			LOG_ERROR("DisplayDeviceD3D", "OUT OF MEMORY creating render target view!");
		TRACE("Failed to create render target view!");
		return false;
	}

	// Get buffer description
	D3D11_TEXTURE2D_DESC backBufferDesc;
	m_pBackBuffer->GetDesc(&backBufferDesc);

	// Determine MSAA sample count for off-screen RT and depth buffer
	UINT msaaSamples = 1;
	UINT msaaQuality = 0;
	if (m_eFSAA != FSAA_NONE)
	{
		DXGI_SWAP_CHAIN_DESC scDesc;
		scDesc.BufferDesc.Format = backBufferDesc.Format;
		if (getSampleMode(scDesc))
		{
			msaaSamples = scDesc.SampleDesc.Count;
			msaaQuality = scDesc.SampleDesc.Quality;
		}
	}

	// Create MSAA off-screen color RT when FSAA is requested
	if (msaaSamples > 1)
	{
		D3D11_TEXTURE2D_DESC msaaDesc = {};
		msaaDesc.Width            = backBufferDesc.Width;
		msaaDesc.Height           = backBufferDesc.Height;
		msaaDesc.MipLevels        = 1;
		msaaDesc.ArraySize        = 1;
		msaaDesc.Format           = backBufferDesc.Format;
		msaaDesc.SampleDesc.Count   = msaaSamples;
		msaaDesc.SampleDesc.Quality = msaaQuality;
		msaaDesc.Usage            = D3D11_USAGE_DEFAULT;
		msaaDesc.BindFlags        = D3D11_BIND_RENDER_TARGET;

		hr = m_pDevice->CreateTexture2D(&msaaDesc, NULL, m_pMSAAColorTex.GetAddressOf());
		if (FAILED(hr))
		{
			LOG_ERROR("DisplayDeviceD3D", CharString().format("updateBuffers: CreateTexture2D MSAA failed HR=0x%08X — falling back to 1×", hr));
			msaaSamples = 1;
			msaaQuality = 0;
		}
		else
		{
			hr = m_pDevice->CreateRenderTargetView(m_pMSAAColorTex.Get(), NULL, m_pMSAAColorRTV.GetAddressOf());
			if (FAILED(hr))
			{
				LOG_ERROR("DisplayDeviceD3D", "updateBuffers: CreateRenderTargetView MSAA failed — falling back to 1×");
				m_pMSAAColorTex.Reset();
				msaaSamples = 1;
				msaaQuality = 0;
			}
		}
	}

	// Create depth stencil with the resolved MSAA sample count
	D3D11_TEXTURE2D_DESC depthDesc = {};
	depthDesc.Width            = backBufferDesc.Width;
	depthDesc.Height           = backBufferDesc.Height;
	depthDesc.MipLevels        = 1;
	depthDesc.ArraySize        = 1;
	depthDesc.Format           = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthDesc.SampleDesc.Count   = msaaSamples;
	depthDesc.SampleDesc.Quality = msaaQuality;
	depthDesc.Usage            = D3D11_USAGE_DEFAULT;
	depthDesc.BindFlags        = D3D11_BIND_DEPTH_STENCIL;
	depthDesc.CPUAccessFlags   = 0;
	depthDesc.MiscFlags        = 0;

	// Create depth stencil texture using RAII wrapper
	try
	{
		hr = m_pDevice->CreateTexture2D(&depthDesc, NULL, m_pDepthStencil.GetAddressOf());
	}
	catch (...)
	{
		hr = E_FAIL;
		LOG_ERROR("DisplayDeviceD3D", "Exception creating depth stencil texture!");
	}
	if (FAILED(hr))
	{
		if (hr == E_OUTOFMEMORY)
			LOG_ERROR("DisplayDeviceD3D", "OUT OF MEMORY creating depth stencil! Try reducing resolution.");
		TRACE("Failed to create depth stencil texture!");
		return false;
	}

	// Create depth stencil view using RAII wrapper
	try { 
		hr = m_pDevice->CreateDepthStencilView(m_pDepthStencil.Get(), NULL, 
			m_pDepthStencilView.GetAddressOf()); 
	}
	catch (...) { 
		hr = E_FAIL; 
		LOG_ERROR("DisplayDeviceD3D", "Exception creating depth stencil view!"); 
	}
	if (FAILED(hr))
	{
		if (hr == E_OUTOFMEMORY)
			LOG_ERROR("DisplayDeviceD3D", "OUT OF MEMORY creating depth stencil view!");
		TRACE("Failed to create depth stencil view!");
		return false;
	}

	// Set render targets — use MSAA off-screen RT when available, otherwise back buffer directly
	ID3D11RenderTargetView* pRTV = activeRTV();
	m_pContext->OMSetRenderTargets(1, &pRTV, m_pDepthStencilView.Get());

	// Set viewport
	D3D11_VIEWPORT vp;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width = (float)backBufferDesc.Width;
	vp.Height = (float)backBufferDesc.Height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;

	m_pContext->RSSetViewports(1, &vp);

	return true;
}

void DisplayDeviceD3D::updateWindow()
{
	HWND hParent = GetParent(m_HWND);
	if (hParent == NULL || hParent == GetDesktopWindow())
	{
		if (m_bWindowed)
		{
			RECT r;
			::GetWindowRect(m_HWND, &r);

			int nTop = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
			if (r.top < nTop)
			{
				int nAdjust = nTop - r.top;
				r.bottom += nAdjust;
				r.top += nAdjust;
			}

			int nLeft = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
			if (r.left < nLeft)
			{
				int nAdjust = nLeft - r.left;
				r.right += nAdjust;
				r.left += nAdjust;
			}

			::SetWindowLong(m_HWND, GWL_STYLE, WS_OVERLAPPEDWINDOW);
			::SetWindowLong(m_HWND, GWL_EXSTYLE, ::GetWindowLong(m_HWND, GWL_EXSTYLE) & ~WS_EX_TOPMOST);

			::SetWindowPos(m_HWND, HWND_NOTOPMOST,
				r.left, r.top,
				r.right - r.left,
				r.bottom - r.top,
				SWP_FRAMECHANGED | SWP_SHOWWINDOW);

			::ClipCursor(NULL);
		}
		else
		{
			::SetWindowLong(m_HWND, GWL_STYLE, WS_POPUP);
			::SetWindowLong(m_HWND, GWL_EXSTYLE, ::GetWindowLong(m_HWND, GWL_EXSTYLE) | WS_EX_TOPMOST);

			HMONITOR hMon = MonitorFromWindow(m_HWND, MONITOR_DEFAULTTONEAREST);
			MONITORINFO mi;
			mi.cbSize = sizeof(mi);
			GetMonitorInfo(hMon, &mi);
			::SetWindowPos(m_HWND, HWND_TOPMOST,
				mi.rcMonitor.left, mi.rcMonitor.top,
				mi.rcMonitor.right  - mi.rcMonitor.left,
				mi.rcMonitor.bottom - mi.rcMonitor.top,
				SWP_FRAMECHANGED | SWP_SHOWWINDOW);

			RECT r;
			::GetWindowRect(m_HWND, &r);
			::ClipCursor(&r);
		}
	}
}

void DisplayDeviceD3D::updateProjection()
{
	if (!m_pContext)
		return;

	// Set viewport
	D3D11_VIEWPORT vp;
	vp.TopLeftX = (float)m_Proj.m_rWindow.left;
	vp.TopLeftY = (float)m_Proj.m_rWindow.top;
	vp.Width = (float)m_Proj.m_rWindow.width();
	vp.Height = (float)m_Proj.m_rWindow.height();
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;

	m_pContext->RSSetViewports(1, &vp);

	// Calculate view and projection matrices (would be passed to shaders via constant buffers)
}

void DisplayDeviceD3D::enumerateTextures()
{
	m_TextureFormats.release();

	// DX11 supports these formats universally
	// NOTE: RGB888 (24-bit) removed — no 24-bit DXGI format exists; engine converts to RGB8888 instead.
	// RGB888L also removed — same 24-bit issue; findFormat maps it to B8G8R8A8 (32-bit) causing data misalignment.
	// RGB555 and RGB4444 removed — no DXGI equivalent; findFormat returns DXGI_FORMAT_UNKNOWN.
	m_TextureFormats.push(ColorFormat::RGB8888);
	m_TextureFormats.push(ColorFormat::RGB565);
	m_TextureFormats.push(ColorFormat::RGB5551);
	m_TextureFormats.push(ColorFormat::DXT1);
	m_TextureFormats.push(ColorFormat::DXT3);
	m_TextureFormats.push(ColorFormat::DXT5);

	TRACE(CharString().format("DisplayDeviceD3D::enumerateTextures, %d valid texture formats found!",
		m_TextureFormats.size()));
}

ColorFormat::Format DisplayDeviceD3D::findFormat(DXGI_FORMAT format)
{
	switch (format)
	{
	case DXGI_FORMAT_R8G8B8A8_UNORM:
		return ColorFormat::RGB8888;
	case DXGI_FORMAT_B8G8R8A8_UNORM:
		return ColorFormat::RGB8888;
	case DXGI_FORMAT_B5G6R5_UNORM:
		return ColorFormat::RGB565;
	case DXGI_FORMAT_B5G5R5A1_UNORM:
		return ColorFormat::RGB5551;
	case DXGI_FORMAT_BC1_UNORM:
		return ColorFormat::DXT1;
	case DXGI_FORMAT_BC2_UNORM:
		return ColorFormat::DXT3;
	case DXGI_FORMAT_BC3_UNORM:
		return ColorFormat::DXT5;
	}

	return ColorFormat::INVALID;
}

DXGI_FORMAT DisplayDeviceD3D::findFormat(ColorFormat::Format format)
{
	switch (format)
	{
	case ColorFormat::RGB888:
	case ColorFormat::RGB8888:
	case ColorFormat::RGB888L:
		return DXGI_FORMAT_B8G8R8A8_UNORM;
	case ColorFormat::RGB565:
		return DXGI_FORMAT_B5G6R5_UNORM;
	case ColorFormat::RGB5551:
		return DXGI_FORMAT_B5G5R5A1_UNORM;
	case ColorFormat::DXT1:
		return DXGI_FORMAT_BC1_UNORM;
	case ColorFormat::DXT3:
		return DXGI_FORMAT_BC2_UNORM;
	case ColorFormat::DXT5:
		return DXGI_FORMAT_BC3_UNORM;
	}

	return DXGI_FORMAT_UNKNOWN;
}

int DisplayDeviceD3D::getFormatDepth(DXGI_FORMAT format)
{
	switch (format)
	{
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
		return 32;
	case DXGI_FORMAT_B5G6R5_UNORM:
	case DXGI_FORMAT_B5G5R5A1_UNORM:
		return 16;
	}

	return 0;
}

void DisplayDeviceD3D::setD3DMatrix(DirectX::XMMATRIX& m, const Matrix33& mRotation, const Vector3& vOffset)
{
	m = XMMatrixSet(
		mRotation.i.x, mRotation.i.y, mRotation.i.z, 0.0f,
		mRotation.j.x, mRotation.j.y, mRotation.j.z, 0.0f,
		mRotation.k.x, mRotation.k.y, mRotation.k.z, 0.0f,
		vOffset.x, vOffset.y, vOffset.z, 1.0f
	);
}

bool DisplayDeviceD3D::updateClientArea(bool a_bAllowReset)
{
	if (m_bWindowed)
	{
		RectInt clientWindow;
		if (IsWindow(m_HWND) && !IsIconic(m_HWND))
		{
			m_ClientPlacement.length = sizeof(m_ClientPlacement);
			GetWindowPlacement(m_HWND, &m_ClientPlacement);

			RECT rect;
			GetClientRect(m_HWND, &rect);
			ClientToScreen(m_HWND, (POINT*)&rect);
			ClientToScreen(m_HWND, ((POINT*)&rect) + 1);

			clientWindow = RectInt(rect.left, rect.top, rect.right - 1, rect.bottom - 1);
			if (!clientWindow.valid())
			{
				clientWindow = RectInt(0, 0, 0, 0);
				m_bMinimized = true;
			}
			else
				m_bMinimized = false;
		}
		else
			m_bMinimized = true;

		if (clientWindow != m_ClientRectangle)
		{
			m_ClientRectangle = clientWindow;
			if (a_bAllowReset)
				resetDirectX();
		}
	}
	else
	{
		m_ClientRectangle = RectInt(0, 0, m_Mode.screenSize);
		m_bMinimized = IsWindow(m_HWND) && !IsIconic(m_HWND) ? false : true;
	}

	return false;
}

void DisplayDeviceD3D::enumerateModes()
{
	sm_ModeList.release();

	DXGIFactoryPtr pFactory;
	if (FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)pFactory.GetAddressOf())))
		return;

	DXGIAdapterPtr pAdapter;
	for (UINT i = 0; pFactory->EnumAdapters(i, pAdapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		DXGI_ADAPTER_DESC adapterDesc;
		pAdapter->GetDesc(&adapterDesc);

		DXGIOutputPtr pOutput;
		if (pAdapter->EnumOutputs(0, pOutput.GetAddressOf()) == S_OK)
		{
			UINT numModes = 0;
			DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;

			pOutput->GetDisplayModeList(format, 0, &numModes, NULL);

			if (numModes > 0)
			{
				DXGI_MODE_DESC* pModes = new DXGI_MODE_DESC[numModes];
				pOutput->GetDisplayModeList(format, 0, &numModes, pModes);

				for (UINT j = 0; j < numModes; ++j)
				{
					int depth = getFormatDepth(pModes[j].Format);
					if (depth > 0)
					{
						CharString description;
						description.format("%d:%dx%dx%d", i,
							pModes[j].Width, pModes[j].Height, depth);

						DisplayDevice::Mode newMode;
						newMode.modeDescription = description;
						newMode.deviceID = (void*)i;
						newMode.screenSize = SizeInt(pModes[j].Width, pModes[j].Height);
						newMode.colorFormat = findFormat(pModes[j].Format);

						bool found = false;
						for (int k = 0; k < sm_ModeList.size() && !found; k++)
							if (newMode.modeDescription == sm_ModeList[k].modeDescription)
								found = true;

						if (!found)
							sm_ModeList.push(newMode);
					}
				}

				delete[] pModes;
			}
		}
	}
}

void DisplayDeviceD3D::updateGlobals()
{
	if (!m_pMatShader.valid() || m_pMatShader->released())
		return;

	DirectX::XMMATRIX mView = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX mProj = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX mWorld = DirectX::XMMatrixIdentity();

	if (m_Proj.m_fFOV < 0.0f)
	{
		float width = (float)m_Proj.m_rWindow.width();
		float height = (float)m_Proj.m_rWindow.height();
		mProj = DirectX::XMMatrixOrthographicOffCenterLH(
			0.0f, width,
			height, 0.0f,
			m_Proj.m_fFront, m_Proj.m_fBack
		);
	}
	else
	{
		float aspect = (float)m_Proj.m_rWindow.width() / (float)m_Proj.m_rWindow.height();
		mProj = DirectX::XMMatrixPerspectiveFovLH(
			m_Proj.m_fFOV,
			aspect,
			m_Proj.m_fFront,
			m_Proj.m_fBack
		);
	}

	DirectX::XMVECTOR vEye = DirectX::XMVectorSet(
		m_Proj.m_vPosition.x, m_Proj.m_vPosition.y, m_Proj.m_vPosition.z, 1.0f);
	DirectX::XMVECTOR vAt = DirectX::XMVectorSet(
		m_Proj.m_vPosition.x + m_Proj.m_mFrame.k.x,
		m_Proj.m_vPosition.y + m_Proj.m_mFrame.k.y,
		m_Proj.m_vPosition.z + m_Proj.m_mFrame.k.z,
		1.0f);
	DirectX::XMVECTOR vUp = DirectX::XMVectorSet(
		m_Proj.m_mFrame.j.x, m_Proj.m_mFrame.j.y, m_Proj.m_mFrame.j.z, 0.0f);

	mView = DirectX::XMMatrixLookAtLH(vEye, vAt, vUp);

	if (m_pCurrentTransform.valid())
	{
		PrimitiveSetTransformD3D* pTransformD3D = (PrimitiveSetTransformD3D*)m_pCurrentTransform.pointer();
		mWorld = pTransformD3D->m_Matrix;
	}

	m_pMatShader->setConstant("mWorld", mWorld);
	m_pMatShader->setConstant("mView", mView);
	m_pMatShader->setConstant("mProj", mProj);

	// Elapsed time in seconds — wraps at ~49 days, sin() is periodic so no discontinuity.
	m_pMatShader->setConstant("fTime", (float)GetTickCount() * 0.001f);

	Vector3 vCameraPos = m_Proj.m_vPosition;
	m_pMatShader->setConstant("vCameraPos", vCameraPos);

	// Propagate the highest-priority scene light (e.g. the system's star, pushed via
	// NodeLight::render()->addDirectionalLight()/addPointLight()) into the shader's
	// nLightType/vLightDirection/vLightPosition constants. Without this, m_Lights was
	// collected every frame but never consumed -- PS_Atmosphere/PS_Planet/PS_PlanetSurface
	// always fell back on the hardcoded default direction (0,0,-1), so the terminator/
	// atmosphere haze never actually tracked the real star position as the camera orbited.
	if (!m_Lights.empty())
	{
		const Light& sunLight = m_Lights.begin()->second;
		if (sunLight.type == Light::DIRECTIONAL)
		{
			m_pMatShader->setConstant("nLightType", 3);	// LIGHT_DIRECTIONAL
			m_pMatShader->setConstant("vLightDirection", sunLight.direction);
		}
		else
		{
			m_pMatShader->setConstant("nLightType", 1);	// LIGHT_POINT
			m_pMatShader->setConstant("vLightPosition", sunLight.position);
		}
	}

	// Propagate shadow map state so the Default shader can perform shadow lookups.
	if (m_bShadowMapActive)
	{
		m_pMatShader->setConstant("mLightView", m_ShadowLightView);
		m_pMatShader->setConstant("mLightProj", m_ShadowLightProj);
		m_pMatShader->setConstant("bEnableShadowMap", true);
		m_pMatShader->setConstant("vShadowMapSize",
			Vector3((float)m_szShadowMap.width, (float)m_szShadowMap.height, 0.0f));
	}
	else
	{
		m_pMatShader->setConstant("bEnableShadowMap", false);
	}
}

//------------------------------------------------------------------------------------------------
// EOF