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

## Fix decided

Expand `Property::prp_sFixName`'s allow-list regex
(`src/core/Properties/property.cpp:285`) to also permit `-` (underscore is
already allowed), i.e. change
`result.remove(QRegExp("[^A-Za-z0-9 _]"));` to
`result.remove(QRegExp("[^A-Za-z0-9 _-]"));`.

This applies uniformly everywhere `prp_sFixName`/`makeNameUnique` runs
(SVG import via `ContainerBox::insertContained`, user-typed renames via
`eboxorsound.cpp`, custom property auto-naming), rather than special-casing
SVG-imported names to skip sanitization. Rejected the "skip sanitizer for
SVG imports" alternative: it would require threading a defaulted parameter
through `insertContained`/`addContained`/`makeNameUniqueForDescendants` and
updating 8 call sites in `svgimporter.cpp`, for no demonstrated benefit —
research turned up no downstream breakage from hyphens in names (XEV export
writes names as plain XML attribute values; SVG export already runs names
through a separate `AppSupport::filterId` sanitizer before using them as
SVG `id`s; expression bindings split paths on `.`, not name content; and
`staticcomplexanimator.cpp:42` already hardcodes a hyphenated property name
— "sub-path effect" — with no issues).

Scope: hyphen only, not a broader punctuation allow-list. `inkscape:label`
is free-text XML and can contain arbitrary characters, so no small
allow-list fully round-trips every possible label — but the confirmed bug
is specifically about hyphens, so the fix targets that without guessing at
a wider "safe" set.

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

- Q: Expand `prp_sFixName`'s allow-list, or make SVG-imported names skip
  sanitization entirely? — A: Expand the allow-list (uniform, one-line
  fix). Researched downstream uses of `prp_getName()` first (expression
  bindings, XEV export, SVG export id sanitization, existing hyphenated
  name precedent) and found no evidence hyphens break anything, making the
  more invasive per-call-site skip unnecessary.
- Q: What counts as "legal" in the source XML/SVG that should inform the
  allow-list's breadth? — A: `inkscape:label` is free-text XML (almost any
  character survives parsing); `id` is constrained to the XML `Name` token
  set. Since labels aren't actually constrained to a small legal set, no
  allow-list can fully round-trip arbitrary labels — so scope the fix to
  the confirmed bug (hyphen) rather than guessing at a broader "safe" set.
