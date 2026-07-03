# Stale flipbook/element tracks are never pruned when their owner is renamed

## Context

Split out from investigating "ruby flipbook not working"
(`issue-019f28b5-3e6a-76a0-975f-1b9fca3b71f0-ruby-flipbook-not-working.md`).
While debugging a mouth flipbook in `ruby.svg` (loaded via `SvgLinkBox` in
`~/Documents/github.com/ehlingers/slime-at-school/episodes/s01e01/scene-01-line-01.friction`),
the user observed friction showing **two** separate `SvgFlipbookTrack` rows
for what should be a single flipbook — `Mouth` and `mouth` — after the
underlying SVG group had previously had a mismatched `id="Mouth"` /
`inkscape:label="mouth"` and was later unified to a single
`id="mouth-flipbook"` / `inkscape:label="mouth-flipbook"`. Only the
lowercase one "sort of" controlled the children. Neither track showed the
timeline's orphaned red-tint indicator.

`log.txt` (captured via `just run-debug-timeline`,
`QT_LOGGING_RULES="SvgElementTrack=true;friction.svgflipbooktrack=true"`)
also shows long-dead `SvgElementTrack`s named `"mouthopen"`/`"mouthclosed"`
(pre-flipbook, legacy element-track names) still being loaded and synced on
every project open, well after that naming scheme was replaced by the
flipbook system.

## Root cause (confirmed)

`SvgLinkBox::collectFlipbookDescs` (`src/core/Boxes/svglinkbox.cpp:232`)
matches an existing track to a live `kind: flipbook` desc purely by name
equality:

```cpp
const QString ownerId = box->prp_getName();
SvgFlipbookTrack* existing = nullptr;
for (const auto& track : mFlipbookTracks) {
    if (track->prp_getName() == ownerId) { existing = track.get(); break; }
}
if (!existing) {
    auto track = enve::make_shared<SvgFlipbookTrack>(ownerId);
    wireFlipbookTrack(track);
    existing = track.get();
}
```

`SvgLinkBox::resolveElementTracks` (`svglinkbox.cpp:108`) resets *existing*
tracks' page maps before re-collecting (`track->setPageMap({})`,
line 133-135), but nothing ever removes a track whose name no longer
matches any live desc in the current SVG. The same applies to
`SvgElementTrack`s created via `collectAnimationNodes`/`addElementTrack`
(`svglinkbox.cpp:89-132`) — matched and created by name, never pruned.

Consequence: renaming a flipbook's (or animation node's) owner element
(its `id` or `inkscape:label`) leaves the old track permanently orphaned in
the `SvgLinkBox`'s persisted state (`mFlipbookTracks`/`mElementTracks`,
serialized via `writeBoundingBox`/`readBoundingBox`,
`svglinkbox.cpp:407-443`), sitting alongside a newly-created track under
the new name — forever, across saves, since nothing ever compares the live
desc set against the existing track list to find removals.

Why no orphan indicator: `SvgFlipbookTrack::mOrphaned`
(`src/core/Boxes/svgflipbooktrack.cpp:92`) only becomes true when a track's
resolved-page set is **completely empty**
(`mPageMap.isEmpty() || !anyResolved`). A stale track isn't guaranteed to
look orphaned — if its old page-map values still coincidentally resolve to
real elements elsewhere (page-label resolution is global, not scoped to
the owning container — see `findDescendantByName` searching from
`svgRoot`), the stale track keeps "working" and silently fights the live
track for control of the same boxes.

## Impact / scope

- Any rename of a flipbook or animation-node owner element accumulates a
  permanent, invisible duplicate track.
- Duplicate tracks with overlapping resolved targets can independently
  drive visibility on the same boxes according to different keyframes,
  producing visible artifacts (multiple flipbook pages appearing active at
  once) that look like a sync bug but are actually two competing tracks.
- The project file grows stale cruft over time with normal editing
  (renaming layers/labels in Inkscape is a completely ordinary workflow).

## Next steps

Resolved design (see Grill Log below):

- In `resolveElementTracks`, after re-collecting from the current SVG,
  determine per track whether its name matches any live desc found in this
  pass:
  - **Element tracks**: `SvgElementTrack::resolveTarget` already searches
    by exact name each pass, so `mOrphaned` is already correct for the
    plain-rename case (no cached pointer involved). The only missing piece
    is removal.
  - **Flipbook tracks**: `SvgFlipbookTrack::resolveTargets` must **not**
    be called at all for a track whose owner name doesn't match any live
    flipbook desc this pass — its page-map values resolve via a global
    (unscoped) search that can coincidentally still succeed after a
    rename, which is why the orphaned indicator never fired for the
    `Mouth`/`mouth` case. Compute the live owner-name set during
    `collectFlipbookDescs` and skip `resolveTargets`/`syncToTargets` for
    any track not in that set.
- For a track whose name doesn't match any live desc this pass:
  - If its animator subtree has **no keyframes** (reuse the existing
    `subtreeHasKeys`-style check already used in
    `svgelementtrack.cpp:339`), auto-remove it silently — there's no user
    data to lose.
  - If it **does** have keyframes, keep it, force `mOrphaned = true` (add
    a `setOrphaned(bool)` setter to `SvgFlipbookTrack` mirroring the one
    `SvgElementTrack` already has), and make sure it no longer syncs to
    any resolved target (for flipbook tracks: skip `resolveTargets`/
    `syncToTargets`, leaving `mResolvedPages` empty). The user then
    notices via the existing red-tint timeline indicator
    (`prp_drawTimelineControls`) and deletes it manually via the existing
    "Delete Track" context-menu action (`svgflipbooktrack.cpp:136-148`,
    `svgelementtrack.cpp:419-448`) — no new UI needed.
- Out of scope for this fix (tracked separately in
  `issue-019f29f2-ca57-7811-8f78-e1039db994b1-tighten-orphan-detection.md`):
  tightening `mOrphaned` to flag *partial* resolution failures, and
  detecting when two tracks' resolved targets collide on the same physical
  box.

## Grill Log

### 2026-07-03

- Investigated as part of the mouth-flipbook debugging session for
  `issue-019f28b5-3e6a-76a0-975f-1b9fca3b71f0-ruby-flipbook-not-working.md`.
  User reported the duplicate `Mouth`/`mouth` track rows directly; root
  cause confirmed by reading `collectFlipbookDescs`/`resolveElementTracks`
  (no pruning logic exists) and cross-checked against `log.txt` showing
  legacy `mouthopen`/`mouthclosed` `SvgElementTrack`s still loading. Split
  into its own issue since it's a distinct track-lifecycle bug, unrelated
  to the SVG-import bugs (hyphen-stripping, single-child flattening) also
  found during the same investigation.
- Q: Should stale-track pruning be silent auto-removal, always-manual, or
  something in between? — A: Auto-remove only if the track has no
  keyframes; otherwise keep it, force it orphaned, and stop it from
  syncing so the user notices and deletes it manually via the existing
  "Delete Track" context-menu action (confirmed this action already
  exists on both track types).
- Q: Is tightening `mOrphaned` for partial resolution / cross-track
  conflicts in scope for this fix? — A: No, out of scope; split into
  `issue-019f29f2-ca57-7811-8f78-e1039db994b1-tighten-orphan-detection.md`.
- Q: Create that follow-up issue file now or just note it? — A: Create it
  now, cross-linked, same as how this issue itself was split out.
