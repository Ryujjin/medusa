# Session Memory Log — DX11 Port Rendering Debug
**Project**: DarkSpace MMO (Palestar / Ryujjin) — DirectX 9 → DirectX 11 port  
**Engine**: Medusa  
**Developer**: Shawn  
**Branch**: `develop` on both repos  
**Date**: 2025  
**Purpose**: Pick up exactly where we left off

---

## 1. Repository Layout

| Repo | Local Path | Remote |
|------|-----------|--------|
| Game | `D:\Projects\Space\Darkspace\VC2026-DX-WORKING\darkspace` | `origin = github.com/Ryujjin/darkspace`, `upstream = github.com/palestar/darkspace` |
| Engine | `D:\Projects\Space\Darkspace\VC2026-DX-WORKING\medusa` | `origin = github.com/palestar/medusa` |

Key projects open in VS solution:
- `medusa\DisplayD3D\DisplayD3D.vcxproj` — D3D11 rendering layer (primary work area)
- `medusa\Render3D\Render3D.vcxproj` — RenderContext, scene graph
- `medusa\Gui3d\Gui3d.vcxproj` — UI/InterfaceContext
- `medusa\World\World.vcxproj` — WorldContext, NodeZone
- `darkspace\DarkSpace\GameContext.cpp` — Game-level render override

---

## 2. Original Problem Statement

> "The trace still shows 0 materials after the fix — meaning push() is not being called at all between beginScene() and endScene()."

The render pipeline was producing empty scenes (0 materials in the pass stacks at `endScene()` time). Prior to this session a **push() key-mismatch fix** had already been applied to `DisplayDeviceD3D::push()` — the concrete `PrimitiveMaterialD3D` key wasn't being recognized as a material because only the abstract `PrimitiveMaterial` key was checked.

---

## 3. Problem Evolution

```
Session start:   push() key-mismatch fixed, but 0 materials still seen
  ↓
Diagnosed:       WorldContext::render() returning early (zoneCount < 1)
                 → beginScene() never called
                 → world never renders
  ↓
Added:           Diagnostic TRACE gates in WorldContext.cpp (see §6)
  ↓
New symptom:     "No user interface rendering its black"
                 → Even HUD / cursor / UI panels invisible
  ↓
Diagnosed:       Two compounding causes (see §8)
```

---

## 4. Full Render Pipeline (Per Frame)

Understanding this flow is critical for all future debugging.

```
InterfaceContext::render()   [medusa/Gui3d/InterfaceContext.cpp ~line 440]
│
├─ pDisplay->lock()
├─ beginRender(Color(0,0,0), Color(0,0,0))
│   → m_Display->clear(black)        // ClearRenderTargetView — does NOT bind RT
│   → m_Display->clearZ(1.0f)        // ClearDepthStencilView
│   → m_Display->setProjection(...)
│   ⚠ does NOT call beginScene() — RT not yet bound in D3D11 pipeline
│
├─ render(m_pRootWindow)             // traverses entire UI tree, pushes all UI primitives
│   └─ ViewTactical::onRender()      [Interface/ViewTactical.cpp]
│       │
│       ├─ context.endScene()  [line 1069]
│       │   → DisplayDeviceD3D::endScene() → executes pre-ViewTactical UI materials
│       │   ⚠ RT may not be bound here on first frame if no prior beginScene()
│       │
│       ├─ pContext->render(context, ...)  [line 1096]
│       │   → GameContext::render()        [DarkSpace/GameContext.cpp ~line 234]
│       │   → WorldContext::render()       [medusa/World/WorldContext.cpp ~line 443]
│       │       ├─ GATE: if zoneCount < 1 → EARLY RETURN (no beginScene ever called)
│       │       ├─ beginScene()  [line 498]
│       │       │   → DisplayDeviceD3D::beginScene()
│       │       │     → Flush(), OMSetRenderTargets() ← RT BOUND HERE
│       │       │     → m_pCurrentMaterial = NULL
│       │       │     → m_pCurrentTransform = NULL
│       │       ├─ renderZones()  [line 507]
│       │       │   → per zone: if childCount > 0 → pZone->preRender()
│       │       └─ endScene()  [line 522]
│       │           → executes world material stack
│       │
│       └─ context.beginScene()  [line 1099]  ← HUD scene start
│           → DisplayDeviceD3D::beginScene()
│             → OMSetRenderTargets() ← RT RE-BOUND
│             → m_pCurrentMaterial = NULL
│
├─ context.beginScene()  [line 444]  ← cursor/overlay scene
│   → m_pCurrentMaterial = NULL
│
├─ [cursor rendering pushes]
├─ context.endScene()  [line 578]  → executes cursor + HUD materials
└─ context.endRender()  [line 581]
    → m_Display->endScene()  (no-op, stack empty)
    → m_Display->present()   → SwapChain::Present(0,0)
```

**Critical rule**: `OMSetRenderTargets` is only called inside `DisplayDeviceD3D::beginScene()`. On frame 0, if `WorldContext::render()` returns early (no zones), the RT is NEVER bound, and all draw calls in the `endScene()` at ViewTactical line 1069 render into the void → black.

From frame 1 onward, the RT persists bound from the previous frame's last `beginScene()`, so it usually works — but this explains first-frame or early-session black screens.

---

## 5. Material Push / Stack System

```
DisplayDeviceD3D::push(DevicePrimitive* p)   [DisplayDeviceD3D.cpp ~line 760]
│
├─ isPrimitiveMaterial = (key == PrimitiveMaterial::staticKey)
│                      || (key == PrimitiveMaterialD3D::staticKey)   ← KEY FIX
│
├─ if isPrimitiveMaterial:
│   ├─ m_pCurrentMaterial = (PrimitiveMaterial*)p
│   ├─ if !m_bPushed: m_Stack[pass].push(p); m_bPushed = true
│   └─ if m_pCurrentTransform: pMaterial->addChild(m_pCurrentTransform)
│
└─ else (geometry / transform):
    ├─ if transform: m_pCurrentTransform = p
    └─ if m_pCurrentMaterial: m_pCurrentMaterial->addChild(p)
        else: WARNING "No current material"

DisplayDeviceD3D::endScene()
└─ for pass 0..PASS_COUNT-1:
    └─ for each material in m_Stack[pass]:
        ├─ pMaterial->execute()   → PrimitiveMaterialD3D::execute()
        └─ pMaterial->clear()     → releases children, resets m_bPushed
```

**Pass constants** (from `DisplayDevice`): `PRIMARY=0`, `SECONDARY=1` (others follow).  
Materials with `Blending == NONE` → `PRIMARY`; any other blending → `SECONDARY`.

---

## 6. Diagnostic Traces Added This Session

### `medusa/World/WorldContext.cpp` — Three TRACE gates

**Gate 1** (line ~451): Early-return when no zones:
```cpp
if ( zoneCount() < 1 )
{
    TRACE("WorldContext::render() - EARLY RETURN: zoneCount=0, nothing to render");
    return;
}
```

**Gate 2** (line ~501): Before `beginScene()` call:
```cpp
TRACE(CharString().format("WorldContext::render() - calling beginScene(), activeZone=%s",
    activeZone() ? activeZone()->name() : "NULL"));
if ( context.beginScene() )
```

**Gate 3** (inside `renderZones()`, line ~536): Per-zone child count:
```cpp
TRACE(CharString().format("WorldContext::renderZones() - zoneCount=%d", m_ZoneArray.size()));
for(int i=0;i<m_ZoneArray.size();i++)
{
    NodeZone * pZone = m_ZoneArray[i];
    TRACE(CharString().format("WorldContext::renderZones() - zone[%d] '%s' childCount=%d",
        i, pZone ? pZone->name() : "NULL", pZone ? pZone->childCount() : -1));
```

### Trace Interpretation Table

| What you see in TRACE | Meaning | Root cause |
|-----------------------|---------|------------|
| `EARLY RETURN: zoneCount=0` | No zones loaded | Game not connected / zone not received |
| `calling beginScene(), activeZone=NULL` | Zones exist, none active | Camera outside all zone radii |
| `zone[0] '...' childCount=0` | Zone loaded, empty | Nouns not yet synced from server |
| `zone[0] '...' childCount=N` (N>0) | Zone has objects | push() key-mismatch fix should engage |

### Existing trace in `GameContext::render()` (~line 242):
```cpp
TRACE(CharString().format("GameContext::render() - zones=%d, activeZone=%s, proxy=%s",
    zoneCount(), activeZone() ? activeZone()->name() : "NULL", bProxy ? "yes" : "no"));
```

---

## 7. The Two Root Causes of the Black Screen

### Cause A — Render Target not bound before first `endScene()` in ViewTactical

**File**: `Interface/ViewTactical.cpp` line 1069  
**Symptom**: Pre-ViewTactical UI elements (background panels, etc.) render to void  
**Why**: `context.endScene()` fires before any `beginScene()` has called `OMSetRenderTargets()`  
**Severity**: Frame-0 only (subsequent frames inherit bound RT from prior frame's last `beginScene()`)  
**Fix needed**: Call `DisplayDeviceD3D::beginScene()` (or `validateRenderTarget()`) at the start of `InterfaceContext::render()` BEFORE `render(rootWindow)`.

`validateRenderTarget()` already exists in `DisplayDeviceD3D.cpp` line 1487:
```cpp
bool DisplayDeviceD3D::validateRenderTarget()
{
    if (!m_pRenderTargetView || !m_pDepthStencilView)
        return false;
    ID3D11RenderTargetView* pRTV = m_pRenderTargetView.Get();
    m_pContext->OMSetRenderTargets(1, &pRTV, m_pDepthStencilView.Get());
    return true;
}
```

### Cause B — Default pixel shader has a hardcoded debug return

**File**: `Shaders/Default_D3D11.fx` line 130  
**Content**:
```hlsl
float4 PS_Main(PS_INPUT input) : SV_Target
{
    // Test 1: Can we see anything at all? Return magenta
    return float4(1, 0, 1, 1);   // ← ALL RENDERING IS MAGENTA, CODE BELOW IS DEAD
    ...
```
**Implication**:
- If screen is **magenta** → shader IS being applied, geometry IS reaching the GPU → good progress
- If screen is **black** → shader is NOT being applied at all

**Why shader might not apply**:
1. `m_pDefaultShader` failed to load (check TRACE output for `"CRITICAL ERROR: NO SHADER LOADED"`)
2. `Shader::valid()` returns false (i.e., `m_pVertexShader == NULL || m_pPixelShader == NULL`)
3. Input layout mismatch causes `CreateInputLayout` to fail silently

---

## 8. Critical Pending Bug: UI Vertex Format Mismatch

**This is the deepest unfixed issue for UI rendering.**

The default shader's input layout (`D3d10shader.cpp` line 418) declares:
```cpp
D3D11_INPUT_ELEMENT_DESC layout[] =
{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0,  0, ... },  // 12 bytes
    { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,   0, 12, ... },  // 12 bytes
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, ... },  //  8 bytes
};
// Total: 32 bytes — 3D world vertex (position + normal + UV)
```

But `PrimitiveWindowD3D` uses `VB_VERTEXTL` (Transformed & Lit) — screen-space vertices:
```cpp
// PrimitiveWindowD3D::execute():
int vbOffset = pDevice->setupDVB(DisplayDeviceD3D::VB_VERTEXTL, 4, m_Quad);
```

`VB_VERTEXTL` is vertex type index 2 (from `DisplayDeviceD3D.h` line 95).  
In DX9, `XYZRHW` vertices bypassed the vertex shader entirely.  
In DX11, **there is no bypass** — a dedicated 2D/UI vertex shader and input layout are required.

**Fix needed**: Create a separate UI shader (e.g., `UI_D3D11.fx`) with:
- VS_INPUT: `float4 Position : POSITION` + `float4 Color : COLOR` + `float2 TexCoord : TEXCOORD`
- VS_Main: pass position through directly (no world/view/proj transform — coordinates already in NDC or screen space)
- PS_Main: sample diffuse texture or output vertex color
- Register a separate input layout for `VB_VERTEXTL` in `DisplayDeviceD3D::m_pInputLayouts[VB_VERTEXTL]`

The actual `VertexTL` struct (from `PrimitiveWindowD3D::initialize()`):
```cpp
m_Quad[0].position = {x, y, z};   // screen coords
m_Quad[0].w        = 1.0f;        // rhw
m_Quad[0].u        = uv.left;
m_Quad[0].v        = uv.top;
m_Quad[0].diffuse  = color;       // DWORD color
```
Layout: `float3 position + float w + float2 uv + DWORD diffuse` (likely 28 bytes)

---

## 9. Key Files and Their Roles

| File | Role | Notes |
|------|------|-------|
| `medusa/DisplayD3D/DisplayDeviceD3D.cpp` | D3D11 device, beginScene/endScene/push, material stacks | Lines 519(beginScene), 654(endScene), 760(push) |
| `medusa/DisplayD3D/DisplayDeviceD3D.h` | Device header; VB_VERTEX/VB_VERTEXTL enum, m_Stack[], m_pDefaultShader | Lines 93-96 (VB enums) |
| `medusa/DisplayD3D/PrimitiveMaterialD3D.cpp` | Material execute(), shader apply, light passes, texture stages | Already fully ported to D3D11 |
| `medusa/DisplayD3D/D3d10shader.cpp` | Shader load/compile/apply, constant buffers, input layout | Lines 53(valid), 122(apply), 275(compileShader), 414(createInputLayout) |
| `medusa/DisplayD3D/ShaderD3D11.h` | Shader class declaration | |
| `medusa/DisplayD3D/PrimitiveWindowD3D.cpp` | 2D UI quad rendering using VB_VERTEXTL | Uses wrong (3D) shader — needs UI shader |
| `medusa/Render3D/RenderContext.cpp` | beginRender/endRender/beginScene/endScene wrappers | Lines 342(beginRender), 404(beginScene), 449(endScene), 463(endRender) |
| `medusa/Gui3d/InterfaceContext.cpp` | Root render loop; beginRender→render(rootWindow)→beginScene→endScene→endRender | Lines 440-581 |
| `medusa/World/WorldContext.cpp` | World render, zone iteration | Lines 443(render), 529(renderZones) — TRACES ADDED |
| `darkspace/DarkSpace/GameContext.cpp` | Game render override, existing trace at line 242 | Lines 234-271 |
| `darkspace/Interface/ViewTactical.cpp` | Tactical view; endScene(1069) + world render(1096) + beginScene(1099) | The critical scene-pair boundary |
| `Shaders/Default_D3D11.fx` | Default 3D world shader | PS_Main line 130: DEBUG `return float4(1,0,1,1)` — blocks real output |

---

## 10. Shader System Details

### `Shader::valid()` (D3d10shader.cpp line 53):
```cpp
return m_pVertexShader != NULL && m_pPixelShader != NULL;
```

### `Shader::apply()` (D3d10shader.cpp line 122):
```cpp
pContext->VSSetShader(m_pVertexShader, NULL, 0);
pContext->PSSetShader(m_pPixelShader, NULL, 0);
pContext->IASetInputLayout(m_pInputLayout);
updateConstantBuffers(pDevice);
pContext->VSSetConstantBuffers(0, 1, &m_pVSConstantBuffer);
pContext->PSSetConstantBuffers(0, 1, &m_pPSConstantBuffer);
```

### Default shader load path (DisplayDeviceD3D.cpp line 1150):
1. `Shaders/Default_D3D11.fx` (primary)
2. `Default_D3D11.fx` (alternate)
3. `Shaders/Default.fx` (DX9 fallback — will fail to compile)

### `ShaderConstants` struct (D3d10shader.cpp line 231):
Must exactly match `cbuffer CB0` in `Default_D3D11.fx`. Contains: world/view/proj matrices, camera pos, light matrices, material properties, light properties, shadow map flags, texture flags.

---

## 11. Render State Objects

| Object | Set in | Purpose |
|--------|--------|---------|
| `m_pRasterizerSolid` | `beginScene()` default + `setupRenderStates()` | Backface culling ON |
| `m_pRasterizerPoint` | `setupRenderStates()` when `m_DoubleSided` | Culling OFF |
| `m_pRasterizerWireframe` | setFillMode | Wireframe |
| `m_pBlendStateOpaque` | `setupBlending(NONE)` | No blending |
| `m_pBlendStateAlpha` | `setupBlending(ALPHA/ADDITIVE)` | Alpha blend (ADDITIVE shares this — TODO: dedicated additive state) |
| `m_pDepthStencilEnabled` | `setupRenderStates()` PRIMARY pass | Depth test+write ON |
| `m_pDepthStencilDisabled` | `setupRenderStates()` non-PRIMARY pass | Depth test+write OFF |
| Samplers `s0-s3` | `beginScene()` default | Linear×3 + Anisotropic×1 |

---

## 12. Immediate Next Steps (Priority Order)

### Step 1 — Bind the RT before first UI `endScene()` [BLOCKING]
In `InterfaceContext::render()` (medusa/Gui3d/InterfaceContext.cpp), immediately after `beginRender()` and before `render(m_pRootWindow)`, add:
```cpp
pDisplay->validateRenderTarget();   // ensures OMSetRenderTargets before any draw call
```
This ensures that on frame 0 (before WorldContext ever calls `beginScene()`), the RT is already bound.

### Step 2 — Remove the debug `return` from `Default_D3D11.fx` [NEXT]
In `Shaders/Default_D3D11.fx` line 130, comment out or remove:
```hlsl
// return float4(1, 0, 1, 1);   // DEBUG — remove for production
```
**Do this AFTER confirming the screen shows magenta (i.e., shader is applying correctly).**  
If screen is still black after Step 1, the shader itself isn't loading — check TRACE output.

### Step 3 — Create `UI_D3D11.fx` and register it for `VB_VERTEXTL` [REQUIRED FOR UI]
- New shader with screen-space vertex input (no world transform)
- New input layout matching `VertexTL` struct (position4+color+uv2)
- Register layout in `m_pInputLayouts[VB_VERTEXTL]`
- `PrimitiveWindowD3D::execute()` must bind this layout before drawing

### Step 4 — Verify world rendering with zones loaded
Once zones are present (childCount > 0 in trace), confirm:
- Geometry renders as magenta (shader applying)
- Then remove debug return and verify lighting/texture output

### Step 5 — Fix ADDITIVE blend state
Create a dedicated `m_pBlendStateAdditive` instead of sharing `m_pBlendStateAlpha` for `ADDITIVE`/`ADDITIVE_INV` blending (marked TODO in `setupBlending()`).

---

## 13. Working Style and Preferences Observed

- **Systematic tracing**: Prefers TRACE-first diagnosis with explicit table interpretation
- **Minimal changes**: Each fix should touch the fewest lines possible
- **Build verification**: Always run `run_build` after changes before declaring done
- **Code style**: Matches existing file style — no unnecessary comments, no new libraries
- **Documentation**: Maintains `.md` files alongside code (BUILD_STATUS, RAII_IMPROVEMENTS, etc.) — this session log continues that pattern
- **Incremental validation**: Wants to see magenta before removing debug shader code — confirms each layer before moving to the next

---

## 14. Known Non-Issues (Things That Are Correct)

- `DisplayDeviceD3D::endScene()` calling `pMaterial->clear()` after `execute()` — this is correct, resets `m_bPushed`
- Multiple `endScene()` calls per frame — later calls are no-ops when stack is empty
- `m_pCurrentMaterial = NULL` in `beginScene()` — correct, new scene starts fresh
- `PrimitiveMaterialD3D::execute()` returning `false` causing `endScene()` to log error — this is intentional, surface errors are fatal per material
- Shadow map path returning `false` from `beginShadowPass()` when `fShadowScale <= 0.05f` — correct culling

---

## 15. Session Git State

No commits were made during this session. All changes are working-copy modifications:
- `medusa/World/WorldContext.cpp` — 3 TRACE statements added (not committed)

---
*Log created at end of debugging session. Resume by reading §12 (Next Steps) first.*

### Session 21 — D3D11 Threading Hang + Font OOM Root Cause (BUILD CLEAN)

| File | Change | Status |
|------|--------|--------|
| Material.cpp read() | Removed createDeviceSurfaces(sm_pCacheDevice) — lazy creation via createDevicePrimitives() | DONE |
| Font.cpp read() | Removed createFontMaterial() + createFontSurface() — lazy creation via Font::push() | DONE |

**D3D11 Threading Rules (CRITICAL):**
- ID3D11DeviceContext methods (Map, Unmap, Flush, Draw, CopyResource) — NOT thread-safe
- ID3D11Device methods (CreateTexture2D, CreateBuffer) — thread-safe
- Rule: Never call m_pContext->* from any thread other than the render thread
- Broker::sm_Lock is released BEFORE pBroker->load() — no deadlock risk with display lock

**Font Surface Memory:**
- MAX_FONT_SURFACE_WIDTH/HEIGHT = 1024 (Font.h) → 32 surfaces for 65,502-char font
- Increase to 4096 → only 2 surfaces (same total pixels, far fewer D3D objects)

**Next Steps:**
1. Clean rebuild (sync Sessions 20+21 source with binary)
2. Increase MAX_FONT_SURFACE_WIDTH/HEIGHT to 4096 in Font.h
3. Search for other sm_pCacheDevice call sites in read() methods
4. Runtime test — verify hang resolved and textures load

### Session 22 — Block-Compressed Texture Buffer Overrun (Access Violation in memset)

**Exception**: `0xC0000005` — Access violation writing `0x192A0000` inside CRT `memset` (SSE2 `movdqa` path)

**Crash Call Chain:**
```
NodeSprite::render → RenderContext::push(Material*) → Material::material
→ createDevicePrimitives → createDeviceSurfaces → getSurface
→ PrimitiveSurfaceD3D::lock(0) → memset(m_pLockBuffer, 0, m_nLockBufferSize) → CRASH
```

**Root Cause — Buffer size miscalculation for block-compressed (BC) texture formats:**

The texture uses `DXGI_FORMAT_BC2_UNORM` (format 74). For BC2, each 4×4 pixel block = 16 bytes.
The running binary calculated the staging buffer size as:
```cpp
m_nLockBufferSize = m_Pitch * mipHeight;  // 4096 * 1024 = 4,194,304 bytes (4 MB)
```
But `mapped.RowPitch` (4096) is bytes per **row of 4×4 blocks**, not per pixel row.
Actual block rows = 1024/4 = 256. Real buffer = `mapped.DepthPitch` = **1,048,576 bytes (1 MB)**.
`memset` wrote 4 MB into 1 MB → overran into unmapped memory at `0x192A0000`.

| Value | Amount | Source |
|-------|--------|--------|
| m_pLockBuffer | 0x19086000 | mapped.pData |
| mapped.RowPitch | 4,096 | D3D11 Map |
| mapped.DepthPitch | 1,048,576 | D3D11 Map (actual size) |
| m_nLockBufferSize | 4,194,304 | **WRONG** (4096 × 1024) |
| Crash address | 0x192A0000 | 0x21A000 past buffer start (beyond 1 MB boundary) |
| Texture | 1024×1024, BC2_UNORM, MipLevels=1 | mainDesc |

**Source/Binary Mismatch (STILL PRESENT):**
The on-disk `PrimitiveSurfaceD3D.cpp` (both copies) has the Session 20 version: no `m_pLockBuffer`,
no `m_nLockBufferSize`, no `memset`, no `unlock()`. The running binary has a newer version with
staging texture reuse, CPU fallback, `memset` zeroing, and the buggy size calculation.
The header `PrimitiveSurfaceD3D.h` declares `m_pLockBuffer` and `m_nLockBufferSize`, confirming
the newer version is the intended target. **Must reconcile source with binary before rebuild.**

**Fix Required (apply when source is synced to running binary version):**

1. **Staging texture path** — use driver-reported size:
   ```cpp
   m_nLockBufferSize = mapped.DepthPitch;  // was: m_Pitch * mipHeight
   ```

2. **CPU fallback path** — compute block rows for BC formats:
   ```cpp
   UINT rowCount = mipHeight;
   if (mainDesc.Format >= DXGI_FORMAT_BC1_TYPELESS && mainDesc.Format <= DXGI_FORMAT_BC5_SNORM)
   {
       UINT numBlocksWide = Max(1u, (mipWidth + 3) / 4);
       rowCount = Max(1u, (mipHeight + 3) / 4);
       UINT bytesPerBlock = ((mainDesc.Format >= DXGI_FORMAT_BC1_TYPELESS
           && mainDesc.Format <= DXGI_FORMAT_BC1_UNORM_SRGB)
           || (mainDesc.Format >= DXGI_FORMAT_BC4_TYPELESS
           && mainDesc.Format <= DXGI_FORMAT_BC4_SNORM)) ? 8 : 16;
       m_Pitch = numBlocksWide * bytesPerBlock;
   }
   else
   {
       UINT bpp = DisplayDeviceD3D::getFormatDepth(mainDesc.Format);
       if (bpp == 0) bpp = 32;
       m_Pitch = mipWidth * (bpp / 8);
   }
   m_nLockBufferSize = m_Pitch * rowCount;
   ```

3. **BC6H/BC7 gap** — formats 94–99 also use 4×4 blocks (16 bytes/block) but are NOT covered
   by the fallback path range check (BC1_TYPELESS..BC5_SNORM = 70..84). The staging path fix
   (`mapped.DepthPitch`) handles all formats correctly. Add BC6H/BC7 to fallback if needed.

**Caller context (Material::getSurface):**
```cpp
void * pPixels = pSurface->lock( i );
memcpy( pPixels, pMipMap->frame( nFrame ), pMipMap->frameSize( nFrame ) );
```
For BC2 1024×1024, `frameSize` = 1,048,576 (correct compressed size). The `memcpy` itself is safe;
only the `memset` zeroing used the inflated size.

**D3D11 Block-Compressed Format Rules (reference):**
- BC1/BC4: 8 bytes per 4×4 block (DXGI_FORMAT 70–76, 79–81)
- BC2/BC3/BC5: 16 bytes per 4×4 block (DXGI_FORMAT 73–75, 76–78, 82–84)
- BC6H/BC7: 16 bytes per 4×4 block (DXGI_FORMAT 94–99)
- `RowPitch` = bytes per row of blocks = (width/4) × bytesPerBlock
- `DepthPitch` = total bytes for 2D subresource = RowPitch × (height/4)
- For non-BC formats: RowPitch = bytes per pixel row, DepthPitch = RowPitch × height

**Next Steps:**
1. Reconcile PrimitiveSurfaceD3D.cpp source with running binary version (restore Session 20 changes + unlock)
2. Apply the DepthPitch fix to staging path
3. Apply BC-aware size calculation to CPU fallback path
4. Clean rebuild all DLLs
5. Runtime test — verify BC2 textures (sprites, materials) load without crash

### Session 23 — Black Textures on HUD/Viewport (Investigation — No Runtime Fix Yet)

**Symptom**: After rebuild with Session 22 fixes, intro screens display (colored quads), but HUD
buttons and 3D viewport are black — no textures rendering.

**Source/binary reconciliation confirmed**:
- Medusa `PrimitiveSurfaceD3D.cpp` (596 lines) is the compiled file (via DisplayD3D.vcxproj)
- Root-level `PrimitiveSurfaceD3D.cpp` (102 lines) is an orphan — NOT compiled
- Session 22 DepthPitch fix + BC fallback are applied on-disk in the medusa copy
- lock() has mipmap guard: `if (m_bMipMap && nLevel != 0) return NULL;`

**Deep code review performed** — 10+ files examined across shader, material, surface, and display layers.

**Verified correct (NOT the cause):**
| Component | Status |
|-----------|--------|
| UI_D3D11.fx texture sampling | ✓ `txDiffuse.Sample` when `bEnableDiffuse=1` |
| Default_D3D11.fx 3D sampling | ✓ With lighting calculations |
| UIShaderConstants ↔ HLSL layout | ✓ Exact match (float2 + int + float = 16 bytes) |
| applyUIShader() SRV detection | ✓ `PSGetShaderResources(0)` before setting bEnableDiffuse |
| setupTextureStages() ordering | ✓ clearTextureStages → surface execute → SRV bound |
| SRV persistence across shader switch | ✓ VS/PS change doesn't unbind SRVs |
| PrimitiveSurfaceD3D::execute() | ✓ Binds SRV to correct slot, returns false if NULL |

**Issues found:**

1. **DXGI format mapping gaps (HIGH RISK)**:
   - `findFormat(ColorFormat::RGB555)` → `DXGI_FORMAT_UNKNOWN` (no D3D11 equivalent)
   - `findFormat(ColorFormat::RGB4444)` → `DXGI_FORMAT_UNKNOWN` (no D3D11 equivalent)
   - Both listed in `m_TextureFormats` as "supported"
   - `findBestFormat` exact-matches images in these formats → CreateTexture2D gets UNKNOWN → FAILS
   - Fix: Remove from `m_TextureFormats` or map to nearest supported format

2. **RGB888 (24-bit) silent mismatch**:
   - `findFormat(RGB888)` → `DXGI_FORMAT_R8G8B8A8_UNORM` (4 bytes/pixel)
   - If image IS RGB888, exact-match in findBestFormat → no conversion
   - `memcpy(frameSize)` copies 3bpp data into 4bpp staging buffer → garbled
   - Fix: Force conversion to RGB8888 for RGB888 images, or remove exact-match path

3. **Mipmap guard causes spurious TRACE errors** (non-blocking):
   - `if (m_bMipMap && nLevel != 0) return NULL;` rejects all mip levels > 0
   - getSurface loop breaks after level 0, logs "ERROR: Failed to lock surface!" per texture
   - Level 0 IS uploaded, GenerateMips fills rest — textures should be correct
   - Noise in TRACE output but not causing black

**Format mapping table**:
```
RGB888   → R8G8B8A8_UNORM   ⚠ 3bpp→4bpp if no conversion
RGB8888  → R8G8B8A8_UNORM   ✓
RGB888L  → B8G8R8A8_UNORM   ✓
RGB565   → B5G6R5_UNORM     ✓
RGB555   → UNKNOWN           ✗
RGB5551  → B5G5R5A1_UNORM   ✓
RGB4444  → UNKNOWN           ✗
DXT1     → BC1_UNORM         ✓
DXT3     → BC2_UNORM         ✓
DXT5     → BC3_UNORM         ✓
```

**Next Steps:**
1. Remove RGB555 and RGB4444 from m_TextureFormats in DisplayDeviceD3D.cpp
2. Add diagnostic TRACE to getSurface: log format, lock result, frameSize vs DepthPitch
3. Verify whether intro screens use textures or just colored quads
4. Clean rebuild + runtime test with Output window monitoring
5. Check if RGB888 images exist in DarkSpace assets (potential garbled textures)

---

## SESSION 37 — Compile/Link Error Fixes in DisplayDeviceD3D

### ✅ CONFIRMED RUNTIME RESULT
**GUI/HUD flickering is gone.** The fixes in this session (C2039 dot product + LNK1120 missing function bodies) resolved the flickering that was present in the UI/HUD rendering. The game is now running with stable screen-space UI.

---

### Errors Resolved

#### Fix 1: C2039 — `dot` not a member of `Vector3`

**Error**: `C2039: 'dot': is not a member of 'Vector3'`

**Location**: `DisplayDeviceD3D.cpp` — `beginShadowPass()`, inside the light-space view matrix block:
```cpp
// BEFORE (broken):
if (fabsf(lightFrame.j.dot(vLightViewDirection)) > 0.9f)

// AFTER (fixed):
float dotProduct = lightFrame.j.x * vLightViewDirection.x +
                   lightFrame.j.y * vLightViewDirection.y +
                   lightFrame.j.z * vLightViewDirection.z;
if (fabsf(dotProduct) > 0.9f)
```

**Root cause**: `Vector3` in the Medusa engine does not have a `.dot()` member method. The dot product must be computed manually using component-wise multiplication and addition.

**Purpose of the guard**: Prevents a degenerate `XMMatrixLookAtLH` call when the up vector (`lightFrame.j`) is nearly parallel to the light direction, which would produce a NaN/invalid matrix.

---

#### Fix 2: LNK1120 — 3 Unresolved External Symbols

**Error**: 
```
LNK2019: unresolved external symbol DisplayDeviceD3D::updateClientArea(bool)
LNK2019: unresolved external symbol DisplayDeviceD3D::enumerateModes(void)
LNK2019: unresolved external symbol DisplayDeviceD3D::updateGlobals(void)
LNK1120: 3 unresolved externals
```

**Root cause**: All three function *bodies* were missing from `DisplayDeviceD3D.cpp` on disk. The declarations existed in the header, the call sites existed in the `.cpp`, but the definitions had never been written to the disk file (they existed only in the IDE's unsaved editor buffer from a prior session).

**Diagnostic process**:
1. Checked build output → LNK2019 named the 3 missing functions
2. Confirmed `DisplayDeviceD3D.cpp` IS in the `.vcxproj` (`<ClCompile Include="DisplayDeviceD3D.cpp">`)
3. Ran `Select-String` on disk file → found only *call sites*, not *definitions*
4. Ran across all `.cpp` files in DisplayD3D → zero matches for the definitions anywhere
5. Confirmed the file ends at `setD3DMatrix` with no further function bodies

**Remedy**: Appended the three missing function bodies to `DisplayDeviceD3D.cpp` before `// EOF`:

| Function | Lines added | Purpose |
|----------|-------------|---------|
| `updateClientArea(bool)` | ~40 | Tracks windowed client rect, triggers `resetDirectX()` on resize, sets `m_bMinimized` |
| `enumerateModes()` | ~50 | Enumerates DXGI display modes via factory/adapter/output chain into `sm_ModeList` |
| `updateGlobals()` | ~40 | Builds view/proj/world XMMATRIX from `m_Proj` + current transform and pushes to active shader CB |

**Final on-disk positions** (file is now ~2280 lines):
- `updateClientArea` → line 2130
- `enumerateModes` → line 2173
- `updateGlobals` → line 2231

---

### Key Technical Notes for This Codebase

**`Vector3` API**: No `.dot()` member. Compute dot products manually:
```cpp
float d = a.x*b.x + a.y*b.y + a.z*b.z;
```
Other confirmed members: `.magnitude()`, `.x/.y/.z` components, `*` scalar multiply, `-` vector subtract, `%` operator (cross product or frame transform — context dependent).

**Editor buffer vs. disk**: The FILE CONTEXT provided to Copilot reflects the IDE editor buffer (which may contain unsaved changes). The disk file is what the compiler actually sees. When diagnosing linker errors, always verify the disk content with `Select-String` rather than trusting the context.

**`DisplayDeviceD3D.cpp` function inventory** (as of end of this session):
```
isLocked, modeCount, mode, primaryFormat, surfaceFormatCount, surfaceFormat,
activeMode, windowed, clientWindow, renderWindow, textureP2, textureSquare,
textureMaxSize, textureMinSize, pixelShaderVersion, vertexShaderVersion,
filterMode, totalVideoMemory, freeVideoMemory, lock, unlock, initialize,
setMode, setFilterMode, release, flushVideoMemory, clear, clearZ, setAmbient,
addDirectionalLight, addPointLight, clearLights, setFillMode, setFog,
disableFog, setProjection, setShadowPass, beginScene, beginShadowPass,
endShadowPass, endScene, abortScene, present, create, push(DevicePrimitive*),
createEffect, push(DisplayEffect*), capture, getShader, releaseShader,
releaseShaders, applyUIShader, registerEffect, allocateDVB, setupDVB,
resetDVB, releaseDVB, getD3DFilterMode, initializeDirectX, resetDirectX,
freeDirectX, readyShadowMap, createStates, releaseStates, validateRenderTarget,
getSampleMode, getSwapChainDesc, updateBuffers, updateWindow, updateProjection,
enumerateTextures, updateClientArea, enumerateModes, findFormat(DXGI_FORMAT),
findFormat(ColorFormat::Format), getFormatDepth, setD3DMatrix, updateGlobals
```

---

### Next Steps (Carried Forward)

1. ~~Build and confirm clean link~~ — **DONE (Session 38)**: both `DisplayD3DD.dll` and `Render3DD.dll` link clean.
2. ~~Format cleanup~~ — **DONE (prior session)**: `RGB555`, `RGB4444`, `RGB888`, `RGB888L` all removed from `m_TextureFormats`; comments at `enumerateTextures()` line 2070–2072 document the removals.
3. ~~RGB888 data alignment~~ — **DONE (prior session / Session 38 confirmed)**: `RGB888` absent from `m_TextureFormats` → `findBestFormat` never exact-matches it → `setFormat(RGB8888)` conversion always fires before lock/memcpy.
4. **Shadow map runtime testing** — verify the new `beginShadowPass` dot-product guard produces valid light-space matrices and shadows render correctly
5. ~~getSurface diagnostic TRACE~~ — **DONE (Session 38)**: two TRACE calls added to `Material::getSurface()` in `medusa/Render3D/Material.cpp`

---

## SESSION 38 — getSurface Diagnostic TRACE + Format Cleanup Audit

### Build Status
Both `DisplayD3DD.dll` (DisplayD3D project) and `Render3DD.dll` (Render3D project) build and link **clean** as of Session 38 start. The C4316 alignment warning in `PrimitiveFactory.h` is pre-existing and benign.

### Audit: Format Cleanup (Sessions 36–37 carry-forward)

| Issue | Status | Location |
|-------|--------|----------|
| RGB555 in m_TextureFormats | ✅ Already removed | `enumerateTextures()` line 2072 comment |
| RGB4444 in m_TextureFormats | ✅ Already removed | `enumerateTextures()` line 2072 comment |
| RGB888 in m_TextureFormats | ✅ Already removed | `enumerateTextures()` line 2070 comment |
| RGB888L in m_TextureFormats | ✅ Already removed | `enumerateTextures()` line 2071 comment |
| RGB888 exact-match mismatch | ✅ Not possible | `m_TextureFormats` never returns RGB888 → no exact-match early return in `findBestFormat` → `setFormat(RGB8888)` conversion fires |

### Change: getSurface Diagnostic TRACE

**File**: `medusa/Render3D/Material.cpp`  
**Function**: `Material::getSurface()`

Two TRACE calls added:

#### TRACE 1 — Per-texture upload summary (fires once per surface, at level 0 before loop)
```cpp
// line ~446
TRACE( CharString().format("getSurface: eFormat=%d size=%dx%d pitch=%d levels=%d frameSize=%d",
    eFormat, surfaceSize.width, surfaceSize.height,
    pSurface->pitch(), pSurface->levels(),
    pImage->frameSize(nFrame)) );
```
**Purpose**: Confirms each texture is reaching the upload path. Shows:
- `eFormat` — the `ColorFormat::Format` enum value used (8=RGB8888, 11=DXT1, 12=DXT3, 13=DXT5, etc.)
- `size` — pixel dimensions of the level-0 surface
- `pitch` — D3D11 staging row pitch in bytes (may be larger than `width * bpp` due to GPU alignment)
- `levels` — number of mip levels allocated
- `frameSize` — source data size in bytes for frame 0

#### TRACE 2 — Pitch mismatch notice (fires per mip level when row-by-row copy is needed)
```cpp
// line ~482
TRACE( CharString().format("getSurface: pitch mismatch level=%d srcPitch=%d dstPitch=%d numRows=%d dstTotal=%d",
    i, srcPitch, dstPitch, numRows, dstPitch * numRows) );
```
**Purpose**: Identifies textures where the GPU requires padded rows. Shows:
- `level` — mip level index (0 = base, 1+ = lower mips)
- `srcPitch` — bytes per source row (or block-row for BC)
- `dstPitch` — bytes per destination row (`pSurface->pitch()` — GPU-aligned)
- `numRows` — number of rows to copy (pixel rows or block-rows for DXT)
- `dstTotal` — `dstPitch * numRows` = effective staging buffer size

**Interpretation table**:

| What you see | Meaning |
|--------------|---------|
| `pitch mismatch level=0 srcPitch=4096 dstPitch=4096` never fires | Pitches match — bulk `memcpy` used (good) |
| `pitch mismatch level=0 srcPitch=2048 dstPitch=4096` | GPU pads rows (common on 256-byte-aligned GPUs) — row-by-row copy is correct |
| `ERROR: Failed to lock surface!` | Staging texture map failed — check device lost / DXGI error |
| `ERROR: Failed to initialize surface` | `CreateTexture2D` failed — check format/size against device caps |

### Next Steps (Carried Forward)

1. **Shadow map runtime testing** — verify `beginShadowPass` dot-product guard (Session 37) produces valid light-space matrices
2. **Runtime test with TRACE** — run the game, capture Output window, paste `getSurface:` lines here
   - If all formats show `eFormat=8` (RGB8888) or DXT variants (11–13), format pipeline is correct
   - If `ERROR: Failed to lock` appears, investigate PrimitiveSurfaceD3D::lock() path for that format
   - If `pitch mismatch` fires for every texture, confirm textures actually render (confirms copy is correct)
3. **Black texture root cause** — if textures still appear black after TRACE shows successful lock/upload, next suspect is SRV binding or shader `bEnableDiffuse` flag not being set (see Session 23 §Issues found)