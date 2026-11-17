# CppBuildToolsUpgrade — Report

**Scenario:** Retarget `ResourcerDoc.vcxproj` PlatformToolset from v143 to v145 to match the rest of the solution.
**Outcome:** ✅ Fully completed
**Projects affected:** 40
**Tasks:** 1/1 completed

---

## Summary

The `Tools.sln` solution's MSVC Build Tools were already at the latest version (`v145`) for most projects, but a full rebuild revealed that **40 `.vcxproj` files** under `Tools\` were still pinned to `PlatformToolset v143`, whose build tools are not installed in this VS 2026 environment. Each blocking project was retargeted to `v145`, resolving all `MSB8020` build errors. The initial assessment only identified `ResourcerDoc.vcxproj` because MSBuild only reports the first project in build order that fails — fixing each one revealed the next, ultimately expanding scope from 1 to 40 projects. The solution now builds with **0 errors**.

---

## What Changed

### Project File Changes

All 40 affected `.vcxproj` files had `<PlatformToolset>v143</PlatformToolset>` changed to `<PlatformToolset>v145</PlatformToolset>` for both `Debug|Win32` and `Release|Win32` configurations. No other project settings (character set, UseOfMfc, WindowsTargetPlatformVersion, compiler/linker flags) were modified, and no source code was changed.

| Project | Change |
|---------|--------|
| ResourcerDoc | v143 → v145 |
| VC2M | v143 → v145 |
| Docs | v143 → v145 |
| CollectionPort | v143 → v145 |
| FontPort | v143 → v145 |
| ImagePort | v143 → v145 |
| InterfaceContextPort | v143 → v145 |
| MakeUN | v143 → v145 |
| MaterialPort | v143 → v145 |
| MedusaExport | v143 → v145 |
| MusicPort | v143 → v145 |
| NodeBlurPort | v143 → v145 |
| NodeBooleanPort | v143 → v145 |
| NodeBooleanTimePort | v143 → v145 |
| NodeComplexMeshPort | v143 → v145 |
| NodeDecalPort | v143 → v145 |
| NodeDetailPort | v143 → v145 |
| NodeFlarePort | v143 → v145 |
| NodeGlarePort | v143 → v145 |
| NodeLightPort | v143 → v145 |
| NodeMeshPort | v143 → v145 |
| NodeParticleField | v143 → v145 |
| NodeParticleSystem2Port | v143 → v145 |
| NodeParticleSystemPort | v143 → v145 |
| NodePathPort | v143 → v145 |
| NodeScalePort | v143 → v145 |
| NodeScenePort | v143 → v145 |
| NodeSoundPort | v143 → v145 |
| NodeSpinPort | v143 → v145 |
| NodeSpritePort | v143 → v145 |
| NodeWindowPort | v143 → v145 |
| ProfilerClient | v143 → v145 |
| Resourcer | v143 → v145 |
| ScenePort | v143 → v145 |
| SoundPort | v143 → v145 |
| TextPort | v143 → v145 |
| UpdateBuildVersion | v143 → v145 |
| WindowButtonPort | v143 → v145 |
| WindowStylePort | v143 → v145 |
| WindowTextPort | v143 → v145 |
| WindowViewPort | v143 → v145 |

### Git Commits

No commits were made during this scenario — all changes remain uncommitted in the working tree (`.vcxproj` edits only). Per workflow policy, commits to `develop` were not made automatically; the user can review and commit the changes themselves.

---

## Task Breakdown

| Task | Description | Outcome | Content | Details |
|------|-------------|---------|---------|---------|
| `01-retarget-resourcerdoc` | Retarget ResourcerDoc.vcxproj (and 39 other discovered v143 projects) to v145 | ✅ All 40 projects now build with 0 errors | [task.md](tasks/01-retarget-resourcerdoc/task.md) | [progress-details.md](tasks/01-retarget-resourcerdoc/progress-details.md) |

---

## Decisions Made

- **Expanded scope beyond the original plan (ResourcerDoc only) to all 40 discovered `v143` projects** — the assessment only surfaced one project because MSBuild halts/reports per build-order position; subsequent rebuilds revealed more, so all were fixed under the same task rather than re-planning for each one individually.
- **Commit strategy**: "After Each Task" was configured, but with only one task, no intermediate commit was created; changes remain uncommitted pending user review.

---

## Build & Test Results

| Project | Build | Warnings |
|---------|-------|----------|
| Tools.sln (all 40 retargeted + existing projects) | ✅ 0 errors | 21 (all pre-existing/out-of-scope) |

Remaining warnings (unrelated to the toolset upgrade, left untouched per scope rules):
- `C4018` signed/unsigned mismatch — `ResourcerDoc\ChildFrm.cpp`, `ScenePort\ChildFrame.cpp`
- `C4603` / `C4651` / `C4653` precompiled header macro/option mismatches — `NodeComplexMeshPort`, `MedusaExport`, `WindowViewPort`
- `C4002` too many macro arguments — `MedusaExport\max50\include\istdplug.h` (third-party SDK header)
- `C5208` unnamed class in typedef — `MedusaExport\max50\include\maxtypes.h` (third-party SDK header)
- `MSB8012` TargetName/TargetPath/TargetExt mismatches — `ResourcerDoc`, `MedusaExport`
- `D9035` deprecated `/Gm` compiler option — `VC2M`, `UpdateBuildVersion`
- `LNK4075` ignoring `/EDITANDCONTINUE` due to `/SAFESEH` — `UpdateBuildVersion`
- `LNK4070` `.EXP` directive mismatch — `MedusaExport`

---

## Known Gaps & Follow-up Items

- **Changes are uncommitted** — review the 40 modified `.vcxproj` files and commit when ready.
- **Pre-existing warnings not addressed** — the 21 warnings listed above existed before this upgrade and were intentionally left out of scope; consider a follow-up cleanup pass if desired.
- **DirectXTK_Desktop_2019\* / xwbtool_Desktop_2019\* projects** still use `v142` — these are separate build variants not part of `Tools.sln`'s active build and were not touched.
