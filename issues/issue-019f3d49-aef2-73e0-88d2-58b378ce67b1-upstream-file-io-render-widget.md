# Upstream pull: file IO changes + render instance widget fixes

## Context

Part of the upstream-pull breakdown tracked in
[issue-019f3869-037a-7843-94b7-da4b8f32ee02-pull-upstream-fixes.md](issue-019f3869-037a-7843-94b7-da4b8f32ee02-pull-upstream-fixes.md).

Upstream's `b8e3e7b86` ("File IO changes + Render instance widget
fixes") and a string of follow-up commits touch render-instance/output
settings widgets, add two new render preset `.conf` files, and adjust
several broadly-shared files (theme qss, `document.h`, splash,
`main.cpp`, `CMakeLists.txt`). This batch is more of a loose "general
render/IO sweep" than one cohesive feature — treat it as one testable
unit but expect a wider diff surface than the other batches.

Commits (oldest first):

- `b8e3e7b86` — File IO changes + Render instance widget fixes
- `9280d45ee` — Update renderinstancewidget.cpp
- `957a265cd` — Update document.h
- `376b8ea7b` — Update friction.qss
- `287a5fd0d` — Create 008-friction-preset-exr.conf
- `d830cc7a3` — Create 007-friction-preset-webm.conf
- `f1964f609` — Update resources.qrc
- `f5656b224` — Update pluginssettingswidget.cpp
- `d3ce52bb4` — Update renderinstancewidget.cpp
- `a19caf14b` — Update AppSupport
- `d720c5285` — Update hardwareinfo.cpp
- `5f4a844d2` — Update canvastoolbar.cpp
- `95a899754` — Update outputsettingsdialog.cpp
- `8ebbfdb2a` — Update main.cpp
- `532d0de7d` — Update splash
- `21984e185` — Update CMakeLists.txt

## Relevant files

- `src/app/GUI/RenderWidgets/renderinstancewidget.cpp`,
  `outputsettingsdialog.cpp`
- `src/core/document.h`, `appsupport.cpp` (via `AppSupport` update),
  `hardwareinfo.cpp`
- `resources/presets/007-friction-preset-webm.conf`,
  `008-friction-preset-exr.conf`
- `resources.qrc`, `friction.qss`, `CMakeLists.txt`, `main.cpp`

## Next steps

- Cherry-pick the 16 commits above (in order) from `upstream/main`.
- Build and manually test rendering with the two new presets (webm,
  exr) plus general output-settings/render-widget behavior.
- Open a PR against `earlye/friction`.
