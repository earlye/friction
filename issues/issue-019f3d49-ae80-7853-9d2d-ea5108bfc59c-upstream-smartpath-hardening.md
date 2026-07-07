# Upstream pull: SmartPath hardening (remove throws)

## Context

Part of the upstream-pull breakdown tracked in
[issue-019f3869-037a-7843-94b7-da4b8f32ee02-pull-upstream-fixes.md](issue-019f3869-037a-7843-94b7-da4b8f32ee02-pull-upstream-fixes.md).

Upstream (`friction2d/friction`) replaced several `throw`s in the
`SmartPath` node/interpolation code with checks and fallbacks, per
https://github.com/friction2d/friction/issues/779. These are crash
fixes — a `throw` deep in path interpolation is a hard crash for the
fork's users too.

Commits (oldest first):

- `cecef826a` — SmartPath: fix throw in nodelist interpolate
  (`src/core/Animators/SmartPath/nodelist.cpp`)
- `64fc71417` — SmartPath: fix throw in interpolate normal/dissolved
  (`src/core/Animators/SmartPath/node.cpp`)
- `1e6d135a1` — SmartPath: harden nodes
  (`src/core/Animators/SmartPath/nodelist.cpp`)
- `58c4d25cd` — SmartPath: add checks and replace throw
  (`src/core/Animators/SmartPath/smartpath.cpp`)

## Relevant files

- `src/core/Animators/SmartPath/nodelist.cpp`
- `src/core/Animators/SmartPath/node.cpp`
- `src/core/Animators/SmartPath/smartpath.cpp`

No overlap with fork-specific SVG import/flipbook-track code.

## Next steps

- Cherry-pick the 4 commits above (in order) from `upstream/main` onto
  a branch off `main`.
- Build and smoke-test path interpolation (dissolve/normal nodes).
- Open a PR against `earlye/friction`.
