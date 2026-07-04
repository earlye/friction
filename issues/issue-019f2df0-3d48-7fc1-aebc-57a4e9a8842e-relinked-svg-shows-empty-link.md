# Deleting and re-linking ruby.svg shows "Empty Link" instead of the imported content

## Context

Surfaced while investigating issue `1148748f4eb9` (ruby's arms not
flipbooking/following properly), during an attempt to get a clean,
uncorrupted link to `ruby.svg` in `ruby-test.friction` (previous sessions'
saved state had accumulated stale/corrupted element-track data — see
`issue-019f2dd6-...-set-source-file-resets-element-track-to-zero.md`).

The user deleted the existing linked box (previously named `"Ruby"`, per
earlier binary inspection of the saved `.friction` scene, pointing at
`~/Documents/github.com/ehlingers/slime-at-school/characters/ruby.svg`)
and re-linked the same `ruby.svg` file fresh. Instead of showing the
imported content (rooted at a box that should be named `"Ruby"` or
similar, containing `head`, `left-arm`, `right-arm`, etc.), the new link
shows as **"Empty Link"**.

Not yet confirmed whether this is a regression from the fixes made this
session (`svglinkbox.cpp`'s `applyPivotDescIfPresent`,
`svgelementtrack.cpp`'s `findDescendantByName` self-check) or a
pre-existing, unrelated bug in the delete-then-relink path specifically
(as opposed to an initial/first-time link, which worked earlier this
session for the minimal repro SVG).

## Reproduction steps

1. In `ruby-test.friction` (or similar), delete the existing `SvgLinkBox`
   pointing at `ruby.svg`.
2. Re-link the same `ruby.svg` file.

**Expected:** the link populates with `ruby.svg`'s content (root box
named `Ruby`/similar, containing `head`, `left-arm`, `right-arm`, etc.).

**Actual:** the link shows as `"Empty Link"`.

## Possible relevant lead (unconfirmed)

`ImportSVG::loadSVGFile` (`src/core/svgimporter.cpp:1220-1225`) returns
`nullptr` if the imported root ends up with zero contained boxes:

```cpp
if(result->getContainedBoxesCount() == 1) {
    return qSharedPointerCast<BoundingBox>(result->takeContained_k(0));
} else if(result->getContainedBoxesCount() == 0) {
    return nullptr;
}
```

`"Empty Link"` is consistent with `SvgLinkBox::updateContent()` receiving
a null/empty `imported` result and falling back to some empty-state
display name. Not yet verified whether that's actually what's happening
here (e.g. via a debug build with `friction.svg.import` logging enabled)
or whether the cause is something else entirely (e.g. a stale file-handle
cache keyed by path, a duplicate-name collision with the deleted box's
old element tracks, etc.).

## Relevant files

- `src/core/svgimporter.cpp:1220-1225` — `ImportSVG::loadSVGFile`'s
  null-return branch on empty import result
- `src/core/Boxes/svglinkbox.cpp` — `SvgLinkBox::updateContent()`,
  `resolveElementTracks()`
- `~/Documents/github.com/ehlingers/slime-at-school/characters/ruby.svg` —
  the file being linked
- `~/Documents/github.com/ehlingers/slime-at-school/test-scenes/ruby-test.friction` —
  the scene where this was reproduced
