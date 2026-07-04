# `kind: flipbook-follower` is not implemented

## Context

`ruby.svg` (`~/Documents/github.com/ehlingers/slime-at-school/characters/ruby.svg`)
contains `desc` YAML blocks declaring `kind: flipbook-follower`, e.g. under
`right-arm-overlap-flipbook`:

```yaml
kind: flipbook-follower
controller: right-arm-flipbook
map:
  0: null
  1: "right-arm-overlap-holla"
  2: null
```

The expected behavior (per the user): this should create a hidden
flipbook that subscribes to page-turn events on the named `controller`
flipbook (`right-arm-flipbook` here), and show/hide its own mapped child
according to the controller's current page index — analogous to how
`kind: animation-follower` mirrors a controller box's transform onto a
follower box.

Confirmed via code search: **no code anywhere in `src/` recognizes the
string `"flipbook-follower"`.** `SvgLinkBox::collectFlipbookDescs`
(`svglinkbox.cpp:265-318`) only matches `kind: flipbook` exactly
(`svglinkbox.cpp:283`: `if (!node["kind"] || node["kind"].as<std::string>() != "flipbook") continue;`) —
a `flipbook-follower` desc simply falls through this check and is
silently ignored. This is either an unimplemented feature or a dead/
unfinished code path; either way, nothing currently makes use of it.

## Expected behavior (per user, analogous to `animation-follower`)

`kind: animation-follower` is implemented via `SvgLinkBox::
collectFollowerDescs` (`svglinkbox.cpp:238-264`): finds boxes declaring
`kind: animation-follower` + `controller: <name>`, resolves the controller
box by name, and records a `FollowerBinding{follower, controller}` in
`mFollowers`. `SvgLinkBox::applyFollowerTransform` (called per-frame,
e.g. from `anim_setAbsFrame`) then syncs the controller's `transform`
ComplexAnimator onto the follower's.

`kind: flipbook-follower` presumably wants the same shape, but for page
index rather than transform: find boxes declaring `kind: flipbook-
follower` + `controller: <flipbook-name>` + a `map` (page-index →
mapped child name, or `null`), resolve the controller `SvgFlipbookTrack`/
flipbook box, and whenever the controller's active page index changes,
show the mapped child (if any) in the follower and hide the rest —
mirroring what `SvgFlipbookTrack::syncToTargets` already does for a
flipbook's *own* pages, but driven by another flipbook's index instead of
its own.

## Relevant files

- `src/core/Boxes/svglinkbox.cpp:238-264` — `collectFollowerDescs`/
  `applyFollowerTransform`, the `animation-follower` implementation to
  mirror
- `src/core/Boxes/svglinkbox.cpp:265-318` — `collectFlipbookDescs`, where
  `kind: flipbook-follower` currently falls through unhandled
  (`svglinkbox.cpp:283`)
- `src/core/Boxes/svgflipbooktrack.h`/`.cpp` — `SvgFlipbookTrack`,
  particularly `syncToTargets`, the page-index-driven show/hide logic to
  mirror for a follower
- `~/Documents/github.com/ehlingers/slime-at-school/characters/ruby.svg` —
  contains live `kind: flipbook-follower` desc blocks (e.g. under
  `right-arm-overlap-flipbook`, `left-arm-overlap-flipbook`) that
  currently have no effect
