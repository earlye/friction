# Upstream PR: audio waveform visualization in animation timeline

## Context

`earlye-fork.md`'s "Audio Waveform in Animation Timeline" feature
(fork PR [#56](https://github.com/earlye/friction/pull/56)) is tagged
`needs PR` in the Upstream tracking column — this is the one entry
where the user disagreed with the default `won't PR` assignment for
New Features. Rationale (from the doc): it adds obvious value and
should probably be extended to cover waveforms for embedded videos or
MIDI (if supported) too, before submitting upstream.

The timeline renders the decoded audio waveform behind clip regions,
giving visual tempo cues for keyframe placement.

## Relevant files

Fork PR #56 touched:

- `src/core/CacheHandlers/soundcachehandler.cpp` / `.h`
- `src/core/Sound/esound.cpp` / `.h`
- `src/core/Sound/esoundobjectbase.cpp` / `.h`

## Next steps

- Extend waveform rendering to cover embedded videos (and MIDI, if
  friction supports it) — not just standalone audio clips — per the
  user's stated reason for wanting this upstreamed.
- Check out latest `upstream/main` (`friction2d/friction`), then
  cherry-pick fork PR #56's commit(s) or rebuild the fix directly
  against upstream's current code — whichever applies more cleanly,
  since upstream has diverged since #56 landed in the fork.
- Open a PR against `friction2d/friction`.
- Upstream explicitly does not want `Co-Authored-By` trailers in git
  commits — omit them from any commits prepared for this PR.
