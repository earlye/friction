# Upstream PR: Windows preview playback frame-cache eviction bug

## Context

`earlye-fork.md`'s Known Issues entry — playback on Windows appears to
drop the frame cache, resulting in audio with a black display — is
tagged `need PR`. It's currently worked around in-fork via
[#52](https://github.com/earlye/friction/pull/52), but the root cause
is in core `Canvas`/`HddCachableCacheHandler`/`VideoEncoder`
cache-eviction handling, not fork-added code, so it's a genuine
upstream bug rather than a fork regression.

Root cause (from fork PR #52): on Windows, rendered preview frames can
be evicted from memory to HDD between the rendering phase and playback
start. `Canvas::setSceneFrame(int)` then receives an HDD-backed
container and passes it blindly to `setSceneFrame(sptr)`, which draws
nothing because the image isn't in memory.

## Relevant files

Fork PR #52 touches a mix of core and fork-only files. Only the core
subset is upstream-relevant:

- `src/core/canvas.cpp` — locks the scene frame cache during preview
  rendering (`setNoClear(true/false)` in `setRenderingPreview` /
  `setPreviewing`)
- `src/core/CacheHandlers/hddcachablecachehandler.cpp` / `.h` —
  `setSceneFrame(int)` checks `storesDataInMemory()` and calls
  `setLoadingSceneFrame` for HDD-evicted frames
- `src/core/videoencoder.cpp` / `.h` — `renderSk` guards against
  drawing frames with no image (`hasImage()` check)

Fork PR #52 also touches `svgelementtrack.cpp`, `svgflipbooktrack.cpp`,
`eboxorsound.cpp`/`.h`, `svgimporter.cpp`/`.h`, and Windows debug
tooling (`DEBUGGING-WINDOWS-PREVIEW.md`,
`src/scripts/run_debug_preview.bat`, `.github/workflows/windows.yml`)
— those are fork-specific or debugging aids and are not upstream
candidates.

## Next steps

- Check out latest `upstream/main` (`friction2d/friction`).
- Rebuild just the core-file fix (`canvas.cpp`,
  `hddcachablecachehandler.cpp`/`.h`, `videoencoder.cpp`/`.h`) against
  upstream's current code rather than cherry-picking #52 wholesale —
  the fork-only files in that PR don't exist upstream and the fix
  needs isolating from them.
- Open a PR against `friction2d/friction`.
- Upstream explicitly does not want `Co-Authored-By` trailers in git
  commits — omit them from any commits prepared for this PR.
