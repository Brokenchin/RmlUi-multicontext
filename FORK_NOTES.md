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

## Modified Behavior

### `ElementUtilities::GetClippingRegion()` — always clip overflow boxes
- **File:** `Source/Core/ElementUtilities.cpp`
- **Change:** `has_clipping_content` no longer depends on `GetScrollWidth/Height() >
  GetClientWidth/Height()`. An element with `overflow != visible` (`clip_enabled`)
  now always contributes its client area to the clip region.
- **Why:** The old scroll-vs-client heuristic only clipped when *in-flow* content
  overflowed. It missed two cases that must still be clipped:
  1. **Absolutely-positioned descendants** — these never grow an ancestor's
     `scrollable_overflow_rectangle`. They are laid out in
     `ContainerBox::ClosePositionedElements()`, which runs *after*
     `SubmitBox()`/`SetScrollableOverflowRectangle()`. So an `overflow:hidden`
     box whose only overflowing content is abs-positioned reported "no clipping
     content" and emitted no scissor region — its descendants rendered unclipped,
     compositing over unrelated siblings (rmlui-godot issue #44).
  2. **Top/left (negative) overflow** — never counted as scrollable overflow, so a
     pannable abs-positioned canvas with negative offsets was never clipped.
- **Trade-off:** Every clipping box now emits a scissor region (and rounded/
  transformed ones a clip mask) even when nothing overflows. This is what browsers
  do; scissor is cheap. This is the only change to an *existing* upstream function.

## Merge Strategy

When updating to a new RmlUI version:

1. Check if upstream added equivalent API (unlikely but possible).
2. Re-apply the 3 functions — they are purely additive (~30 lines total),
   touch only `Core.h`, `Core.cpp`, `CallbackTexture.h`, `CallbackTexture.cpp`.
3. Re-apply the `GetClippingRegion()` `has_clipping_content` change above if upstream
   still gates clipping on scrollable overflow (see "Modified Behavior").
4. Forward-declare `RenderManager` was added to `Core.h` (line after `RenderInterface`).
