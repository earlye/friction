# Upstream pull: layer-type icons on timeline

## Context

Part of the upstream-pull breakdown tracked in
[issue-019f3869-037a-7843-94b7-da4b8f32ee02-pull-upstream-fixes.md](issue-019f3869-037a-7843-94b7-da4b8f32ee02-pull-upstream-fixes.md).

Upstream adds a per-box-type icon (circle, rect, text, null, image,
video, sound, group, link, image-sequence, path) shown on each layer
row in the Boxes list, ref
https://github.com/friction2d/friction/issues/504. Follow-up commits
add a colorAnimatorButton width tweak.

`b92fb382b` was grouped into this batch purely because it's another
`boxsinglewidget.cpp` commit adjacent in the log — its actual content
is unrelated: it just reworks the HiDPI icon-scaling warning dialog's
text. Pull it along with the rest for convenience, but don't expect it
to interact with the icon feature.

Commits (oldest first):

- `b92fb382b` — Update boxsinglewidget.cpp (unrelated: HiDPI
  scaling-warning text change, not layer-icon work)
- `e1c213522` — UX: add icons (for layer type) on timeline
- `1a4d7d422` — Update boxsinglewidget.cpp
- `e8ce39a7c` — make colorAnimatorButton match other widgets width
  (also touches `src/core/GUI/coloranimatorbutton.cpp`, which the fork
  doesn't otherwise modify — no conflict risk there)

## Relevant files / conflict risk

- `src/app/GUI/BoxesList/boxsinglewidget.cpp` — the fork also actively
  modifies this file (lock-icon flash, macOS-only widget limiting,
  key-event handling — see fork commits `09da7b7e0`, `8711748ec`,
  `4855bba98`). The upstream `e1c213522` hunk that adds the icon
  button is purely additive (new `mBoxButton` inserted into the
  layout, new static `QPixmap*` members) and doesn't touch the same
  lines as the fork's additions, so a clean textual merge is expected
  — but verify with `git blame` on the merged result, and check
  interaction with the fork's "limit boxes widget changes to macOS"
  commit (does the new icon button need the same macOS gating?).
- `src/core/GUI/coloranimatorbutton.cpp` — touched by `e8ce39a7c`, not
  otherwise modified by the fork, no conflict expected.

## Next steps

- Cherry-pick the 4 commits above (in order) from `upstream/main`.
- Use `git blame` on `boxsinglewidget.cpp` post-merge to confirm no
  fork logic was silently overwritten.
- Build and manually verify: layer icons show correctly per box type,
  and the fork's lock-icon-flash / macOS-limiting / key-event features
  still work.
- Open a PR against `earlye/friction`.
