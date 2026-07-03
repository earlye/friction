# Tighten SvgFlipbookTrack/SvgElementTrack orphan detection for partial resolution and cross-track conflicts

## Context

Split out from
`issue-019f29c7-3258-7d03-b02c-f016a38ddff0-stale-flipbook-tracks-never-pruned.md`
while scoping the fix for stale/never-pruned tracks. That issue's fix makes
`mOrphaned` reliably true for tracks whose owner name no longer matches any
live SVG desc (the common rename case). It does **not** address two
related, narrower gaps in orphan detection that were explicitly deferred as
out of scope:

1. **Partial resolution isn't flagged.** `SvgFlipbookTrack::mOrphaned`
   (`src/core/Boxes/svgflipbooktrack.cpp:92`) only becomes true when the
   resolved-page set is **completely** empty
   (`mPageMap.isEmpty() || !anyResolved`). A track where some but not all
   pages resolve looks fully healthy in the UI, even though part of its
   page map is silently broken.
2. **Cross-track box-claim conflicts aren't detected.** Because page/label
   resolution in both `SvgFlipbookTrack::resolveTargets` and
   `SvgElementTrack::resolveTarget` searches globally from `svgRoot` (not
   scoped to the owning container — see `findDescendantByName` /
   `findDescendantByLabel` in `svgflipbooktrack.cpp`), two different
   tracks can legitimately end up resolving to the same physical
   `BoundingBox` and silently fight over its state (visibility, keyframe
   values) without any indicator that this is happening.

## Why deferred

Fixing the name-mismatch pruning case (the original issue) is a targeted,
low-risk change confined to `SvgLinkBox::resolveElementTracks`. Detecting
partial resolution and cross-track conflicts requires touching the
resolution logic inside `SvgFlipbookTrack`/`SvgElementTrack` themselves and
deciding on new UI treatment (e.g. a distinct partial-orphan visual state,
or a conflict warning), which is a larger, separate design surface.

## Next steps

- Decide what "partially orphaned" should mean for `SvgFlipbookTrack`
  (e.g. flag if `mResolvedPages.size() < mPageMap.size()`) and whether it
  needs a distinct visual treatment from fully-orphaned.
- Decide how to detect two tracks resolving to the same `BoundingBox*`
  (would need resolution results compared across all tracks on the
  `SvgLinkBox`, likely from `resolveElementTracks` after per-track
  resolution completes) and how to surface that conflict to the user.
- Consider whether page/label resolution should become scoped to the
  owning container instead of global, which would eliminate a lot of the
  false-positive "still resolves" cases outright rather than just
  detecting them after the fact.

## Grill Log

### 2026-07-03

- Split out during grilling of
  `issue-019f29c7-3258-7d03-b02c-f016a38ddff0-stale-flipbook-tracks-never-pruned.md`.
  User confirmed this broader orphan-detection tightening is out of scope
  for the pruning fix and should be tracked separately.
