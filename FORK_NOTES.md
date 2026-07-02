# RmlUI Fork — CaveCrawler Additions

Upstream: https://github.com/mikke89/RmlUi

## Added Functions

These additions fill a gap in RmlUI's multi-context cleanup API.
`ReleaseRenderManagers()` (the existing plural function) couples render manager
removal with `ReleaseFontResources()`, which globally destroys all font glyph
caches across all contexts. When a temporary context (e.g. a drag preview ghost)
is destroyed while other contexts are still rendering, the global font nuke
kills their fonts permanently — `Update()` fails to regenerate handles because
the style system short-circuits when `font-size` hasn't changed.

### 1. `Rml::GetRenderManager(RenderInterface*)`
- **File:** `Include/RmlUi/Core/Core.h`, `Source/Core/Core.cpp`
- **Purpose:** Lookup a render manager by its render interface pointer.
- **Why:** Needed to get the `RenderManager*` before releasing font atlas textures.

### 2. `Rml::ReleaseRenderManager(RenderInterface*)`
- **File:** `Include/RmlUi/Core/Core.h`, `Source/Core/Core.cpp`
- **Purpose:** Remove a single render manager entry from `core_data->render_managers`
  without calling `ReleaseFontResources()`.
- **Why:** Allows destroying a temporary context's render manager without
  affecting font caches of surviving contexts.

### 3. `CallbackTextureSource::ReleaseForRenderManager(RenderManager*)`
- **File:** `Include/RmlUi/Core/CallbackTexture.h`, `Source/Core/CallbackTexture.cpp`
- **Purpose:** Release and erase the cached `CallbackTexture` entry for a
  specific render manager from a `CallbackTextureSource`'s internal map.
- **Why:** Font atlas textures (`CallbackTextureSource`) cache per-render-manager
  entries. When a render manager is destroyed, those entries become dangling
  pointers. This method allows cleaning them up before the render manager dies.

## Layout-Boundary Elements (branch `layout-boundary`)

RmlUI layout is per-document and non-incremental: any layout-affecting change
re-formats the whole document. When sub-documents are embedded as real DOM
subtrees of a host document (rmlui-godot's `<embed-doc>`), every reflow of the
host re-formats every embedded subtree too — measured at ~22 ms for one
populated panel, paid on every host-level change even when nothing embedded
actually moved. These two hooks let a host element opt its subtree out of
parent-driven layout via a `layout-boundary` attribute, dropping that same
reflow to ~0.3 ms (O(parent shell) instead of O(parent + all embeds)).

The contract: the boundary element is *replaced* (its `GetIntrinsicDimensions`
override reports the embedded document's formatted size), its children stay
real DOM (they render, hit-test, and update style normally), and the embedded
document formats itself out-of-band against the host's box (in rmlui-godot,
`RmlContext::_update_embed_layout` calls `UpdateDocument()` per embed and pins
the document's offset to the host's content origin).

### 4. `ReplacedFormattingContext::Format` — skip `layout-boundary` children
- **File:** `Source/Core/Layout/ReplacedFormattingContext.cpp`
- **Change:** the "format normal DOM children of a replaced element" fallback
  is gated on `!element->HasAttribute("layout-boundary")` (one condition).
- **Why:** that fallback is the recursion point through which a host reflow
  re-formats embedded subtrees. A boundary element's children format
  themselves, so the parent must treat it as an opaque fixed box.

### 5. `ElementDocument::UpdateLayout` — viewport containing block under a boundary
- **File:** `Source/Core/ElementDocument.cpp`
- **Change:** when the document's parent has the `layout-boundary` attribute,
  the containing block passed to `LayoutEngine::FormatElement` is the context
  dimensions instead of the parent's box (guarded, ~4 lines).
- **Why:** the boundary host is sized FROM this document (replaced, intrinsic
  = doc size), so using the host's box as the containing block is circular —
  percentage widths/maxima resolve against 0 and stick there (a `max-width:
  98%` panel collapsed to its 2 px of border). Resolving % against the
  viewport matches the intent of %-maxima on floating panels ("never exceed
  the screen").

### Integration notes (consumer side, rmlui-godot)
- The host element must be **block-level or absolutely positioned**. Two
  RmlUI code paths ignore replaced intrinsic dimensions: inline layout
  (an inline replaced host gets its intrinsic height but width 0) and
  shrink-to-fit width (an auto-width wrapper around a boundary host
  collapses to 0, e.g. an `{ position: absolute; right: 16dp }` wrapper —
  anchor the host itself instead).
- The consumer must re-dirty the parent when the embedded document's outer
  size changes (rmlui-godot compares with a half-pixel epsilon so sub-pixel
  text-metric wobble never triggers a host reflow).

## Merge Strategy

When updating to a new RmlUI version:

1. Check if upstream added equivalent API (unlikely but possible).
2. Re-apply the 3 cleanup functions — they are purely additive (~30 lines
   total), touch only `Core.h`, `Core.cpp`, `CallbackTexture.h`,
   `CallbackTexture.cpp`. No existing functions are modified.
3. Forward-declare `RenderManager` was added to `Core.h` (line after
   `RenderInterface`).
4. Re-apply the 2 layout-boundary hooks (§4–5): one gate condition in
   `ReplacedFormattingContext.cpp`, one guarded containing-block override in
   `ElementDocument.cpp` (~15 lines total). Both are behavior-preserving for
   any element without the `layout-boundary` attribute.
