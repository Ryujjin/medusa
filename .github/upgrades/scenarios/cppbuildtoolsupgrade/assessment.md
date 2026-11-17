# MSVC Build Tools Upgrade — Assessment

## Summary

The solution's MSVC Build Tools are already at the latest version for most projects (PlatformToolset `v145`). The retargeting wizard confirmed: "MSVC Build Tools are already up to date, no need to retarget the solution."

A full rebuild of `Tools\Tools.sln` was performed to check for build issues. Result: **1 error, 0 warnings**, across 1 project.

## In-Scope Issues (caused by / blocking the build tools upgrade)

### 1. ResourcerDoc.vcxproj — MSB8020 — Missing v143 build tools

- **Project**: `F:\Projects\Space\VC2026-DX11-WORKING_rev02-Claude - Copy\medusa\Tools\ResourcerDoc\ResourcerDoc.vcxproj`
- **Platform Toolset**: `v143` (line 22 and 28 of the .vcxproj)
- **Error**:
  ```
  MSB8020 The build tools for Visual Studio 2022 (Platform Toolset = 'v143') cannot be found.
  To build using the v143 build tools, please install Visual Studio 2022 build tools.
  Alternatively, you may upgrade to the current Visual Studio tools by selecting the Project menu
  or right-click the solution, and then selecting "Retarget solution".
  ```
  Location: `D:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Microsoft\VC\v180\Microsoft.CppBuild.targets (473, 5)`
- **Root cause**: `ResourcerDoc.vcxproj` still references the older `v143` platform toolset, while sibling projects in the solution (e.g. `AudioDS`, `DisplayD3D`, `World`, `GCQ`, `Gui3d`, `Docs`, `MovieDS`) have already been retargeted to `v145`. The `v143` build tools are not installed in this VS 2026 (Community) environment, causing the build to fail.
- **Classification**: In-scope — directly blocks building with the latest installed MSVC build tools. Fix: retarget `ResourcerDoc.vcxproj` to `v145` (consistent with the rest of the solution), verify Windows Target Platform Version compatibility, and rebuild to confirm no new issues are introduced by the toolset bump.

## Out-of-Scope Issues

None found. The rebuild reported only the single MSB8020 error above; no other errors or warnings were present in the solution at this time. (Note: `DirectXTK_Desktop_2019*` and `xwbtool_Desktop_2019*` projects still use `v142`, but they are not part of `Tools.sln`'s build and were not reported as issues in this rebuild — these will be re-checked after the fix to ensure nothing changes.)

## Notes

- Other projects in the solution already use `v145`, confirming this is a localized/missed retarget on `ResourcerDoc.vcxproj`.
- `DirectXTK_Desktop_2022*.vcxproj` and `xwbtool_Desktop_2022.vcxproj` use `v143` intentionally (named for VS2022/Desktop 2022 configs) — these are separate project variants, not part of this solution's active build per the rebuild report, and will be left untouched unless rebuild evidence shows otherwise.

## Next Step

Proceed to Planning stage to investigate the exact change needed in `ResourcerDoc.vcxproj` (PlatformToolset version, and any related settings), then fix and verify with a rebuild.
