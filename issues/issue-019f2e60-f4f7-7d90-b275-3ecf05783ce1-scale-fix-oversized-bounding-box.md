# Bounding box incredibly oversized after zero-scale gizmo fix

## Context

While manually testing the fix for
`issues/resolved-019f2ded-3c57-7ae1-af77-66b971d67224-scale-permanently-locked-at-zero.md`
(PR #79) via `just run-debug-66b971d67224`, the user confirmed the
scale gizmo is no longer permanently locked at 0 — but observed a new
problem: the box's bounding box is now incredibly oversized.

Not yet known whether this is:
- A pre-existing bug in bounding-box computation that was simply masked
  while the box was stuck at scale 0 (and is now exposed once the
  gizmo can move the scale again), or
- A side effect of the 0.1 seed value introduced by the fix in
  `src/core/Animators/qrealanimator.cpp`
  (`QrealAnimator::multSavedValueToCurrentValue`).

No logs, screenshots, or reproduction steps captured yet.
