# Upstream pull: AskDialog UI widget

## Context

Part of the upstream-pull breakdown tracked in
[issue-019f3869-037a-7843-94b7-da4b8f32ee02-pull-upstream-fixes.md](issue-019f3869-037a-7843-94b7-da4b8f32ee02-pull-upstream-fixes.md).

Upstream commit `9623be1ad` ("Ui: added AskDialog") adds a new
`AskDialog` widget to use instead of `QMessageBox` when offering a
"Don't ask again" / "Don't show again" option, with built-in settings
persistence via `AskDialog::ask(...)`.

This is a standalone new file addition (`src/ui/dialogs/askdialog.cpp`
/ `.h`) with no callers in this commit — later upstream commits may
adopt it, but none of those are in this fork's 60-commit pull range,
so pulling this commit alone just adds an unused-but-available widget.

## Relevant files

- `src/ui/dialogs/askdialog.cpp` / `.h` (new)
- `src/ui/CMakeLists.txt`

## Next steps

- Cherry-pick `9623be1ad` from `upstream/main`.
- Build to confirm the new files compile cleanly (nothing to
  functionally test since it's unused so far).
- Open a PR against `earlye/friction`.
