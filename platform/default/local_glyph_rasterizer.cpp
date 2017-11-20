#include <mbgl/text/local_glyph_rasterizer.hpp>

namespace mbgl {

bool LocalGlyphRasterizer::canGenerateGlyph(const FontStack&, GlyphID) {
    return false;
}

Glyph LocalGlyphRasterizer::generateRasterGlyph(const FontStack&, GlyphID) {
    return Glyph();
}

} // namespace mbgl
