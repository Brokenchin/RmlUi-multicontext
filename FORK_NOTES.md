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

## Merge Strategy

When updating to a new RmlUI version:

1. Check if upstream added equivalent API (unlikely but possible).
2. Re-apply the 3 functions — they are purely additive (~30 lines total),
   touch only `Core.h`, `Core.cpp`, `CallbackTexture.h`, `CallbackTexture.cpp`.
3. No existing functions are modified.
4. Forward-declare `RenderManager` was added to `Core.h` (line after `RenderInterface`).
