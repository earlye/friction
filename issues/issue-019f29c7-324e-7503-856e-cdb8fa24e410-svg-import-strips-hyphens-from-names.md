# SVG-imported box names silently lose hyphens (and other characters)

## Context

Split out from investigating "ruby flipbook not working"
(`issue-019f28b5-3e6a-76a0-975f-1b9fca3b71f0-ruby-flipbook-not-working.md`).
While debugging why `SvgFlipbookTrack` page resolution behaved
inconsistently in `ruby.svg` (loaded via `SvgLinkBox` in
`~/Documents/github.com/ehlingers/slime-at-school/episodes/s01e01/scene-01-line-01.friction`),
a debug build (`just build-debug` + `just run-debug-timeline`, logging
`SvgElementTrack=true;friction.svgflipbooktrack=true` to `log.txt`) showed
every imported box's name with all hyphens removed — e.g. an element whose
SVG source is `inkscape:label="right-arm-pointing"` shows up in friction as
`"rightarmpointing"`. Same pattern confirmed for `leftarmpointing`,
`rightarmholla`, `rightarmneutral`, `leftleg0`, `rightleg0`, `mouthaheh`,
`mouthopen`, `mouthsmile`, `rightarm`, `leftarm`, `head`, etc. — every
labeled box in the tree, not something specific to flipbooks.

## Root cause (confirmed)

`BoxSvgAttributes::apply` (`src/core/svgimporter.cpp:1632-1636`) correctly
sets the box's name from the SVG label:

```cpp
if (!mLabel.isEmpty()) {
    box->prp_setName(mLabel);
    box->setProperty("svgInkscapeLabel", mLabel);
} else if (!mId.isEmpty()) { box->prp_setName(mId); }
```

But every newly-created box then goes through
`ContainerBox::insertContained` (`src/core/Boxes/containerbox.cpp:1194-1199`):

```cpp
const QString oldName = child->prp_getName();
const auto nameCtxt = parentScene ? parentScene : this;
const QString newName = nameCtxt->makeNameUniqueForDescendants(oldName);
child->prp_setName(newName);
```

`makeNameUniqueForDescendants` (`containerbox.cpp:488-491`) calls
`NameFixer::makeNameUnique` (`src/core/namefixer.cpp:41-47`), whose first
step is `Property::prp_sFixName` (`src/core/Properties/property.cpp:282-293`):

```cpp
result.remove(QRegExp("[^A-Za-z0-9 _]"));
```

This allow-lists only letters, digits, space, and underscore, so every `-`
(and any other punctuation) in the label is silently deleted, overwriting
the correct name `BoxSvgAttributes::apply` just set. `prp_sFixName` is a
general-purpose "make this a legal property name" sanitizer, also used for
manual layer renames (`src/core/Animators/eboxorsound.cpp:524-533`) and
autonaming (`src/core/customproperties.cpp:40/83`) — it doesn't appear to
have been designed with SVG-imported labels in mind, but
`ContainerBox::insertContained` runs it unconditionally on every imported
box regardless of source.

## Impact / scope

- Breaks any name-based lookup that expects the box's `prp_getName()` to
  match its original SVG `inkscape:label` exactly whenever that label
  contains a hyphen (or other non-`[A-Za-z0-9 _]` character) — e.g.
  `SvgFlipbookTrack`/`SvgElementTrack`'s primary name-match path in
  `findDescendantByName` (`src/core/Boxes/svgflipbooktrack.cpp`). In
  practice this class of lookup still often works via the fallback
  `svgElementId`/`svgInkscapeLabel` properties, which are set via
  `setProperty` and are *not* run through the sanitizer — so this bug is
  frequently masked rather than fully blocking, but makes names shown in
  the UI/logs diverge from the source SVG, which made this whole
  investigation much harder than it needed to be.
- Any hyphen-based (or otherwise punctuated) naming convention in imported
  SVG artwork silently degrades on import.

## Next steps

- Decide what `prp_sFixName`'s allow-list should actually protect against
  (likely: names that would break something specific downstream — XEV
  export attribute values, undo/redo naming, etc.) and whether `-` (and
  maybe other common punctuation) can simply be added to the allow-list.
- Alternatively, consider whether SVG-imported names should skip
  `prp_sFixName` entirely (they come from `id`/`inkscape:label`, which are
  already valid XML attribute values) and only apply the sanitizer to
  user-typed renames.
- After fixing, re-verify with `just run-debug-timeline` that box names in
  `log.txt` match their source `inkscape:label` exactly, including
  hyphens.

## Grill Log

### 2026-07-03

- Investigated as part of the mouth-flipbook debugging session for
  `issue-019f28b5-3e6a-76a0-975f-1b9fca3b71f0-ruby-flipbook-not-working.md`.
  Root-caused via code search (see citations above) after noticing every
  box name in `log.txt` was missing hyphens present in the source SVG.
  Confirmed as real but not the cause of the specific "three mouths visible
  at once" symptom (that was single-child `<g>` flattening, tracked
  separately) — split into its own issue since it's a distinct, more
  systemic bug in a different subsystem (name sanitization vs. SVG group
  construction).
