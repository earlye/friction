# Replace `<desc>` YAML metadata with `inkscape:label` query-string conventions

## Context

Today, node "kind" (`animation-node`, `flipbook`, `animation-follower`,
`flipbook-follower`, `pivot`) and their fields (`map:`, `controller:`) are
declared via a YAML block inside each SVG element's `<desc>` child, parsed by
`SvgLinkBox`/`svgimporter.cpp` (see `src/core/Boxes/svglinkbox.cpp`,
`src/core/svgimporter.cpp:1299` `parseDescDocuments`, and the schema
documented in `earlye-fork.md:23-101`). Authoring this in Inkscape means
opening the XML editor and hand-editing a `<desc>` element — there's no
Inkscape UI affordance for it, so it's fiddly and error-prone for artists.

## Final convention (resolved via grilling, see Grill Log)

Metadata moves onto `inkscape:label` itself as a **query string**, not a
prefix/suffix scheme (the originally-proposed `anim--`/`flip--`/`follow--`
prefixes were superseded during grilling — see log). Grammar:

```
{display-name}?kind={animation|flipbook|follow|pivot}[&controller={name}][&page=N]
```

- `nodeA?kind=animation` — animation node (was `kind: animation-node`), no
  other params.
- `nodeA?kind=flipbook` — flipbook (was `kind: flipbook`), no other params;
  page children are declared on the children themselves (see below), not via
  a `map:`-equivalent on the flipbook node.
- `armNeutral?page=0` — a direct child of a flipbook, declares its own page
  index. Duplicate `page=` values across siblings are a hard error. Gaps /
  non-contiguous indices are allowed and expected (needed for
  flipbook-follower sparse maps, where some pages intentionally show no
  child).
- `nodeB?kind=follow&controller=nodeA` — follower (collapses the old
  `animation-follower`/`flipbook-follower` distinction into a single `follow`
  kind — see Grill Log for why). Behavior (mirror-transform vs
  mirror-flipbook-page) is dispatched at resolve time by looking up
  `controller`'s own `kind`, not declared on the follower itself. If
  `controller` resolves to a node with no `kind=` at all, or to another
  `follow` node (chained following), that's a hard error — chained following
  is explicitly unsupported for now.
- `?kind=pivot` — pivot circle. No display-name portion required (nothing
  ever references a pivot by name), no other params.
- A label with no `?` at all, or a `?` with no `kind=` key, is just a plain
  node with no special behavior — same as today.
- `controller={name}` resolves against the target's raw pre-`?` display name
  first, falling back to the underlying SVG `id` if no label match (same
  two-tier fallback as today's `findBoxByName`/`findDescendantByName`).
- Literal `?` in an artist's intended display name is **not supported** —
  no escaping mechanism. The first `?` in a label always starts the query
  string; this is a documented, reserved-character constraint (same
  category as reserved chars in `id`/`class` elsewhere). Artists who
  genuinely need a `?` character can URL-encode it.
- Error posture: bad/ambiguous references (unresolvable `controller`,
  duplicate `page=`) are hard errors, not silent skips — see
  `issue-{TODO-followup-error-surfacing}` (spun off, see below) for making
  Friction's error surfacing generally less silent.

## Scope

- **Coexistence, not replacement**: the existing YAML-in-`<desc>` mechanism
  stays fully functional indefinitely; both parsers run side by side. A box
  may be described via `kind: flipbook` YAML *or* a `?kind=flipbook` label,
  whichever is present.
- **`ruby.svg` migration is explicitly out of scope** for this issue — no
  existing YAML-authored SVGs need to be converted as part of this work.
- `kind: pivot` **is** in scope (unlike the original placeholder's ambiguity
  on this point).

## Relevant files (current YAML-desc mechanism, for reference)

- `src/core/svgimporter.cpp:1299` — `parseDescDocuments`, splits `<desc>`
  text on `---` into YAML documents
- `src/core/svgimporter.cpp:1508-1514` — `inkscape:label` is read into
  `mLabel`; `<desc>` child (if present) is parsed into `mDescYaml`
- `src/core/svgimporter.cpp:1668-1678` — `BoxSvgAttributes::apply`:
  `mLabel` (verbatim, no stripping) becomes both the box's display name
  (`prp_setName`) and the `svgInkscapeLabel` Qt property used for exact-match
  lookups; `mDescYaml` is stored via `box->setDescYaml()`
- `src/core/Boxes/svglinkbox.cpp:143-160` — `collectAnimationNodes`
  (`kind: animation-node`)
- `src/core/Boxes/svglinkbox.cpp:396-454` — `applyFlipbookDescIfPresent`/
  `collectFlipbookDescs` (`kind: flipbook`, `map:`)
- `src/core/Boxes/svglinkbox.cpp:303-329` — `collectFollowerDescs`
  (`kind: animation-follower`, `controller:`)
- `src/core/Boxes/svglinkbox.cpp:343-394` — `collectFlipbookFollowerDescs`
  (`kind: flipbook-follower`, `controller:` + `map:`)
- `src/core/svgimporter.cpp:527-550` — `kind: pivot`, parsed directly off a
  `<circle>` element
- `src/core/Boxes/svgflipbooktrack.cpp:39-99` — `findDescendantByName`/
  `findDescendantByLabel`, current label/id resolution for flipbook page
  targets — the new label-query parser should reuse this same fallback
  order (label, then id)
- `src/core/Boxes/svglinkbox.cpp:245-254` — `findBoxByName`, current
  controller-name resolution for followers — new `follow` kind reuses this,
  then additionally inspects the resolved controller's own `kind`
- `earlye-fork.md:23-101` — canonical doc of the current YAML schema
  (missing `flipbook-follower`, a pre-existing doc gap; will also need a new
  section for the label-query convention once implemented)

## Next steps

- **Audit the code for gaps before implementing**: walk through
  `svglinkbox.cpp`/`svgimporter.cpp`/`svgflipbooktrack.cpp` against the
  final grammar above and confirm every resolution path (label parsing,
  controller/page lookup, dispatch-by-controller's-kind) has a clean
  implementation point. Resurface any open question here if the audit finds
  a gap the grill session didn't anticipate.
- Update `earlye-fork.md` with the new convention once implemented (and fix
  the pre-existing `flipbook-follower` doc gap while there).
- Two related issues spun off from this grilling session (see below).

## Related issues

- `issues/issue-019f2ff5-ec1f-7121-a121-6273b7a1731c-friction-silent-failures-need-visibility.md`
  — Friction's tendency to silently swallow errors (relevant here since the
  new label-query parser adopts a hard-error posture that needs *some* way
  to surface those errors to the artist).
- `issues/issue-019f2ff5-ec27-7c83-9c95-3dc99fd1649a-flipbook-track-editor-page-combobox.md`
  — flipbook track editor should let you pick a page by its `inkscape:label`
  in a combo box, not by raw page number.

## Grill Log

### 2026-07-04

- Q: Coexist with the YAML-in-`<desc>` mechanism, or fully replace it
  (converting `ruby.svg`)? — A: Coexist, at least temporarily. Converting
  `ruby.svg` is out of scope for this issue.
- Q: Do the `anim--`/`flip--`/`follow--`/`flip-follow--` prefixes get
  stripped before display in Friction's tree/timeline, since labels are
  shown verbatim today and also double as the exact-match identity key? —
  A: This drove a full convention change: drop prefixes entirely, use pure
  query-string style instead (`nodeA?kind=animation`,
  `nodeB?kind=follow&controller=nodeA`). Everything before the first `?` is
  simultaneously the display name and the resolvable identity key — no
  stripping or second derived-name concept needed.
- Q: Does the flipbook page-ordering convention (originally proposed as an
  `N--child-label` prefix) change too, given the query-string pivot? — A:
  Yes — `?page=N` on the child, e.g. `arm-neutral?page=0`. Implicit
  ordering by SVG sibling/document order was considered and rejected:
  reordering elements in Inkscape would silently break any scene referring
  to a specific page index.
- Q: What are the exact `kind=` value strings? — A: `animation`,
  `flipbook`, and (see next question) a single collapsed `follow` — no
  separate `flip-follow`.
- Q: How does one `kind=follow` value cover both the old
  `animation-follower` and `flipbook-follower` behaviors? — A: The follower
  doesn't declare which kind of following it does. At resolve time, look up
  `controller`, then dispatch behavior based on *that* node's own `kind`
  (mirror-transform if `animation`, mirror-page if `flipbook`). This
  structurally eliminates kind-mismatch authoring mistakes, since there's
  only one follower kind.
- Q: What happens if `controller` points at a kind-less node, or chains to
  another `follow` node? — A: Hard error for now; chained following is not
  supported. Separately, the user wants a follow-up issue tracking
  Friction's general tendency to silently swallow errors rather than
  surfacing them — not "annoying popups," but *some* visible signal that
  something didn't resolve as expected.
- Q: Is `kind: pivot` in scope for this convention, or does it stay
  YAML-only? — A: In scope.
- Q: Does the pivot circle's label need a display-name portion? — A: No —
  `?kind=pivot` alone (empty name before `?`) is sufficient, since nothing
  ever references a pivot by name.
- Q: Does `controller={name}` resolve by label match only, or also fall
  back to the underlying SVG `id` (like today's two-tier
  `findBoxByName`/`findDescendantByName`)? — A: Label match is sufficient
  in practice (editing `id`s in Inkscape is annoying), but keeping the `id`
  fallback isn't objected to — so both, same as today, label first.
- Q: Should duplicate/gapped/non-contiguous `page=` values hard-error, same
  as the controller-resolution error posture? — A: Duplicates are a hard
  error. Gaps are explicitly fine — they're a required pattern for
  flipbook-followers with sparse maps (some pages intentionally show
  nothing). Also requested during this answer: a new issue for the
  flipbook track editor to show a page-selector combo box using
  `inkscape:label` instead of raw page numbers.
- Q: Could a literal `?` be part of an artist's intended display name? — A:
  Not supported, no escaping mechanism added; it's a documented reserved
  character. Artists who need it can URL-encode.
- Q: Final grammar sanity check (full spec above) — confirmed as correct;
  user asked that the code be audited for implementation gaps against this
  grammar before work starts, and that this issue be resurfaced if the
  audit finds anything the grilling session didn't anticipate.
