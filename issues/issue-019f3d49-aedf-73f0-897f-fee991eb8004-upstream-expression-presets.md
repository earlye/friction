# Upstream pull: Expression Presets / frame remapping

## Context

Part of the upstream-pull breakdown tracked in
[issue-019f3869-037a-7843-94b7-da4b8f32ee02-pull-upstream-fixes.md](issue-019f3869-037a-7843-94b7-da4b8f32ee02-pull-upstream-fixes.md).

Upstream adds Expression Presets for easy "frame remapping" (a
feature branch `expressions-presets-frame-remap-v1`, merged into
`main`), plus follow-up tweaks after the merge.

Commits (oldest first, per branch history):

- `9ff91e5bb` — Update ExpressionPresets
- `5e7ffd799` — Update MainWindow
- `e9d892c18` — Update expressiondialog.cpp
- `b7df5c737` — add Expression Presets for easy "frame remapping"
- `8f38c869d` — add "expressions preset description" to Presets
  dropdown tooltip
- `4508b4681` — Update expressiondialog.cpp
- `464dd5aa8` — Merge branch 'expressions-presets-frame-remap-v1' into
  v1.0.0-rc.4-dev (structural — content already covered by the
  commits above; only needed if merging rather than cherry-picking)
- `e3163c5bd` — Update expr presets (post-merge follow-up)

## Relevant files

- `src/core/Expressions/expressionpresets.cpp`
- `src/app/GUI/Expressions/expressiondialog.cpp`
- `src/app/GUI/mainwindow.cpp`

No overlap with fork-specific code.

## Next steps

- Cherry-pick the non-merge commits above in the listed order from
  `upstream/main` (the merge commit `464dd5aa8` itself is structural
  and can be skipped if cherry-picking its constituent commits
  directly).
- Build and manually test expression presets / frame remapping in the
  Expression dialog.
- Open a PR against `earlye/friction`.
