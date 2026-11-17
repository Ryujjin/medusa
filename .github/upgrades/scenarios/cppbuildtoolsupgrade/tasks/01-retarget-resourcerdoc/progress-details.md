# Progress: 01-retarget-resourcerdoc

## What Changed

Expanded beyond the original single-project scope: a full rebuild exposed that **38 `.vcxproj` files** under `Tools\` were still pinned to the unavailable `PlatformToolset v143` (the solution only reveals one blocking error project at a time per build order, so each fix uncovered the next). All were retargeted to `v145` to match the rest of the solution:

- ResourcerDoc, VC2M, Docs (fixed individually first)
- CollectionPort, FontPort, ImagePort, InterfaceContextPort, MakeUN, MaterialPort, MedusaExport, MusicPort, NodeBlurPort, NodeBooleanPort, NodeBooleanTimePort, NodeComplexMeshPort, NodeDecalPort, NodeDetailPort, NodeFlarePort, NodeGlarePort, NodeLightPort, NodeMeshPort, NodeParticleField, NodeParticleSystem2Port, NodeParticleSystemPort, NodePathPort, NodeScalePort, NodeScenePort, NodeSoundPort, NodeSpinPort, NodeSpritePort, NodeWindowPort, ProfilerClient, Resourcer, ScenePort, SoundPort, TextPort, UpdateBuildVersion, WindowButtonPort, WindowStylePort, WindowTextPort, WindowViewPort (fixed in bulk via script)

Each change: `<PlatformToolset>v143</PlatformToolset>` → `<PlatformToolset>v145</PlatformToolset>` for both `Debug|Win32` and `Release|Win32` configurations. No other project settings or source files were modified.

## Process

1. Unloaded each project before editing its `.vcxproj`.
2. Edited `PlatformToolset` entries (individually for the first 3, then via a one-off PowerShell helper script for the remaining 35 to apply the identical mechanical replacement).
3. Validated each `.vcxproj` with `cppupgrade_validate_vcxproj_file` before reload.
4. Reloaded all projects.
5. Ran `cppupgrade_rebuild_and_get_issues` after each batch to confirm progress and discover the next blocking project.

## Build Results

- **Before**: 1 error (MSB8020, ResourcerDoc only reported — masked the rest)
- **After**: **0 errors**, 21 warnings across 7 projects

## Remaining Warnings (Out-of-Scope — Not Toolset-Upgrade Related)

All remaining warnings are pre-existing and unrelated to the toolset bump, per the scope rules (these codes/categories are explicitly listed as unlikely-upgrade-caused, or are generic project/linker setting mismatches that existed independent of toolset version):

- `C4018` signed/unsigned mismatch — `ResourcerDoc\ChildFrm.cpp`, `ScenePort\ChildFrame.cpp`
- `C4603` / `C4651` / `C4653` precompiled header macro/option mismatches — `NodeComplexMeshPort`, `MedusaExport`, `WindowViewPort`
- `C4002` too many macro arguments — `MedusaExport\max50\include\istdplug.h` (third-party SDK header)
- `C5208` unnamed class in typedef — `MedusaExport\max50\include\maxtypes.h` (third-party SDK header)
- `MSB8012` TargetName/TargetPath/TargetExt mismatches — `ResourcerDoc`, `MedusaExport` (pre-existing output-naming convention, e.g. `ResourcerDocD.dll` vs `ResourcerDoc.dll`, `.dle` vs `.dll`)
- `D9035` deprecated `/Gm` compiler option — `VC2M`, `UpdateBuildVersion`
- `LNK4075` ignoring `/EDITANDCONTINUE` due to `/SAFESEH` — `UpdateBuildVersion`
- `LNK4070` `.EXP` directive mismatch — `MedusaExport`

None of these were introduced by the toolset upgrade; they are pre-existing project configuration/code issues out of scope for this task.

## Done When — Verified

- `ResourcerDoc.vcxproj` and all other affected projects build successfully (Debug|Win32) with `PlatformToolset v145`.
- Full solution rebuild (`Tools.sln`) shows **0 errors**.
- No new warnings were introduced by the toolset change; all 21 remaining warnings are pre-existing/out-of-scope.
