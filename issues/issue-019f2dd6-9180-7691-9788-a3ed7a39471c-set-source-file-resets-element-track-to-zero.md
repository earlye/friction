# "Set Source File" reload resets an element track's transform to all zeros

## Context

Surfaced while investigating issue `1148748f4eb9` (ruby's arms not
flipbooking/following properly). During that investigation, a separate,
confirmed bug was found and fixed this session: `SvgElementTrack::resolveTarget`
(via `findDescendantByName` in `svgelementtrack.cpp`) never checked whether the
search root (`svgRoot`) itself was the box being searched for by name — only its
children. This matters because `ImportSVG::loadSVGFile`
(`svgimporter.cpp:1220-1222`) silently unwraps a single-top-level-child SVG,
returning that child directly as the import result. When the SVG's root
`<svg>` has exactly one top-level group (e.g. a minimal repro SVG with a single
`<g id="parent">`), that group becomes `svgRoot` itself, and any
`SvgElementTrack` named after it (from a `kind: animation-node` desc) could
never resolve its own target — `collectPivotDescs` would report
`track "parent" has no resolved target"` even though the box existed.

Both `findDescendantByName` (`svgelementtrack.cpp`) and the analogous
`collectPivotDescs`/`collectFlipbookDescs` walk (`svglinkbox.cpp`) were fixed
this session to also check the container/root argument itself, not just its
children. See commits/diffs in this session's branch (`svglinkbox.h/.cpp`:
extracted `applyPivotDescIfPresent`; `svgelementtrack.cpp`: `findDescendantByName`
now checks `container` itself first).

After that fix, resolution for a self-referential root track ("parent")
started succeeding — but doing so exposed this new issue: the track's own
stored transform properties (`scale.x`, `scale.y`, `opacity`, etc.) were
already at framework-default zero, because the track was originally created
*before* the resolve fix, when `initFromTarget` never got a chance to seed it
with the target's real values (resolution failed, so seeding never happened).
That zero state got saved into the `.friction` project file. Once resolution
started succeeding, `SvgLinkBox::resolveElementTracks()`'s first loop
(`svglinkbox.cpp:127-136`) called `track->syncToTarget(targetBox)`
unconditionally on successful resolve — pushing the track's stale zeroed
values onto the target, collapsing its scale to 0 ("everything looks terrible,
have to zoom to a ridiculous scale, and it's garbled").

The user then reproduced the same zeroing symptom through a different,
more concerning path: reloading the *same, unchanged* source file via the
"Set Source File" action on an element already in a working, non-corrupted
track.

## Reproduction steps

1. Right-click an element in the animation track.
2. Choose **Actions → Set Source File**.
3. Choose the *same* file that's already loaded (no actual content change).

**Expected outcome:** reload leaves the track unchanged (aside from
orphaning tracks whose target disappeared, or discovering genuinely new
tracks).

**Actual outcome:** the `"parent"` track gets reset to all zeros
(scale/opacity/etc.), even though nothing about the source file changed.

## Root cause hypothesis

`SvgLinkBox::resolveElementTracks()` (`svglinkbox.cpp:121-176`) reruns on
every source-file (re)load. Its first loop over `mElementTracks`
(`svglinkbox.cpp:127-136`) does:

```cpp
BoundingBox* targetBox = track->resolveTarget(svgRoot);
if (targetBox) {
    track->reconcileWithTarget(targetBox);
    track->syncToTarget(targetBox);   // <-- unconditional push, track -> target
}
```

`syncToTarget` always pushes the *track's own* stored values onto the
target, regardless of whether the track's values are meaningfully
up to date. If a track's stored values are stale or were never properly
seeded (as happens for any track created while its target failed to
resolve, per the bug above — or potentially for other reasons not yet
identified, since this repro path doesn't obviously involve a
never-resolved track), reloading will stomp the target with those stale
values instead of leaving an already-correct target alone.

Not yet confirmed: why the *same-file* "Set Source File" repro path hits
this — whether it's the exact same "track's stored values are stale
zeros from before the resolve-fix" mechanism, or a distinct issue where
`syncToTarget` fires even when nothing meaningful changed and should be a
no-op relative to an already-correct target.

## Relevant files

- `src/core/Boxes/svglinkbox.cpp:121-176` — `resolveElementTracks`, first
  loop that calls `syncToTarget` unconditionally whenever a track resolves
- `src/core/Boxes/svgelementtrack.cpp:195-232` — `syncToTarget`
  (push track → target)
- `src/core/Boxes/svgelementtrack.cpp:234-259` — `initFromTarget`
  (pull target → track; only ever called once, at track creation, in
  `SvgLinkBox::addElementTrack`)
- `src/core/Boxes/svgelementtrack.cpp:68-80` — `findDescendantByName`
  (fixed this session to check `container` itself, not just children)
- `src/core/Boxes/svglinkbox.cpp` — `applyPivotDescIfPresent` (extracted
  this session so `collectPivotDescs` also checks its own root argument)
- `src/core/svgimporter.cpp:1220-1222` — single-top-level-child unwrap in
  `ImportSVG::loadSVGFile`, which is what makes `svgRoot` self-referential
  in the first place
- `tmp/pivot-repro.svg` — minimal repro SVG used this session (single
  `<g id="parent">` root, with a `kind: pivot` circle and a `kind:
  animation-node` desc directly on `parent`)
