# Bounding box incredibly oversized after zero-scale gizmo fix

## Context

While manually testing the fix for
`issues/resolved-019f2ded-3c57-7ae1-af77-66b971d67224-scale-permanently-locked-at-zero.md`
(PR #79) via `just run-debug-66b971d67224`, the user confirmed the
scale gizmo is no longer permanently locked at 0 — but observed a new
problem: the box's bounding box is now incredibly oversized.

Two hypotheses going in:
- A pre-existing bug in bounding-box computation that was simply masked
  while the box was stuck at scale 0 (and is now exposed once the
  gizmo can move the scale again), or
- A side effect of the 0.1 seed value introduced by the fix in
  `src/core/Animators/qrealanimator.cpp`
  (`QrealAnimator::multSavedValueToCurrentValue`).

## Root cause (confirmed) — two independent bugs

Reproduced by linking `ruby.svg` into a brand-new scene (no gizmo drag
involved at all), which ruled out the scale-seed hypothesis: the
oversized box appeared on link/import, before any interaction with the
scale gizmo. `friction.animator=true` logging showed zero calls to
`multSavedValueToCurrentValue` during the repro.

### Bug 1: pivot-desc move doubles the rotation's pivot-offset translate

`SvgLinkBox::applyPivotDescIfPresent` (`src/core/Boxes/svglinkbox.cpp`)
lets an artist mark a pivot point in the source SVG via a `<circle>`
tagged `kind: pivot` (`src/core/svgimporter.cpp:526-534`,
`svgPivotPos` property). When resolving that marker onto a target box,
it did:

```cpp
transformAdv->getPivotAnimator()->setBaseValue(pivot);
```

This overwrites the box's pivot but leaves `posX`/`posY` untouched.
Those position values came from `MatrixDecomposition::decompose()` at
import time, which always assumes `pivot == (0,0)` and bakes the
rotation's pivot-offset directly into `fMoveX`/`fMoveY`
(`src/core/matrixdecomposition.cpp:31-32, 66-67`). Reconstructing the
transform via `AdvancedTransformAnimator::valuesToMatrix()`
(`src/core/Animators/transformanimator.cpp:527-534`,
`translate(pivot+pos)·rotate·scale·shear·translate(-pivot)`) with the
new pivot on top of the untouched `pos` applies the rotation's
pivot-offset a second time. Verified algebraically: the extra term
introduced is exactly `2 × (original pivot-compensation translate)`,
matching the measured ~2x growth in `left-arm`/`left-arm-pointing`'s
`dx`/`dy` in `ruby.svg` (a group with an off-origin `rotate(angle,cx,cy)`
transform that also gets a pivot-desc applied).

**Fix:** use `MatrixDecomposition::decomposePivoted()` (which already
existed for exactly this purpose) to re-derive position for the new
pivot instead of setting the pivot animator directly.

### Bug 2: empty flipbook-follower groups pollute ancestor bounding boxes

`ruby.svg` has `kind: flipbook-follower` wrapper groups (e.g.
`left-arm-overlap` → `left-arm-overlap-flipbook`) whose only child
(`left-arm-overlap-holla`) is `display:none` by default and only shown
for one flipbook page (`map: {0: null, 1: "...-holla", 2: null}`). At
page 0, the child is correctly hidden via `setVisibleFromAnimation(false)`
(`src/core/Boxes/svglinkbox.cpp:290`), but the *wrapping* groups stay
visible with zero visible content. `ContainerBox::updateRelBoundingRect()`
(`src/core/Boxes/containerbox.cpp:615-632`) then computes their rect as
Skia's empty-path default, `(0,0,0,0)` — and that literal origin point
still gets unioned into every ancestor's bounding box, anchoring it to
`(0,0)` regardless of where the real content sits.

This bug predates the PR #79 scale fix and existed independently; it was
just dwarfed by bug 1's much larger displacement, so nobody noticed until
bug 1 was fixed and the box shrank enough to reveal it.

**Fix:** skip children with a zero-area rect (`width() <= 0 && height() <= 0`)
when computing the bounding-path union in `updateRelBoundingRect()`.

## Verification

After both fixes, linking `ruby.svg` into a fresh scene produces a
`Ruby` bounding box of `21.79 x 37.09` (canvas units), matching the
SVG's own declared size of `20.618134mm x 36.673035mm`.

## Relevant files

- `src/core/Boxes/svglinkbox.cpp` — `applyPivotDescIfPresent` (bug 1 fix)
- `src/core/matrixdecomposition.cpp` / `.h` — `decomposePivoted`,
  `setPivotKeepTransform` (pre-existing helper, now used)
- `src/core/Animators/transformanimator.cpp:527-534` — `valuesToMatrix`
- `src/core/Boxes/containerbox.cpp:615-632` — `updateRelBoundingRect` (bug 2 fix)
- `src/core/svgimporter.cpp:1527-1537` — decompose call site, now warns if
  a decomposed scale exceeds 10x (diagnostic added during investigation,
  did not end up being the trigger but kept as a future tripwire)
- `src/core/Animators/qrealanimator.cpp:689-696` —
  `multSavedValueToCurrentValue`, now logs seed/multBy/result under
  `friction.animator` (diagnostic added during investigation, ruled this
  function out but kept for future scale-drag debugging)
