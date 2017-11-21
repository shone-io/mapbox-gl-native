#pragma once

#include <mbgl/util/image.hpp>

namespace mbgl {
namespace util {

/*
    C++ port of https://github.com/mapbox/tiny-sdf, which is in turn based on the
    Felzenszwalb/Huttenlocher distance transform (https://cs.brown.edu/~pff/dt/)
 
    Takes an alpha channel raster input and transforms it into an alpha channel
    Signed Distance Field (SDF) output of the same dimensions.
*/
AlphaImage generateTinySDF(const AlphaImage& rasterInput, double radius, double cutoff);

} // namespace util
} // namespace mbgl
