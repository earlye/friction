# Friction silently swallows resolution errors instead of surfacing them

## Context

Surfaced while grilling `issue-019f2f85-d63c-7382-ab59-6d33bbeb7e3f`
(replacing `<desc>` YAML with `inkscape:label` query-string conventions).
That issue's design adopts a hard-error posture for bad references (e.g. an
unresolvable `controller=`, or duplicate `?page=N` values) rather than
silently skipping them — but today's equivalent YAML-parsing code mostly
just swallows errors (`try {} catch (...) {}` around `YAML::Load` in several
places in `src/core/Boxes/svglinkbox.cpp`, with only some paths logging via
`qCWarning`).

The user's framing: they don't want crashes or intrusive popups, but they
do want to *know* when something didn't resolve the way they expected,
instead of just seeing a flipbook/follower silently not work with no
indication why.

## Next steps

- Survey where resolution/parsing failures currently go unreported (start
  with the `catch (...)` blocks in `svglinkbox.cpp` around the `kind:`
  parsers) and decide on a consistent, non-intrusive surfacing mechanism —
  e.g. a dedicated status/warnings panel, a badge on the affected box in the
  tree, or a consolidated log view — rather than ad hoc `qCWarning` calls
  that require enabling a `QT_LOGGING_RULES` category to ever see.
- This issue is a placeholder pending its own grilling session before
  implementation — no design decisions made yet on the mechanism.
