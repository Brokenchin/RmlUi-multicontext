#include "FontFace.h"
#include "../../../Include/RmlUi/Core/Log.h"
#include "FontFaceHandleDefault.h"
#include "FreeTypeInterface.h"

namespace Rml {

FontFace::FontFace(FontFaceHandleFreetype _face, Style::FontStyle _style, Style::FontWeight _weight)
{
	style = _style;
	weight = _weight;
	face = _face;
}

FontFace::~FontFace()
{
	if (face)
		FreeType::ReleaseFace(face);
}

Style::FontStyle FontFace::GetStyle() const
{
	return style;
}

Style::FontWeight FontFace::GetWeight() const
{
	return weight;
}

FontFaceHandleDefault* FontFace::GetHandle(int size, bool load_default_glyphs)
{
	auto it = handles.find(size);
	if (it != handles.end())
		return it->second.get();

	// See if this face has been released.
	if (!face)
	{
		Log::Message(Log::LT_INFO, "[diag] FontFace::GetHandle(%d) — face released, cannot regenerate", size);
		Log::Message(Log::LT_WARNING, "Font face has been released, unable to generate new handle.");
		return nullptr;
	}

	Log::Message(Log::LT_INFO, "[diag] FontFace::GetHandle(%d) — regenerating handle (face alive)", size);

	// Construct and initialise the new handle.
	auto handle = MakeUnique<FontFaceHandleDefault>();
	if (!handle->Initialize(face, size, load_default_glyphs))
	{
		Log::Message(Log::LT_INFO, "[diag] FontFace::GetHandle(%d) — Initialize FAILED", size);
		handles[size] = nullptr;
		return nullptr;
	}

	FontFaceHandleDefault* result = handle.get();

	// Save the new handle to the font face
	handles[size] = std::move(handle);

	Log::Message(Log::LT_INFO, "[diag] FontFace::GetHandle(%d) — regenerated OK", size);
	return result;
}

void FontFace::ReleaseFontResources()
{
	Log::Message(Log::LT_INFO, "[diag] FontFace::ReleaseFontResources — destroying %zu sized handles", handles.size());
	HandleMap().swap(handles);
}

} // namespace Rml
