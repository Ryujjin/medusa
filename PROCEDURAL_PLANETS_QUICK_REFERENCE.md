# Procedural Planets - Quick Reference & Decision Guide

## TL;DR

| Item | Status | Notes |
|------|--------|-------|
| **Sphere Mesh Generation** | ✅ Exist (pattern available) | Use UV sphere; see PrimitiveStarSurfaceD3D as reference |
| **Vertex Shader** | ✅ Implemented | Standard world → clip space transform |
| **Tessellation Pipeline** | ✅ Implemented | VS_Tess + HS_Main + DS_Main ready to use |
| **Pixel Shader** | ⚠️ Partial | Default.hlsl has lighting; need to add noise functions |
| **Noise Functions** | ❌ Missing | Must implement Perlin/Worley/FBM (see templates) |
| **Scene Integration** | ⚠️ Partial | Pattern exists (NodeRealStarField); need NodeProceduralPlanet |
| **Resource Management** | ✅ Implemented | PrimitiveFactory caching system ready |

---

## Architecture Overview

```
Application Layer
	└─ NodeProceduralPlanet (Render3D)
	   └─ initialize(rings, segments, scale, octaves)
	   └─ render() calls DisplayDevice

Render Layer
	└─ DisplayDeviceD3D
	   └─ m_pMatShader->applyProceduralPlanet()
	   └─ Constant buffer updates

Primitive Layer
	└─ PrimitiveProceduralPlanetD3D
	   └─ CPU sphere generation (one-time)
	   └─ GPU vertex/index buffer management
	   └─ execute() → draw call

Shader Layer
	└─ Default.hlsl
	   └─ VS_Main: Transform mesh
	   └─ HS/DS: Optional tessellation
	   └─ PS_ProceduralPlanet: FBM noise + height coloring
```

---

## Implementation Difficulty Ratings

| Phase | Difficulty | Time | Prerequisites |
|-------|-----------|------|---|
| Sphere Generation | ⭐ Easy | 2-3 hrs | Math (trigonometry), D3D11 basics |
| Noise in Shader | ⭐⭐ Medium | 3-4 hrs | HLSL, perlin math |
| Integration | ⭐⭐ Medium | 2-3 hrs | Scene graph pattern understanding |
| Tessellation Variant | ⭐⭐⭐ Hard | 4-6 hrs | Hull/domain shader knowledge |
| Normal Perturbation | ⭐⭐⭐ Hard | 3-4 hrs | Normal mapping, finite differences |
| **Total (MVP)** | **⭐⭐ Medium** | **12-14 hrs** | ✅ Doable in 1-2 days |

---

## What Your Code Already Has

### 1. **Sphere Mesh Support**
- File: `DisplayD3D/PrimitiveStarSurfaceD3D.cpp`
- Takes pre-generated triangles
- Builds immutable GPU buffers
- **You need**: CPU code to generate sphere triangles (ez math)

### 2. **Shader System**
- File: `DisplayD3D/ShaderD3D11.h`
- Compiles multiple entry points
- Constant buffer management
- **You need**: New shader functions `applyProceduralPlanet()`

### 3. **Tessellation**
- Already compiled: `VS_Tess`, `HS_Main`, `DS_Main`
- Method: `applyTessellation()`
- **You can use**: As-is, just need to point domain shader at noise

### 4. **Scene Nodes**
- File: `Render3D/NodeRealStarField.cpp`
- Follows: preRender() → initialize primitive, render() → execute
- **You need**: Copy pattern for NodeProceduralPlanet

---

## Decision Tree: Which Approach?

```
START: Want procedural planets?
│
├─ "I just want static terrain on a sphere"
│  └─ CHOICE: Simple Approach
│     ├─ CPU sphere mesh (32×64)
│     ├─ PS_ProceduralPlanet shader with FBM
│     ├─ No tessellation
│     ├─ Estimated work: 8-10 hours
│     └─ Quality: Medium (32-64k triangles visible)
│
├─ "I want adaptive detail levels (LOD)"
│  └─ CHOICE: Tessellation Approach
│     ├─ CPU sphere mesh (16×32 base)
│     ├─ Hardware tessellation with distance-based factors
│     ├─ Domain shader applies noise
│     ├─ Estimated work: 14-18 hours
│     └─ Quality: High (adaptive 10k-100k triangles)
│
└─ "I want the best-looking planets (no frame budget limit)"
   └─ CHOICE: Advanced Approach
	  ├─ Tessellation + normal perturbation
	  ├─ Per-frame noise parameter variation
	  ├─ Parallax occlusion mapping optional
	  ├─ Estimated work: 20-24 hours
	  └─ Quality: Very High (artistic control)
```

---

## Quick Implementation Checklist

### Phase 1: Sphere (1-2 days)
- [ ] Create `DisplayD3D/ProceduralPlanetD3D.h`
- [ ] Implement UV sphere generation in `.cpp`
- [ ] Test sphere generates correct triangles (`m_nRings × m_nSegments × 2` tris)
- [ ] Build vertex buffer (immutable, USAGE_IMMUTABLE)
- [ ] Build index buffer
- [ ] Test rendering: white sphere, single color

### Phase 2: Noise Shader (1-2 days)
- [ ] Add hash3() to Default.hlsl
- [ ] Add perlinNoise() function
- [ ] Add fbm() function
- [ ] Add PS_ProceduralPlanet entry point
- [ ] Test noise function: visualize as grayscale
- [ ] Add height-based coloring (ocean → mountain)
- [ ] Add lighting (Phong)

### Phase 3: Integration (0.5-1 day)
- [ ] Create `Render3D/NodeProceduralPlanet.h/cpp`
- [ ] Follow NodeRealStarField pattern
- [ ] Register PrimitiveFactory
- [ ] Update ShaderD3D11 with `applyProceduralPlanet()`
- [ ] Test in scene: create node, add to scene, render
- [ ] Test parameter tweaking

### Phase 4: Polish (0.5-1 day, optional)
- [ ] Add normal perturbation if desired
- [ ] Test multiple planets in scene
- [ ] Profile: target <1ms per planet
- [ ] Tweak noise parameters for aesthetic
- [ ] Document usage

---

## Key Code Patterns

### Pattern 1: Sphere Generation (C++)
```cpp
for (int ring = 0; ring <= rings; ++ring) {
	float v = ring / (float)rings;
	float phi = v * PI;

	for (int seg = 0; seg <= segments; ++seg) {
		float u = seg / (float)segments;
		float theta = u * 2*PI;

		float3 normal(sin(phi)*cos(theta), cos(phi), sin(phi)*sin(theta));
		vertices.push(normal * radius);
	}
}
```

### Pattern 2: FBM Noise (HLSL)
```hlsl
float fbm(float3 p, int octaves) {
	float v = 0, a = 1;
	for (int i = 0; i < octaves; i++) {
		v += a * perlinNoise(p);
		p *= 2;
		a *= 0.5;
	}
	return v;
}
```

### Pattern 3: Height-to-Color (HLSL)
```hlsl
float h = (fbm(normal / scale) + 1) * 0.5;  // [0, 1]
float3 color;
if (h < 0.4) color = lerp(blue, cyan, h/0.4);
else if (h < 0.5) color = sand;
else if (h < 0.7) color = lerp(grass, dark, (h-0.5)/0.2);
else color = rock;
```

---

## Common Pitfalls & Solutions

| Problem | Cause | Solution |
|---------|-------|----------|
| Sphere has gaps | Winding order wrong (clockwise vs CCW) | Check index generation: CW from above |
| Black sphere | Normal pointing inward | Verify: `normal = p.normalized()` where p is vertex position |
| Grayscale noise | No height coloring | Add `if (h < threshold)` in PS |
| Blocky appearance | Too few octaves | Increase FBM octaves: 4 → 6-8 |
| Slow rendering | Pixel shader expensive | Use fewer octaves, or precompute noise to texture |
| Shader doesn't compile | Entry point name wrong | Check: `PS_ProceduralPlanet` matches in shader |
| Primitive doesn't render | Device not set | Call `primitive->setDevice(pDisplay)` before execute |
| Blurry at distance | LOD too low | Add tessellation, or multi-res spheres |

---

## Testing Strategy

### Unit Tests (C++)
```cpp
// Verify sphere generation
ProceduralPlanetD3D planet;
planet.initialize(4, 4, 1.0f);
ASSERT_EQ(planet.vertexCount(), 25);       // (4+1) × (4+1)
ASSERT_EQ(planet.triangleCount(), 32);     // 4×4×2
ASSERT_LT(planet.vertexCount(), 1000);     // Reasonable memory
```

### Rendering Tests
- Render single planet on backbuffer
- Verify no D3D errors
- Check frame time: <5ms per planet
- Visual inspection: sphere looks round, no artifacts

### Shader Tests
- Visualize noise as grayscale
- Verify colors change smoothly with height
- Check lighting from multiple angles
- Test with different octave counts

---

## Performance Targets

| Component | Target | Notes |
|-----------|--------|-------|
| Sphere Gen | <1ms | CPU, one-time |
| Vertex Buffer | <1MB | For 32×64 sphere |
| Pixel Shader | <0.5ms | Per planet (FBM cost) |
| Total/Planet | <1ms | At 1080p |
| **5 Planets** | **<5ms** | Total frame budget |

---

## File Summary

### New Files to Create
```
DisplayD3D/
├── ProceduralPlanetD3D.h          ~150 lines
└── ProceduralPlanetD3D.cpp        ~250 lines

Render3D/
├── NodeProceduralPlanet.h         ~80 lines
└── NodeProceduralPlanet.cpp       ~150 lines
```

### Files to Modify
```
DisplayD3D/
├── ShaderD3D11.h                  +2 methods, +2 member vars
├── ShaderD3D11.cpp                +30-50 lines per method
└── PrimitiveFactory.cpp           +2 lines (factory registration)

DirectXTK/Src/Shaders/
└── Default.hlsl                   +150 lines (noise functions)
								   +50 lines (PS_ProceduralPlanet)
```

---

## Next Steps

1. **Read the analysis document** (PROCEDURAL_PLANETS_ANALYSIS.md) for architecture details
2. **Study PrimitiveStarSurfaceD3D.cpp** to understand the pattern
3. **Review NodeRealStarField.cpp** for scene integration pattern
4. **Start with Phase 1**: Generate a simple sphere and render it white
5. **Incrementally add**: Noise, coloring, lighting
6. **Polish**: Tweak parameters, add LOD, profile performance

---

## Resource Links & References

### Noise Function References
- **Perlin Noise**: Classic Perlin paper (2002), or Scratchapixel tutorials
- **FBM**: "Making Planets" series (various game dev sites)
- **Worley/Cellular**: Cellular noise explanations on Inigo Quilez site

### DirectX 11 References
- Tessellation tutorial: Microsoft documentation on hull/domain shaders
- Constant buffers: "Structured Buffer" vs "Constant Buffer" pitfalls
- Shader compilation: D3DCompile flags (Optimize, Debug, etc.)

### Similar Implementations
- **No Man's Sky**: GPU procedural planets (octave-based noise)
- **Spore**: Legendary procedural creature system (similar FBM concept)
- **Astroneer**: Seamless procedural planets (voxel-based alternative)

---

## Summary

**You have ~80% of the infrastructure.** The 20% gap is:

1. **Sphere generation** — Simple trigonometry in C++
2. **Noise functions** — Copy-paste from standard libraries (Shadertoy, etc.)
3. **Scene integration** — Follow existing patterns

**Realistic timeline**: 12-18 hours of focused work for a solid MVP.

**Recommendation**: Start with the Simple Approach (no tessellation) to prove the concept quickly, then iterate with advanced features if needed.

Good luck! 🚀
