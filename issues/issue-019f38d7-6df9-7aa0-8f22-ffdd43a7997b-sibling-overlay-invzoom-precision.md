# Other canvas overlays may share the gizmo's extreme-zoom precision bug

## Context

`issue-019f2ded-3c61-7983-a202-2cbfd30ced28` (resolved) fixed the
transform gizmo distorting at extreme canvas zoom. Root cause: the
gizmo built its handle geometry in world/scene coordinates
(`pixelConstant * invZoom`, added to an absolute pivot), then narrowed
that to float32 (`SkScalar`) for drawing — at extreme zoom this loses
precision via catastrophic cancellation (a large absolute pivot
coordinate absorbs the tiny scaled offset once narrowed), shattering
the drawn shape. Hit-testing was unaffected (it stayed in `qreal`
throughout).

The fix draws the gizmo in device/screen space instead: project the
pivot to screen space once per frame, then build handle geometry from
raw pixel constants scaled only by `devicePixelRatio` — no `invZoom`
multiply at all, since the reciprocal-scaling round-trip (`* invZoom`
then, via the view matrix, `* zoom`) was the actual defect, not merely
the narrowing.

Several other overlays drawn in `src/core/canvas.cpp` use the
identical vulnerable pattern — a pixel constant multiplied by
`invZoom` and added to an absolute world-space point, still inside the
canvas's concatenated zoom transform (`canvas->concat(skViewTrans)`,
applied before any of these draw calls and not reset until after).
That's the exact shape of the bug the gizmo had. They were
deliberately left out of the gizmo fix's scope, to be investigated
separately once the pattern was proven — which it now has been.

**These have not yet been reproduced or confirmed as actually
distorting** — they were identified purely by pattern-matching during
code tracing while investigating the gizmo issue. Confirming each one
actually exhibits the bug (and isn't, like `BoundingBox::drawHoveredSk`
turned out to be, using a different and safe pattern) is the first
step here.

Also worth checking as prior art: while fixing the gizmo issue, an
unrelated-but-related bug was found and fixed in the same PR —
`canvas.cpp`'s `pixelRatio` computation used
`qApp->devicePixelRatio()` (the system-wide max DPR across all
connected screens) instead of the correct per-window value
(`Canvas::mDevicePixelRatio`, set via `Canvas::setWorldToScreen`),
causing wrong sizing on mixed-DPI multi-monitor setups. That's now
fixed for the gizmo and the general `qInvZoom` computation, so it does
*not* need to be part of this issue — but if any of the overlays below
turn out to have their own independent DPR-sourcing logic, it's worth
checking they don't have the same mistake.

## Relevant files

- `src/core/canvas.cpp` — `mHoveredPoint_d->drawHovered(canvas, invZoom)` (~line 542 as of the gizmo fix; line numbers may have shifted)
- `src/core/canvas.cpp` — `mHoveredNormalSegment.drawHoveredSk(canvas, invZoom)` (~line 545)
- `src/core/canvas.cpp` — `mRotPivot->drawTransforming(...)` / `mRotPivot->drawSk(...)` (~lines 453-456)
- `src/core/canvas.cpp` — `cam->drawCameraBox(canvas, invZoom, ...)` (~line 441)
- `src/core/canvas.cpp` — draw-path node circles (~lines 459-514)
- `src/core/Boxes/boundingbox.cpp:240-259` — `BoundingBox::drawHoveredSk`/`drawHoveredPathSk`, already investigated and confirmed **not** vulnerable (draws a relative path through one transform matrix; `invScale` is only used as a stroke-width scalar, never added to an absolute coordinate before narrowing) — do not include this one
- `src/core/canvasgizmos.cpp` — the resolved gizmo fix, useful as a reference implementation for the screen-space pattern
- `src/core/canvas.cpp:274-284` — `Canvas::renderSk`'s corrected `pixelRatio`/`mDevicePixelRatio` handling, useful as reference if any sibling overlay needs the same DPR-source fix

## Next steps

- For each of the five call sites above, reproduce at extreme zoom
  (in vs. out) and confirm whether the drawn shape actually distorts,
  the same way the gizmo did.
- For whichever ones confirm as vulnerable, apply the same fix
  pattern: draw in device/screen space (project the relevant anchor
  point once, build geometry from raw pixel constants scaled by
  `devicePixelRatio`, no `invZoom`) rather than world space.
- For whichever ones don't confirm as vulnerable, document why (as was
  done for `BoundingBox::drawHoveredSk`) so the pattern-matching isn't
  re-litigated later.
