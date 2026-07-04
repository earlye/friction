# SvgLinkBox never adopts the imported content's name — always shows "Empty Link"

## Context

Originally filed as "deleting and re-linking ruby.svg shows Empty Link instead
of the imported content," suspected to be a real import failure (0 imported
boxes) possibly caused by this session's other SVG-import fixes. Grilled and
resolved via a debug-build repro with `friction.svg.import` /
`friction.containerbox.insert` / `SvgElementTrack` logging
(`just run-debug-57a4e9a8842e`, see `justfile`): the import fully succeeds.
`log.txt` showed the full 11-child `"Ruby"` subtree (`head-underlap`,
`left-arm`, `left-arm-flipbook`, `hair`, `glasses-*`, etc.) correctly rebuilt
and nested one level inside the relinked box, both before and after the
delete-then-relink. Confirmed by the user: the content is present and
correct — the only problem is the box's own *name*.

**Root cause (confirmed):** `SvgLinkBox` (`src/core/Boxes/svglinkbox.h`,
built on `ExternalLinkBoxT<ContainerBox, SvgFileCacheHandler>`) is the only
type instantiating `ExternalLinkBoxT`. Its constructor
(`src/core/Boxes/externallinkboxt.h:41`) hardcodes
`prp_setName("Empty Link")`, and `SvgLinkBox::updateContent()`
(`src/core/Boxes/svglinkbox.cpp:73-87`) never renames the box based on the
imported content, regardless of import success. The imported root (e.g.
`"Ruby"`, named from the SVG's `inkscape:label`/`id` per
`svgimporter.cpp:1639/1641`) is nested one level inside as a child — so
`"Empty Link"` is the permanent, by-design top-level label for every
`SvgLinkBox`, whether or not the link is working.

This is *not* a regression from this session's SVG-import fixes (#73/#74/#75
etc.) — those only affect nested single-child wrapper groups deeper in the
tree (e.g. flipbook mouth frames), not the top-level linked box's own name or
box count. `ruby.svg`'s top-level `<g id="layer1" label="Ruby">` already has
11 direct children, well above the single-child flattening threshold, so
behavior there is unchanged by those fixes.

**Why this was confusing:** the user's production scenes (e.g.
`episodes/s01e01/scene-01-line-01.friction`) show clean top-level names like
`"Ruby 0"`, `"Desks 0"` with no manual renaming — but those scenes use
**Import** (`Actions::importFile` → `ImportHandler::sInstance->import()`,
`src/core/actions.cpp:773-835`), a one-time embed where the imported root
becomes a top-level box directly (uniquified via
`ContainerBox::insertContained`'s `makeNameUniqueForDescendants`,
`containerbox.cpp:1201-1211`, giving the `" 0"`/`" 1"` suffix pattern). That's
a structurally different feature from **Link** (`Actions::linkFile`,
`src/core/actions.cpp:884-901`), which creates a live `SvgLinkBox` that
re-reads the file from disk on every load/reload — the behavior the user
actually wants (live sync to on-disk SVG edits), but which has never synced
its own display name to match.

## Desired behavior (feature request, agreed with user)

`SvgLinkBox` should track two names:
- **auto-name**: derived from the imported content's root name (e.g.
  `"Ruby"`), refreshed every time `updateContent()` successfully imports,
  uniquified against scene collisions the same way `insertContained` already
  does (`ContainerBox::makeNameUniqueForDescendants`, excluding self).
- **manual-name**: set only when the user explicitly renames the box via the
  normal rename UI (i.e. through `eBoxOrSound::rename()` /
  `Property::prp_setNameAction()`, the undo-tracked path).

Effective displayed name = manual-name if set, else auto-name. This means:
- A freshly linked, never-renamed box shows the content's real name instead
  of `"Empty Link"`.
- Reloading the live-linked file (edits on disk) never stomps a name the
  user deliberately chose.
- A `"reset to automatic name"` menu action is a **separate, follow-up
  issue** (out of scope here) — the manual/auto split makes it a small
  addition later (just clear the manual-name and re-derive from auto-name).

**Implementation note:** this genuinely requires new state on `SvgLinkBox` —
that's the point, not something to avoid. Add two member fields: `mAutoName`
(QString, recomputed on every successful `updateContent()` import, not
persisted — Link always re-derives it from the freshly re-imported content
on load) and `mManualName` (QString, empty until the user renames the box,
persisted across save/load so a deliberate rename survives reload). The
displayed name is `mManualName` if non-empty, else `mAutoName`. This pair is
exactly what the future "reset to automatic name" action needs: it just
clears `mManualName` and re-syncs the displayed name to `mAutoName`, so it's
worth building the fields now even though that menu action itself is a
separate follow-up issue.

To distinguish "user renamed it" from "internal auto-sync," reuse the
existing split in the codebase: `ContainerBox::insertContained()`
(`containerbox.cpp:1201-1211`) already calls the low-level `prp_setName()`
directly (bypassing `rename()`/`prp_setNameAction()`) when uniquifying names
on insert. So: auto-name sync in `updateContent()` should call `prp_setName()`
directly (does not touch `mManualName`), while the user-facing rename path
(`eBoxOrSound::rename()` / `Property::prp_setNameAction()`) should be the
one place that sets `mManualName`.

## Reproduction steps (for the original, now-resolved "empty content" theory)

1. In `ruby-test.friction` (or similar), delete the existing `SvgLinkBox`
   pointing at `ruby.svg`.
2. Re-link the same `ruby.svg` file.

**Actual (confirmed via debug build):** content imports correctly and is
nested under the box; the box itself is simply named `"Empty Link"`, as it
always is for `SvgLinkBox`, whether freshly linked or previously working.

## Relevant files

- `src/core/Boxes/externallinkboxt.h:41` — hardcoded `"Empty Link"` default
  name in the `ExternalLinkBoxT` constructor
- `src/core/Boxes/svglinkbox.cpp:73-87` — `SvgLinkBox::updateContent()`,
  where auto-name sync would be added after a successful import
- `src/core/Boxes/containerbox.cpp:1201-1211` — `insertContained()`'s
  existing uniquify-on-insert pattern (`prp_setName` +
  `makeNameUniqueForDescendants`), the template to reuse for auto-naming
- `src/core/actions.cpp:773-835` — `Actions::importFile`, the working
  content-naming behavior in the (structurally different) Import path
- `src/core/actions.cpp:884-901` — `Actions::linkFile`, creates the
  `SvgLinkBox` with no post-creation rename step
- `justfile` — `run-debug-57a4e9a8842e` recipe added for this issue's
  repro logging (`friction.svg.import`, `friction.containerbox.insert`,
  `SvgElementTrack`)

## Grill Log

### 2026-07-04

- Q: Is "Empty Link" evidence of failed import, or just the box's default
  name? — A: (resolved via debug-build repro) it's the default name;
  content imports fine.
- Q: Is "Ruby 0"/"Desks 0" (seen in other scenes) the outer link box's name
  or a nested child's, and could it be ordinary duplicate-name handling? —
  A: it's the outer box's name, but those scenes use Import, not Link — a
  structurally different, unrelated code path.
- Q: Could this be an artifact of the recent "stop-flattening" fix (#73)? —
  A: no — `ruby.svg`'s top-level `"Ruby"` group already has 11 children,
  above the single-child flattening threshold, so fix #73 doesn't change
  its top-level box count/name in either old or new code.
- Q: (after repro) does the relinked box actually have empty content, or
  just an unhelpful name? — A: content is present and correct; only the
  name is the problem.
- Q: should Link auto-rename overwrite a manual rename on every reload, or
  only apply once? — A: neither exactly — track separate manual-name and
  auto-name fields; manual-name wins when set, auto-name updates freely
  otherwise. "Reset to automatic name" is a separate follow-up issue.
- Q: (follow-up) is adding `mAutoName`/`mManualName` state undesirable
  "extra plumbing" to minimize? — A: no — that's the intentional design,
  not a side effect to avoid; it's exactly the state the future "reset to
  automatic name" action needs.
