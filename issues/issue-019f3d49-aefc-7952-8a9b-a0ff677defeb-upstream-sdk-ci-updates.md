# Upstream pull: SDK/CI updates (macOS/Linux FFmpeg)

## Context

Part of the upstream-pull breakdown tracked in
[issue-019f3869-037a-7843-94b7-da4b8f32ee02-pull-upstream-fixes.md](issue-019f3869-037a-7843-94b7-da4b8f32ee02-pull-upstream-fixes.md).

Upstream infra/SDK updates:

- `1d60fbe04` — Update supported containers/codecs (ffmpeg)
- `06c773c83` — Update macOS SDK/CI
- `c9f95d593` — Linux SDK: rebuild FFmpeg

Note: the fork has its own SDK-download caching work (see resolved
issue `cache-sdk-download-across-worktrees`) and its own CI workflow
files (`.github/workflows/macos.yml`, `linux.yml`, `windows.yml` — 10+
fork commits each). These upstream SDK/CI commits need careful
comparison against the fork's current CI setup rather than a blind
cherry-pick.

## Relevant files

- macOS/Linux SDK build scripts and FFmpeg build config
- Possibly `.github/workflows/*.yml` if upstream also touched CI YAML
  (verify per-commit before assuming)

## Next steps

- Diff each commit's touched files against the fork's current CI/SDK
  setup before applying — use `git blame` to see if the fork has
  already diverged in these files.
- Cherry-pick or manually port whichever pieces still apply.
- Verify CI still builds after the change (this is the one batch
  where "test" means "green CI run", not a manual app smoke test).
- Open a PR against `earlye/friction`.
