#pragma once

#include <mbgl/text/glyph.hpp>

namespace mbgl {

/*
    Given a font stack and a glyph ID, platform-specific implementations of
    LocalGlyphRasterizer will decide which, if any, local fonts to use and
    then generate a matching glyph object with a greyscale rasterization of
    the glyph and appropriate metrics. GlyphManager will then use TinySDF to
    transform the rasterized bitmap into an SDF.
 
    The JS equivalent of this functionality will only generate glyphs in the
    'CJK Unified Ideographs' and 'Hangul Syllables' ranges, for which it can
    get away with rendering a fixed 30px square image and GlyphMetrics of:

        width: 24,
        height: 24,
        left: 0,
        top: -8,
        advance: 24

    The JS equivalent also uses heuristic evaluation of the font stack name
    to control the font-weight it uses during rasterization.
 
    It is left to platform-specific implementation to decide how best to
    map a FontStack to a particular rasterization.
*/

class LocalGlyphRasterizer {
public:
    bool canGenerateGlyph(const FontStack&, GlyphID);
    Glyph generateRasterGlyph(const FontStack&, GlyphID);
};

} // namespace mbgl
