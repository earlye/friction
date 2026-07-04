# glasses-flipbook silently orphaned from the tree during SVG import

## Context

`ruby.svg` (`~/Documents/github.com/ehlingers/slime-at-school/characters/ruby.svg`,
loaded via `SvgLinkBox` in
`~/Documents/github.com/ehlingers/slime-at-school/episodes/s01e01/scene-01-line-01.friction`)
has a `glasses-flipbook` group at `Ruby > head > eyes > glasses-flipbook`
with a `kind: flipbook` desc mapping to three frames: `glasses-1`,
`glasses-angry`, `glasses-3`. It never appears in friction's tracklist at
all — not orphaned-and-red-tinted, not partially broken, just completely
absent, as if the desc were never there.

## Root cause (confirmed)

`glasses-3` (SVG `<g inkscape:label="glasses-3" .../>`, self-closing, zero
children) is a genuinely empty group. `svgimporter.cpp`'s empty-group
pruning (`loadElement`, `src/core/svgimporter.cpp:1107-1121`, added when the
single-child-flattening bug was fixed) removes it:

```cpp
if(group->getContainedBoxesCount() == 0) {
    group->removeFromParent_k();
}
```

`removeFromParent_k()` (`src/core/Animators/eboxorsound.cpp:59-62`) calls
`mParentGroup->removeContained_k(ref<eBoxOrSound>())` — the **cascading**
removal variant. `ContainerBox::removeContained_k`
(`src/core/Boxes/containerbox.cpp:1365-1370`):

```cpp
void ContainerBox::removeContained_k(const qsptr<eBoxOrSound> &child) {
    removeContained(child);
    if(mContained.isEmpty() && getParentGroup()) {
        removeFromParent_k();
    }
}
```

This auto-prunes a container from *its own* parent if removing `child`
leaves it empty — a reasonable feature for interactive editing (deleting
the last shape in a group cleans up the now-empty group), but unsafe
during import, where a group can be **momentarily** empty mid-construction.

`glasses-3` is the first child processed inside `glasses-flipbook` (SVG
document order: `glasses-3`, `glasses-angry`, `glasses-1`). At the instant
`glasses-3` is pruned, `glasses-flipbook` has had none of its other
children added yet, so it is — for that one instant — empty. The cascade
in `removeContained_k` fires and detaches `glasses-flipbook` from `eyes`
right then, permanently. Moments later `glasses-angry` (4 paths) and
`glasses-1` (3 paths) are added to `glasses-flipbook` as normal, and it
ends up a fully-formed, valid `ContainerBox` with 2 real children, correct
name, and correct `kind: flipbook` desc attached — but it's an orphan.
Nothing in `SvgLinkBox`'s tree (rooted at `svgRoot`, reached via
`getContainedBoxes()`) can reach it anymore, so
`SvgLinkBox::collectFlipbookDescs` never visits it and no
`SvgFlipbookTrack` is ever created.

Confirmed via a debug build with temporary `qCDebug` tracing added to
`svgimporter.cpp` (`BoxSvgAttributes::apply`, `loadBoxesGroup`),
`svglinkbox.cpp` (`collectFlipbookDescs`), and `containerbox.cpp`
(`insertContained`/`removeContained`, new category
`friction.containerbox.insert`) — see Grill Log for the exact trace. This
tracing is left in place (all `QtWarningMsg`-level, silent unless enabled
via `QT_LOGGING_RULES`) per the project's existing debug-logging
convention; `just run-debug-290e8ced0979` runs with all of it enabled.

## Impact / scope

Any SVG group that is (a) genuinely empty (no children at all, not just
hidden) and (b) the *first* child processed within a still-otherwise-empty
parent group will silently orphan that parent from the tree — not just
flipbook frames. This is a general `ContainerBox`/import interaction bug,
not specific to `kind: flipbook` groups; it happens to have been discovered
via a flipbook because the desc conveniently makes the missing group show
up as "expected element never shows in tracklist" instead of just "some
paths are missing," which would be much harder to notice.

## Fix (implemented, confirmed)

Gave the cascading-removal path an explicit opt-out instead of special-
casing the importer's call site with lower-level primitives:

- `eBoxOrSound::removeFromParent_k(bool cascadeIfParentEmptied = true)`
  (`src/core/Animators/eboxorsound.{h,cpp}`) and
  `ContainerBox::removeContained_k(child, bool cascadeIfEmptied = true)`
  (`src/core/Boxes/containerbox.{h,cpp}`) both gained a defaulted bool
  parameter. Default preserves the existing cascading behavior for every
  other call site (interactive delete in `canvasselectedboxesactions.cpp`,
  `ContainerBox::setIsCurrentGroup_k`'s empty-group cleanup, ungroup) —
  those are all genuinely user-triggered actions on an already-fully-built
  tree, where the cascade is correct.
- `svgimporter.cpp`'s empty-group pruning
  (`loadElement`, ~line 1121) now calls
  `group->removeFromParent_k(/*cascadeIfParentEmptied=*/false)` — the only
  call site that needed the non-default value, since it's the only one
  that runs during bulk tree construction where a container can be
  momentarily, meaninglessly empty mid-loop.

Rejected alternatives (see Grill Log): a full `PruningPolicy`
enum/abstraction (over-engineered for one real call site); reordering
`loadBoxesGroup` to populate children before attaching to the parent
(would also incidentally fix this, but breaks the invariant that a box's
full ancestor chain is linked *during* import, which blend-effect
propagation in `ContainerBox::insertContained` — `getFirstParentLayer()` —
relies on; much larger blast radius for no added benefit).

Verified via `just run-debug-290e8ced0979`: `glasses-flipbook` now survives
pruning of its empty `glasses-3` child, gets visited by
`SvgLinkBox::collectFlipbookDescs`, resolves its `kind: flipbook` desc, and
`syncToTargets` correctly toggles `glasses-1`/`glasses-angry` visibility
across frames.

All temporary debug tracing added during the investigation
(`svgimporter.cpp`, `svglinkbox.cpp`, `containerbox.cpp`'s new
`friction.containerbox.insert` category, and the `run-debug-290e8ced0979`
justfile recipe) is being kept as permanent, filtered-by-default
diagnostics (user's explicit call) rather than stripped.

## Grill Log

### 2026-07-03

- Q: What is "Ruby" / "glasses" in this issue? — A: `ruby.svg`, a character
  rig in `~/Documents/github.com/ehlingers/slime-at-school`, with a
  `glasses-flipbook` group under `eyes`.
- Q: What does "not showing up" mean exactly — invisible on load, or
  disappears during use? — A: the `glasses-flipbook` *track* never appears
  in the tracklist at all (not a visibility-sync issue on an existing
  track).
- Investigated hypothesis "maybe `glasses-3` being empty breaks something"
  (user's own hypothesis, stated up front) — confirmed true, but the
  mechanism was far more indirect than simple non-resolution of one page:
  it orphans the *entire* `glasses-flipbook` group from the tree, so no
  track is ever created for any of its 3 pages, not just the empty one.
- Ruled out (via live debug builds, `just build-debug` +
  `QT_LOGGING_RULES`-based tracing, following the same methodology as
  `resolved-...ruby-flipbook-not-working.md`):
  - Hyphen-stripping from names — separate bug, already fixed upstream
    (`resolved-...svg-import-strips-hyphens-from-names.md`) before this
    session; ruled out early since box names showed hyphens correctly.
  - Single-child `<g>` flattening — separate bug, already fixed upstream;
    not applicable here since `glasses-flipbook` has 2+ children.
  - Stale/duplicate flipbook tracks from renaming — separate bug, fixed
    upstream mid-session (`resolved-...stale-flipbook-tracks-never-pruned.md`);
    confirmed unrelated since `glasses-flipbook` was never created in the
    first place, stale or otherwise.
  - `SvgFlipbookTrack::resolveTargets`/`mOrphaned` partial-resolution gaps
    (tracked separately in
    `issue-019f29f2-ca57-7811-8f78-e1039db994b1-tighten-orphan-detection.md`) —
    not the mechanism; the track object itself never gets created, so
    orphan-flagging logic never even runs.
  - Desc-yaml parsing/YAML exceptions — ruled out by adding
    `qCWarning` on parse failure; none fired for `glasses-flipbook`, and
    `BoxSvgAttributes::apply` confirmed the desc *was* attached correctly
    at import time.
- Root-caused by adding `qCDebug` box-visit tracing to
  `SvgLinkBox::collectFlipbookDescs` (revealed `glasses-flipbook` is never
  visited as a child of `eyes`, though sibling boxes `scleras`, `pupils`,
  `glasses-arm` are) and to `ContainerBox::insertContained`/
  `removeContained` (revealed the exact cascade: pruning `glasses-3`
  triggers `removeContained_k`, which sees `glasses-flipbook` momentarily
  empty and cascades to remove it from `eyes` too).
- Method note: two live debug sessions were lost to using stale
  `QT_LOGGING_RULES` (`just run-debug-timeline`'s hardcoded categories
  didn't include the new ones being added) before settling on a dedicated
  `just run-debug-290e8ced0979` recipe kept in sync with whatever tracing
  is currently in place, at the user's request, to avoid repeated
  copy-paste/shell-history mistakes during a multitasking debugging
  session.
- Q: Fix shape — bypass the cascade locally in svgimporter.cpp using
  existing public primitives (`getContainedIndex` +
  `removeContainedFromList`), add a formal `PruningPolicy`
  enum/abstraction, or add a simple defaulted bool parameter to the
  cascading removal API? — A: bool parameter
  (`cascadeIfParentEmptied`/`cascadeIfEmptied`) on
  `removeFromParent_k`/`removeContained_k`. User's reasoning: a full
  policy type is unjustified with only one real non-default caller, but a
  bare low-level workaround at the call site doesn't self-document *why* —
  a named bool threaded through the two relevant methods is the minimal
  option that still states intent clearly, and default-`true` leaves every
  other (interactive-editing) call site's behavior untouched.
- Q: Keep or strip the temporary debug tracing now that the fix is
  confirmed working, including the hot-path `containerbox.cpp`
  insert/remove tracing that was pure scaffolding for this one bug? — A:
  keep all of it, including `containerbox.cpp`'s new
  `friction.containerbox.insert` category — explicit user call, overriding
  the initial recommendation to strip the containerbox-specific tracing.
- Fix implemented and verified live via `just run-debug-290e8ced0979`
  (see Fix section above for the trace confirming the cascade no longer
  fires and the track resolves/syncs correctly).
