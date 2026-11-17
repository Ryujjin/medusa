/*
	DisplayDeviceD3D.h
	(c)2005 Palestar, Richard Lyle
	Updated to DirectX 11
*/

#ifndef DISPLAY_DEVICE_D3D_H
#define DISPLAY_DEVICE_D3D_H

#include "Display/DisplayDevice.h"
#include "Display/PrimitiveMaterial.h"
#include "Display/PrimitiveSetTransform.h"
#include "Standard/Array.h"
#include "Standard/List.h"
#include "Standard/CriticalSection.h"
#include <functional>

#include "ShaderD3D11.h" // Shawn added for shader reflection
#include "D3D11Ptr.h"     // RAII wrappers for D3D11 resources
//#include "Shader.h"

// Helper macro to safely execute D3D11 calls with exception handling
// Usage: D3D_SAFE_CALL(hr, pDevice->CreateBuffer(...), "Failed to create buffer")
#define D3D_SAFE_CALL(hrVar, d3dCall, errorMsg) \
	do { \
		try { \
			hrVar = (d3dCall); \
		} catch (...) { \
			hrVar = E_FAIL; \
			LOG_ERROR("D3D11", "Exception in " errorMsg); \
		} \
	} while(0)

// Helper macro for CreateBuffer calls that assigns pointer
// Usage: D3D_SAFE_CREATE_BUFFER(hr, pDevice->CreateBuffer(..., &pBuffer), pBuffer, "vertex buffer")
#define D3D_SAFE_CREATE_BUFFER(hrVar, d3dCall, ptrVar, desc) \
	do { \
		try { \
			hrVar = (d3dCall); \
		} catch (...) { \
			hrVar = E_FAIL; \
			ptrVar = NULL; \
			LOG_ERROR("D3D11", "Exception creating " desc); \
		} \
		if (FAILED(hrVar) && ptrVar != NULL) ptrVar = NULL; \
	} while(0)

#if _MSC_VER > 1310
// Fix for VS2005+
#ifndef POINTER_64
#define POINTER_64		__ptr64
#endif
#endif

#if defined(WIN32)
#include <d3d11.h>
#include <d3dcompiler.h>
//#include <dxgi1_2.h>
#include <DirectXMath.h>
#elif defined(XBOX)
#error "Xbox not supported with DirectX 11"
#endif

//---------------------------------------------------------------------------------------------------

//! Define to non-zero to enable shader debugging
#define ENABLE_SHADER_DEBUGGING			0
//! Enable shadow map depth surface
#define ENABLE_SHADOW_MAP_DS			1
//! Define to 1 to enable rendering to shadow map into the back buffer for debugging purposes
#define ENABLE_SHADOW_MAP_DEBUG			0
//! How big to render each shadow map in the main display area
#define DEBUG_SHADOW_MAP_SIZE			256

//----------------------------------------------------------------------------

#define RELEASEDD( qinterface )	if (qinterface != NULL) { qinterface->Release(); qinterface = NULL; }

//----------------------------------------------------------------------------

class PrimitiveSurfaceD3D;		// forward declare

class DisplayDeviceD3D : public DisplayDevice
{
public:
	DECLARE_WIDGET_CLASS();

	// Constants
	enum {
		PRIMITIVE_STACK_SIZE = 1024 * 8,		// the default size for the primitive stacks
		DYNAMIC_VB_SIZE = 1024 * 256,	// size of the preallocate dynamic vertex buffers

		VB_VERTEX = 0,			// untransformed and unlit verticies
		VB_VERTEXL = 1,			// untransformed and lit vertices
		VB_VERTEXTL = 2,			// transformed and lit verticies
		VB_COUNT = 3,			// number of streams
	};

	// Types
	typedef Reference<DisplayDeviceD3D>			Ref;
	typedef PrimitiveMaterial::Blending			Blending;
	typedef Array<DisplayDevice::Mode>			ModeList;
	typedef Array<DisplayDeviceD3D*>			DeviceList;

	typedef Array<ColorFormat::Format>						TextureFormat;
	typedef Hash< int, Array< PrimitiveMaterial::Ref > >	StackHash;

	// Construction
	DisplayDeviceD3D();
	virtual ~DisplayDeviceD3D();

	// Accessors
	virtual bool					isLocked() const;
	virtual int						modeCount() const;
	virtual const Mode* mode(int n) const;
	virtual ColorFormat* primaryFormat() const;
	virtual int						surfaceFormatCount() const;
	virtual ColorFormat::Format		surfaceFormat(int n) const;
	virtual const Mode* activeMode() const;
	virtual bool					windowed() const;
	virtual RectInt					clientWindow() const;
	virtual RectInt					renderWindow() const;

	virtual bool					textureP2() const;				// always false in DX11
	virtual bool					textureSquare() const;			// always false in DX11
	virtual SizeInt					textureMaxSize() const;
	virtual SizeInt					textureMinSize() const;
	virtual float					pixelShaderVersion() const;		// returns 5.0 for DX11
	virtual float					vertexShaderVersion() const;	// returns 5.0 for DX11
	virtual FilterMode				filterMode() const;

	virtual dword					totalVideoMemory() const;
	virtual dword					freeVideoMemory() const;

	// Mutators
	virtual void					lock();
	virtual void					unlock();
	virtual bool					initialize(void* hWnd, const Mode* pMode,
		bool bWindowed, bool bHardware, FSAA eFSAA = FSAA_NONE);
	virtual bool					setMode(const Mode* pMode, bool bWindowed);
	virtual void					setFilterMode(FilterMode eMode);
	virtual void					release();

	virtual void					flushVideoMemory() const;

	virtual void					clear(Color nColor);
	virtual void					clearZ(float fDepth = 1.0f);

	virtual void					setAmbient(Color nColor);
	virtual int						addDirectionalLight(int nPriroty,
		Color nColor,
		const Vector3& vDirection);
	virtual int						addPointLight(int nPriority,
		Color nColor,
		const Vector3& vPosition,
		float fRadius);
	virtual void					clearLights();

	virtual void					setFillMode(FillMode nMode);
	virtual void					setFog(FogMode nMode,
		float fBegin,
		float fEnd,
		Color nColor);
	virtual void					disableFog();

	virtual void					setProjection(
		const Matrix33& mFrame,
		const Vector3& vPosition,
		const RectInt& rWindow,
		float fFOV,
		float fFront,
		float fBack);
	virtual void					setShadowPass(
		int a_nMaxShadowLights,
		const Vector3& a_vFocus,
		float a_fShadowRadius,
		const SizeInt& a_szShadowMap);

	virtual bool					beginScene();
	virtual bool					beginShadowPass(Transform& a_LightTransform);
	virtual bool					endShadowPass();
	virtual bool					endScene();
	virtual void					abortScene();
	virtual void					present();

	virtual DevicePrimitive* create(const PrimitiveKey& key);
	virtual bool					push(DevicePrimitive* primitive);

	virtual DisplayEffect::Ref		createEffect(const char* pName);
	virtual bool					push(DisplayEffect* pEffect);

	virtual bool					capture(const char* pFilename);

	// Shader management
	// nVertexType: 0=VB_VERTEX (3D), 1=VB_VERTEXL (lit), 2=VB_VERTEXTL (screen-space)
	Shader::Ref						getShader(const char* pShader, int nVertexType = 0);
	bool							releaseShader(const char* pShader);
	void							releaseShaders();

	// Apply UI shader for VB_VERTEXTL primitives (screen-space vertices)
	bool							applyUIShader();

	// Register effect factory by name
	void							registerEffect(const char* pName, Factory* pFactory);

	// Allocate dynamic vertex buffer space
	int								allocateDVB(int vertexType, int vertexCount,
		ID3D11Buffer** ppBuffer);
	int								setupDVB(int vertexType, int vertexCount, void* pVerts);
	void							resetDVB();
	void							releaseDVB();

	DWORD							getD3DFilterMode() const;

	// DirectX 11 specific accessors (return raw pointers for compatibility)
	ID3D11Device* getDevice() { return m_pDevice.Get(); }
	ID3D11DeviceContext* getContext() { return m_pContext.Get(); }

	//----------------------------------------------------------------------------

	//! Types
	struct Light {
		enum Type {
			DIRECTIONAL,
			POINT
		};

		Type			type;
		Color			color;
		Vector3			direction;		// for directional lights
		Vector3			position;		// for point lights
		float			radius;			// for point lights
		float			attenuation0;
		float			attenuation1;
		float			attenuation2;
	};

	typedef std::multimap< int, Light, std::greater<int> >		LightMap;
	typedef std::map< CharString, Shader::Ref >				ShaderMap;
	typedef std::map< CharString, Factory* >					EffectMap;
	typedef std::list< DisplayEffect::Ref >						EffectList;
	typedef std::list< DisplayEffect::WeakRef >					WeakEffectList;

	struct Projection {
		Projection() : m_mFrame(Matrix33::IDENTITY),
			m_vPosition(Vector3::ZERO),
			m_rWindow(PointInt(0, 0), SizeInt(1024, 1024)),
			m_fFOV(PI / 4.0f),
			m_fFront(1.0f),
			m_fBack(20000.0f)
		{
		};

		Matrix33						m_mFrame;
		Vector3							m_vPosition;
		RectInt							m_rWindow;
		float							m_fFOV;
		float							m_fFront;
		float							m_fBack;
	};

	struct ShadowPass
	{
		ShadowPass() : m_bOrthoProj(false),
			m_LightView(DirectX::XMMatrixIdentity()), m_LightProj(DirectX::XMMatrixIdentity())
		{
		}
		// Destructor no longer needed - RAII handles cleanup

		Array< DevicePrimitive::Ref >	m_Primitives;
		Transform						m_LightTransform;
		DirectX::XMMATRIX				m_LightView;
		DirectX::XMMATRIX				m_LightProj;
		D3D11Texture2DPtr				m_pShadowMap;           // RAII wrapper
		D3D11RenderTargetViewPtr		m_pShadowMapRTV;        // RAII wrapper
		D3D11ShaderResourceViewPtr		m_pShadowMapSRV;        // RAII wrapper
		bool							m_bOrthoProj;
	};
	typedef std::list< ShadowPass >		ShadowPassList;

	// Data
	CriticalSection					m_Lock;
	dword							m_dwLockingThread;
	dword							m_nLockCount;

	bool							m_bBeginScene;
	Array< PrimitiveMaterial::Ref >	m_Stack[PASS_COUNT];
	PrimitiveMaterial::Ref			m_pCurrentMaterial;
	int								m_nTextureStage;
	bool							m_bDiffuseBound;				// true when a DIFFUSE surface SRV was bound by setupTextureStages()
	PrimitiveSetTransform::Ref		m_pCurrentTransform;

	TextureFormat					m_TextureFormats;
	bool							m_TextureP2;					// always false in DX11
	bool							m_TextureSquare;				// always false in DX11
	SizeInt							m_TextureMaxSize;
	SizeInt							m_TextureMinSize;
	float							m_fPixelShaderVersion;			// 5.0 for DX11
	float							m_fVertexShaderVersion;			// 5.0 for DX11
	FilterMode						m_eFilterMode;
	int								m_MaxAnisotropy;

	HWND							m_HWND;
	bool							m_bHardware;
	ColorFormat::Ref				m_pFormat;
	Mode							m_Mode;
	bool							m_bWindowed;
	FSAA							m_eFSAA;

	RectInt							m_ClientRectangle;
	bool							m_bMinimized;
	WINDOWPLACEMENT					m_ClientPlacement;
	Projection						m_Proj;

	// DirectX 11 Core Objects (using RAII wrappers for automatic cleanup)
	D3D11DevicePtr					m_pDevice;
	D3D11ContextPtr					m_pContext;
	D3D11SwapChainPtr				m_pSwapChain;
	D3D11Texture2DPtr				m_pBackBuffer;
	D3D11RenderTargetViewPtr		m_pRenderTargetView;		// 1× swap chain RTV (post-process / present)
	D3D11Texture2DPtr				m_pDepthStencil;
	D3D11DepthStencilViewPtr		m_pDepthStencilView;
	D3D11Texture2DPtr				m_pMSAAColorTex;			// Off-screen MSAA color RT (null when FSAA_NONE)
	D3D11RenderTargetViewPtr		m_pMSAAColorRTV;			// RTV for MSAA color RT

	// Returns the active scene RTV: MSAA off-screen RT when FSAA is on, back buffer otherwise
	ID3D11RenderTargetView* activeRTV() const
	{
		return m_pMSAAColorRTV ? m_pMSAAColorRTV.Get() : m_pRenderTargetView.Get();
	}

	Color							m_cAmbientLight;
	LightMap						m_Lights;
	dword							m_NextLightId;

	dword							m_VBOffset[VB_COUNT];
	D3D11BufferPtr					m_VB[VB_COUNT];              // RAII wrapper
	D3D11InputLayoutPtr				m_pInputLayouts[VB_COUNT];   // RAII wrapper

	ShaderMap						m_ShaderMap;
	D3D11VertexShaderPtr			m_pVS;                       // RAII wrapper
	D3D11PixelShaderPtr				m_pPS;                       // RAII wrapper
	D3D11BufferPtr					m_pVSConstantBuffer;         // RAII wrapper
	D3D11BufferPtr					m_pPSConstantBuffer;         // RAII wrapper
	D3D11BufferPtr					m_pFanIndexBuffer;           // Shared fan→triangle-list IB

	EffectMap						m_EffectMap;
	WeakEffectList					m_CreatedEffects;
	EffectList						m_EffectList;

	bool							m_bShadowMapReady;
	bool							m_bShadowMapSupported;
	Vector3							m_vShadowFocus;
	float							m_fShadowRadius;
	SizeInt							m_szShadowMap;
	bool							m_bShadowPass;
	int								m_nMaxShadowLights;
	int								m_nShadowMapPass;
	ShadowPassList					m_ShadowPassList;
	D3D11DepthStencilViewPtr		m_pShadowMapDSV;             // RAII wrapper

	bool							m_bFirstShadowPass;
	LightMap::iterator				m_iCurrentShadowLight;
	ShadowPassList::iterator		m_iCurrentShadowPass;
	bool							m_bUsingFixedFunction;			// always false in DX11

	bool							m_bShadowMapActive;			// true after at least one shadow pass has rendered this frame
	DirectX::XMMATRIX				m_ShadowLightView;			// light-space view matrix for main-pass shadow lookup
	DirectX::XMMATRIX				m_ShadowLightProj;			// light-space projection matrix for main-pass shadow lookup

	Shader::Ref						m_pDefaultShader;
	Shader::Ref						m_pUIShader;			// screen-space shader for VB_VERTEXTL primitives
	Shader::Ref						m_pShadowMapShader;
	Shader::Ref						m_pMatShader;

	// Render States (using RAII wrappers for automatic cleanup)
	D3D11RasterizerStatePtr			m_pRasterizerSolid;
	D3D11RasterizerStatePtr			m_pRasterizerWireframe;
	D3D11RasterizerStatePtr			m_pRasterizerPoint;
	D3D11BlendStatePtr				m_pBlendStateOpaque;
	D3D11BlendStatePtr				m_pBlendStateAlpha;
	D3D11BlendStatePtr				m_pBlendStateAdditive;
	D3D11DepthStencilStatePtr		m_pDepthStencilEnabled;
	D3D11DepthStencilStatePtr		m_pDepthStencilDisabled;
	D3D11DepthStencilStatePtr		m_pDepthStencilTestOnly;	// depth test ON, write OFF — for additive/alpha transparent geometry
	D3D11SamplerStatePtr			m_pSamplerLinear;
	D3D11SamplerStatePtr			m_pSamplerPoint;
	D3D11SamplerStatePtr			m_pSamplerAnisotropic;

	// Static
	static DXGIFactoryPtr			sm_pDXGIFactory;             // RAII wrapper
	static ModeList					sm_ModeList;
	static DeviceList				sm_DeviceList;

	// Mutators
	bool							updateClientArea(bool a_bAllowReset);

	bool							initializeDirectX();
	bool							resetDirectX();
	void							freeDirectX();
	bool							readyShadowMap();

	bool							createStates();
	void							releaseStates();

	bool							validateRenderTarget();
	bool							getSampleMode(DXGI_SWAP_CHAIN_DESC& scDesc);
	bool							getSwapChainDesc(DXGI_SWAP_CHAIN_DESC& scDesc);
	bool							updateBuffers();
	void							updateWindow();
	void							updateProjection();
	void							enumerateTextures();

	DXGI_FORMAT						findDepthFormat(DXGI_FORMAT adapterFormat);
	bool							checkDepthFormat(DXGI_FORMAT adapterFormat, DXGI_FORMAT depthFormat);

	bool							setPixelShader(ID3D11PixelShader* pPixelShader);
	bool							setVertexShader(ID3D11VertexShader* pVertexShader);
	void							updateGlobals();
	void							clearShaders();
	void							clearPixelShader();
	void							clearVertexShader();

	// Static
	static void						enumerateModes();
	static ColorFormat::Format		findFormat(DXGI_FORMAT format);
	static DXGI_FORMAT				findFormat(ColorFormat::Format format);
	static int						getFormatDepth(DXGI_FORMAT format);

	static void						setD3DMatrix(DirectX::XMMATRIX& m, const Matrix33& mRotation, const Vector3& vOffset);
};

//----------------------------------------------------------------------------

#endif

//------------------------------------------------------------------------------------
// EOF
