# Upstream PR: SVG stroke-width import bug

## Context

`earlye-fork.md` Bug Fixes / SVG Import: fork PR
[#66](https://github.com/earlye/friction/pull/66) fixed SVG
stroke-width import — style-parsed widths were reset when the direct
attribute was absent, and element transform scale was double-applied
(once at import, once at render). Tagged `pre-existing` (bug was in
upstream `BoxSvgAttributes::loadBoundingBoxAttributes`) and `need PR`.

## Relevant files

Fork PR #66 touched:

- `src/core/svgimporter.cpp`

## Next steps

- Check out latest `upstream/main` (`friction2d/friction`).
- Cherry-pick fork PR #66's commit or rebuild the fix directly against
  upstream's current `svgimporter.cpp` if it has diverged since #66
  landed in the fork.
- Open a PR against `friction2d/friction`.
- Upstream explicitly does not want `Co-Authored-By` trailers in git
  commits — omit them from any commits prepared for this PR.
