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

## Root cause (confirmed)

- `Canvas::renderGizmos(SkCanvas*, qreal qInvZoom, float invZoom)`
  (`src/core/canvasgizmos.cpp:31`) draws the whole widget (rotate handle,
  translate arrows/axis lines, scale squares, uniform-scale polygon, shear
  handles).
- `Canvas::updateRotateHandleGeometry(qreal invScale)`
  (`src/core/canvasgizmos.cpp:571`) computes world-space geometry for every
  handle as `pixelConstant * invScale`, e.g.:
  ```cpp
  const qreal axisWidthWorld = mGizmos.fConfig.axisWidthPx * invScale;   // line 644
  ```
- The inverse-zoom value itself comes from `src/core/canvas.cpp:279-282`:
  ```cpp
  const qreal zoom = viewTrans.m11();
  const qreal qInvZoom = 1/viewTrans.m11() * pixelRatio;   // line 281, qreal (double) — fine
  const float invZoom = toSkScalar(qInvZoom);              // line 282 — narrowed to float32
  ```
- `renderGizmos` builds Skia paths/points using this narrowed `float`
  (`SkScalar`), e.g. `canvasgizmos.cpp:107`:
  ```cpp
  toSkScalar(mGizmos.fConfig.rotateStrokePx * qInvZoom * 0.2f)
  ```

The `1/zoom` arithmetic in `qreal` (double) does not itself become
singular or overflow at extreme zoom. The break happens after narrowing to
32-bit `SkScalar`: a tiny float offset (pixel-space constant times a very
small `invZoom`) gets added to the pivot's absolute scene coordinates
(which can be large) when building `SkPoint`/`SkPath` geometry — classic
catastrophic cancellation / precision floor in float32 (~7 significant
digits). No clamping exists anywhere in this path (`canvas.cpp`,
`canvasgizmos.cpp`, `canvasmouseevents.cpp`).

The same `invScale`/`invZoom` convention is shared with hover-outline
stroke width (`src/core/Boxes/boundingbox.cpp:240-259`,
`drawHoveredSk`/`drawHoveredPathSk`) and point-handle drawing
(`boundingbox.cpp:278-283`), and independently reimplemented for mouse hit
-testing (`src/core/canvasmouseevents.cpp:82`:
`(1 / e.fScale) * devicePixelRatio`). Those other call sites were not
audited for the same precision issue, but use the identical pattern and
may be equally susceptible.

## Impact / scope

Purely visual/interaction — the gizmo becomes hard or impossible to use
accurately at extreme zoom. Does not corrupt any stored transform data.

## Relevant files

- `src/core/canvas.cpp:279-282` — `qInvZoom`/`invZoom` computation, narrows `qreal` to `SkScalar`
- `src/core/canvasgizmos.cpp:31` — `Canvas::renderGizmos`
- `src/core/canvasgizmos.cpp:571` — `Canvas::updateRotateHandleGeometry`
- `src/core/canvasgizmos.cpp:433-460+` — `pointOnRotateGizmo`/`pointOnScaleGizmo` hit-testing, same `invScale` pattern
- `src/core/Boxes/boundingbox.cpp:240-259` — `drawHoveredSk`/`drawHoveredPathSk`, shares the convention
- `src/core/canvasmouseevents.cpp:82` — independently reimplemented inverse-scale for hit-testing

## Next steps (not yet decided)

No fix chosen yet. Likely direction: compute gizmo handle geometry in a
local/relative coordinate space near the origin (offset-only, without
adding to the large absolute pivot coordinate until the final device-space
transform), rather than building absolute-coordinate float32 geometry
directly — avoids the large-magnitude-plus-tiny-offset cancellation
pattern.
