#pragma once

#include <mbgl/text/glyph.hpp>

namespace mbgl {

class LocalGlyphGenerator {
public:
    bool canGenerateGlyph(char16_t) { return false; }
    Glyph generateGlyph(char16_t) { return Glyph(); }
};

} // namespace mbgl
