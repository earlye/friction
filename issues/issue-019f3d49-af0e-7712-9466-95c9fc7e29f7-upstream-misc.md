# Upstream pull: misc (settingsdialog GLES fix + structural merges)

## Context

Part of the upstream-pull breakdown tracked in
[issue-019f3869-037a-7843-94b7-da4b8f32ee02-pull-upstream-fixes.md](issue-019f3869-037a-7843-94b7-da4b8f32ee02-pull-upstream-fixes.md).

The only real content commit left over after the other 12 batches:

- `0f10e7f22` — Update settingsdialog.cpp ("Don't add the plugins
  widget if GLES"). **Dependency**: only makes sense after
  [upstream-gles3-support](issue-019f3d49-aec7-7e31-aba0-37417806d215-upstream-gles3-support.md)
  (`7bed5df2c`) has landed — pull that first.

The remaining commits in the 60-commit range are **structural merge
commits** that fold upstream's `v1.0.0-rc.4-dev` integration branch
back into `main` (`9a77c55b8`, `b4dc56d9f`, `91c99afbb`, `353eec530`,
`9817ec231`). They carry no independent content beyond what's already
captured by cherry-picking the topic batches above — no action needed
for these once every other batch has landed, they're listed here only
so the full 60-commit range is accounted for.

## Relevant files

- `src/app/GUI/Settings/settingsdialog.cpp`

## Next steps

- Confirm `upstream-gles3-support` has already landed on `main`.
- Cherry-pick `0f10e7f22` from `upstream/main`.
- Build and confirm the plugins settings widget is hidden when GLES is
  active.
- Once all 13 sub-issues have landed, confirm `git log
  bed346462745c9ae3f55c47e5329d4e63c24efe5..upstream/main` is now
  fully accounted for (every remaining commit should be one of the
  structural merges above), then update `CLAUDE.md`'s "last upstream
  commit incorporated" pointer to `upstream/main`'s current HEAD.
- Open a PR against `earlye/friction`.
