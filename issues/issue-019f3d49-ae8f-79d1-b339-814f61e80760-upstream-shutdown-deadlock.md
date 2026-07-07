# Upstream pull: workaround deadlock on shutdown

## Context

Part of the upstream-pull breakdown tracked in
[issue-019f3869-037a-7843-94b7-da4b8f32ee02-pull-upstream-fixes.md](issue-019f3869-037a-7843-94b7-da4b8f32ee02-pull-upstream-fixes.md).

Upstream commit `804a94c7b` ("Workaround deadlock on shutdown") fixes a
deadlock that can occur when the app closes, touching the task
scheduler and main window shutdown path.

## Relevant files

- `src/app/GUI/mainwindow.cpp`
- `src/core/Private/Tasks/taskscheduler.cpp`
- `src/ui/widgets/uilayout.cpp` / `.h`

No overlap with fork-specific code.

## Next steps

- Cherry-pick `804a94c7b` from `upstream/main` onto a branch off `main`.
- Build and manually test app shutdown (close while a render/task is
  in flight, if reproducible).
- Open a PR against `earlye/friction`.
