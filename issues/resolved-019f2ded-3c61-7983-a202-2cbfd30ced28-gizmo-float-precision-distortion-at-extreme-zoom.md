# Transform gizmo distorts at extreme canvas zoom (float32 precision loss)

## Context

Discovered while investigating issue `1148748f4eb9`. At extreme zoom
levels, the on-canvas transform manipulation widget (translate arrows,
scale-handle squares, uniform-scale yellow inverted-L, rotate handle) for
a selected box visually distorts/breaks apart. This is purely a rendering
artifact of the gizmo overlay — not a bug in the underlying box transform
or SVG-imported content (the user initially suspected this distortion was
the same thing as the `1148748f4eb9` content-offset symptom; it is not —
they are unrelated, and this is being tracked separately).

User's own hypothesis, confirmed by code trace: the gizmo is drawn at a
constant on-screen size regardless of canvas zoom by scaling its geometry
by the inverse of the zoom level; at extreme zoom, the floating-point math
for that inverse scale breaks down.

## Root cause (confirmed, reframed as architectural — see Grill Log 2026-07-06)

The float32-narrowing symptom described in the original trace is real, but
it's a downstream *consequence* of a design choice, not the root defect
itself: the gizmo is built entirely in **scene/world coordinates**, scaled
by the inverse of the zoom so it nets out to a constant on-screen size —
rather than being drawn in screen/device space and simply *placed* at the
projected pivot.

Confirmed call sequence in `src/core/canvas.cpp`:

- `canvas->concat(skViewTrans);` (line 310) applies the pan/zoom transform
  (a float32 `SkMatrix`) to the canvas.
- `renderGizmos(canvas, qInvZoom, invZoom);` (line 446) runs *after* that
  concat and *before* the only `canvas->resetMatrix()` call in the
  function (line 559) — so the entire gizmo is drawn while still inside
  the world-space transform.
- `Canvas::updateRotateHandleGeometry(qreal invScale)`
  (`src/core/canvasgizmos.cpp:571`) computes every handle's shape as
  `pixelConstant * invScale` added to the world-space `pivot`, e.g.:
  ```cpp
  const qreal axisWidthWorld = mGizmos.fConfig.axisWidthPx * invScale;   // line 644
  ```
- `renderGizmos` then narrows those absolute world-space `qreal` points to
  32-bit `SkScalar` when building `SkPoint`/`SkPath` geometry, e.g.
  `canvasgizmos.cpp:107`. Skia's already-concatenated view matrix then
  re-multiplies by `zoom` to project back to device pixels.

So the gizmo does two reciprocal-scaling passes (`* invZoom` then, via the
view matrix, `* zoom`) over a potentially huge dynamic range, just to net
out to "N screen pixels." The float32 narrowing after the first pass is
where that round-trip actually loses the tiny offset against the large
absolute pivot coordinate (classic catastrophic cancellation, ~7
significant digits). No clamping exists anywhere in this path.

**Corrected scope** — traced and confirmed NOT equally susceptible,
contrary to the original draft of this issue:

- Hit-testing (`pointOnRotateGizmo`, `pointOnScaleGizmo`,
  `pointOnAxisGizmo`, `pointOnShearGizmo`, `canvasgizmos.cpp:433-460+`)
  reads only the cached double-precision `mGizmos.fState.*Geom` fields and
  never calls `toSkScalar()`. It stays in `qreal` end-to-end, so hit
  areas remain accurate even when the drawn shape shatters — the
  practical impact is "user can't see where to click," not "clicking
  doesn't work."
- `src/core/canvasmouseevents.cpp:82`'s independently-computed
  `invScaleUi` only feeds that same double-precision hit-test path — not
  vulnerable either.
- `BoundingBox::drawHoveredSk`/`drawHoveredPathSk`
  (`src/core/Boxes/boundingbox.cpp:240-259`) uses a different, safe
  pattern: it draws a *relative* path (`mSkRelBoundingRectPath`) through
  one transform matrix, using `invScale` only as a stroke-width scalar —
  never adding a tiny offset onto a large absolute coordinate before
  narrowing. Not the same bug.

**Sibling call sites confirmed to share the actual vulnerable pattern**
(pixel constant `* invZoom` added to an absolute world-space point, still
inside the concatenated zoom transform, in `src/core/canvas.cpp`) — traced
by inspection, not yet reproduced, and deliberately **out of scope for
this issue**; tracked as a follow-up (see Next steps):

- `mHoveredPoint_d->drawHovered(canvas, invZoom)` (line 542)
- `mHoveredNormalSegment.drawHoveredSk(canvas, invZoom)` (line 545)
- `mRotPivot->drawTransforming(...)` / `drawSk(...)` (lines 453–456)
- `cam->drawCameraBox(canvas, invZoom, ...)` (line 441)
- draw-path node circles (lines 459–514)

## Impact / scope

Purely visual — the gizmo's drawn shape distorts/breaks apart at extreme
zoom. Hit-testing/interaction remains accurate (see corrected root cause
above); the practical impact is that the user can't see where to click,
not that clicks land wrong. Does not corrupt any stored transform data.
Scoped to the transform gizmo only for this issue.

## Relevant files

- `src/core/canvas.cpp:310` — `canvas->concat(skViewTrans)`, world-space transform applied before gizmo draw
- `src/core/canvas.cpp:446` — `renderGizmos(...)` call site, still inside the world-space transform
- `src/core/canvas.cpp:559` — the one `canvas->resetMatrix()` in this function, happens *after* gizmos are drawn
- `src/core/canvas.cpp:279-282` — `qInvZoom`/`invZoom` computation, narrows `qreal` to `SkScalar`
- `src/core/canvasgizmos.cpp:31` — `Canvas::renderGizmos`
- `src/core/canvasgizmos.cpp:571` — `Canvas::updateRotateHandleGeometry`
- `src/core/gizmos.h` — defines the `mGizmos.fState.*Geom` fields; confirmed only read within `canvasgizmos.cpp` (render + hit-test), so free to restructure without touching other files

## Next steps (decided — see Grill Log 2026-07-06)

Architectural fix: draw the gizmo in device/screen space instead of
world/scene space.

1. Extract each handle's shape into a single source of truth: a list of
   dimensionless pixel offsets (currently baked as literal
   `pivot + QPointF(10.0 * invScale, ...)`-style polygons in
   `updateRotateHandleGeometry`). Apply it two ways:
   - hit-testing (unchanged behavior): `offset * invScale + worldPivot`
   - rendering (new): `offset + screenPivot`, no `invScale` multiply at all
   This avoids maintaining two copies of every handle's polygon literals.
2. In `renderGizmos`, project the pivot to device/screen space **once**
   per frame (single-point transform — any float32 error there is a
   sub-pixel placement wobble, not a shape-shattering one), then bracket
   the gizmo draw in `canvas->save()` / `canvas->resetMatrix()` / ... /
   `canvas->restore()` and build all handle geometry from raw pixel
   constants offset from that projected point.
3. Add a new `lcGizmo` `QLoggingCategory` (none currently exists —
   `canvasgizmos.cpp` has zero `qCDebug` calls; `lcCanvas` is a general
   catch-all). Since there's no known reproduction zoom level (see Grill
   Log), this is required, not optional: log `invZoom`, pivot, and
   computed geometry per frame so the actual precision-loss threshold can
   be found by zooming in during manual testing in the debug build.
4. Add an automated unit test for the extracted pure offset-projection
   function, asserting it stays non-degenerate at an extreme `invZoom`
   where the old world-space math would collapse. This is the first unit
   test in this repository — no test infrastructure exists yet (no
   `QTest`, no `enable_testing()`/`add_test()` anywhere). Bootstrap it
   properly via the Qt Test framework (`find_package(Qt Test)`,
   `QTEST_MAIN`/`QTest` macros) as the project's test framework going
   forward, rather than a minimal standalone harness.
5. Verification is otherwise manual/visual in the debug build — no known
   exact repro zoom/scene setup exists (discovered incidentally while
   investigating `1148748f4eb9`, confirmed only by code trace and an
   unrecorded eyeballed zoom level).
6. Once this pattern is proven here, run `md-issue-track` to file a
   separate follow-up issue for the sibling fixed-size-overlay widgets
   listed above (hovered point, hovered normal segment, rotation pivot
   draw, camera box, draw-path nodes) — do not expand this issue's scope
   to cover them now.

## Grill Log

### 2026-07-06

- Q: is the gizmo scaled to match scene coordinates before drawing (rather
  than defined in screen coordinates and just placed)? — A: yes, confirmed
  by tracing `canvas.cpp` (`concat(skViewTrans)` at line 310 precedes the
  `renderGizmos` call at line 446; `resetMatrix()` only happens at line
  559, after gizmos are drawn). This reframes the root cause from "a
  narrowing bug" to "the wrong coordinate space entirely."
- Q: given that, pursue the architectural fix (draw in screen space) or a
  minimal patch (keep world-space design, reorder arithmetic to avoid
  cancellation)? — A: architectural fix, for sure.
- Q: the same `pixelConstant * invZoom`-added-to-absolute-world-point
  pattern is used by several other overlays in `canvas.cpp` (hovered
  point, hovered normal segment, rotation pivot, camera box, draw-path
  nodes) — expand this issue's scope to cover them, or stay scoped to the
  gizmo and track the rest separately? — A: stay scoped to the gizmo; log
  a related follow-up issue for the others once this fix proves the
  pattern.
- Q: extract each handle's shape into a single dimensionless-offset
  source of truth (applied via `*invScale` for hit-testing,
  no-multiply for screen-space rendering), or accept duplicated polygon
  literals between the hit-test and render paths? — A: yes, extract it.
- Q: is there a known concrete zoom level / scene setup that reproduces
  this? — A: no; discovered incidentally while investigating
  `1148748f4eb9`, confirmed by code trace, only eyeballed at an unrecorded
  zoom. Visual confirmation will suffice for verifying the fix, which
  makes the new `lcGizmo` logging category required (not optional) —
  it's how the actual precision-loss threshold will get found.
- Q: do you want an automated unit test for the extracted pure
  offset-projection function, or is manual visual confirmation enough? —
  A: yes, add a unit test.
- Q: since this repo has zero existing test infrastructure, how should
  that test be wired up (minimal CTest target, Qt Test framework, or an
  unwired standalone debug tool)? — A: Qt Test framework — adopt it
  properly as the project's test framework going forward.
