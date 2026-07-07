# Pull upstream fixes

## Context

Per `CLAUDE.md`'s "Pulling Upstream Changes" section, the last upstream
commit incorporated into this fork is `bed346462745c9ae3f55c47e5329d4e63c24efe5`
("Disable offline docs"). As of this issue's creation, `upstream/main`
has **60 commits** beyond that point which have not been pulled into
the fork.

These 60 commits are a mix of:

- **Bug/crash fixes**: SmartPath hardening (`58c4d25cd`, `1e6d135a1`,
  `64fc71417`, `cecef826a` — replace throws with checks in
  interpolate/nodelist code), shutdown deadlock workaround
  (`804a94c7b`), default to CPU mode on raster effects (`de226cc52`),
  Flatpak/earlySettings fixes (`52a654d8d`).
- **New features**: Quick Setup wizard (`34d0b0ff9` and ~10 related
  commits under `src/ui/wizards/`), initial GLES3 support
  (`7bed5df2c`), step-rotation UX (`cfe2d62d2`), Expression Presets /
  "frame remapping" (`b7df5c737`, `8f38c869d`, `e3163c5bd`,
  `464dd5aa8`), layer-type icons on timeline (`e1c213522`).
- **Infra/SDK updates**: macOS/Linux SDK + FFmpeg rebuilds
  (`06c773c83`, `c9f95d593`, `1d60fbe04`), CMakeLists/resources changes.
- **File IO / render widget changes**: `b8e3e7b86` and several
  `renderinstancewidget.cpp` / `outputsettingsdialog.cpp` updates.

Full list: `git log --oneline bed346462745c9ae3f55c47e5329d4e63c24efe5..upstream/main`

### Conflict risk

Comparing files touched by the fork (`git log
3701b0559990eae4fa315b7215a11d46122cb97b..main --stat`) against files
touched by these 60 upstream commits, only one file overlaps:
`src/app/GUI/BoxesList/boxsinglewidget.cpp` — touched by upstream
commits `e8ce39a7c`, `1a4d7d422`, `e1c213522` (layer-type icons, +76
lines), and `b92fb382b`. The fork's SVG-import/flipbook-track work
(`svglinkbox.cpp`, `svgimporter.cpp`, `svgflipbooktrack.*`,
`svgelementtrack.*`) is untouched by these upstream commits, so no
conflict is expected there.

## Root cause / motivation

N/A — this is a maintenance task to keep the fork current with
upstream, not a bug fix.

## Plan

This issue is a **meta/tracking issue only** — it does not itself pull
any commits. The 60 unincorporated commits are split into independently
pullable, independently testable topic batches, each tracked as its
own `issue-{uuid7}-upstream-{description}.md` file (mirroring the
existing outbound `upstream-pr-*` naming convention, but for inbound
pulls):

1. [upstream-smartpath-hardening](issue-019f3d49-ae80-7853-9d2d-ea5108bfc59c-upstream-smartpath-hardening.md) — `58c4d25cd`, `1e6d135a1`, `64fc71417`, `cecef826a`
2. [upstream-shutdown-deadlock](issue-019f3d49-ae8f-79d1-b339-814f61e80760-upstream-shutdown-deadlock.md) — `804a94c7b`
3. [upstream-cpu-raster-default](issue-019f3d49-ae9f-77d0-ac16-20917aa9d657-upstream-cpu-raster-default.md) — `de226cc52` (depends on #5)
4. [upstream-flatpak-earlysettings-fixes](issue-019f3d49-aeac-7bb1-a46c-eabe1fd4330a-upstream-flatpak-earlysettings-fixes.md) — `52a654d8d` (depends on #5)
5. [upstream-quicksetup-wizard](issue-019f3d49-aeba-7d60-aa6a-4be82f3bd0a9-upstream-quicksetup-wizard.md) — `34d0b0ff9`, `6fa448fc2`, `c7ec58a7d`, `d1cfa6d24`, `400373669`, `41029ece5`, `092211f6a`, `6aa0cb58a`, `f3c6e673c`, `c0cdc5d71`, `3f6dc71e0`
6. [upstream-gles3-support](issue-019f3d49-aec7-7e31-aba0-37417806d215-upstream-gles3-support.md) — `7bed5df2c` (dependency for #13)
7. [upstream-step-rotation](issue-019f3d49-aed2-7a60-be40-6e7da6059a01-upstream-step-rotation.md) — `cfe2d62d2`, `ebee130d5`
8. [upstream-expression-presets](issue-019f3d49-aedf-73f0-897f-fee991eb8004-upstream-expression-presets.md) — `b7df5c737`, `8f38c869d`, `e3163c5bd`, `464dd5aa8`, `e9d892c18`, `5e7ffd799`, `9ff91e5bb`, `4508b4681`
9. [upstream-layer-type-icons](issue-019f3d49-aee9-7af0-9f7d-e0a6172151f1-upstream-layer-type-icons.md) — `e1c213522`, `1a4d7d422`, `e8ce39a7c`, `b92fb382b` (touches the one file the fork also modifies — see Conflict risk above; upstream's hunk is purely additive so a clean textual merge is expected)
10. [upstream-file-io-render-widget](issue-019f3d49-aef2-73e0-88d2-58b378ce67b1-upstream-file-io-render-widget.md) — `b8e3e7b86` + 15 related commits
11. [upstream-sdk-ci-updates](issue-019f3d49-aefc-7952-8a9b-a0ff677defeb-upstream-sdk-ci-updates.md) — `06c773c83`, `c9f95d593`, `1d60fbe04`
12. [upstream-askdialog-ui](issue-019f3d49-af05-76d1-be50-b96bc4989a1d-upstream-askdialog-ui.md) — `9623be1ad`
13. [upstream-misc](issue-019f3d49-af0e-7712-9466-95c9fc7e29f7-upstream-misc.md) — `0f10e7f22` + structural merge commits (`9a77c55b8`, `b4dc56d9f`, `91c99afbb`, `353eec530`, `9817ec231`) (depends on #6)

No priority order was otherwise decided — every remaining batch is
independently tracked and testable, so whichever is convenient can be
pulled first. Note the three dependencies discovered while writing the
sub-issues (marked above): #5 must land before #3/#4 (they edit files
#5 creates), and #6 must land before #13 (its GLES check depends on
GLES3 support existing).

## Next steps

- [x] Run `/md-issue-track` to create the 13 sub-issues listed above
- [ ] For each sub-issue, run `/md-issue-fix` to pull/cherry-pick,
      build, test, and PR that batch independently (respecting the 3
      dependencies noted above)
- [ ] Use `git blame` to resolve the `boxsinglewidget.cpp` overlap when
      pulling `upstream-layer-type-icons`
- [ ] Update `CLAUDE.md`'s "last upstream commit incorporated" pointer
      once all 13 batches have landed
- [ ] Update `earlye-fork.md` if any incorporated changes affect
      previously documented fork behavior
- [ ] Close/resolve this meta-issue once all sub-issues are done

## Grill Log

### 2026-07-07

- Q: Pull everything, or filter to fixes-only and defer big features?
  — A: Neither pre-filter; pull each independently and test as we go.
- Q: What's the unit of "independently" — commit-by-commit or grouped
  by topic? — A: Grouped by topic (confirmed the 13-batch breakdown
  above).
- Q: How should the topic batches be organized/tracked? — A: Each
  batch becomes its own `/md-issue-track`ed issue
  (`issue-{uuid7}-upstream-{description}.md`); this issue becomes the
  meta-tracker rather than doing the pull itself.
- Q: Create the 13 sub-issues now, or just record the plan for later?
  — A: Log the plan into this issue now, then proceed straight to
  `/md-issue-fix`, which will write the sub-issue files and open a PR.
