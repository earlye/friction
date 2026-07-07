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

**Correction (2026-07-07 self-review)**: an earlier version of this
section claimed only one file overlapped. That was wrong — it compared
against a truncated (`--stat` head-limited) file list. A full
`comm -12` between every `.cpp`/`.h` file the fork has touched since
`3701b0559990eae4fa315b7215a11d46122cb97b` and every file these 60
upstream commits touch found **18 overlapping files**:

- `src/app/GUI/BoxesList/boxsinglewidget.cpp` / `.h` —
  `upstream-layer-type-icons`. Additive upstream hunk, low risk (see
  that sub-issue).
- `src/app/GUI/canvaswindow.cpp`, `src/core/Boxes/svglinkbox.cpp`,
  `videobox.cpp`, `src/core/CacheHandlers/soundcachehandler.cpp`,
  `src/core/GUI/edialogs.cpp`/`.h`, `src/ui/dialogs/exportsvgdialog.cpp`
  — all via `b8e3e7b86` in `upstream-file-io-render-widget`, the
  **highest-risk batch**: it touches the fork's SVG-link source-file
  handling and its audio-waveform feature (PRs #56/#60). See that
  sub-issue's own Conflict risk section.
- `src/core/appsupport.cpp` and `src/app/GUI/mainwindow.cpp` are each
  touched by upstream commits spread across **four different
  batches** (`appsupport.cpp`: `upstream-sdk-ci-updates`,
  `upstream-misc`, `upstream-expression-presets`,
  `upstream-flatpak-earlysettings-fixes`,
  `upstream-file-io-render-widget`; `mainwindow.cpp`:
  `upstream-quicksetup-wizard`, `upstream-shutdown-deadlock`,
  `upstream-expression-presets`, `upstream-file-io-render-widget`) —
  no single sub-issue owns the fork-conflict risk here, and pulling
  these batches in different orders/PRs raises the chance of one
  batch's PR failing to apply cleanly after another already landed.
  Not blocking, but worth resolving each batch's conflicts (if any)
  against whatever's already on `main` at pull time, not against this
  plan's assumptions.
- `src/app/GUI/menu.cpp` — `upstream-quicksetup-wizard` and
  `upstream-misc` (both touch it; land wizard first).
- `src/core/Animators/SmartPath/nodelist.cpp` —
  `upstream-smartpath-hardening`. Fork's only touch is a trivial
  unused-variable-warning fix; low risk.
- `src/core/canvasmouseinteractions.cpp`, `src/core/grid.cpp` —
  `upstream-step-rotation`. Fork's lock-icon-flash and camera-matrix
  fixes live in `canvasmouseinteractions.cpp`; moderate risk, review
  the merge.
- `src/core/Expressions/expressionpresets.cpp` —
  `upstream-expression-presets`. Fork's only touch silences its
  logging; low risk.
- `src/ui/widgets/uilayout.cpp` — `upstream-shutdown-deadlock`. Fork's
  touch is a mechanical logging-category conversion; low risk.

Each affected sub-issue above now carries its own conflict-risk note
with these specifics. `upstream-askdialog-ui`, `upstream-gles3-support`,
and `upstream-sdk-ci-updates` have no fork overlap.

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
9. [upstream-layer-type-icons](issue-019f3d49-aee9-7af0-9f7d-e0a6172151f1-upstream-layer-type-icons.md) — `e1c213522`, `1a4d7d422`, `e8ce39a7c`, `b92fb382b` (touches `boxsinglewidget.cpp`, which the fork also modifies — see Conflict risk above; upstream's hunk is purely additive so a clean textual merge is expected)
10. [upstream-file-io-render-widget](issue-019f3d49-aef2-73e0-88d2-58b378ce67b1-upstream-file-io-render-widget.md) — `b8e3e7b86` + 15 related commits (highest conflict risk — see above)
11. [upstream-sdk-ci-updates](issue-019f3d49-aefc-7952-8a9b-a0ff677defeb-upstream-sdk-ci-updates.md) — `06c773c83`, `c9f95d593`, `1d60fbe04`
12. [upstream-askdialog-ui](issue-019f3d49-af05-76d1-be50-b96bc4989a1d-upstream-askdialog-ui.md) — `9623be1ad`
13. [upstream-misc](issue-019f3d49-af0e-7712-9466-95c9fc7e29f7-upstream-misc.md) — `0f10e7f22`, `e6d16e4e9` + structural merge commits (`9a77c55b8`, `b4dc56d9f`, `91c99afbb`, `353eec530`, `9817ec231`) (depends on #5 and #6)

No priority order was otherwise decided beyond the four known
dependencies (marked above): #5 must land before #3/#4 (they edit
files #5 creates) and before #13 (shared `menu.cpp` touch), and #6
must land before #13 (its GLES check depends on GLES3 support
existing). Every other batch is independently tracked and testable, so
whichever is convenient can be pulled first — but see the Conflict
risk section above for the `appsupport.cpp`/`mainwindow.cpp`
cross-batch caution.

## Next steps

- [x] Run `/md-issue-track` to create the 13 sub-issues listed above
- [ ] For each sub-issue, run `/md-issue-fix` to pull/cherry-pick,
      build, test, and PR that batch independently (respecting the
      dependencies noted above)
- [ ] Use `git blame` to resolve each sub-issue's noted file overlaps
      after pulling it (see Conflict risk above; `boxsinglewidget.cpp`
      and `b8e3e7b86`'s file set are the highest-priority checks)
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

### 2026-07-07 (self-review correction, after PR #90 was opened)

A second-pass background review (agent-to-agent, independently
verified before acting) found the initial breakdown had real
inaccuracies, since fixed:

- The preset-file path in `upstream-file-io-render-widget` was wrong
  (`resources/presets/...` → `src/app/presets/render/...`).
- `e6d16e4e9` ("Update menu.cpp") was in the 60-commit range but wasn't
  assigned to any sub-issue — added to `upstream-misc`. A full
  reconciliation confirmed it was the only genuinely missing commit.
- The Conflict risk section's "only one file overlaps" claim was
  false — it was based on a `--stat`-truncated file list. Redone with
  a full `comm -12` diff: 18 files actually overlap, spread across 8
  of the 13 batches. `upstream-file-io-render-widget` is now flagged
  as the highest-risk batch (touches `svglinkbox.cpp`, `videobox.cpp`,
  `soundcachehandler.cpp` — the fork's SVG-link and audio-waveform
  code). Each affected sub-issue got its own risk note.
- `upstream-layer-type-icons`: added `coloranimatorbutton.cpp` to
  Relevant files, and corrected `b92fb382b`'s description — it's an
  unrelated HiDPI warning-text change, not layer-icon work.
