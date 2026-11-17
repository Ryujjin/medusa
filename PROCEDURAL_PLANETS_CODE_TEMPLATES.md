# Procedural Planets - Implementation Templates & Details

## Part 1: Sphere Generation Implementation

### Template: ProceduralPlanetD3D.h

```cpp
/*
	ProceduralPlanetD3D.h
	(c)2025 DarkSpace DX11 Port

	GPU primitive for procedurally-generated planets with Perlin noise terrain.
	Usage:
		auto planet = PrimitiveFactory<ProceduralPlanetD3D>::create();
		planet->initialize(32, 64, 1.0f);  // rings, segments, radius
		planet->setTerrainParams(0.1f, 4);  // scale, octaves
		planet->execute();
*/

#ifndef PROCEDURAL_PLANET_D3D_H
#define PROCEDURAL_PLANET_D3D_H

#include "Display/DevicePrimitive.h"
#include "Display/Types.h"
#include "Math/Vector3.h"
#include <d3d11.h>
#include <vector>

class DisplayDeviceD3D;

class PrimitiveProceduralPlanetD3D : public DevicePrimitive
{
public:
	// Construction
	PrimitiveProceduralPlanetD3D();
	virtual ~PrimitiveProceduralPlanetD3D();

	// DevicePrimitive interface
	bool        execute();
	void        clear();
	void        release();

	// Interface
	bool        initialize(int rings, int segments, float radius);
	void        setTerrainParams(float terrainScale, int octaves);
	void        setColor(const Vector3& color);

	// Accessors
	int         vertexCount() const { return m_nVerts; }
	int         triangleCount() const { return m_nIndices / 3; }

private:
	// Helpers
	bool        generateSphere();
	bool        buildVertexBuffer(ID3D11Device* pDevice);
	bool        buildIndexBuffer(ID3D11Device* pDevice);

	// Configuration
	int         m_nRings;
	int         m_nSegments;
	float       m_fRadius;
	float       m_fTerrainScale;
	int         m_nOctaves;
	Vector3     m_vColor;

	// Vertex data (CPU)
	struct Vertex {
		Vector3 position;
		Vector3 normal;
		Vector3 color;
	};

	std::vector<Vertex>     m_Verts;
	std::vector<unsigned int> m_Indices;
	int                     m_nVerts;
	int                     m_nIndices;

	// GPU buffers
	ID3D11Buffer*           m_pVB;
	ID3D11Buffer*           m_pIB;
};

//----------------------------------------------------------------------------

#endif // PROCEDURAL_PLANET_D3D_H
```

### Template: ProceduralPlanetD3D.cpp (Core Sphere Generation)

```cpp
/*
	ProceduralPlanetD3D.cpp

	Sphere generation: UV-mapped sphere with rings × segments topology.
	North pole at (0, 1, 0), equator at y=0, south pole at (0, -1, 0).
	U texture coord wraps around azimuth (0..1 = 0..2π).
	V texture coord goes from north to south pole (0..1).
*/

#include "DisplayD3D/ProceduralPlanetD3D.h"
#include "DisplayD3D/DisplayDeviceD3D.h"
#include "DisplayD3D/PrimitiveFactory.h"
#include "Debug/Trace.h"
#include <cmath>

IMPLEMENT_PRIMITIVE_FACTORY(PrimitiveProceduralPlanetD3D);

static const float PI_F = 3.14159265358979f;

PrimitiveProceduralPlanetD3D::PrimitiveProceduralPlanetD3D()
	: m_nRings(32), m_nSegments(64), m_fRadius(1.0f)
	, m_fTerrainScale(0.1f), m_nOctaves(4)
	, m_vColor(1.0f, 1.0f, 1.0f)
	, m_nVerts(0), m_nIndices(0)
	, m_pVB(NULL), m_pIB(NULL)
{
}

PrimitiveProceduralPlanetD3D::~PrimitiveProceduralPlanetD3D()
{
	release();
}

bool PrimitiveProceduralPlanetD3D::initialize(int rings, int segments, float radius)
{
	if (rings < 3 || segments < 3 || radius <= 0.0f)
		return false;

	m_nRings = rings;
	m_nSegments = segments;
	m_fRadius = radius;

	return generateSphere();
}

void PrimitiveProceduralPlanetD3D::setTerrainParams(float terrainScale, int octaves)
{
	m_fTerrainScale = terrainScale;
	m_nOctaves = (octaves > 0) ? octaves : 1;
}

void PrimitiveProceduralPlanetD3D::setColor(const Vector3& color)
{
	m_vColor = color;
}

bool PrimitiveProceduralPlanetD3D::generateSphere()
{
	m_Verts.clear();
	m_Indices.clear();

	// Vertex generation: (rings + 1) × (segments + 1) vertices
	// Ring 0 = north pole, Ring rings = south pole
	for (int ring = 0; ring <= m_nRings; ++ring) {
		float v = (float)ring / (float)m_nRings;  // 0 at north, 1 at south
		float phi = v * PI_F;                      // 0 (top) to π (bottom)

		float sinPhi = sinf(phi);
		float cosPhi = cosf(phi);

		for (int seg = 0; seg <= m_nSegments; ++seg) {
			float u = (float)seg / (float)m_nSegments;  // 0 to 1 around azimuth
			float theta = u * 2.0f * PI_F;              // 0 to 2π

			float sinTheta = sinf(theta);
			float cosTheta = cosf(theta);

			// Unit sphere point (normal direction)
			Vector3 normal(
				sinPhi * cosTheta,
				cosPhi,
				sinPhi * sinTheta
			);

			// Position = normal × radius
			Vector3 position = normal * m_fRadius;

			Vertex vert;
			vert.position = position;
			vert.normal = normal;      // Already unit length
			vert.color = m_vColor;

			m_Verts.push_back(vert);
		}
	}

	m_nVerts = (int)m_Verts.size();

	// Index generation: quads become triangles
	for (int ring = 0; ring < m_nRings; ++ring) {
		for (int seg = 0; seg < m_nSegments; ++seg) {
			// Quad vertices (CCW from above)
			int a = ring * (m_nSegments + 1) + seg;
			int b = a + 1;
			int c = (ring + 1) * (m_nSegments + 1) + seg;
			int d = c + 1;

			// First triangle (a, b, c)
			m_Indices.push_back(a);
			m_Indices.push_back(b);
			m_Indices.push_back(c);

			// Second triangle (b, d, c)
			m_Indices.push_back(b);
			m_Indices.push_back(d);
			m_Indices.push_back(c);
		}
	}

	m_nIndices = (int)m_Indices.size();

	TRACE(CharString().format("ProceduralPlanetD3D: Generated sphere with %d vertices, %d triangles",
		m_nVerts, m_nIndices / 3));

	return true;
}

bool PrimitiveProceduralPlanetD3D::buildVertexBuffer(ID3D11Device* pDevice)
{
	if (!pDevice || m_Verts.empty())
		return false;

	D3D11_BUFFER_DESC bd = {};
	bd.ByteWidth = (UINT)(m_Verts.size() * sizeof(Vertex));
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA init = {};
	init.pSysMem = m_Verts.data();

	HRESULT hr = pDevice->CreateBuffer(&bd, &init, &m_pVB);
	if (FAILED(hr)) {
		TRACE("ProceduralPlanetD3D: CreateBuffer (VB) failed");
		return false;
	}
	return true;
}

bool PrimitiveProceduralPlanetD3D::buildIndexBuffer(ID3D11Device* pDevice)
{
	if (!pDevice || m_Indices.empty())
		return false;

	D3D11_BUFFER_DESC bd = {};
	bd.ByteWidth = (UINT)(m_Indices.size() * sizeof(unsigned int));
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA init = {};
	init.pSysMem = m_Indices.data();

	HRESULT hr = pDevice->CreateBuffer(&bd, &init, &m_pIB);
	if (FAILED(hr)) {
		TRACE("ProceduralPlanetD3D: CreateBuffer (IB) failed");
		return false;
	}
	return true;
}

bool PrimitiveProceduralPlanetD3D::execute()
{
	DisplayDeviceD3D* pDisplayD3D = (DisplayDeviceD3D*)m_pDevice;
	if (!pDisplayD3D || !pDisplayD3D->getContext())
		return false;

	ID3D11Device* pDevice = pDisplayD3D->getDevice();
	ID3D11DeviceContext* pContext = pDisplayD3D->getContext();
	if (!pDevice || !pContext)
		return false;

	// Lazy-build buffers on first execute
	if (!m_pVB && !buildVertexBuffer(pDevice))
		return false;
	if (!m_pIB && !buildIndexBuffer(pDevice))
		return false;

	// Set shader constants
	if (pDisplayD3D->m_pMatShader.valid()) {
		DirectX::XMFLOAT4 color(m_vColor.x, m_vColor.y, m_vColor.z, 1.0f);
		pDisplayD3D->m_pMatShader->setConstant("vPlanetColor", color);
		pDisplayD3D->m_pMatShader->setConstant("fTerrainScale", m_fTerrainScale);
		pDisplayD3D->m_pMatShader->setConstant("nOctaves", m_nOctaves);

		// Apply shader
		if (!pDisplayD3D->m_pMatShader->applyProceduralPlanet(pDisplayD3D))
			return false;
	}

	// Bind and draw
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	pContext->IASetVertexBuffers(0, 1, &m_pVB, &stride, &offset);
	pContext->IASetIndexBuffer(m_pIB, DXGI_FORMAT_R32_UINT, 0);
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pContext->DrawIndexed((UINT)m_nIndices, 0, 0);

	return true;
}

void PrimitiveProceduralPlanetD3D::clear()
{
}

void PrimitiveProceduralPlanetD3D::release()
{
	if (m_pVB) { m_pVB->Release(); m_pVB = NULL; }
	if (m_pIB) { m_pIB->Release(); m_pIB = NULL; }
	m_Verts.clear();
	m_Indices.clear();
	m_nVerts = 0;
	m_nIndices = 0;
}
```

---

## Part 2: Noise Function Implementations for HLSL

### Shader Entry Points (Default.hlsl additions)

```hlsl
//=============================================================================
// Noise Functions
//=============================================================================

// Pseudo-random hash function (3D -> 1D)
float hash3(float3 p) {
	p = frac(p * 0.3183099 + 0.1) * 17.0;
	return frac(p.x * p.y * p.z * (p.x + p.y + p.z));
}

// Hash function with multiple outputs
float3 hash3x3(float3 p) {
	float3 q = float3(
		dot(p, float3(127.1, 311.7, 74.7)),
		dot(p, float3(269.5, 183.3, 246.1)),
		dot(p, float3(113.5, 271.9, 124.6))
	);
	return frac(sin(q) * 43758.5453);
}

//-----------------------------------------------------------------------------
// Perlin Noise (Value Noise)
//
// Classic interpolation-based noise. Smooth gradients, good for terrain.
// Cost: ~40 operations per octave
//-----------------------------------------------------------------------------

float smoothstep3(float t) {
	// Hermite curve (3t² - 2t³) gives smooth falloff at edges
	return t * t * (3.0 - 2.0 * t);
}

float perlinNoise(float3 p) {
	float3 pi = floor(p);
	float3 pf = frac(p);

	// Smoothed interpolation weights
	float3 w = pf * pf * (3.0 - 2.0 * pf);  // smoothstep

	// Sample hash values at 8 cube corners
	float h000 = hash3(pi + float3(0, 0, 0));
	float h100 = hash3(pi + float3(1, 0, 0));
	float h010 = hash3(pi + float3(0, 1, 0));
	float h110 = hash3(pi + float3(1, 1, 0));
	float h001 = hash3(pi + float3(0, 0, 1));
	float h101 = hash3(pi + float3(1, 0, 1));
	float h011 = hash3(pi + float3(0, 1, 1));
	float h111 = hash3(pi + float3(1, 1, 1));

	// Trilinear interpolation
	float h00 = lerp(h000, h100, w.x);
	float h10 = lerp(h010, h110, w.x);
	float h01 = lerp(h001, h101, w.x);
	float h11 = lerp(h011, h111, w.x);

	float h0 = lerp(h00, h10, w.y);
	float h1 = lerp(h01, h11, w.y);

	return lerp(h0, h1, w.z);
}

//-----------------------------------------------------------------------------
// Worley Noise (Cellular / Voronoi)
//
// Distance to nearest point in a cellular grid. Good for crackles, patterns.
// Cost: ~80 operations (checks 27 neighbors)
//-----------------------------------------------------------------------------

float worleyNoise(float3 p) {
	float3 pi = floor(p);
	float3 pf = frac(p);

	float minDist = 1.0;

	// Check 3×3×3 neighborhood
	for (int z = -1; z <= 1; z++) {
		for (int y = -1; y <= 1; y++) {
			for (int x = -1; x <= 1; x++) {
				float3 cell = pi + float3(x, y, z);
				float3 cellPoint = cell + hash3x3(cell);
				float dist = distance(pf, cellPoint - float3(x, y, z));
				minDist = min(minDist, dist);
			}
		}
	}

	return minDist;
}

//-----------------------------------------------------------------------------
// Fractional Brownian Motion (FBM)
//
// Combines multiple noise octaves for natural-looking turbulence.
// Parameters:
//   p: input coordinate
//   octaves: number of noise layers (2-8 typical)
//   persistence: amplitude reduction per octave (0.5 default)
//   lacunarity: frequency increase per octave (2.0 default)
//
// Result: ranges approximately [-1, 1]
//
// Cost: O(octaves) × O(perlinNoise) = ~40-80 ops per octave
//-----------------------------------------------------------------------------

float fbm(float3 p, int octaves, float persistence, float lacunarity) {
	float value = 0.0;
	float amplitude = 1.0;
	float frequency = 1.0;
	float maxValue = 0.0;

	for (int i = 0; i < octaves; i++) {
		value += amplitude * perlinNoise(p * frequency);
		maxValue += amplitude;

		amplitude *= persistence;
		frequency *= lacunarity;
	}

	return value / maxValue;
}

// Overload for typical usage (persistence=0.5, lacunarity=2.0)
float fbm(float3 p, int octaves) {
	return fbm(p, octaves, 0.5, 2.0);
}

//=============================================================================
// Procedural Planet Shaders
//=============================================================================

// Constant buffer additions (in cbPerFrame or new cbPerMaterial)
// These would typically be set from C++:
//   fTerrainScale: 0.1-1.0 (controls noise frequency)
//   nOctaves: 2-8 (more = more detail, more expensive)
//   vPlanetColor: base color before height mapping

float4 PS_ProceduralPlanet(PS_INPUT input) : SV_Target
{
	// Normalize the sphere normal
	float3 normal = normalize(input.vNormal);

	// Generate 3D noise coordinate from position
	// Scale by terrain frequency
	float3 noiseCoord = normal * (1.0 / fTerrainScale);

	// Sample noise (-1 to 1, approximately)
	float heightNoise = fbm(noiseCoord, nOctaves);

	// Remap to [0, 1]
	float height = (heightNoise + 1.0) * 0.5;

	// Color based on height (simple gradient)
	float3 color;
	if (height < 0.4) {
		// Ocean
		color = lerp(float3(0.0, 0.1, 0.3), float3(0.1, 0.3, 0.5), height / 0.4);
	}
	else if (height < 0.45) {
		// Beach
		color = float3(0.8, 0.7, 0.5);
	}
	else if (height < 0.6) {
		// Forest
		color = lerp(float3(0.0, 0.4, 0.0), float3(0.2, 0.6, 0.2), (height - 0.45) / 0.15);
	}
	else if (height < 0.8) {
		// Mountain
		color = lerp(float3(0.4, 0.3, 0.2), float3(0.6, 0.6, 0.6), (height - 0.6) / 0.2);
	}
	else {
		// Snow peak
		color = float3(0.95, 0.95, 1.0);
	}

	// Simple lighting: Phong with ambient + directional
	float3 lightDir = normalize(float3(1, 1, 1));
	float diffuse = max(0.0, dot(normal, lightDir));
	float3 lighting = vAmbientLight + diffuse * float3(1, 1, 1);

	// Apply base color modulation
	color *= (vDiffuseColor.rgb * vPlanetColor);
	color *= lighting;

	return float4(color, 1.0);
}

// Advanced variant with normal perturbation (for more surface detail)
float4 PS_ProceduralPlanetAdvanced(PS_INPUT input) : SV_Target
{
	float3 normal = normalize(input.vNormal);

	// Sample noise at slightly offset coordinates for normal perturbation
	float epsilon = 0.01;
	float h = fbm(normal * (1.0 / fTerrainScale), nOctaves);
	float hx = fbm((normal + float3(epsilon, 0, 0)) * (1.0 / fTerrainScale), nOctaves);
	float hy = fbm((normal + float3(0, epsilon, 0)) * (1.0 / fTerrainScale), nOctaves);

	// Compute perturbed normal via finite differences
	float3 dx = float3(epsilon, hx - h, 0);
	float3 dy = float3(0, hy - h, epsilon);
	float3 perturbedNormal = normalize(cross(dy, dx));

	// Blend with original normal (usually 70-90% perturbation)
	normal = normalize(lerp(normal, perturbedNormal, 0.8));

	// Rest is same as basic variant...
	float height = (h + 1.0) * 0.5;

	float3 color;
	if (height < 0.4) color = float3(0.0, 0.2, 0.5);
	else if (height < 0.5) color = float3(0.8, 0.7, 0.5);
	else if (height < 0.7) color = float3(0.2, 0.6, 0.2);
	else color = float3(0.6, 0.6, 0.6);

	float3 lightDir = normalize(float3(1, 1, 1));
	float diffuse = max(0.0, dot(normal, lightDir));
	float3 lighting = vAmbientLight + diffuse * float3(1, 1, 1);

	color *= (vDiffuseColor.rgb * vPlanetColor);
	color *= lighting;

	return float4(color, 1.0);
}
```

---

## Part 3: Scene Node Wrapper

### NodeProceduralPlanet.h

```cpp
/*
	NodeProceduralPlanet.h

	Scene graph node for rendering a procedural planet.
	Usage:
		auto planet = new NodeProceduralPlanet();
		planet->setProperties(32, 64, 1.0f);
		planet->setTerrainParams(0.1f, 4);
		scene->add(planet);
*/

#ifndef NODE_PROCEDURAL_PLANET_H
#define NODE_PROCEDURAL_PLANET_H

#include "Render3D/NodeTransform.h"
#include "Render3D/Render3dDll.h"
#include "Math/Vector3.h"
#include "Standard/Reference.h"

class DisplayDeviceD3D;
class PrimitiveProceduralPlanetD3D;

class DLL NodeProceduralPlanet : public NodeTransform
{
public:
	DECLARE_WIDGET_CLASS();
	DECLARE_PROPERTY_LIST();

	// Types
	typedef Reference<NodeProceduralPlanet> Ref;

	// Construction
	NodeProceduralPlanet();
	virtual ~NodeProceduralPlanet();

	// NodeTransform interface
	void        render(RenderContext& context, const Matrix33& frame, const Vector3& position);
	void        preRender(RenderContext& context, const Matrix33& frame, const Vector3& position);
	BoxHull     hull() const;

	// Configuration
	void        setResolution(int rings, int segments);  // Grid density
	void        setRadius(float radius);
	void        setTerrainParams(float scale, int octaves);
	void        setPlanetColor(const Vector3& color);

	// Accessors
	int         rings() const { return m_nRings; }
	int         segments() const { return m_nSegments; }
	float       radius() const { return m_fRadius; }
	float       terrainScale() const { return m_fTerrainScale; }
	int         octaves() const { return m_nOctaves; }

private:
	// Configuration (serialized)
	int         m_nRings;
	int         m_nSegments;
	float       m_fRadius;
	float       m_fTerrainScale;
	int         m_nOctaves;
	Vector3     m_vColor;

	// Runtime
	Reference<PrimitiveProceduralPlanetD3D> m_pPrimitive;
};

#endif // NODE_PROCEDURAL_PLANET_H
```

### NodeProceduralPlanet.cpp

```cpp
#define RENDER3D_DLL

#include "Render3D/NodeProceduralPlanet.h"
#include "DisplayD3D/ProceduralPlanetD3D.h"
#include "DisplayD3D/PrimitiveFactory.h"
#include "DisplayD3D/DisplayDeviceD3D.h"
#include "Render3D/RenderContext.h"
#include "Math/BoxHull.h"
#include "Debug/Trace.h"

IMPLEMENT_PROPERTY_LIST(NodeProceduralPlanet);
IMPLEMENT_PROPERTY(NodeProceduralPlanet, m_nRings, int);
IMPLEMENT_PROPERTY(NodeProceduralPlanet, m_nSegments, int);
IMPLEMENT_PROPERTY(NodeProceduralPlanet, m_fRadius, float);
IMPLEMENT_PROPERTY(NodeProceduralPlanet, m_fTerrainScale, float);
IMPLEMENT_PROPERTY(NodeProceduralPlanet, m_nOctaves, int);
// Color would be Vector3 property...

NodeProceduralPlanet::NodeProceduralPlanet()
	: m_nRings(32), m_nSegments(64), m_fRadius(1.0f)
	, m_fTerrainScale(0.1f), m_nOctaves(4)
	, m_vColor(1.0f, 1.0f, 1.0f)
{
}

NodeProceduralPlanet::~NodeProceduralPlanet()
{
}

void NodeProceduralPlanet::setResolution(int rings, int segments)
{
	m_nRings = rings;
	m_nSegments = segments;
	m_pPrimitive = NULL;  // Force regeneration
}

void NodeProceduralPlanet::setRadius(float radius)
{
	m_fRadius = radius;
}

void NodeProceduralPlanet::setTerrainParams(float scale, int octaves)
{
	m_fTerrainScale = scale;
	m_nOctaves = octaves;
}

void NodeProceduralPlanet::setPlanetColor(const Vector3& color)
{
	m_vColor = color;
}

void NodeProceduralPlanet::preRender(RenderContext& context, const Matrix33& frame, const Vector3& position)
{
	// Lazy-allocate primitive on first preRender
	if (!m_pPrimitive.valid()) {
		m_pPrimitive = PrimitiveFactory<PrimitiveProceduralPlanetD3D>::create();
		if (m_pPrimitive.valid()) {
			m_pPrimitive->initialize(m_nRings, m_nSegments, m_fRadius);
			m_pPrimitive->setTerrainParams(m_fTerrainScale, m_nOctaves);
			m_pPrimitive->setColor(m_vColor);

			DisplayDevice* pDisplay = context.display();
			if (pDisplay) {
				m_pPrimitive->setDevice(pDisplay);
			}
		}
	}
}

void NodeProceduralPlanet::render(RenderContext& context, const Matrix33& frame, const Vector3& position)
{
	DisplayDeviceD3D* pDisplay = (DisplayDeviceD3D*)context.display();
	if (!pDisplay)
		return;

	// Ensure primitive is ready
	preRender(context, frame, position);
	if (!m_pPrimitive.valid())
		return;

	// Push transform
	pDisplay->pushMatrix();
	pDisplay->setMatrix(frame, position);

	// Render
	m_pPrimitive->execute();

	// Pop transform
	pDisplay->popMatrix();
}

BoxHull NodeProceduralPlanet::hull() const
{
	Vector3 extent(m_fRadius, m_fRadius, m_fRadius);
	return BoxHull(Vector3(0, 0, 0), extent);
}
```

---

## Part 4: Registration & Integration

### Changes to ShaderD3D11.h

Add in the Shader class declaration:

```cpp
// Apply procedural planet shader (PS_ProceduralPlanet)
bool applyProceduralPlanet(DisplayDeviceD3D* pDevice);
```

And add member variables:

```cpp
ID3DBlob* m_pPSProcBlob;                    // PS_ProceduralPlanet bytecode
ID3D11PixelShader* m_pPixelShaderProceduralPlanet;
```

### Changes to PrimitiveFactory.cpp

After the includes:

```cpp
#include "DisplayD3D/ProceduralPlanetD3D.h"

// ... other factory initializations ...

// In initializeFactories():
new PrimitiveFactory<PrimitiveProceduralPlanetD3D>();
```

---

## Performance Notes

### Optimization 1: Vertex Data Layout
Consider packing normals as oct-encoded 16-bit values to reduce vertex size:
```cpp
struct Vertex {
	Vector3 position;      // 12 bytes
	uint32_t normal_oct;   // 4 bytes (octahedral encoding)
	Vector3 color;         // 12 bytes
	// Total: 28 bytes instead of 36 bytes
};
```

### Optimization 2: LOD Variants
Pre-generate multiple sphere resolutions:
```cpp
// In NodeProceduralPlanet
enum LOD {
	LOD_NEAR = 0,    // 64 × 128 (high detail, near camera)
	LOD_MID = 1,     // 32 × 64
	LOD_FAR = 2,     // 16 × 32
	LOD_DISTANT = 3  // 8 × 16 (far planets, billboards)
};
```

### Optimization 3: Texture-Based Noise
For very high quality, precompute 3D Perlin noise to texture:
```hlsl
Texture3D<float> txNoiseVolume : register(t5);
float height = txNoiseVolume.Sample(samLinear, normal * (1.0 / fTerrainScale)).r;
```

---

## Testing Checklist

- [ ] Sphere generates correct vertex/triangle counts
- [ ] Normals point outward (check with wireframe)
- [ ] Noise functions compile without errors
- [ ] Noise values produce visible height variation
- [ ] Planet rotates correctly with camera
- [ ] Lighting updates as camera moves
- [ ] Multiple planets render without artifacts
- [ ] Performance: single planet <1ms at 1080p
- [ ] FBM with 4-8 octaves produces natural look
- [ ] Tessellation (if enabled) adapts with LOD
