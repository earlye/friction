# SVG import flattens single-child `<g>` groups, dropping their id/label/desc

## Context

Originally filed as "ruby flipbook not working" — a flipbook in
`ruby.svg` (`~/Documents/github.com/ehlingers/slime-at-school/characters/ruby.svg`,
loaded via `SvgLinkBox` in
`~/Documents/github.com/ehlingers/slime-at-school/episodes/s01e01/scene-01-line-01.friction`)
was scrubbing correctly on the timeline but not showing/hiding its child
elements. Investigation found this was actually four independent bugs; see
Grill Log below for the full trail. This issue tracks the one confirmed to
have the largest real-world impact. The other three were split out:

- Hyphens stripped from all imported SVG box names —
  `issue-019f29c7-324e-7503-856e-cdb8fa24e410-svg-import-strips-hyphens-from-names.md`
- Stale flipbook/element tracks never pruned —
  `issue-019f29c7-3258-7d03-b02c-f016a38ddff0-stale-flipbook-tracks-never-pruned.md`
- Missing `left-arm-holla` label — pure `ruby.svg` data typo, already fixed
  by the user directly in the artwork; not tracked as a code issue.

## Root cause (confirmed)

`loadBoxesGroup` in `src/core/svgimporter.cpp:465-489`:

```cpp
if (allRootChildNodes.count() > 1 || hasTransform || !parentGroup) {
    boxesGroup = enve::make_shared<ContainerBox>(eBoxType::group);
    attributes.apply(boxesGroup.get());   // id/label applied HERE only
    if (parentGroup) parentGroup->addContained(boxesGroup);
} else {
    boxesGroup = parentGroup->ref<ContainerBox>();  // reuse PARENT, no apply()
}
```

When an SVG `<g>` element has exactly one DOM child node and no transform,
friction does not create a `ContainerBox` for it — it silently reuses the
*parent* container and never calls `attributes.apply()`. This means the
group's `id`, `inkscape:label`, and any `<desc>` (including `kind: ...`
desc-yaml) are discarded entirely. The lone child (e.g. a single `<path>`)
is added as an anonymous sibling inside the parent container instead.
Nothing inherits the dropped name —
`BoxSvgAttributes::setParent` (`svgimporter.cpp:1241-1246`) only propagates
fill/stroke/text/fillRule attributes, not id/label/desc.

Confirmed reproduction: `mouth-flipbook` in `ruby.svg` has 6 frame groups.
`mouth-closed`, `mouth-sad`, `mouth-hmm` each contain exactly **one** child
`<path>` and are silently flattened — they never become locatable
`BoundingBox`es, so `SvgFlipbookTrack::resolveTargets`
(`src/core/Boxes/svgflipbooktrack.cpp:79`) can never find them; they're
completely absent from `mResolvedPages` (not mismatched — just gone).
`mouth-ah-eh` (3 paths), `mouth-open` (4 paths), `mouth-smile` (2 paths)
each have 2+ children, take the `ContainerBox` branch, keep their name, and
resolve correctly.

Confirmed via a debug build (`just build-debug`) run with
`just run-debug-timeline`
(`QT_LOGGING_RULES="SvgElementTrack=true;friction.svgflipbooktrack=true"`,
output to `log.txt`) against `scene-01-line-01.friction`: setting the mouth
flipbook to index 0 (`mouth-closed`) visibly turned on `mouth-closed`,
`mouth-sad`, and `mouth-hmm` simultaneously, because none of the three were
ever hidden by `syncToTargets` in the first place — they were never part of
the resolved set to begin with, so they just sat at whatever default
visibility SVG import left them in.

This is likely the single biggest contributor to "flipbook children don't
show/hide" in general, not just for this one flipbook — any SVG artwork
with a labeled, single-child wrapper group (a common, simple authoring
pattern) will silently lose that label on import.

## Impact / scope

Affects any use of a named/labeled `<g>` wrapping exactly one child element
with no transform — not specific to flipbooks. Any friction feature that
locates elements by id/label (element tracks, pivots, followers, flipbooks)
is subject to this if the target happens to be a single-child group.

## Next steps

- **Fix shape decided: remove the single-child flattening optimization
  entirely.** `loadBoxesGroup` (`svgimporter.cpp:465-489`) should always
  create a `ContainerBox` and call `attributes.apply()` for every `<g>`,
  regardless of `allRootChildNodes.count()`. The `hasTransform`/
  `!parentGroup` conditions become moot once the child-count branch is
  removed — the `if`/`else` collapses to always taking the current `if`
  body. Rejected alternatives: conditioning the skip on label/desc presence
  (adds complexity, still leaves anonymous single-child groups silently
  losing identity if that assumption is ever wrong) and transferring
  attributes to the surviving child (risks silently overwriting the
  child's own id/label/desc, changes box type at that name). Simplest,
  most predictable behavior was preferred over preserving the box-count
  optimization.
- After fixing, re-verify with `just run-debug-timeline` against the mouth
  flipbook that all 6 pages resolve and toggle correctly.
- Watch for fallout: removing this optimization will increase the box
  count for any SVG with anonymous single-child wrapper groups (a common
  Inkscape artifact) — worth a quick sanity check that this doesn't
  noticeably regress import time or UI responsiveness on larger character
  files.

## Grill Log

### 2026-07-03

- Q: Which flipbook(s) did you observe "not showing/hiding at all" on? —
  A: Started broad, then focused specifically on the mouth flipbook for
  in-depth debugging; noted this looks like it may be multiple issues.
- Q: When set to index 0, the flipbook turned on `mouth-closed`,
  `mouth-sad`, and `mouth-hmm` simultaneously instead of just
  `mouth-closed` — A: root-caused via debug log + code investigation to
  single-child `<g>` flattening in `loadBoxesGroup`: those three frames
  each have exactly one child `<path>`, so their wrapping `<g>` (and its
  id/label) is dropped entirely during import, making them permanently
  unresolvable — not a visibility-sync bug, an import bug.
- Captured method: built a debug binary (`just build-debug`), ran with
  `just run-debug-timeline` (writes `log.txt`), reproduced the mouth
  flipbook behavior live, then grepped/read `log.txt` for
  `friction.svgflipbooktrack`/`SvgElementTrack` lines to get ground-truth
  `resolveTargets`/`syncToTargets` output rather than guessing further from
  static code reading alone.
- Decision: split the original combined investigation into separate issues
  per root cause (hyphen-stripping, stale tracks, this flattening bug);
  the missing `left-arm-holla` label was a pure artwork typo already fixed
  by the user directly in `ruby.svg`, so it's not tracked separately.
- Q: How should the single-child `<g>` flattening bug be fixed — skip
  flattening when label/desc is present, transfer attributes to the
  surviving child, or remove the optimization entirely? — A: remove the
  optimization entirely; always create a `ContainerBox` for every `<g>`
  regardless of child count. Simplicity and predictability preferred over
  preserving the box-count optimization.
