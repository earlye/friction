# Only run CI builds/releases on code changes

## Context

CI currently builds (and, on `main`, releases) on every push/PR regardless
of what files changed. Doc-only or issue-tracking-only changes (e.g. editing
`issues/*.md`, `earlye-fork.md`, `README.md`, `docs/**`) still trigger full
Linux/macOS/Windows builds and, when merged to `main`, a full release.

Current trigger setup:

- `.github/workflows/linux.yml`, `macos.yml`, `windows.yml`: `on: pull_request`
  (no path filters) + `on: workflow_call` (invoked by `release.yml`).
- `.github/workflows/release.yml`: `on: push: branches: [main]`, calls the
  three build workflows via `workflow_call`, then creates a GitHub Release.
- `.github/workflows/ubuntu.yml`: `on: workflow_dispatch` only — manual,
  unrelated to automatic CI, **out of scope** for this issue.
- `main` has no branch protection / required status checks (confirmed via
  `gh api repos/earlye/friction/branches/main/protection` → 404 "Branch not
  protected"), so skipping checks for docs-only PRs won't strand anything.
- `README.md` and `LICENSE.md` are **not** pure docs from a build standpoint:
  `src/app/CMakeLists.txt` installs both into the packaged app (under
  `INSTALL_DOCS`), and `src/scripts/build.bat` copies `LICENSE.md` into the
  Windows output dir. Changes to these two files must still trigger builds.
- Everything else text-only is safe to ignore: `CLAUDE.md`, `earlye-fork.md`,
  `earlye-fork/**` (images used only by earlye-fork.md), `docs/**` (currently
  empty), `issues/**`, and the scattered `src/**/README.md` files (icon/script
  folder docs, not referenced by any install/build rule).

## Goal

PRs or `main`-branch pushes that only touch docs or `issues/*` should not
trigger builds or releases — except changes to root `README.md`/`LICENSE.md`,
which do affect packaged release artifacts and must still build.

## Root cause

No path filtering exists on any of the build/release workflow triggers.

## Decided approach

- **Path filter list** (applied identically wherever gating is added):
  ```yaml
  paths-ignore:
    - '**/*.md'
    - '!README.md'
    - '!LICENSE.md'
    - 'earlye-fork/**'
    - 'docs/**'
    - 'issues/**'
  ```
  GitHub Actions path patterns without a leading `**/` only match the
  repo-root file, so `!README.md` / `!LICENSE.md` re-include only the
  root-level files (nested `src/**/README.md` stay ignored via `**/*.md`).
  `paths` and `paths-ignore` can't both be set on the same trigger, so the
  re-inclusion is expressed as negated entries inside `paths-ignore` instead
  of a separate `paths:` key.
- **PR builds**: add the list above directly under each of `linux.yml`,
  `macos.yml`, `windows.yml`'s own `pull_request:` key. Their `workflow_call:`
  trigger (used only by `release.yml`) stays unfiltered — `workflow_call`
  doesn't support path filters, and `release.yml` is gated separately (below),
  so it never invokes them for a docs-only merge anyway.
- **Push-to-main / release**: add the same list to `release.yml`'s own
  `on: push:` trigger. If a merge to `main` only touches docs/issues,
  `release.yml` never runs at all — no build calls, no GitHub Release created.
  (Rejected alternative: keep `release.yml` always running and gate its
  internal steps with a `paths-filter` action — unnecessary complexity since
  nothing depends on the workflow run existing for docs-only pushes.)
- `ubuntu.yml` is untouched — manual dispatch only, not part of this issue.
- **Upstream tracking**: this will be logged in `earlye-fork.md`'s
  "CI / Build" section as `won't PR`, consistent with every other fix to this
  fork-introduced CI/release infrastructure (upstream friction2d/friction has
  no equivalent workflow to patch).

## Next steps

- Implement the `paths-ignore` blocks in `linux.yml`, `macos.yml`,
  `windows.yml` (`pull_request:` only) and `release.yml` (`push:`).
- Add an `earlye-fork.md` "CI / Build" table row for this fix, marked
  `won't PR`.
- Manually verify: open a PR touching only `issues/*.md` and confirm no
  Linux/macOS/Windows build runs; merge a docs-only change to `main` and
  confirm `release.yml` doesn't run; confirm a PR touching `README.md` or
  `LICENSE.md` still builds.

## Grill Log

### 2026-07-07

- Q: Should the doc-path ignore list be broad (`**/*.md` everywhere) or
  hand-listed to just top-level docs? — A: Broad, contingent on confirming no
  `.md` files are embedded in the installer/release artifacts.
- Q: Are any `.md` files embedded in build output? — A: Yes — confirmed via
  `grep` that `src/app/CMakeLists.txt` installs root `README.md`/`LICENSE.md`
  under `INSTALL_DOCS`, and `build.bat` copies `LICENSE.md` on Windows. Both
  must keep triggering builds; everything else `.md` is safe to ignore.
- Q: How should the push-to-main release path be gated, given
  `workflow_call` can't be path-filtered? — A: Put `paths-ignore` directly on
  `release.yml`'s own `push:` trigger so the whole workflow (including its
  build calls) is skipped for docs-only merges, rather than letting it run
  and gating internal steps.
- Q: Should PR builds (`linux.yml`/`macos.yml`/`windows.yml`) get the same
  filter on their `pull_request:` trigger, and is `ubuntu.yml` in scope? — A:
  Yes, filter `pull_request:` directly on each; `ubuntu.yml` is
  `workflow_dispatch`-only and out of scope.
- Q: Should this be tracked `won't PR` or `need PR` in `earlye-fork.md`? — A:
  `won't PR`, matching the precedent set by every other fix to this
  fork-introduced CI/release infrastructure.
