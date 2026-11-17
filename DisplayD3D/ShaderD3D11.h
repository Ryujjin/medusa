#pragma once
/*
	Replacement for
	Shader.h
*/

#ifndef SHADER_H
#define SHADER_H

#include "Standard/Reference.h"
#include "Standard/CharString.h"
#include "Math/Vector3.h"
#include "Math/Matrix33.h"
#include "Display/Color.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <map>
#include <string>

//---------------------------------------------------------------------------------------------------

class DisplayDeviceD3D;

class Shader : public Referenced
{
public:
	// Types
	typedef Reference<Shader>		Ref;

	// Construction
	Shader();
	virtual ~Shader();

	// Accessors
	bool					valid() const;		// returns true if shader is loaded and compiled
	bool					released() const;	// returns true if shader has been released

	// Mutators
	// nVertexType: 0=VB_VERTEX (3D), 1=VB_VERTEXL (lit), 2=VB_VERTEXTL (screen-space UI)
	bool					load(DisplayDeviceD3D* pDevice, const char* pShaderFile, int nVertexType = 0);
	void					release();

	bool					apply(DisplayDeviceD3D* pDevice);
	// Switches to the pre-lit vertex shader (VS_MainL) + VertexL input layout.
	// Call just before drawing VB_VERTEXL geometry so per-vertex COLOR (diffuse
	// + alpha) reaches the pixel shader.  Returns false if VS_MainL was not
	// compiled (shader file predates this feature).
	bool					applyVertexL(DisplayDeviceD3D* pDevice);
	// Switches to VS_Galaxy + PS_Galaxy (SV_VertexID fullscreen triangle, no VB).
	// Re-uploads the constant buffer so galaxy constants (vGnpDir/vGalCenDir) are
	// on the GPU before Draw(3,0).  Returns false if galaxy shaders were not compiled.
	bool					applyGalaxy(DisplayDeviceD3D* pDevice);
	// Switches to VS_GalaxySprite + PS_GalaxySprite + VERTEXL input layout.
	// Used by PrimitiveGalaxySpriteSetD3D to draw textured billboard quads from a
	// 7x6 atlas (t6 = galaxy_sprites.png).  Returns false if shaders not compiled.
	bool					applyGalaxySprite(DisplayDeviceD3D* pDevice);
	// Switches to VS_Star + PS_Star + VERTEXL input layout.
	// Used by PrimitiveStarSurfaceD3D to draw the procedural stellar surface billboard.
	// Returns false if star shaders were not compiled.
	bool					applyStarSurface(DisplayDeviceD3D* pDevice);
	// Switches to VS_Tess/HS_Main/DS_Main + PS_StarSphere + standard VERTEX input layout.
// Used by PrimitiveStarSurfaceD3D to draw a tessellated sphere with the procedural star shader.
// Returns false if required shaders were not compiled (non-fatal).
	bool					applyStarSphere(DisplayDeviceD3D* pDevice);
	// Switches to VS_StarSphere + PS_StarSphere + standard VERTEX input layout.
	// Draws a CPU sphere mesh as a true 3D object (natural clip depth, no tessellation).
	// Returns false if shaders were not compiled (non-fatal).
	bool					applyStarSphere3D(DisplayDeviceD3D* pDevice);
	// Switches to VS_StarSphere + PS_CoronaShell + standard VERTEX input layout.
	// Draws the corona geometry shell sphere (rendered on top of the photosphere sphere).
	// Returns false if shaders were not compiled (non-fatal).
	bool					applyStarCoronaShell(DisplayDeviceD3D* pDevice);
// Switches to VS_Tess/HS_Main/DS_Main + standard VERTEX input layout for hardware tessellation.
	// Sets VS, HS, DS stages; PS_Main remains active. Returns false if not compiled (non-fatal).
	bool					applyTessellation(DisplayDeviceD3D* pDevice);
	// Clears HS and DS stages to restore the standard non-tessellated pipeline.
	void					clearTessellation(DisplayDeviceD3D* pDevice);
	// Binds CS_Main for compute dispatch. Returns false if not compiled.
	bool					applyCompute(DisplayDeviceD3D* pDevice);
	// Dispatches the bound compute shader with the given thread group counts.
	void					dispatch(DisplayDeviceD3D* pDevice, UINT x, UINT y, UINT z);
	// Switches to VS_Video + PS_Video (SV_VertexID fullscreen triangle, no VB).
	// Caller binds the decoded video frame at t8 before calling Draw(3,0).
	// vVideoParams must be set via setConstant("vVideoParams",...) before this call.
	// Returns false if video shaders were not compiled (non-fatal).
	bool					applyVideo(DisplayDeviceD3D* pDevice);
	// Switches to standard VERTEX input layout + VS_Main + PS_ProceduralPlanet.
	// Renders procedurally-generated planets with Perlin noise terrain.
	// Set vPlanetColor, fTerrainScale, nOctaves via setConstant() before calling.
	// Returns false if PS_ProceduralPlanet was not compiled (non-fatal).
	bool					applyProceduralPlanet(DisplayDeviceD3D* pDevice);
	// Switches to VS_Main + PS_PlanetSurface + standard VERTEX input layout.
	// Renders the Option-2 FBM surface layer (continent/ocean zones, limb darkening,
	// day/night modulation, specular ocean hotspot, polar ice caps) as an additive
	// overlay sphere at 1.005x planet radius, drawn BEFORE the PS_Planet rim pass.
	// Returns false if PS_PlanetSurface was not compiled (non-fatal).
	bool					applyPlanetSurface(DisplayDeviceD3D* pDevice);
	// Switches to VS_MainL + PS_Atmosphere + VertexL input layout.
	// Renders the planet atmosphere/cloud shell overlay sphere (drawn additively
	// slightly larger than the planet surface radius).
	// Returns false if PS_Atmosphere was not compiled (non-fatal).
	bool					applyAtmosphere(DisplayDeviceD3D* pDevice);

	// Constant management
	void					setConstant(const char* pName, bool value);
	void					setConstant(const char* pName, int value);
	void					setConstant(const char* pName, float value);
	void					setConstant(const char* pName, const Vector3& value);
	void					setConstant(const char* pName, const Matrix33& value);
	void					setConstant(const char* pName, const DirectX::XMMATRIX& value);
	void					setConstant(const char* pName, const DirectX::XMFLOAT4& value);
	void					setConstant(const char* pName, Color value);

	void					clearConstant(const char* pName);

	// Static helpers
	static bool				applyConstant(DisplayDeviceD3D* pDevice, ID3D11Buffer* pConstantBuffer,
		const char* pName, bool value);
	static bool				applyConstant(DisplayDeviceD3D* pDevice, ID3D11Buffer* pConstantBuffer,
		const char* pName, int value);
	static bool				applyConstant(DisplayDeviceD3D* pDevice, ID3D11Buffer* pConstantBuffer,
		const char* pName, float value);
	static bool				applyConstant(DisplayDeviceD3D* pDevice, ID3D11Buffer* pConstantBuffer,
		const char* pName, const Vector3& value);
	static bool				applyConstant(DisplayDeviceD3D* pDevice, ID3D11Buffer* pConstantBuffer,
		const char* pName, const Matrix33& value);
	static bool				applyConstant(DisplayDeviceD3D* pDevice, ID3D11Buffer* pConstantBuffer,
		const char* pName, const DirectX::XMMATRIX& value);
	static bool				applyConstant(DisplayDeviceD3D* pDevice, ID3D11Buffer* pConstantBuffer,
		const char* pName, Color value);

private:
	// Types
	struct Constant {
		enum Type {
			BOOL,
			INT,
			FLOAT,
			VECTOR3,
			MATRIX33,
			XMMATRIX,
			XMFLOAT4,
			COLOR
		};

		Type		type;
		union {
			bool			bValue;
			int				nValue;
			float			fValue;
			Vector3			vValue;
			Matrix33		mValue;
			DirectX::XMMATRIX xmMatrix;
			DirectX::XMFLOAT4 xmFloat4;
			Color			cValue;
		};

		// Constructors and destructor
		Constant() : type(FLOAT), fValue(0.0f) {}
		
		~Constant() {
			// Union members don't need explicit destruction for these types
		}
		
		Constant(const Constant& other) : type(other.type), fValue(0.0f) {
			switch (type) {
			case BOOL: bValue = other.bValue; break;
			case INT: nValue = other.nValue; break;
			case FLOAT: fValue = other.fValue; break;
			case VECTOR3: vValue = other.vValue; break;
			case MATRIX33: mValue = other.mValue; break;
			case XMMATRIX: xmMatrix = other.xmMatrix; break;
			case XMFLOAT4: xmFloat4 = other.xmFloat4; break;
			case COLOR: cValue = other.cValue; break;
			}
		}
		
		Constant& operator=(const Constant& other) {
			if (this != &other) {
				type = other.type;
				switch (type) {
				case BOOL: bValue = other.bValue; break;
				case INT: nValue = other.nValue; break;
				case FLOAT: fValue = other.fValue; break;
				case VECTOR3: vValue = other.vValue; break;
				case MATRIX33: mValue = other.mValue; break;
				case XMMATRIX: xmMatrix = other.xmMatrix; break;
				case XMFLOAT4: xmFloat4 = other.xmFloat4; break;
				case COLOR: cValue = other.cValue; break;
				}
			}
			return *this;
		}
	};

	// Data
	CharString				m_sShaderFile;
	bool					m_bReleased;
	int						m_nVertexType;	// VB_VERTEX=0, VB_VERTEXL=1, VB_VERTEXTL=2

	// DirectX 11 shader objects
	ID3DBlob* m_pVSBlob;			// Vertex shader bytecode (VS_Main)
	ID3DBlob* m_pVSLBlob;			// Pre-lit vertex shader bytecode (VS_MainL); NULL if not compiled
	ID3DBlob* m_pPSBlob;			// Pixel shader bytecode
	ID3D11VertexShader* m_pVertexShader;	// VS_Main — for Vertex geometry (no COLOR)
	ID3D11VertexShader* m_pVertexShaderL;	// VS_MainL — for VertexL geometry (with COLOR)
	ID3D11PixelShader* m_pPixelShader;
	ID3D11InputLayout* m_pInputLayout;		// Input layout for Vertex / VertexTL
	ID3D11InputLayout* m_pInputLayoutL;		// Input layout for VertexL (includes COLOR at offset 24)
	ID3DBlob* m_pVSGBlob;					// VS_Galaxy bytecode; NULL if not compiled
	ID3DBlob* m_pPSGBlob;					// PS_Galaxy bytecode; NULL if not compiled
	ID3D11VertexShader* m_pVertexShaderGalaxy;	// VS_Galaxy -- fullscreen SV_VertexID triangle
	ID3D11PixelShader*  m_pPixelShaderGalaxy;	// PS_Galaxy -- per-pixel Milky Way
	ID3DBlob* m_pVSGSBlob;					// VS_GalaxySprite bytecode; NULL if not compiled
	ID3DBlob* m_pPSGSBlob;					// PS_GalaxySprite bytecode; NULL if not compiled
	ID3D11VertexShader* m_pVertexShaderGalSpr;	// VS_GalaxySprite -- billboard atlas quads
	ID3D11PixelShader*  m_pPixelShaderGalSpr;	// PS_GalaxySprite -- atlas sample + modulate
	ID3DBlob* m_pVSSTBlob;					// VS_Star bytecode; NULL if not compiled
	ID3DBlob* m_pPSSTBlob;					// PS_Star bytecode; NULL if not compiled
	ID3D11VertexShader* m_pVertexShaderStar;	// VS_Star -- procedural stellar surface billboard
	ID3D11PixelShader*  m_pPixelShaderStar;
	ID3DBlob* m_pPSSPHBlob;					// PS_StarSphere bytecode; NULL if not compiled
	ID3D11PixelShader*  m_pPixelShaderStarSphere;	// PS_StarSphere -- sphere-normal variant of PS_Star		// PS_Star -- turbulence/corona/eruption effects
	ID3DBlob* m_pPSCSBlob;					// PS_CoronaShell bytecode; NULL if not compiled
	ID3D11PixelShader*  m_pPixelShaderCoronaShell;	// PS_CoronaShell -- 3D geometry shell corona
	ID3DBlob* m_pVSSPH2Blob;				// VS_StarSphere bytecode; NULL if not compiled
	ID3D11VertexShader* m_pVertexShaderStarSphere;	// VS_StarSphere -- 3D sphere mesh vertex shader
	ID3DBlob* m_pVSTBlob;					// VS_Tess bytecode; NULL if not compiled
	ID3DBlob* m_pHSBlob;					// HS_Main bytecode; NULL if not compiled
	ID3DBlob* m_pDSBlob;					// DS_Main bytecode; NULL if not compiled
	ID3DBlob* m_pCSBlob;					// CS_Main bytecode; NULL if not compiled
	ID3D11VertexShader*  m_pVertexShaderTess;	// VS_Tess -- world-space output for tessellation
	ID3D11HullShader*    m_pHullShader;			// HS_Main -- patch constant + passthrough
	ID3D11DomainShader*  m_pDomainShader;		// DS_Main -- barycentric interp + clip-space project
	ID3D11ComputeShader* m_pComputeShader;		// CS_Main -- GPU particle update (u0 UAV)
	ID3DBlob* m_pVSVIDBlob;					// VS_Video bytecode; NULL if not compiled
	ID3DBlob* m_pPSVIDBlob;					// PS_Video bytecode; NULL if not compiled
	ID3D11VertexShader* m_pVertexShaderVideo;	// VS_Video -- fullscreen SV_VertexID triangle
	ID3D11PixelShader*  m_pPixelShaderVideo;	// PS_Video -- samples t8 (decoded video frame)
	ID3DBlob* m_pPSPPBlob;					// PS_ProceduralPlanet bytecode; NULL if not compiled
	ID3D11PixelShader*  m_pPixelShaderProcPlanet;	// PS_ProceduralPlanet -- Perlin noise terrain
	ID3DBlob* m_pPSSURFBlob;				// PS_PlanetSurface bytecode; NULL if not compiled
	ID3D11PixelShader*  m_pPixelShaderPlanetSurface;	// PS_PlanetSurface -- Option 2 FBM surface overlay
	ID3DBlob* m_pPSATMBlob;					// PS_Atmosphere bytecode; NULL if not compiled
	ID3D11PixelShader*  m_pPixelShaderAtmosphere;	// PS_Atmosphere -- planet atmosphere/cloud shell overlay
	ID3D11Buffer* m_pVSConstantBuffer;
	ID3D11Buffer* m_pPSConstantBuffer;

	// Constant tracking
	std::map<std::string, Constant>		m_Constants;

	// Helper methods
	bool					compileShader(DisplayDeviceD3D* pDevice, const char* pShaderFile);
	bool					createInputLayout(DisplayDeviceD3D* pDevice);
	bool					createConstantBuffers(DisplayDeviceD3D* pDevice);
	void					updateConstantBuffers(DisplayDeviceD3D* pDevice);
};

//---------------------------------------------------------------------------------------------------

#endif

//---------------------------------------------------------------------------------------------------
// EOF
