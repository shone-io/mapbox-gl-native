#pragma once

#include <mbgl/util/image.hpp>

namespace mbgl {
namespace util {

AlphaImage generateTinySDF(const PremultipliedImage& rasterInput);

} // namespace util
} // namespace mbgl
