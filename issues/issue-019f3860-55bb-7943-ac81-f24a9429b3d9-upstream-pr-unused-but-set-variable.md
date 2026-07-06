# Upstream PR: -Wunused-but-set-variable warnings

## Context

`earlye-fork.md` Bug Fixes / Compiler Warnings: fork PR
[#17](https://github.com/earlye/friction/pull/17) fixed
`-Wunused-but-set-variable` warnings across the codebase. Tagged
`pre-existing` (warnings were in existing upstream code) and
`need PR`.

## Relevant files

Fork PR #17 touched:

- `src/app/GUI/BoxesList/boxsinglewidget.cpp`
- `src/app/GUI/graphboxeslist.cpp`
- `src/core/Animators/SmartPath/nodelist.cpp`
- `src/core/Boxes/boundingbox.cpp`
- `src/core/Boxes/containerbox.cpp`
- `src/core/Expressions/propertybinding.cpp`
- `src/core/TransformEffects/transformeffectcollection.cpp`
- `src/core/canvasselectedboxesactions.cpp`

## Next steps

- Check out latest `upstream/main` (`friction2d/friction`).
- Cherry-pick fork PR #17's commit or rebuild the fix directly against
  upstream's current code where files have diverged since #17 landed
  in the fork.
- Open a PR against `friction2d/friction`.
- Upstream explicitly does not want `Co-Authored-By` trailers in git
  commits — omit them from any commits prepared for this PR.
