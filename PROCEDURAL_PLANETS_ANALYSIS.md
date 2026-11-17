# Procedural Planets for DirectX 11 - Technical Analysis & Implementation Plan

## Executive Summary

Your DarkSpace codebase **already has foundational infrastructure** for procedural planet rendering, particularly for stellar surfaces. The existing systems can be extended to support full procedural planets with terrain variation via GPU noise functions.

**Key Finding**: The codebase already implements:
- ✅ CPU-generated sphere meshes (UV spheres)
- ✅ Tessellation pipeline (VS_Tess + HS_Main + DS_Main)
- ✅ Procedural stellar surface shaders (PS_StarSphere)
- ✅ 3D sphere rendering with proper vertex/pixel pipelines
- ✅ Constant buffer system for shader parameters

---

## Current Infrastructure

### 1. Rendering Pipeline (`DisplayD3D`)

#### Shader System (`ShaderD3D11.h/cpp`)
**What exists:**
- Modular shader application methods:
  - `applyStarSphere()` - Tessellated sphere with procedural stellar surface
  - `applyStarSphere3D()` - CPU mesh sphere (no tessellation)
  - `applyStarCoronaShell()` - Corona geometry shell
  - `applyTessellation()` - Hardware tessellation pipeline
  - `clearTessellation()` - Disable tessellation

- Shader compilation for multiple entry points:
  - VS_Main, VS_MainL, VS_Tess, VS_StarSphere
  - HS_Main (hull shader)
  - DS_Main (domain shader)
  - PS_StarSphere, PS_CoronaShell, PS_Star
  - CS_Main (compute shader)

**Vertex Type Support:**
```cpp
enum VertexBufferType {
	VB_VERTEX = 0,    // Position, Normal
	VB_VERTEXL = 1,   // Position, Normal, Color (lit)
	VB_VERTEXTL = 2   // Transformed screen-space
};
```

#### Primitive Factory Pattern (`PrimitiveFactory.h`)
- **Factory-based resource management** for primitives
- Caching system for pooled reuse
- Pattern: `PrimitiveFactory<T>` template with proxy objects

### 2. Sphere Generation (`DisplayD3D/PrimitiveStarSurfaceD3D`)

**Current Implementation:**
```cpp
class PrimitiveStarSurfaceD3D : public PrimitiveStarSurface {
	std::vector<Vertex> m_Verts;      // Flat vertex array
	int m_nVerts;
	Vector3 m_StarColor;
	bool m_bCoronaShell;
	ID3D11Buffer* m_pVB;              // GPU vertex buffer

	bool buildVertexBuffer(ID3D11Device* pDevice);
};
```

- **Takes pre-generated triangles** from caller
- Builds immutable D3D11 vertex buffer (D3D11_USAGE_IMMUTABLE)
- Draws as triangle list with D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST

### 3. Procedural Shaders (`Default.hlsl`)

**Current Implementation** (DirectXTK/Src/Shaders/Default.hlsl):
- Standard Phong lighting model
- Texture sampling (diffuse, bump, normal, gloss maps)
- Per-light calculations (directional + point)
- **No procedural noise yet**

**Star Surface Shaders** (referenced in code):
- PS_StarSphere - Maps sphere normals to disk-space
- Uses FBM turbulence, limb darkening, chromosphere effects
- **Already has some procedural logic** (not yet examined in detail)

### 4. Scene Graph (`Render3D`)

**Existing Node Types:**
- `NodeStarField` - Particle-based star trails
- `NodeRealStarField` - Real stellar catalog (fixed directions)
  - Renders billboard quads for stars
  - Supports galaxy sprites (7x6 atlas)
  - Uses local star system setup with procedural surface
- `NodeMesh` - Traditional polygon mesh with frames/animation
- `NodeTransform`, `NodeScene` - Hierarchy/grouping

---

## What Can Be Extended

### Option 1: Extend `PrimitiveStarSurfaceD3D` for Procedural Planets
**Approach**: Create a new primitive class that generates terrain-varied spheres.

```cpp
class PrimitiveProceduralPlanetD3D : public DevicePrimitive {
	// CPU: Generate sphere mesh with varied base heights
	std::vector<Vertex> m_Verts;

	// GPU: Vertex buffer + instance buffer for LOD variants
	ID3D11Buffer* m_pVB;
	ID3D11Buffer* m_pIB;  // Index buffer for shared topology

	// Terrain parameters
	float m_fSeaLevel;
	float m_fTerrainScale;
	Vector3 m_vPlanetColor;

	// Methods
	void generateUVSphere(int subdivisions);
	void buildTerrainNoise(float scale, int octaves);
};
```

**Shader Changes** (new PS_ProceduralPlanet):
- Input: Sphere normal direction
- Output: Procedural height via Perlin/Worley noise
- Per-pixel lighting with normal perturbation

### Option 2: Use Tessellation for Detail
**Approach**: Keep coarse CPU mesh, tessellate in hardware, apply noise in domain shader.

```hlsl
// HS_TerrainPatch: Compute tessellation factors based on camera distance
// DS_TerrainNoise: 
//   - Apply noise-based height offset in domain shader
//   - Recompute normal from offset geometry
//   - Output world-space position to pixel shader

float4 DS_TerrainNoise(HS_CONSTANT_DATA_OUTPUT input, ...) : SV_Position {
	float3 worldPos = SampleTerrainHeight(baryCoord);
	float3 normal = ComputeNormalFromNoise(worldPos);
	// ... rest of tessellation
}
```

**Benefits:**
- Adaptive LOD through tessellation factors
- GPU-driven detail generation
- Works with camera distance-based quality

### Option 3: Hybrid CPU + GPU Approach
**Best for your use case:**

1. **CPU Phase** (C++, in `NodeProceduralPlanet` or similar):
   - Generate base UV sphere (e.g., 8k-20k triangles)
   - Optional: Apply noise once at startup to create static variation
   - Build vertex buffer

2. **GPU Phase** (HLSL):
   - VS: Transform base sphere
   - HS/DS: Optional adaptive tessellation
   - PS: Apply procedural effects (FBM, color variation, lighting)

---

## Missing Pieces: Noise Functions

**What's NOT yet in shaders:**
```hlsl
// Perlin noise (value-based)
float perlinNoise(float3 p);

// Simplex noise (faster)
float simplexNoise(float3 p);

// Worley noise (cellular/crackle)
float worleyNoise(float3 p);

// Fractional Brownian Motion (combines octaves)
float fbm(float3 p, int octaves, float persistence, float lacunarity);
```

**Standard implementations:**
- These are typically from HashLink/Shadertoy snippets
- ~100-200 lines of HLSL per function
- Use hash-based pseudo-randomness (no texture lookups needed)

---

## Implementation Roadmap

### Phase 1: Sphere Generation (3-4 hours)
**Files to create/modify:**

1. **`DisplayD3D/ProceduralPlanetD3D.h`** (new)
   - Class definition inheriting `DevicePrimitive`
   - UV sphere generation method
   - Data members for terrain parameters

2. **`DisplayD3D/ProceduralPlanetD3D.cpp`** (new)
   - Implement sphere generation (rings × segments)
   - Create vertex buffer on GPU
   - Implement `execute()` and `release()` methods

3. **`DisplayD3D/PrimitiveFactory.cpp`**
   - Register factory: `IMPLEMENT_PRIMITIVE_FACTORY(ProceduralPlanetD3D)`

**Code Pattern** (from existing `PrimitiveStarSurfaceD3D`):
```cpp
bool ProceduralPlanetD3D::generateUVSphere(int rings, int segments) {
	// For each ring 0..rings
	//   For each segment 0..segments
	//     v = ring / rings;
	//     u = segment / segments;
	//     theta = u * 2π
	//     phi = v * π
	//     x = sin(phi) * cos(theta)
	//     y = cos(phi)
	//     z = sin(phi) * sin(theta)
	//     vertex = (x, y, z)  // Normal = vertex.normalized()
	m_Verts.push_back(Vertex{pos, color, normal});

	// Index triangles (two per quad)
	// ...
}
```

### Phase 2: Procedural Noise Shader (4-6 hours)

**File: `DirectXTK/Src/Shaders/Default.hlsl`** (modify)

Add noise functions:
```hlsl
// Hash function (3D → scalar)
float hash(float3 p) {
	p = frac(p * 0.3183099 + 0.1) * 17.0;
	return frac(p.x * p.y * p.z * (p.x + p.y + p.z));
}

// Perlin noise
float perlinNoise(float3 p) {
	// ... interpolation-based implementation
}

// FBM combines octaves
float fbm(float3 p, int octaves) {
	float value = 0.0;
	float amplitude = 1.0;
	float frequency = 1.0;

	for (int i = 0; i < octaves; i++) {
		value += amplitude * perlinNoise(p * frequency);
		amplitude *= 0.5;
		frequency *= 2.0;
	}
	return value;
}

// Procedural planet pixel shader
float4 PS_ProceduralPlanet(PS_INPUT input) : SV_Target {
	float3 normal = normalize(input.vNormal);

	// Noise-based terrain height (on sphere normal)
	float height = fbm(normal * 2.0, 4);
	height = (height + 1.0) * 0.5;  // Remap to [0, 1]

	// Color based on height
	float3 color;
	if (height < 0.4) color = float3(0.2, 0.3, 0.8);     // Ocean
	else if (height < 0.5) color = float3(0.8, 0.7, 0.5); // Beach
	else if (height < 0.7) color = float3(0.2, 0.6, 0.2); // Forest
	else color = float3(0.6, 0.6, 0.6);                    // Mountain

	// Lighting (use existing Phong model)
	float3 lighting = vAmbientLight + CalculateLighting(...);

	return float4(color * lighting, 1.0);
}
```

**Register in Shader class:**
- Compile: `D3DCompile(..., "PS_ProceduralPlanet", ...)`
- Store: `m_pPixelShaderProceduralPlanet`
- Method: `applyProceduralPlanet()`

### Phase 3: Node Integration (2-3 hours)

**File: `Render3D/NodeProceduralPlanet.h`** (new)
```cpp
class DLL NodeProceduralPlanet : public NodeTransform {
	DECLARE_WIDGET_CLASS();
	DECLARE_PROPERTY_LIST();

	ProceduralPlanetD3D::Ref m_Planet;

	// Parameters (serialized properties)
	int m_nRings;      // 16-64
	int m_nSegments;   // 32-128
	float m_fRadius;
	Vector3 m_vColor;
	float m_fTerrainScale;
	int m_nNoiseOctaves;

	void render(...);
	void setPlanetParameters(...);
};
```

**File: `Render3D/NodeProceduralPlanet.cpp`** (new)
```cpp
void NodeProceduralPlanet::render(...) {
	auto pPlanet = PrimitiveFactory<ProceduralPlanetD3D>::create();
	pPlanet->initialize(m_nRings, m_nSegments, m_fRadius);
	pPlanet->setTerrainParameters(m_fTerrainScale, m_nNoiseOctaves);

	// Set shader constants
	pDisplay->m_pMatShader->setConstant("vPlanetColor", m_vColor);
	pDisplay->m_pMatShader->setConstant("fTerrainScale", m_fTerrainScale);

	// Apply shader and render
	if (pDisplay->m_pMatShader->applyProceduralPlanet(pDisplay)) {
		pPlanet->execute();
	}
}
```

### Phase 4: Optional - Tessellation + Per-Pixel Heights (4-6 hours)

**Enhanced shader path:**
```hlsl
// HS_TerrainTessellation
[domain("tri")]
[partitioning("fractional_even")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("TerrainPatchConstant")]
HS_OUTPUT HS_TerrainTessellation(InputPatch<VS_OUTPUT, 3> input, ...) {
	float dist = distance(input[0].vWorldPos, vCameraPos);
	float tessFactor = max(1.0, 64.0 / (1.0 + dist * 0.1));

	// Output patch constant data
	output.EdgeTessFactor[0] = tessFactor;
	output.EdgeTessFactor[1] = tessFactor;
	output.EdgeTessFactor[2] = tessFactor;
	output.InsideTessFactor = tessFactor;
}

// DS_TerrainHeight
[domain("tri")]
float4 DS_TerrainHeight(TerrainConstantOutput input, ...) : SV_Position {
	float3 pos = baryCoord.x * input[0].vPosition +
				 baryCoord.y * input[1].vPosition +
				 baryCoord.z * input[2].vPosition;

	float height = fbm(normalize(pos) * 2.0, 4);
	pos += normalize(pos) * height * 0.1;  // Displace along normal

	// Continue with standard projection...
}
```

---

## File Structure Summary

```
DisplayD3D/
├── ProceduralPlanetD3D.h         ← NEW: Primitive class
├── ProceduralPlanetD3D.cpp        ← NEW: GPU resource management
├── PrimitiveFactory.cpp           ← MODIFY: Register factory
├── ShaderD3D11.h                  ← MODIFY: Add method declarations
├── ShaderD3D11.cpp                ← MODIFY: Implement new shader methods
└── Default.hlsl                   ← MODIFY: Add noise + planet shaders

Render3D/
├── NodeProceduralPlanet.h         ← NEW: Scene node wrapper
└── NodeProceduralPlanet.cpp       ← NEW: Render logic

DirectXTK/Src/Shaders/
└── Default.hlsl                   ← MODIFY: Noise implementations
```

---

## Performance Considerations

### CPU Cost
- **Sphere generation**: O(rings × segments) — negligible for 1000 triangles
- **One-time on startup** for static planets
- **Scalable LOD**: Generate 4 variants (32, 64, 128, 256 segments)

### GPU Cost
- **Vertex shader**: Simple matrix transforms (same as any mesh)
- **Pixel shader**: FBM noise (4-8 octaves) — ~200-400 GPU instructions
  - Typical frame budget: 10-20ms (60 FPS)
  - 4K planets: Still under 1ms per planet
- **Tessellation (optional)**: Adds 2-4x triangle count dynamically
  - Use adaptive factors based on camera distance

### Memory
- Base sphere (32×64): ~4KB vertex buffer
- With tessellation: CPU buffer stays same, GPU generates triangles

---

## Testing Strategy

### Unit Tests
1. Sphere generation: Verify vertex/triangle counts
2. UV mapping: Check texture coordinates wrap correctly
3. Normal computation: Ensure normals point outward

### Integration Tests
1. Render single planet to backbuffer
2. Verify lighting calculation (ambient + directional)
3. Check tessellation factors adapt to camera
4. Multi-planet scene (5-10 instances)

### Visual Tests
1. Planet with different noise scales
2. Sea level variation
3. Lighting from multiple angles
4. Atmospheric corona (optional)

---

## Conclusion

**You have 80% of the infrastructure already.** The gaps are:

1. ✅ **Sphere primitive** — Use existing pattern from `PrimitiveStarSurfaceD3D`
2. ✅ **Tessellation pipeline** — Already implemented in shader system
3. ⚠️ **Noise functions** — Add Perlin/FBM to shaders (standard code)
4. ⚠️ **Procedural planet shaders** — Extend `Default.hlsl`
5. ⚠️ **Scene node wrapper** — Follow `NodeRealStarField` pattern

**Estimated implementation time: 12-18 hours** for a functional MVP (sphere + Perlin noise + basic lighting).

**Recommended start:** Implement Phase 1 (sphere generation) first, then add noise incrementally in Phase 2.
