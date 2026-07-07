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

### Conflict risk

`mainwindow.cpp` and `uilayout.cpp` both overlap with fork changes,
but only via the fork's mechanical qDebug→qCDebug logging conversion
(`98c52ecd2`) and a prior upstream-incorporation commit (`c2ac58b2c`)
— low risk. Per the fork's stated preference, keep `QLoggingCategory`
usage intact rather than reverting to plain `qDebug()` if this
commit's diff touches any of those call sites.

## Next steps

- Cherry-pick `804a94c7b` from `upstream/main` onto a branch off `main`.
- Build and manually test app shutdown (close while a render/task is
  in flight, if reproducible).
- Open a PR against `earlye/friction`.
