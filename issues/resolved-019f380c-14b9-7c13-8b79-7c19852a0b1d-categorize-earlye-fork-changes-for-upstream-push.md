# Track upstream-PR status for each earlye-fork.md change

## Context

The fork (`earlye/friction`) has accumulated changes — some fork-specific
features, but also bug fixes that are really fixes to pre-existing upstream
bugs (already tagged `[pre-existing]` in `earlye-fork.md`'s Bug Fixes
tables). Being a good citizen of the upstream `friction2d/friction`
community means those incidental fixes should become independent PRs
against latest upstream, rather than staying fork-only.

To track that effort, `earlye-fork.md` needs a per-change field recording
upstream PR status, with values like:

- `won't PR` — fork-specific, not applicable upstream
- `need PR` — should be PR'd upstream, not yet done
- `PR #n` — upstream PR opened, pending review
- `PR #n (merged)` — accepted upstream
- `PR #n (rejected)` — upstream declined it

Note: the `#n` here refers to an upstream `friction2d/friction` PR number,
which is a different numbering space than the existing `#n` links in
`earlye-fork.md` (those are all `earlye/friction` fork PR numbers).

## Design

- **Bug Fixes tables**: add an `Upstream` column alongside the existing
  `#` / `Fix` / `Origin` columns, in every category table (CI/Build,
  Rendering/Playback, Camera/Viewport, SvgElementTrack/Flipbook, Lock
  System, SVG Import, Compiler Warnings, Developer Tooling).
- **New Features section**: since these aren't tables, add one summary
  line per feature (not per History bullet), e.g. under the feature's
  heading/description or right above its `#### History` subsection:
  `**Upstream:** won't PR — fork-specific feature`.
- **Default values** (my proposal, to review/correct entry-by-entry when
  applied):
  - Bug fixes tagged `pre-existing` → `need PR` (the bug exists in
    upstream code, so it's a real candidate).
  - Bug fixes tagged `fork-introduced` or `likely fork-introduced` →
    `won't PR` (the affected code path doesn't exist upstream — it was
    introduced by this fork).
  - All entries under New Features → `won't PR` (fork-specific by
    definition; nothing here has an upstream equivalent to submit a fix
    or feature against).
  - Known Issues section (currently just the Windows frame-cache
    playback bug tracked as fork PR #52): treat like a bug fix entry —
    needs its own `need PR`/`won't PR` call once triaged.
- **PR numbering namespace collision**: `earlye-fork.md` already uses
  bare `#n` links for `earlye/friction` fork PRs. To avoid confusing a
  fork PR number with an upstream `friction2d/friction` PR number in the
  same row, upstream-state values use an `up#n` prefix instead of a bare
  `#n`, e.g. `PR up#123`, `PR up#123 (merged)`, `PR up#123 (rejected)`.
  Full value vocabulary: `won't PR`, `need PR`, `PR up#n`,
  `PR up#n (merged)`, `PR up#n (rejected)`.

## Next steps

- This issue's design is settled (see Grill Log below); the actual edit
  to `earlye-fork.md` — adding the `Upstream` column/tags and assigning
  a value to every existing entry — is a separate implementation step
  (e.g. via `/md-issue-fix` against this issue), not done as part of
  this grilling session.
- That same implementation step must also update `CLAUDE.md` (the
  "Starting Work on an Issue" / earlye-fork.md maintenance guidance) to
  say that any *new* entry added to `earlye-fork.md` going forward must
  include upstream-state tracking (per the vocabulary above) at the time
  it's added — not just backfilled once for existing entries. Otherwise
  this tracking rots the same way the untracked history did.

## Grill Log

### 2026-07-06

- Q: How should upstream-state be represented in earlye-fork.md? — A: New
  `Upstream` column in the existing Bug Fixes tables.
- Q: Which entries need an upstream-state value — only pre-existing bug
  fixes, or everything? — A: Everything, including New Features and
  fork-introduced fixes (so nothing is silently omitted).
- Q: The New Features section isn't a table — how do those get tagged? —
  A: One summary line per feature (not per History bullet), e.g.
  `**Upstream:** won't PR — fork-specific feature`.
- Q: Who decides won't-PR vs. need-PR per entry? — A: Generally,
  fork-specific features aren't ready for upstream (`won't PR`); bug
  fixes are (`need PR`) — this becomes the default-assignment rule,
  applied per-Origin-tag when the column is actually filled in.
- Q: How to avoid confusing fork PR numbers with upstream PR numbers in
  the same row? — A: Prefix upstream PR references with `up#` (e.g.
  `PR up#123`) instead of a bare `#n`.
- Q: Should this session also apply the change to earlye-fork.md? — A:
  No — stop after finalizing the issue's design; implementation is a
  separate step.
- Q: How do we keep this from rotting once new entries get added? — A:
  Implementation must also update CLAUDE.md so new earlye-fork.md
  entries are required to carry upstream-state tracking from the start.
