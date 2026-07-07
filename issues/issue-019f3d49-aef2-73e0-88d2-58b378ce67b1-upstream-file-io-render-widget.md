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

### Conflict risk — highest of all 13 batches

`b8e3e7b86` alone touches 27 files, including several the fork has
substantively customized: `src/core/Boxes/svglinkbox.cpp` (fork's core
SVG-import/flipbook work — the hunk here is a mechanical refactor of
`changeSourceFile()` from `eDialogs::openFile` to
`AppSupport::getOpenFile`, not a behavior change, but the commit's
stated goal is "Replaced eDialogs with functions in AppSupport"
project-wide, so expect more `eDialogs` callsites to need the same
treatment — `grep -rn eDialogs:: src/` before merging to find them
all), `src/core/Boxes/videobox.cpp` and
`src/core/CacheHandlers/soundcachehandler.cpp` (both carry the fork's
audio-waveform-visualization feature, PRs #56/#60), and
`src/app/GUI/mainwindow.cpp` / `src/core/appsupport.cpp` (touched by
several other fork commits and by other upstream batches in this same
pull — see the meta-issue's Conflict risk section for the full list).
Also removes `src/core/GUI/edialogs.cpp`/`.h` entirely — confirm
nothing fork-specific still depends on that class before deleting it.

This batch needs a careful `git blame` pass after merging, not just a
build-and-smoke-test.

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
- `src/core/Private/document.h`, `appsupport.cpp` (via `AppSupport`
  update), `hardwareinfo.cpp`
- `src/app/presets/render/007-friction-preset-webm.conf`,
  `008-friction-preset-exr.conf`
- `src/app/resources.qrc`, `friction.qss`, `CMakeLists.txt`, `main.cpp`
- `src/core/Boxes/svglinkbox.cpp`, `videobox.cpp`,
  `src/core/CacheHandlers/soundcachehandler.cpp`,
  `src/core/GUI/edialogs.cpp`/`.h` (removed),
  `src/ui/dialogs/exportsvgdialog.cpp`, `src/app/GUI/canvaswindow.cpp`
  — see Conflict risk above

## Next steps

- Cherry-pick the 16 commits above (in order) from `upstream/main`.
- Before merging: `grep -rn eDialogs:: src/` to find any other
  fork-touched callsites the `eDialogs`-removal refactor should also
  update.
- Build and manually test rendering with the two new presets (webm,
  exr), general output-settings/render-widget behavior, SVG-link
  source-file changing, and audio-waveform display on video/sound
  boxes (to confirm the fork's waveform feature survived the refactor).
- `git blame` the merged `svglinkbox.cpp`, `videobox.cpp`,
  `soundcachehandler.cpp` to confirm no fork logic was silently
  dropped.
- Open a PR against `earlye/friction`.
