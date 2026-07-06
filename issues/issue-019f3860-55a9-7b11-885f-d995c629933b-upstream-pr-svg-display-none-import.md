# Upstream PR: SVG import ignoring display:none/visibility:hidden

## Context

`earlye-fork.md` Bug Fixes / SVG Import: fork PR
[#82](https://github.com/earlye/friction/pull/82) fixed SVG
import/link silently ignoring `display:none`/`visibility:hidden` —
values were parsed but discarded as no-ops, so hidden groups/layers
(including nested Inkscape layers) always rendered visible. Tagged
`pre-existing` (bug was in upstream
`BoxSvgAttributes::loadBoundingBoxAttributes`) and `need PR`.

**Important**: #82's original fix was incomplete/buggy on its own —
it inherited hidden state down through every descendant of a hidden
ancestor (`BoxSvgAttributes::setParent`), which broke SVG flipbook
pages authored as Inkscape layers (Inkscape saves `display:none` on
all-but-the-edited layer as an editing artifact). Fork PR
[#83](https://github.com/earlye/friction/pull/83) corrected this:
`setParent` no longer inherits `mHidden` from the parent — each
element's hidden state reflects only its own explicit attribute,
relying on `ContainerBox::processChildData` already stopping recursion
into an invisible box's children at render time to still hide the
whole subtree. **The upstream PR must include both #82 and #83's
changes together** — porting #82 alone would reintroduce the #83
regression upstream.

## Relevant files

- `src/core/svgimporter.cpp` — both #82 and #83
- `src/core/Boxes/containerbox.cpp` — #83 only (the early-return
  cross-reference comment; the actual render-time skip behavior this
  relies on already existed)

## Next steps

- Check out latest `upstream/main` (`friction2d/friction`).
- Cherry-pick fork PR #82 **and** #83 together (or rebuild the
  combined, corrected fix directly against upstream's current code if
  it has diverged) — do not port #82 without #83.
- Open a single PR against `friction2d/friction` covering both.
- Upstream explicitly does not want `Co-Authored-By` trailers in git
  commits — omit them from any commits prepared for this PR.
