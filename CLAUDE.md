# Friction

## Building

- **Debug build**: `just build-debug` (builds to `build-debug-arm64/`)
- **Release build**: `just build-mac-arm` (builds to `build-release-arm64/`)

Use `just build-debug` when iterating on code changes. Use `just build-mac-arm` for release/packaging.

Please keep earlye-fork.md updated as new features are built and introduced.

## Starting Work on an Issue

- The first thing to do when starting work on an issue is add a
  `just run-debug-{issue-id}` recipe to the `justfile` (see the
  existing `run-debug-*` recipes for examples), enabling whatever
  `QT_LOGGING_RULES` categories are relevant to the issue. This gives
  the user an easy way to manually reproduce and test the issue
  throughout the session, and keeps a record of which log categories
  were relevant.
- `{issue-id}` is the short id used in the issue's filename and branch
  name (e.g. `issues/issue-<uuid7>-{issue-id}-slug.md` and branch
  `issues/{issue-id}`).
- Keep the recipe up to date as tracing needs change during
  investigation.

## Pull Requests

- Always create PRs against the fork (`earlye/friction`), never against the upstream (`friction2d/friction`).

## Pulling Upstream Changes

- When incorporating upstream changes, use `git blame` to compare authorship and understand what changed.

- Stop and ask the user what to do if any upstream changes are incompatible or contradictory to fork changes.

- Prefer `QLoggingCategory`-based logging when it is already present in the fork; do not remove it in favor of plain `qDebug()`.

- When done incorporating upstream changes, update the list of incorporated upstream commits below.

- The last upstream commit before this fork was created: `3701b0559990eae4fa315b7215a11d46122cb97b` ("Update skia")

- The last upstream commit incorporated into this fork: `bed346462745c9ae3f55c47e5329d4e63c24efe5` ("Disable offline docs")
