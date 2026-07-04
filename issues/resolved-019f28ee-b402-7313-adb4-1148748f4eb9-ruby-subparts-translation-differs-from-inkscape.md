# Ruby subparts translation differs from Inkscape (SVG transform regex drops scientific notation)

## Context

Originally reported as "ruby's arms not flipbooking/following properly,"
narrowed down to: `right-arm-flipbook` index 0 (`right-arm-pointing`)
rendered with visibly wrong translation for its `path2164-1`/`g33`
sub-part — appeared to rotate around the correct pivot but at a larger
radius than expected. Index 3 (`right-arm-pointing-2`, a duplicate where
the user had ungrouped-then-regrouped the same content in Inkscape) looked
correct.

Investigation ruled out several plausible-but-wrong hypotheses along the
way (see Grill Log) before finding the actual cause.

## Root cause (confirmed)

`REGEX_FLOAT` (`src/core/regexhelpers.h:30`) was:

```cpp
#define REGEX_FLOAT "(-?[\\.|\\d]+)"
```

This matches an optional `-`, then a run of `.`/digits — no support for
scientific/exponential notation (`e`/`E`, exponent sign, exponent digits).

`g33`'s SVG transform (`ruby.svg`) is `translate(-4.9683458,-2.6458333e-4)`.
`extractTranslation` (`src/core/svgimporter.cpp:619-640`) requires
`QRegExp::exactMatch()` against the *whole* `translate(...)` string. The
regex can only match `-2.6458333` of the second coordinate — it stops at
`e` — so the full-string match fails, `extractTranslation` returns
`false`, and the entire translate is silently dropped: **both**
coordinates, not just the one using exponential notation.

Confirmed via a debug build (`just run-debug-1148748f4eb9`, tracing added
to `BoundingBox::getRelativeTransformAtFrame` gated by
`friction.box.pivot`, kept as permanent diagnostics per this session's
methodology): `g33`'s computed relative transform showed `dx=0 dy=0`
despite its SVG-declared translate, while the left arm's structurally
identical `path2164` (`transform="translate(-0.54388439)"`, no scientific
notation) correctly showed `dx=-0.543884`.

This went unnoticed everywhere else in `ruby.svg` because the file is full
of tiny Inkscape-rounding artifacts like `translate(0,-3.3333334e-5)` —
those also silently failed to parse, but dropping a ~0.00003-unit offset
is visually imperceptible. `g33` is the one case where a scientific-
notation number is paired with a *substantial* co-argument
(`-4.9683458`), so losing the whole translate was actually visible.

## Impact / scope

Any SVG `transform` attribute (`translate`/`scale`/etc., anything parsed
via `REGEX_FLOAT`) containing a number in scientific notation, paired with
any other non-negligible coordinate in the same transform function, would
have its entire transform silently dropped on import — not just a
precision loss on that one number. `ruby.svg` has 91 occurrences of
scientific-notation numbers; only this one visibly mattered, but the same
class of drop was silently happening everywhere else too, undetected
because those instances only involved sub-visible offsets.

## Fix (implemented, confirmed)

```cpp
#define REGEX_FLOAT "(-?[\\.|\\d]+(?:[eE][-+]?\\d+)?)"
```

Added an optional non-capturing exponent suffix. Verified via
`just run-debug-1148748f4eb9`: `g33` now parses its full translate and
`right-arm-pointing` (index 0) renders correctly without the
ungroup/regroup workaround.

## Grill Log

### 2026-07-03 – 2026-07-04

- Started from a live symptom: `path2164-1`/`path5-7` under
  `right-arm-pointing` offset ~50%, "rotates around the correct point but
  larger radius." Confirmed via Inkscape that the same file renders
  correctly there — ruled out an artwork/pivot-placement authoring
  mistake, confirming it was a Friction bug.
- Built a minimal isolated repro SVG (`tmp/pivot-repro.svg`, deleted) to
  test the "translate-only wrapper group nested under a pivoted ancestor"
  theory in isolation from the full character rig.
- Found and fixed a real, independent bug along the way: `SvgLinkBox::
  collectPivotDescs`/`collectFlipbookDescs` never checked their own
  top-level `svgRoot` argument, only its children — invisible unless
  `ImportSVG::loadSVGFile`'s single-top-level-child unwrap
  (`svgimporter.cpp:1220-1222`) made `svgRoot` itself the box carrying the
  pivot/flipbook desc. Fixed via `SvgLinkBox::applyPivotDescIfPresent`
  (`svglinkbox.h`/`.cpp`) and a self-check added to
  `SvgElementTrack::findDescendantByName` (`svgelementtrack.cpp:68-70`).
  This bug was real but turned out to be orthogonal to this issue —
  `ruby.svg`'s actual root has many top-level siblings, so the unwrap
  never triggers there.
- Hit and diagnosed a stale-corrupted-state trap: an element track created
  before the above fix (when its target never resolved) got saved with
  framework-default zero values; once resolution started succeeding, a
  later reload pushed those stale zeros onto the target, corrupting scale/
  opacity. Traced this to `SvgLinkBox::resolveElementTracks()`'s
  unconditional `syncToTarget` on every resolve
  (`svglinkbox.cpp:127-136`). Filed separately rather than fixed:
  `issue-019f2dd6-...-set-source-file-resets-element-track-to-zero.md`.
- Found and filed two more independent, real bugs surfaced during the
  same investigation (neither is this issue's root cause):
  - Gizmo handle geometry loses float32 precision at extreme canvas zoom
    (`issue-019f2ded-...-gizmo-float-precision-distortion-at-extreme-zoom.md`).
  - Scale is permanently multiplicative-locked at exactly `0`
    (`issue-019f2ded-...-scale-permanently-locked-at-zero.md`).
  - A delete-and-relink of `ruby.svg` showing as "Empty Link"
    (`issue-019f2df0-...-relinked-svg-shows-empty-link.md`), not yet
    root-caused.
- Re-linking `ruby.svg` from scratch (avoiding the stale-corruption trap)
  showed scale/rotation working fine in general, which for a while
  suggested the original symptom might not be reproducible at all — until
  a precise A/B was found: `right-arm-flipbook` index 0
  (`right-arm-pointing`, still has the `translate(...)` wrapper) vs. index
  3 (`right-arm-pointing-2`, a duplicate the user had ungrouped+regrouped
  in Inkscape, baking the translate into the path coordinates and removing
  the wrapper transform). Same ancestors, same pivot, differing only in
  whether the content sits under a translate-only wrapper `<g>`. This
  clean, rotation-independent, static comparison — captured via the same
  debug tracing — is what surfaced `g33`'s `dx=0 dy=0` despite its
  SVG-declared translate, leading directly to the regex root cause.
- User's own hypothesis, stated up front and confirmed: "I suspect
  there's just multiplication going on, and 0 * n = 0" (re: the separate
  scale-lock bug) — an example of a correct intuition reached before the
  code trace confirmed it, consistent with this session's general
  methodology of forming a hypothesis then verifying against live debug
  output rather than static reading alone.
- Decision: keep the temporary debug tracing added to
  `BoundingBox::getRelativeTransformAtFrame` (gated by
  `friction.box.pivot`) and the `run-debug-1148748f4eb9` justfile recipe
  as permanent diagnostics, per this project's established convention from
  the earlier `glasses-flipbook` investigation — explicit user call.
