# Upstream pull: default to CPU mode on raster effects

## Context

Part of the upstream-pull breakdown tracked in
[issue-019f3869-037a-7843-94b7-da4b8f32ee02-pull-upstream-fixes.md](issue-019f3869-037a-7843-94b7-da4b8f32ee02-pull-upstream-fixes.md).

Upstream commit `de226cc52` ("Default to CPU mode on raster effects")
changes several raster effects (blur, brightness/contrast, colorize,
motion blur, noise fade, shadow, wipe) to default to CPU mode, to avoid
"blank" frames on some GPU configurations. Users can still switch back
to GPU mode in preferences. Also adds a CPU-mode toggle to the Quick
Setup wizard (`quicksetup.cpp`).

**Dependency**: this commit touches `src/ui/wizards/quicksetup.cpp`,
which is created by the
[upstream-quicksetup-wizard](issue-019f3d49-aeba-7d60-aa6a-4be82f3bd0a9-upstream-quicksetup-wizard.md)
batch. Pull that batch first, or the `quicksetup.cpp` hunk in this
commit won't apply.

## Relevant files

- `src/core/RasterEffects/*.cpp` (blur, brightnesscontrast, colorize,
  motionblur, noisefade, shadow, wipe)
- `src/ui/widgets/performancesettingswidget.cpp`
- `src/ui/wizards/quicksetup.cpp`

## Next steps

- Confirm `upstream-quicksetup-wizard` has already landed on `main`.
- Cherry-pick `de226cc52` from `upstream/main`.
- Build and visually check motion blur / other raster effects still
  render correctly in CPU mode.
- Open a PR against `earlye/friction`.
