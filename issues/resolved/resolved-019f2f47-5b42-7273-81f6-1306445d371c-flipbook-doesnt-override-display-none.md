# SVG flipbook pages stay hidden regardless of selected page

## Context

`ruby.svg` (`~/Documents/github.com/ehlingers/slime-at-school/characters/ruby.svg`)
has a `glasses-flipbook` group with pages `glasses-1`, `glasses-angry`,
`glasses-3`, each authored as an Inkscape layer. Inkscape always saves
all-but-the-currently-edited layer with `style="display:none"` — that's an
editing artifact, not intended-permanent-hidden content.

After #82 (fixing SVG import/link silently ignoring `display:none` and
`visibility:hidden`), Ruby's glasses are hidden regardless of the
`glasses-flipbook`'s selected page/index.

## Root cause (confirmed via code trail + one runtime log capture)

`BoxSvgAttributes::setParent` (`src/core/svgimporter.cpp`) inherited
`mHidden` from the parent unconditionally, and a child's own attributes
could only turn it further on, never back off. So every descendant of a
`display:none` page group (e.g. `glasses-bridge`, `glasses-border-l/r`
inside `glasses-1`) got its own `mHidden = true` baked in, and each one
individually got `box->setVisibleFromAnimation(false)` called on it at
import (`BoxSvgAttributes::apply`).

`SvgFlipbookTrack::syncToTargets` (`src/core/Boxes/svgflipbooktrack.cpp:112`)
only calls `setVisibleFromAnimation(visible)` on the resolved **page-level**
box (e.g. `glasses-1`), never recursing into its children. So toggling the
page container visible did nothing — every child inside it had already been
individually, irreversibly stamped invisible at import.

This propagation was also unnecessary: `ContainerBox::processChildData`
(`src/core/Boxes/containerbox.cpp:1002`) already stops recursing into a
box's children the moment the box itself fails
`isFrameFVisibleAndInDurationRect` — an invisible container already hides
everything under it at render time, without needing each descendant's own
visibility flag touched. Baking `mHidden` into descendants during import
was solving an already-solved problem, and did so in an irreversible way
that broke anything (like a flipbook) that re-toggles an ancestor's
visibility after import.

Confirmed one part live: ran the existing debug build (pre-fix, commit
`dd6eb08e`) against
`~/Documents/github.com/ehlingers/slime-at-school/test-scenes/ruby-test-3ecf05783ce1.friction`
with `friction.svg.import`/`friction.svgflipbooktrack` logging
(`just run-debug-1306445d371c`). That scene's flipbook index is `2`
(`glasses-3`), which is a genuinely empty `<g>` (self-closing, no children)
pruned during import — correctly resolves to "no glasses" and is not part
of this bug. The `glasses-1`/`glasses-angry` descendant-stamping mechanism
was confirmed via static code trail (SVG source inspection +
`setParent`/`apply` reading) rather than a second live repro at a different
flipbook index; not yet re-verified against the fix build.

## Fix (implemented, verified)

`src/core/svgimporter.cpp`:
- `BoxSvgAttributes::setParent` no longer inherits `mHidden` from the
  parent. Each element's `mHidden` now reflects only its own explicit
  `display:none`/`visibility:hidden`.
- Updated the comment on `mHidden` to explain why: rendering already stops
  descending into invisible containers, so hiding an ancestor still hides
  the whole subtree; baking the state into descendants was both redundant
  and irreversible for anything that re-toggles a container's own
  visibility later (flipbooks).

This also incidentally fixes the previously-documented "known limitation"
that CSS lets a descendant's `visibility:visible` override an ancestor's
`visibility:hidden` — without inheritance, a descendant that never declares
its own hidden state simply isn't affected by an ancestor's, matching CSS
semantics for free.

Re-verified against the *original* bug this replaced (#82 / issue
`cc0c71ff5807`: `layer16` with `style="display:none"` and children with
their own `style="display:inline"`): `layer16` itself still imports hidden
(own explicit attribute), and rendering still hides its children today
since paint stops recursing into `layer16`. If `layer16` is later toggled
visible (by the user or a flipbook), children now correctly follow their
own explicit `display:inline` instead of staying stuck invisible — which is
the correct CSS-cascade behavior, not just an accident of this fix.

## Verification

Built the fix in this worktree (`just build-debug`, first build here) and
ran it against `ruby-test.friction`
(`just run-debug-1306445d371c`), whose `glasses-flipbook` defaults to
index 0. Log confirmed page-level toggling as expected
(`"glasses-1" visible: true`, `"glasses-angry" visible: false`; switching
to index 1 flips them). More importantly, the user visually confirmed in
the running app: the glasses geometry itself renders (not just the empty
page container), and switching the flipbook page swaps the glasses
geometry correctly.
