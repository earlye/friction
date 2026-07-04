# Scale gizmo is permanently locked at exactly 0 (multiplicative update)

## Context

Discovered while investigating issue `1148748f4eb9` (ruby's arms not
flipbooking/following properly). A box whose current `scale.x`/`scale.y`
is exactly `0` (e.g. from the stale-track corruption tracked in
`issue-019f2dd6-...-set-source-file-resets-element-track-to-zero.md`, or
from a user directly typing `0` into a scale field) can never be scaled
away from `0` using the on-canvas scale gizmo — translation and rotation
gizmos work fine on the same box, but dragging any scale handle produces
no visible change no matter how far or which direction it's dragged.

## Root cause (confirmed)

Traced the full drag chain:

- `Canvas::startScaleConstrainedMove` (`src/core/canvasgizmos.cpp:919-950`)
  starts the drag, `mStartTransform = true`.
- `Canvas::scaleSelected` (`src/core/canvasmouseinteractions.cpp:758-792`)
  computes a multiplier from total drag distance since mouse-press:
  ```cpp
  qreal scaleBy = 1 + distSign({distMoved.x(), -distMoved.y()}) * 0.003;
  ```
- `Canvas::scaleSelectedBy` (`src/core/canvasselectedboxesactions.cpp:403-431`)
  calls `box->startScaleTransform()` once, then `box->scale(...)` /
  `box->scaleRelativeToSavedPivot(...)` repeatedly per box in
  `mSelectedBoxes`, using the same multiplier for every box.
- `BasicTransformAnimator::scale` (`src/core/Animators/transformanimator.cpp:127-129`):
  ```cpp
  void BasicTransformAnimator::scale(const qreal sx, const qreal sy) {
      mScaleAnimator->multSavedValueToCurrentValue(sx, sy);
  }
  ```
- `QrealAnimator::multSavedValueToCurrentValue`
  (`src/core/Animators/qrealanimator.cpp:689-691`):
  ```cpp
  void QrealAnimator::multSavedValueToCurrentValue(const qreal multBy) {
      setCurrentBaseValue(mSavedCurrentValue * multBy);
  }
  ```
  `mSavedCurrentValue` is captured once at drag-start
  (`prp_startTransform()`, `qrealanimator.cpp:729`) and held fixed for the
  whole drag.

New scale = `startScale * (1 + dragDistance*0.003)`. If `startScale == 0`,
the result is `0` for any drag distance/direction — a permanent lock with
no escape via that gizmo. Confirmed per-box in multi-select: siblings with
nonzero starting scale scale normally; only a box already at exactly `0`
gets stuck.

## Impact / scope

Any box whose scale is exactly `0` (however it got there) cannot be
rescaled via the on-canvas gizmo. The scale-value text field (if it allows
direct numeric entry) may be the only escape route, if it uses an
absolute set rather than the same multiplicative update.

## Relevant files

- `src/core/Animators/qrealanimator.cpp:689-691` — `multSavedValueToCurrentValue`
- `src/core/Animators/qrealanimator.cpp:729` — `prp_startTransform` saving `mSavedCurrentValue`
- `src/core/Animators/transformanimator.cpp:127-129` — `BasicTransformAnimator::scale`
- `src/core/Animators/transformanimator.cpp:275-288` — `scaleRelativeToSavedPivot` (global-pivot mode, same underlying formula)
- `src/core/canvasselectedboxesactions.cpp:403-431` — `Canvas::scaleSelectedBy`
- `src/core/canvasmouseinteractions.cpp:758-792` — `Canvas::scaleSelected`
- `src/core/canvasgizmos.cpp:919-950` — `Canvas::startScaleConstrainedMove`

## Next steps (not yet decided)

Possible fixes, none chosen yet:
- Have the scale drag fall back to treating a zero saved value as a small
  nonzero seed (e.g. `1`) so the multiplier has something to act on.
- Have the scale value field/UI reject or clamp direct entry of exactly
  `0` in the first place, preventing the trap from being reachable at all.
