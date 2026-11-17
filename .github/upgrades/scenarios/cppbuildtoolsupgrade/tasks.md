# CppBuildToolsUpgrade Progress

## Overview

Upgrading the MSVC Build Tools for the Tools solution. One project (`ResourcerDoc`) was missed in a prior retarget and still references the unavailable `v143` toolset; it will be bumped to `v145` to match the rest of the solution.

**Progress**: 1/1 tasks complete <progress value="100" max="100"></progress> 100%

## Tasks

- ✅ 01-retarget-resourcerdoc: Retarget ResourcerDoc.vcxproj to v145 ([Content](tasks/01-retarget-resourcerdoc/task.md), [Progress](tasks/01-retarget-resourcerdoc/progress-details.md))
