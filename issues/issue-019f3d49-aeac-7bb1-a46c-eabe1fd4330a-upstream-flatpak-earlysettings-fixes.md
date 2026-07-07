# Upstream pull: Flatpak/earlySettings fixes

## Context

Part of the upstream-pull breakdown tracked in
[issue-019f3869-037a-7843-94b7-da4b8f32ee02-pull-upstream-fixes.md](issue-019f3869-037a-7843-94b7-da4b8f32ee02-pull-upstream-fixes.md).

Upstream commit `52a654d8d` ("Flatpak/earlySettings fixes") reworks
early-startup settings handling in `main.cpp` and `appsupport.cpp`,
and adjusts the Quick Setup wizard (`quicksetup.cpp`,
`quicksetupgeneral.cpp`) accordingly.

**Dependency**: this commit touches `src/ui/wizards/quicksetup.cpp`
and `quicksetupgeneral.cpp`, created by the
[upstream-quicksetup-wizard](issue-019f3d49-aeba-7d60-aa6a-4be82f3bd0a9-upstream-quicksetup-wizard.md)
batch. Pull that batch first.

## Relevant files

- `src/app/main.cpp`
- `src/core/appsupport.cpp`
- `src/ui/wizards/quicksetup.cpp`
- `src/ui/wizards/quicksetupgeneral.cpp`

## Next steps

- Confirm `upstream-quicksetup-wizard` has already landed on `main`.
- Cherry-pick `52a654d8d` from `upstream/main`.
- Build and test first-run / Flatpak-specific settings behavior if
  reproducible locally.
- Open a PR against `earlye/friction`.
