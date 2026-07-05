# Flipbook track editor should select pages by label, not raw number

## Context

Surfaced while grilling `issue-019f2f85-d63c-7382-ab59-6d33bbeb7e3f`
(replacing `<desc>` YAML with `inkscape:label` query-string conventions).
Under the new convention, flipbook pages are direct children labeled
`{name}?page=N` — the artist already gives each page a meaningful display
name. The flipbook track editor UI should let the artist pick a page by
that name instead of by its raw numeric index.

## Current state (from codebase research)

- No dedicated `FlipbookBox`/`FlipbookAnimator` class exists. "Flipbook" =
  `SvgFlipbookTrack` (a `StaticComplexAnimator`,
  `src/core/Boxes/svgflipbooktrack.h`/`.cpp`) owned by `SvgLinkBox`
  (`src/core/Boxes/svglinkbox.h`/`.cpp`) in `mFlipbookTracks`.
- `SvgFlipbookTrack::mIndex` is a plain `qsptr<IntAnimator>` named `"index"`
  (range `[-9999, 9999]`, step 1) — the actual animatable "current page"
  value, editable/keyframable like any other numeric animator.
- `SvgFlipbookTrack::mResolvedPages` is `QMap<int, BoundingBox*>` (page
  index -> child box), populated via
  `SvgLinkBox::applyFlipbookLabelIfPresent()` -> `collectPageChildren()`
  (`svglinkbox.cpp:569-599`), which parses each direct child's
  `inkscape:label` via `parseSvgLabel()` (`src/core/svglabel.cpp`) looking
  for `page=N`. Duplicate page indices among siblings are a hard error
  (whole map discarded). No page display-name is cached anywhere — must be
  read live off `resolvedBox->prp_getName()` / `parseSvgLabel(label).baseName`.
- Today there is no flipbook-specific UI at all: `mIndex` renders through
  the fully generic property-row machinery in
  `BoxSingleWidget::setTargetAbstraction()`
  (`src/app/GUI/BoxesList/boxsinglewidget.cpp`) — since `IntAnimator`
  derives from `QrealAnimator`, it falls into the generic
  `QrealAnimatorValueSlider` numeric spin/slider widget (same widget used
  app-wide for every real/int animator). This is the only existing editing
  surface for this value (confirmed no other file wires up
  `QrealAnimatorValueSlider` for this purpose, and there's no separate
  timeline/keyframe-row editor for it).
- Closest existing "picker" pattern, `ComboBoxProperty`
  (`src/core/Properties/comboboxproperty.h`/`.cpp`) +
  `BoxSingleWidget::setComboProperty()`, only supports a static/fixed
  `QStringList` set once at construction — nothing in the codebase today
  builds a combo dynamically populated from a container's live children.
  `BoxTargetProperty`/`BoxTargetWidget` (drag/drop "point at a box") is the
  other close pattern but doesn't fit a name-list combo either.
- `mFlipbookFollowers` (a box that mirrors another box's page visibility,
  via `applyFlipbookFollower()`) has no independent index property of its
  own — it always mirrors its controller track's index. **Out of scope**
  for this picker.
- `SvgFlipbookTrack` already has a `pageChanged()` signal, but it only
  fires from `syncToTargets()` when the *current index* changes — not when
  the *page set* changes (relink, rename, add/remove page children). No
  existing signal covers that case.
- No tests exist for flipbook anywhere in the repo.

## Design decisions (resolved via grilling, see log below)

1. **Combo sits alongside the existing numeric slider, does not replace
   it.** Both edit the same `mIndex` animator row in `BoxSingleWidget`
   (the Boxes-list properties panel) — the only place this value is
   editable today. The numeric slider stays for keyframe/timeline
   scrubbing workflows; the combo is a friendlier "jump to named page"
   control on the same row (mirrors the existing X/Y dual-slider pattern
   used for `QPointFAnimator`).
2. **Selecting an item must go through the same commit path as the
   slider** (`prp_startTransform()` / set-value / `prp_finishTransform()`
   in `qrealanimatorvalueslider.cpp`), so undo-grouping and
   auto-keyframing-when-recording behave identically to a slider drag. If
   no reusable wrapper exists for that sequence today, extract one so both
   widgets call the same code path.
3. **Unresolved current index** (no matching page in `mResolvedPages`):
   show a synthetic entry using the unified label scheme (see #5), e.g.
   `5-(unresolved)`, rendered with a **red background** when it is the
   currently selected item — consistent with the existing orphaned-track
   red overlay in `prp_drawTimelineControls()`.
4. **Page with an empty base name** (label is just `?page=3`, no name
   before the `?`): fall back to `3-(unnamed)` rather than leaving a blank
   entry.
5. **Unified label format** for every entry, index-prefixed:
   - Normal page: `0-Walk`, `1-Walk`, `2-Run` (index prefix applied to
     *every* entry, not just on name collisions — simpler than a dedup
     pass, and duplicate names across different pages are legal since
     `collectPageChildren()` only rejects duplicate *indices*).
   - Empty name: `3-(unnamed)`.
   - Unresolved current index: `5-(unresolved)` (red background when
     selected).
   - Ordering: ascending by `page=` index (already the natural iteration
     order of `mResolvedPages`, a `QMap<int, BoundingBox*>`).
6. **Live repopulation**: add a new `pagesChanged()` signal on
   `SvgFlipbookTrack`, emitted from both `setResolvedPagesDirect()` and
   the non-direct-resolve path in `resolveTargets()` (distinct from the
   existing `pageChanged()`, which only covers index changes). The combo
   listens to `pagesChanged()` to repopulate its item list, and to the
   existing `pageChanged()` to keep the *selected* item in sync.
   `pagesChanged()` must also be wired in `wireFlipbookTrack()` the same
   way `pageChanged()` already is (`svglinkbox.cpp:741-749`), so any bound
   `mFlipbookFollowers` get re-applied whenever the page set changes —
   symmetric with how index changes already trigger follower re-sync.

## Next steps

- Add a combo box to the flipbook track's `index` property row in
  `BoxSingleWidget`, special-cased similarly to how `ComboBoxProperty` is
  special-cased there, per the design decisions above.
- Extract/confirm a shared value-commit helper so both the numeric slider
  and the new combo box go through identical
  start-transform/set/finish-transform semantics.
- Add `SvgFlipbookTrack::pagesChanged()` and wire it in
  `wireFlipbookTrack()` alongside the existing `pageChanged()` follower
  wiring.
- No dedicated tests exist for flipbook today; consider whether this is
  the moment to add coverage (out of scope of this grilling session to
  decide test strategy in detail).

## Grill Log

### 2026-07-04

- Q: Should the combo box replace the existing numeric slider for
  `mIndex`, or sit alongside it? — A: Alongside; keep the numeric slider
  for keyframe/timeline workflows.
- Q: Where should the combo live — is `BoxSingleWidget`'s property row the
  only place this value is edited? — A: Confirmed (verified no other
  `QrealAnimatorValueSlider` usage or timeline-row editor exists for this
  value); combo goes in the same row.
- Q: What should the combo show for a current index with no matching page
  (orphaned/mismatched)? — A: A synthetic `(unresolved: N)`-style entry,
  with a red background when selected.
- Q: What should the combo show for a page child with an empty base name?
  — A: A placeholder like `(page N)`/`(unnamed)` rather than a blank
  entry.
- Q: Should selecting a combo item go through the same
  transform/undo/auto-keying path as the numeric slider? — A: Yes, and if
  no shared wrapper exists for that path today, build one so both widgets
  use it.
- Q: Are `mFlipbookFollowers` in scope for their own picker? — A: No —
  confirmed they have no independent index property; they always mirror
  their controller track's index.
- Q: How should duplicate/ambiguous page names be disambiguated? — A:
  Index-prefixed for every entry (`0-Walk`, `1-Walk`, `2-Run`), not just
  on collisions.
- Q: Unify the label format across all cases (normal/empty-name/
  unresolved) into one scheme? — A: Agreed on the prefixed scheme:
  `0-Walk`, `3-(unnamed)`, `5-(unresolved)` (red background when selected).
- Q: How should the combo repopulate when the page set changes (relink,
  rename, add/remove pages), given the existing `pageChanged()` signal
  only covers index changes? — A: Add a new `pagesChanged()` signal for
  page-set changes, separate from `pageChanged()` (index changes); this
  should also trigger re-applying any bound flipbook followers, the same
  way `pageChanged()` already does via `wireFlipbookTrack()`.
