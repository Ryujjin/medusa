# MSVC Build Tools Upgrade Plan

## Overview

**Target**: Retarget `Tools\ResourcerDoc\ResourcerDoc.vcxproj` from PlatformToolset `v143` to `v145`, matching the rest of the solution.
**Scope**: Single project, single file change (`.vcxproj`), no source code changes expected.

## Tasks

### 01-retarget-resourcerdoc

`ResourcerDoc.vcxproj` currently sets `<PlatformToolset>v143</PlatformToolset>` for both `Debug|Win32` and `Release|Win32` configurations (lines 22 and 28). The `v143` build tools are not installed in this environment, causing `MSB8020` build errors. All sibling projects in the solution (`AudioDS`, `DisplayD3D`, `DisplayGL`, `Gui3d`, `Docs`, `GCQ`, `World`, `MovieDS`) already use `v145`.

Update both `PlatformToolset` entries to `v145`. No other settings (character set, UseOfMfc, WindowsTargetPlatformVersion) need to change, since this is a toolset-version-only bump with no language standard or coroutine flags involved in this project.

**Done when**: `ResourcerDoc.vcxproj` builds successfully (Debug|Win32 and Release|Win32) with `PlatformToolset v145`, a full solution rebuild shows 0 errors, and no new warnings appear compared to the assessment baseline (0 warnings).
