# Upstream pull: initial OpenGL ES 3.0 (GLES3) support

## Context

Part of the upstream-pull breakdown tracked in
[issue-019f3869-037a-7843-94b7-da4b8f32ee02-pull-upstream-fixes.md](issue-019f3869-037a-7843-94b7-da4b8f32ee02-pull-upstream-fixes.md).

Upstream commit `7bed5df2c` ("Initial support for OpenGL ES 3.0
(GLES3)") adds a GLES3 rendering path alongside the existing OpenGL
path. New feature, not a fix.

**This batch is a dependency for**
[upstream-misc](issue-019f3d49-af0e-7712-9466-95c9fc7e29f7-upstream-misc.md)
(`0f10e7f22`, which hides the plugins settings widget when GLES is
active — only makes sense once this lands).

## Relevant files

- `src/app/GUI/Settings/pluginssettingswidget.cpp`
- `src/app/main.cpp`
- `src/cmake/friction-common.cmake`
- `src/core/Private/Tasks/offscreenqgl33c.h`
- `src/core/etexture.cpp`
- `src/core/glhelpers.cpp` / `.h`
- `src/ui/widgets/glwidget.cpp`, `glwindow.cpp`

## Next steps

- Cherry-pick `7bed5df2c` from `upstream/main`.
- Build and manually test rendering under GLES3 if a suitable test
  environment is available (otherwise confirm existing GL path still
  builds/works unaffected).
- Open a PR against `earlye/friction`.
