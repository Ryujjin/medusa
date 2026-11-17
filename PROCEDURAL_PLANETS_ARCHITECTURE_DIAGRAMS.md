# Procedural Planets - Visual Architecture Diagrams

## System Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          APPLICATION LAYER                              │
│                                                                          │
│  GameContext / Scene Manager                                           │
│    ├─ NodeProceduralPlanet* pPlanet = new NodeProceduralPlanet()       │
│    └─ pPlanet->setResolution(32, 64)                                   │
│        pPlanet->setTerrainParams(0.1f, 4)                              │
└────────────────────────────┬────────────────────────────────────────────┘
							 │
							 ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                     RENDER3D LAYER (Scene Graph)                        │
│                                                                          │
│  NodeProceduralPlanet : NodeTransform                                  │
│    ├─ Properties: rings, segments, radius, scale, octaves             │
│    ├─ preRender() → Create PrimitiveProceduralPlanetD3D (lazy)         │
│    │              → planet.initialize(rings, segments, radius)         │
│    │              → planet.setTerrainParams(scale, octaves)            │
│    └─ render()    → pushMatrix() + primitive.execute() + popMatrix()   │
└────────────────────────────┬────────────────────────────────────────────┘
							 │
							 ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                      DISPLAYD3D LAYER (Device)                          │
│                                                                          │
│  DisplayDeviceD3D                                                       │
│    ├─ pushMatrix() / popMatrix()  → World transform stack               │
│    ├─ m_pMatShader                → Shader management                   │
│    │  ├─ applyProceduralPlanet()  → Set VS/PS, bind constant buffers   │
│    │  └─ setConstant()            → Update CB0 with scene uniforms      │
│    │                                                                     │
│    └─ Device/Context              → D3D11 API calls                    │
│       ├─ getDevice()              → ID3D11Device*                      │
│       └─ getContext()             → ID3D11DeviceContext*               │
└────────────────────────────┬────────────────────────────────────────────┘
							 │
							 ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                    PRIMITIVE LAYER (GPU Resource)                       │
│                                                                          │
│  PrimitiveProceduralPlanetD3D : DevicePrimitive                        │
│    ├─ initialize(rings, segments, radius)                             │
│    │  └─ generateSphere()  ← CPU: trigonometry                        │
│    │     ├─ Compute (rings+1) × (segments+1) vertices                 │
│    │     ├─ Position = normal × radius                                │
│    │     └─ Generate (rings) × (segments) × 2 triangles               │
│    │                                                                     │
│    ├─ execute()            ← CPU: every frame                         │
│    │  ├─ Lazy: buildVertexBuffer(), buildIndexBuffer()                │
│    │  ├─ setConstant("vPlanetColor", ...)                             │
│    │  ├─ setConstant("fTerrainScale", ...)                            │
│    │  ├─ setConstant("nOctaves", ...)                                 │
│    │  └─ D3D11 DrawIndexed()                                          │
│    │                                                                     │
│    └─ GPU Resources:                                                    │
│       ├─ m_pVB  → ID3D11Buffer (immutable, VB)                        │
│       └─ m_pIB  → ID3D11Buffer (immutable, IB)                        │
└────────────────────────────┬────────────────────────────────────────────┘
							 │
							 ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                       SHADER LAYER (GPU Code)                           │
│                                                                          │
│  Input Layout: POSITION (12B) | NORMAL (12B) | COLOR (12B)             │
│                                                                          │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │  Vertex Shader (VS_Main)                                        │   │
│  │  ─────────────────────────────                                 │   │
│  │  input.position → mWorld → mView → mProj → SV_Position        │   │
│  │  input.normal → (float3x3)mWorld                               │   │
│  └───────────────────────────┬─────────────────────────────────────┘   │
│                              │                                         │
│  ┌───────────────────────────▼─────────────────────────────────────┐   │
│  │  Pixel Shader (PS_ProceduralPlanet)                             │   │
│  │  ──────────────────────────────────────                         │   │
│  │                                                                 │   │
│  │  1. normal = normalize(input.vNormal)                          │   │
│  │  2. height = fbm(normal / fTerrainScale, nOctaves)             │   │
│  │  3. height = (height + 1) * 0.5  → [0, 1]                     │   │
│  │  4. if (height < 0.4)  color = ocean (blue)                   │   │
│  │     else if (height < 0.5) color = beach (sand)               │   │
│  │     else if (height < 0.7) color = forest (green)             │   │
│  │     else                   color = mountain (gray)             │   │
│  │  5. Apply Phong lighting                                       │   │
│  │  6. return float4(color, 1.0)                                  │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                          │
│  Helper Functions:                                                      │
│  ┌────────────────────────────────┐                                    │
│  │ hash3(float3) → float          │ Pseudo-random [0,1]               │
│  └────────────────────────────────┘                                    │
│  ┌────────────────────────────────┐                                    │
│  │ perlinNoise(float3) → float    │ Interpolation-based noise         │
│  │  ├─ pi = floor(p)              │                                   │
│  │  ├─ pf = frac(p)               │                                   │
│  │  ├─ w = smoothstep(pf)         │                                   │
│  │  ├─ sample 8 cube corners      │                                   │
│  │  └─ trilinear interpolate      │                                   │
│  └────────────────────────────────┘                                    │
│  ┌────────────────────────────────┐                                    │
│  │ fbm(float3, int) → float       │ Fractional Brownian Motion        │
│  │  ├─ amplitude = 1.0            │                                   │
│  │  ├─ frequency = 1.0            │                                   │
│  │  ├─ for octaves:               │                                   │
│  │  │  v += amp × perlinNoise(p)  │                                   │
│  │  │  amp *= 0.5                 │                                   │
│  │  │  freq *= 2.0                │                                   │
│  │  └─ return v                   │                                   │
│  └────────────────────────────────┘                                    │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## Data Flow Diagram (Single Frame)

```
┌─────────────────────────────────────────────────────────────────────────┐
│ FRAME N: Render Single Procedural Planet                                │
└─────────────────────────────────────────────────────────────────────────┘

1. APPLICATION UPDATES
   ├─ Node updates: rotation, position, scale
   └─ Parameter tweaks: terrainScale, octaves

2. RENDER PHASE
   │
   ├─ RenderContext::render(NodeProceduralPlanet)
   │
   ├─ NodeProceduralPlanet::preRender()
   │  ├─ if (!m_pPrimitive.valid())
   │  │  ├─ m_pPrimitive = PrimitiveFactory<ProceduralPlanetD3D>::create()
   │  │  ├─ m_pPrimitive->initialize(32, 64, 1.0f)
   │  │  │  └─ generateSphere() CPU-side, once
   │  │  │     ├─ Allocate m_Verts[1625]
   │  │  │     ├─ Allocate m_Indices[2048]
   │  │  │     └─ Mark "needs GPU upload"
   │  │  └─ m_pPrimitive->setDevice(pDisplay)
   │  └─ [Primitive ready for rendering]
   │
   ├─ NodeProceduralPlanet::render()
   │  ├─ pDisplay->pushMatrix()
   │  ├─ pDisplay->setMatrix(frame, position)  → Constant buffer update
   │  │
   │  ├─ PrimitiveProceduralPlanetD3D::execute()
   │  │  │
   │  │  ├─ if (!m_pVB)
   │  │  │  └─ buildVertexBuffer(pDevice)
   │  │  │     └─ pDevice->CreateBuffer(..., m_Verts.data(), &m_pVB)
   │  │  │        └─ GPU upload: 1625 × 36 bytes = ~58KB
   │  │  │
   │  │  ├─ if (!m_pIB)
   │  │  │  └─ buildIndexBuffer(pDevice)
   │  │  │     └─ pDevice->CreateBuffer(..., m_Indices.data(), &m_pIB)
   │  │  │        └─ GPU upload: 2048 × 4 bytes = ~8KB
   │  │  │
   │  │  ├─ pDisplay->m_pMatShader->setConstant("vPlanetColor", vColor)
   │  │  │  └─ Updates CB1[4 bytes]
   │  │  │
   │  │  ├─ pDisplay->m_pMatShader->setConstant("fTerrainScale", fScale)
   │  │  │  └─ Updates CB1[4 bytes]
   │  │  │
   │  │  ├─ pDisplay->m_pMatShader->setConstant("nOctaves", nOctaves)
   │  │  │  └─ Updates CB1[4 bytes]
   │  │  │
   │  │  ├─ pDisplay->m_pMatShader->applyProceduralPlanet(pDisplay)
   │  │  │  ├─ pContext->VSSetShader(VS_Main, ...)
   │  │  │  ├─ pContext->PSSetShader(PS_ProceduralPlanet, ...)
   │  │  │  ├─ pContext->VSSetConstantBuffers(0, 1, &CB0)  [mWorld, mView, mProj]
   │  │  │  ├─ pContext->PSSetConstantBuffers(1, 1, &CB1)  [vColor, fScale, nOct]
   │  │  │  └─ [Shaders and constants bound]
   │  │  │
   │  │  ├─ pContext->IASetVertexBuffers(0, 1, &m_pVB, ...)
   │  │  ├─ pContext->IASetIndexBuffer(m_pIB, DXGI_FORMAT_R32_UINT, 0)
   │  │  ├─ pContext->IASetPrimitiveTopology(TRIANGLELIST)
   │  │  │
   │  │  └─ pContext->DrawIndexed(2048, 0, 0)
   │  │     └─ ┌─────────────────────────────────────┐
   │  │        │ GPU EXECUTION (Vertex Shader)        │
   │  │        │ ─────────────────────────────────────│
   │  │        │ For each vertex (1625 invocations):  │
   │  │        │  ├─ vPos' = vPos × mWorld × mView    │
   │  │        │  └─ vPos'' = vPos' × mProj           │
   │  │        └─────────────────────────────────────┘
   │  │             └─ ~1-2 ms (minimal work)
   │  │
   │  │     └─ ┌─────────────────────────────────────────┐
   │  │        │ GPU EXECUTION (Rasterizer + Pixel Sh.)  │
   │  │        │ ───────────────────────────────────────│
   │  │        │ For each pixel in triangle (~1024 px):  │
   │  │        │  ├─ normal = interpolate + normalize    │
   │  │        │  ├─ noise = fbm(normal/scale, octaves)  │
   │  │        │  │         ├─ perlinNoise × 4-8×        │
   │  │        │  │         └─ ~200-400 GPU instructions │
   │  │        │  ├─ height = (noise + 1) × 0.5          │
   │  │        │  ├─ color = height-based (if-else)      │
   │  │        │  ├─ lighting = phong(normal, lightDir)  │
   │  │        │  └─ output = color × lighting           │
   │  │        └─────────────────────────────────────────┘
   │  │             └─ ~2-4 ms (FBM cost dominant)
   │  │
   │  ├─ pDisplay->popMatrix()
   │  │
   │  └─ [Rendered to backbuffer]
   │
   └─ Total: ~3-6 ms per planet (at 1920×1080)

3. PRESENT
   └─ Present backbuffer to screen
```

---

## Memory Layout Diagram

### CPU (System RAM)

```
NodeProceduralPlanet (size: ~128 bytes)
├─ int m_nRings = 32                        (4 bytes)
├─ int m_nSegments = 64                     (4 bytes)
├─ float m_fRadius = 1.0f                   (4 bytes)
├─ float m_fTerrainScale = 0.1f             (4 bytes)
├─ int m_nOctaves = 4                       (4 bytes)
├─ Vector3 m_vColor = (1,1,1)               (12 bytes)
├─ Reference<PrimitiveProceduralPlanetD3D>  (8 bytes, pointer)
├─ Matrix33 m_mFrame                        (36 bytes)
├─ Vector3 m_vPosition                      (12 bytes)
└─ ... other NodeTransform data

PrimitiveProceduralPlanetD3D (size: ~2 MB during generation, 0 after GPU upload)
├─ std::vector<Vertex> m_Verts              (1625 × 36 bytes = ~58 KB)
│  └─ [Vertex0: pos(12) + normal(12) + color(12)]
│  └─ [Vertex1: ...]
│  └─ ...
│  └─ [Vertex1624: ...]
├─ std::vector<uint> m_Indices              (2048 × 4 bytes = ~8 KB)
│  └─ [idx0, idx1, idx2, idx3, ...]
├─ int m_nVerts = 1625                      (4 bytes)
├─ int m_nIndices = 2048                    (4 bytes)
├─ ID3D11Buffer* m_pVB = NULL (after GPU upload) (8 bytes, pointer)
└─ ID3D11Buffer* m_pIB = NULL (after GPU upload) (8 bytes, pointer)

[Sphere data freed after GPU upload ✓]
```

### GPU (Video RAM)

```
Vertex Buffer (58 KB, immutable)
├─ [Vertex0: POSITION(12) + NORMAL(12) + COLOR(12) = 36 bytes]
├─ [Vertex1: ...]
├─ ...
└─ [Vertex1624: ...]

Index Buffer (8 KB, immutable)
├─ [0, 1, 65, 1, 66, 65, ...]  (indices into VB)
└─ [... 2048 indices total]

Constant Buffer 0 (cbPerFrame, 256 bytes, dynamic)
├─ matrix mWorld       (64 bytes)
├─ matrix mView        (64 bytes)
├─ matrix mProj        (64 bytes)
├─ float3 vCameraPos   (12 bytes)
├─ float fTime         (4 bytes)
├─ float3 vAmbientLight (12 bytes)
└─ ...

Constant Buffer 1 (cbPerMaterial, 256 bytes, dynamic)
├─ float3 vPlanetColor (12 bytes)
├─ float fTerrainScale (4 bytes)
├─ int nOctaves        (4 bytes)
└─ ... (padding: 236 bytes)

Total GPU: ~66 KB (negligible for modern GPUs)
```

---

## Shader Execution Flow (Pixel Shader Detail)

```
┌──────────────────────────────────────────────────────────────────────┐
│ For each pixel in triangle:                                          │
└──────────────────────────────────────────────────────────────────────┘

1. INTERPOLATION (Automatic)
   ├─ vNormal(3D) interpolated from triangle vertices
   ├─ vWorldPos(3D) interpolated from triangle vertices
   └─ vColor(4D) interpolated from triangle vertices

2. NORMALIZE
   └─ normal = normalize(vNormal)  ← direction from sphere center

3. NOISE EVALUATION
   ├─ noiseCoord = normal / fTerrainScale
   │
   ├─ fbm(noiseCoord, nOctaves):
   │  ├─ Loop octave 0..nOctaves-1
   │  │  ├─ perlinNoise(noiseCoord × freq):
   │  │  │  ├─ pi = floor(noiseCoord × freq)
   │  │  │  ├─ pf = frac(noiseCoord × freq)
   │  │  │  ├─ w = smoothstep(pf)
   │  │  │  ├─ hash3(pi + [0..7]) × 8 lookups
   │  │  │  ├─ trilinear interpolate
   │  │  │  └─ result: float [-1, 1]
   │  │  ├─ value += amplitude × result
   │  │  ├─ amplitude *= 0.5
   │  │  └─ freq *= 2.0
   │  └─ return value / maxValue
   │
   └─ height = (noiseValue + 1.0) × 0.5  → [0, 1]

4. HEIGHT-BASED COLORING
   ├─ if (height < 0.4)
   │  └─ color = lerp(darkBlue, cyan, height / 0.4)
   ├─ else if (height < 0.5)
   │  └─ color = sand
   ├─ else if (height < 0.7)
   │  └─ color = lerp(grass, darkGreen, (height - 0.5) / 0.2)
   └─ else
	  └─ color = rock

5. LIGHTING (Phong Model)
   ├─ lightDir = normalize(float3(1, 1, 1))
   ├─ diffuse = max(0, dot(normal, lightDir))
   ├─ lighting = vAmbientLight + diffuse × sunColor
   └─ color *= lighting

6. FINAL COLOR
   ├─ color *= vPlanetColor  (if different from white)
   └─ return float4(color, 1.0)  ← to RTV (render target view)

Total instructions: ~200-300 per pixel
Cost: ~0.2-0.5 µs per pixel (modern GPU)
1920×1080 full-screen: 2.1M pixels × 0.3 µs = ~0.6 ms
Sphere coverage: ~100K pixels average → ~30 µs per planet
```

---

## State Transition Diagram

```
┌─────────────────────────────────────────┐
│    NodeProceduralPlanet Created         │
│    ├─ m_pPrimitive = NULL              │
│    └─ m_nRings = 32, m_nSegments = 64  │
└────────────────┬────────────────────────┘
				 │
				 │ preRender() called
				 ▼
┌─────────────────────────────────────────┐
│    Create & Initialize Primitive        │
│    ├─ m_pPrimitive created             │
│    ├─ generateSphere() CPU side        │
│    │  ├─ m_Verts built (1625 items)    │
│    │  └─ m_Indices built (2048 items)  │
│    └─ setDevice() called               │
└────────────────┬────────────────────────┘
				 │
				 │ execute() called (frame 1)
				 ▼
┌─────────────────────────────────────────┐
│    GPU Buffers Created (Lazy)           │
│    ├─ buildVertexBuffer()               │
│    │  └─ pDevice->CreateBuffer(VB)      │
│    │     └─ m_pVB != NULL now           │
│    │                                     │
│    ├─ buildIndexBuffer()                │
│    │  └─ pDevice->CreateBuffer(IB)      │
│    │     └─ m_pIB != NULL now           │
│    │                                     │
│    ├─ CPU-side m_Verts/m_Indices        │
│    │  └─ Can be freed (optional)        │
│    └─ [Ready for rendering]             │
└────────────────┬────────────────────────┘
				 │
				 │ execute() called (frames 2+)
				 ▼
┌─────────────────────────────────────────┐
│    Render Loop (Steady State)           │
│    ├─ Skip buildVertexBuffer() (exists) │
│    ├─ Skip buildIndexBuffer() (exists)  │
│    ├─ Update constants (vColor, scale)  │
│    ├─ Bind VB & IB                      │
│    ├─ DrawIndexed(2048, ...)            │
│    └─ Repeat frame 2..N                 │
└────────────────┬────────────────────────┘
				 │
				 │ release() / ~Node()
				 ▼
┌─────────────────────────────────────────┐
│    Cleanup                              │
│    ├─ m_pVB->Release()                  │
│    ├─ m_pIB->Release()                  │
│    ├─ m_Verts.clear()                   │
│    ├─ m_Indices.clear()                 │
│    └─ Memory freed                      │
└─────────────────────────────────────────┘
```

---

## Noise Octave Visualization

```
Single Octave 1 (frequency = 1x):
  ╭─────╮       ╭──────╮         ╭─────╮
  │     ╰─╮   ╭─╯      ╰──╮   ╭──╯     │
  │       ╰───╯            ╰───╯       │
  ▼                                      ▼
 Wavelength: Long (base terrain)

Sum Octave 2 (frequency = 2x, amplitude = 0.5x):
	  ╭╮   ╭╮ ╭╮    ╭╮      ╭╮   ╭╮
	╭╯╰╮ ╭╯╰╮╰╯╰╮  ╭╯╰╮╭───╭╯╰╮╭╯╰╮
	╯ ╰─╯ ╰ ╰  ╰─╯  ╯ ╰╯   ╯ ╰─╯ ╰
 Wavelength: Medium

Sum Octave 3 (frequency = 4x, amplitude = 0.25x):
  ╱╲╱╲╱╲╱╲╱╲╱╲  ... (many small ripples)
 Wavelength: Short (fine detail)

FBM Result = Octave1 + Octave2 + Octave3 + ...
  ╭─────────╮           ╭─────────╮
  │  ╭─╮ ╭─╮│  ╭──╮  ╭─│  ╭─╮ ╭─╮│
  │ ╭╯ ╰╮╭╯ │╭╯  ╰─╮╭╯ │╭╯ ╰╮╭╯ │
  ╰╯   ╰╯   ╰╯     ╰╯   ╰╯   ╰╯   ╰

Interpretation: Mix of large continents + medium mountains + small cracks
```

---

## LOD Comparison

```
┌──────────────────────────────────────────────────────────────────┐
│ Level of Detail (LOD) Strategy                                   │
├──────────────────────────────────────────────────────────────────┤

LOD 0 (Distant)
├─ Rings × Segments: 8 × 16
├─ Vertex count: 153
├─ Triangle count: 256
├─ Use case: Far planets (>5 units away)
├─ Render cost: <0.5 ms
└─ Memory: ~5.5 KB

LOD 1 (Medium)
├─ Rings × Segments: 16 × 32
├─ Vertex count: 561
├─ Triangle count: 1024
├─ Use case: Normal planets (1-5 units)
├─ Render cost: 1-2 ms
└─ Memory: ~20 KB

LOD 2 (Close)
├─ Rings × Segments: 32 × 64
├─ Vertex count: 2145
├─ Triangle count: 4096
├─ Use case: Nearby planets (<1 unit)
├─ Render cost: 2-4 ms
└─ Memory: ~77 KB

LOD 3 (Very Close, with Tessellation)
├─ Base Rings × Segments: 16 × 32
├─ Hardware tessellation: 4-8x refinement
├─ Effective vertex count: variable (GPU computed)
├─ Effective triangle count: 4k-32k
├─ Use case: Landing/approach
├─ Render cost: 3-6 ms (due to tessellation overhead)
└─ Memory: VB + IB only (base), rest computed in HS/DS

┌──────────────────────────────────────────────────────────────────┐
│ Distance-based LOD Selection (in NodeProceduralPlanet)          │
├──────────────────────────────────────────────────────────────────┤

float distance = length(cameraPos - planetPos);

if (distance > 5.0)
	return LOD_DISTANT;  // 8×16
else if (distance > 2.0)
	return LOD_FAR;      // 16×32
else if (distance > 0.5)
	return LOD_MID;      // 32×64
else
	return LOD_NEAR;     // Tessellated
```

---

## Summary

This architecture provides:

✅ **Clean Separation of Concerns**
- Application layer doesn't know about D3D
- Render layer doesn't know about scene details
- GPU code is isolated in shader file

✅ **Efficient Resource Management**
- Lazy GPU upload (first frame)
- Immutable buffers (no CPU update overhead)
- Factory pattern for reuse/pooling

✅ **Extensibility**
- Easy to add new noise types (Worley, Simplex)
- Easy to add new shader variants (tessellation, parallax)
- LOD system ready to integrate

✅ **Performance**
- O(1) render after GPU upload
- ~3-6 ms per planet typical
- Scales to 5+ planets per frame
