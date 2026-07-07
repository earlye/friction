# Upstream pull: step-rotation UX (Ctrl/Shift)

## Context

Part of the upstream-pull breakdown tracked in
[issue-019f3869-037a-7843-94b7-da4b8f32ee02-pull-upstream-fixes.md](issue-019f3869-037a-7843-94b7-da4b8f32ee02-pull-upstream-fixes.md).

Upstream adds step-rotation while dragging: Ctrl snaps to 1° steps,
Shift to 15°, configurable in grid settings.

Commits (oldest first):

- `cfe2d62d2` — UX: support step rotation (Ctrl/Shift)
- `ebee130d5` — Update canvasmouseinteractions.cpp (follow-up tweak)

## Relevant files

- `src/core/canvasmouseinteractions.cpp`
- `src/core/grid.cpp` / `.h`
- `src/ui/widgets/toolinteract.cpp`

No overlap with fork-specific code.

## Next steps

- Cherry-pick `cfe2d62d2` then `ebee130d5` from `upstream/main`.
- Build and manually test rotation with Ctrl/Shift held during drag.
- Open a PR against `earlye/friction`.
