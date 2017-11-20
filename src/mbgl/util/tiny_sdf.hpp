#pragma once

#include <mbgl/util/image.hpp>

namespace mbgl {
namespace util {

AlphaImage generateTinySDF(const AlphaImage& rasterInput, double radius, double cutoff);

} // namespace util
} // namespace mbgl
