# SVG import/link silently ignores `display:none` and `visibility:hidden`

## Context

`~/Documents/github.com/ehlingers/slime-at-school/characters/ruby.svg` has a nested Inkscape layer at `svg1 > layer1 > g26 > layer16`:

- `g26` (`inkscape:label="right-leg"`) has `style="display:inline"`
- `layer16` (`inkscape:groupmode="layer"`, `inkscape:label="Snapping Point 1"`) has `style="display:none"`
- `layer16`'s own children have `style="display:inline"` set individually

Per SVG/CSS semantics, `display:none` on `layer16` should hide the entire subtree regardless of the children's own `display` value. When this SVG is imported *or* linked into a `.friction` file, `layer16`'s children render visible anyway.

The file has 20 elements total with `style="display:none"` (not just Inkscape layers), so this is not an edge case specific to this one file.

## Reproduction

1. Open/link `ruby.svg` (or import it) into Friction.
2. Observe that the contents of `layer16` (and any other `display:none` group) render visible instead of hidden.

## Root cause

In `src/core/svgimporter.cpp`, `BoxSvgAttributes::loadBoundingBoxAttributes(const QDomElement &element)` (~lines 1331–1494) parses the `style` attribute's CSS properties by name, and both `display` and `visibility` are matched but discarded as no-ops:

```cpp
case 'd':
    if(name == "display") {
        //display = value;
    }
    break;
...
case 'v':
    if(name == "vector-effect") {
        //vectorEffect = value;
    } else if(name == "visibility") {
        //visibility = value;
    }
    break;
```

`BoxSvgAttributes` (class defined ~line 167) has no field to store either value. `BoxSvgAttributes::apply(BoundingBox *box)` (~lines 1636–1676) never calls anything like `box->setVisible(false)` / `setVisibleForScene(false)` (the hook exists at `src/core/Boxes/boundingbox.h:427`). There is also no ancestor-to-descendant propagation of hidden state — `setParent()` (~line 169) doesn't carry visibility context, only fill/stroke/decomposed-transform.

`inkscape:groupmode="layer"` gets no special handling anywhere in `src/core/` (only `inkscape:label` is read, ~line 1496). A repo-wide grep for `groupmode` only turns up matches inside the app's own bundled icon SVGs, not in any parsing code. So this is a generic "`display`/`visibility` are parsed and thrown away for every element, everywhere" bug — not specific to Inkscape layers, and not an inheritance bug per se, since the value is never captured even for the element itself.

**Import vs. link:** confirmed no divergence. `src/core/Boxes/svglinkbox.cpp:83` (link) and `src/app/eimporters.cpp:49` (import) both call `ImportSVG::loadSVGFile(...)`, which drives the same shared `BoxSvgAttributes` / `loadBoundingBoxAttributes` / `apply` pipeline in `svgimporter.cpp`.

## Next steps

Fix scope: handle both `display:none` and `visibility:hidden` (both are currently identical no-ops in the same function — fixing one but not the other would be inconsistent).

1. Add fields to `BoxSvgAttributes` to capture parsed `display`/`visibility` values per element.
2. Propagate cumulative "effectively hidden" state through `setParent()` so a child's own `display:inline` doesn't override an ancestor's `display:none`.
3. In `apply()`, call the appropriate visibility hook (e.g. `setVisibleForScene(false)`, `src/core/Boxes/boundingbox.h:427`) when the resolved state is hidden.
4. Design decision needed: import hidden groups as Friction layers/groups with visibility toggled off (so the user can turn them back on), rather than skipping their contents entirely. (Confirmed by user — see Grill Log.)
5. Since the fix lives in the shared `svgimporter.cpp` pipeline, it applies uniformly to both the import path (`eimporters.cpp`) and the link path (`svglinkbox.cpp`) — no separate fix needed per call site.

## Grill Log

### 2026-07-04

- Q: Do both import and link exhibit the identical symptom, or does one differ? — A: User believes import/link path doesn't matter (planned to verify with `just run-debug-{this-issue-shortid}`); confirmed by code research: both call `ImportSVG::loadSVGFile(...)` through the same shared parsing pipeline, so there is no divergence.
- Q: What should the correct behavior be — hidden layer/group in Friction, or skip content entirely? — A: Import as a hidden layer/group in Friction (visibility toggled off), matching Inkscape's own display:none semantics so the user can toggle it back on.
- Q: Should the fix cover both `display` and `visibility` CSS properties, or just `display:none` as originally reported? — A: Both, since they're both currently no-op parsed-and-discarded in the same function.
- Q: Does the identified root cause (values parsed then discarded, never applied, no ancestor propagation, no `groupmode=layer` special-casing, same code path for import+link) match expectations? — A: Confirmed as-is.
