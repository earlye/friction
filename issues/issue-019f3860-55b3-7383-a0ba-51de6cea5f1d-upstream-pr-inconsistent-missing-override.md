# Upstream PR: -Winconsistent-missing-override warning in SvgLinkBox

## Context

`earlye-fork.md` Bug Fixes / Compiler Warnings: fork PR
[#11](https://github.com/earlye/friction/pull/11) fixed a
`-Winconsistent-missing-override` warning in `SvgLinkBox`. Tagged
`pre-existing` (warning was in existing upstream `SvgLinkBox`) and
`need PR`.

## Relevant files

Fork PR #11 touched:

- `src/cmake/friction-common.cmake`
- `src/core/Boxes/svglinkbox.h`

## Next steps

- Check out latest `upstream/main` (`friction2d/friction`).
- Cherry-pick fork PR #11's commit or rebuild the fix directly against
  upstream's current `svglinkbox.h` if it has diverged since #11
  landed in the fork.
- Open a PR against `friction2d/friction`.
- Upstream explicitly does not want `Co-Authored-By` trailers in git
  commits — omit them from any commits prepared for this PR.
