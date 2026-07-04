# Cache SDK download across worktrees

## Context

Every new worktree's first `just build-debug` re-downloads and extracts the
SDK tarball from scratch (`justfile:21-34`, the `sdk` recipe), even though
the same `SDK_VERSION`/`SDK_REV` tarball was already fetched for a sibling
worktree. This makes starting work on a new issue (which creates a new
worktree) slow every time.

Resolved design (see Grill Log): cache only the raw, checksum-verified
tarball at `~/.cache/friction-sdk/`, keyed by a filename that embeds both
the version/rev and the SHA256. The `sdk` recipe extracts directly from
that cached path into the worktree's `./sdk/` — no copy step, since `tar`
doesn't care where its input lives. A download only happens on an actual
cache miss (including a corrupt/partial cached file, detected by checksum
mismatch).

## Relevant files

- `justfile:1-5` — `SDK_VERSION`, `SDK_REV`, `SDK_TAR`, `SDK_URL`, `SDK_SHA256` constants
- `justfile:20-34` — the `sdk` recipe that downloads/verifies/extracts into `./sdk/` (to be split, see Design)
- `justfile:37,45` — `build-mac`/`build-mac-arm` depend on `sdk` (to be repointed at `unpack-sdk`)
- `justfile:149` — `build-debug` depends on `sdk` (to be repointed at `unpack-sdk`)

## Design

- **Recipe split**: the current `sdk` recipe is retired and replaced by two
  recipes:
  - `cache-sdk` — ensures the checksum-verified tarball exists at
    `~/.cache/friction-sdk/...` (downloads on miss/corruption, no-ops on a
    valid hit). No worktree-local side effects.
  - `unpack-sdk: cache-sdk` — extracts the tarball from the cache path into
    the worktree's `./sdk/` (skipping extraction if `./sdk/` already
    exists, same as today's early-out).
  - `build-debug`, `build-mac`, and `build-mac-arm` all depend on
    `unpack-sdk` instead of the old `sdk` recipe, so release builds get the
    shared cache too and there's only one code path.
- **Cache contents**: raw tarball only, not the extracted tree. Extraction
  stays a per-worktree step (it's cheap local CPU/IO, no network).
- **Cache path/key**: `~/.cache/friction-sdk/friction-sdk-{SDK_VERSION}{SDK_REV}-macOS.{SDK_SHA256}.tar.xz`
  — combines the human-readable version/rev with the SHA256, so a version
  bump gets a new filename automatically, and even a same-named tarball
  whose contents changed without a version bump still gets a new cache
  entry.
- **Cache hit**: verify the cached file's checksum against `SDK_SHA256`
  before trusting it. If verification fails (e.g. truncated from a
  previously-killed download), treat it as a miss: delete the bad file and
  re-download. Presence of the file is never assumed to imply validity.
- **Cache miss / download**: `curl` into a temp file in the same cache
  directory (e.g. `.tmp-$$-<name>`), verify its checksum, then atomically
  `mv` it into the final cache path. This avoids any lockfile: a second
  concurrent worktree either doesn't see the final file yet (and downloads
  its own temp copy — wasted bandwidth but never a correctness problem) or
  sees the fully-formed final file.
- **Extraction**: `tar xf` reads directly from the cache path into `./sdk/`
  in the worktree. No intermediate copy of the tarball into the worktree.
- **Old versions in the cache**: never auto-delete. Multiple worktrees may
  be building different SDK versions concurrently, so pruning could yank a
  file out from under an in-flight build in another worktree. Instead, if
  the cache directory contains tarballs other than the one currently
  needed, print a warning listing them to stdout — phrased so that an agent
  running the build on a user's behalf will likely surface it and ask
  whether it's a good time to clean up, rather than deleting anything
  itself.

## Verification plan

The `cache-sdk`/`unpack-sdk` split makes the cache behavior directly
testable via `just build-debug`:

1. Ensure `~/.cache/friction-sdk/` is empty (or the target tarball isn't
   there). Run `just build-debug` — this is a **global cache miss**: it
   should download, verify, and populate the cache. Confirm the tarball
   now exists under `~/.cache/friction-sdk/`.
2. Delete the worktree's `./sdk/` and run `just build-debug` again — this
   is a **local cache miss, global cache hit**: no download should happen
   (`unpack-sdk` extracts straight from the already-cached tarball). This
   run should be measurably faster than step 1.
3. Run `just build-debug` a third time without deleting anything — this is
   a **full hit** (`./sdk/` already present): should be at least as fast as
   step 2, and confirms the existing "skip if `./sdk/` exists" early-out
   still works.

## Next steps

- Implement `cache-sdk` and `unpack-sdk` in the justfile per the Design
  section above, retire the old `sdk` recipe, and repoint `build-debug`/
  `build-mac`/`build-mac-arm` at `unpack-sdk`.
- Run the Verification plan above to confirm both cache tiers actually
  save time.

## Grill Log

### 2026-07-04

- Q: Cache the raw tarball or the extracted SDK tree? — A: Raw tarball only; extraction stays per-worktree.
- Q: How should the cache filename encode the version/rev so a bump doesn't reuse a stale entry? — A: Combine version+rev with SHA256 in the filename (`friction-sdk-{version}{rev}-macOS.{sha256}.tar.xz`) — covers version bumps and a same-named tarball changing contents.
- Q: What happens if a cached file with the expected name fails checksum verification? — A: Treat as a cache miss, delete it, re-download — cache must self-heal, never silently trust presence-by-name.
- Q: How does concurrent download-into-cache avoid a race between two worktrees building at once? — A: Download to a temp file, verify, then atomic `mv` into the final cache path; no lockfile needed.
- Q: Given only the tarball is cached, should the worktree copy it locally before extracting, or extract straight from the cache path? — A: Extract straight from the cache path — no copy step needed at all.
- Q: Should old SDK versions be pruned from the cache automatically? — A: No auto-delete (concurrent worktrees may be building different versions); instead print a warning listing other cached tarballs so an agent/human can decide when to clean up.
- Q: Should the `sdk` recipe be split so the caching behavior is independently testable? — A: Yes — split into `cache-sdk` (populate/verify the shared cache) and `unpack-sdk: cache-sdk` (extract into the worktree), giving a 3-run verification plan (global miss → local miss/global hit → full hit) that directly measures the speedup.
- Q: Should `build-mac`/`build-mac-arm` also move off the old `sdk` recipe? — A: Yes — retire `sdk` entirely, repoint all three build recipes at `unpack-sdk` so release builds share the cache too and there's a single code path.
