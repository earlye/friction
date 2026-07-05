# Flipbook track editor should select pages by label, not raw number

## Context

Surfaced while grilling `issue-019f2f85-d63c-7382-ab59-6d33bbeb7e3f`
(replacing `<desc>` YAML with `inkscape:label` query-string conventions).
Under the new convention, flipbook pages are direct children labeled
`{name}?page=N` — the artist already gives each page a meaningful display
name. The flipbook track editor UI should let the artist pick a page by
that name instead of by its raw numeric index.

## Next steps

- Add a combo box (or similar picker) to the flipbook track editor that
  lists pages by their `inkscape:label` display name (the part before `?`),
  ordered by `page=` index, instead of requiring the artist to know/type
  the numeric page index directly.
- This issue is a placeholder pending its own grilling session before
  implementation — no design decisions made yet on the exact UI/location.
