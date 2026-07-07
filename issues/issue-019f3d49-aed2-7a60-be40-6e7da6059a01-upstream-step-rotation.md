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

### Conflict risk

`canvasmouseinteractions.cpp` is a real overlap, not a clean miss: the
fork touched it for the lock-icon-flash feature (`09da7b7e0`) and a
camera-view-matrix fix (`950a1d3d4`). `grid.cpp` overlaps too, but only
via the fork's mechanical qDebug→qCDebug conversion (`98c52ecd2`) — low
risk there. Review the `canvasmouseinteractions.cpp` merge carefully;
it's plausible but not guaranteed to be a clean textual merge.

## Next steps

- Cherry-pick `cfe2d62d2` then `ebee130d5` from `upstream/main`.
- `git blame` the merged `canvasmouseinteractions.cpp` to confirm the
  lock-icon-flash and camera-matrix fixes are both still intact.
- Build and manually test rotation with Ctrl/Shift held during drag,
  plus the lock-icon-flash and camera-view behaviors.
- Open a PR against `earlye/friction`.
