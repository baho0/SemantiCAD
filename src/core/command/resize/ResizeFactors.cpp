#include "core/command/resize/ResizeFactors.h"

#include <cmath>

namespace semanticad::core::command::resize {

namespace {
// A scale multiplier must be finite and > 0: zero collapses the geometry and a
// negative value silently turns a resize into a reflection.
bool isValidFactor(double v) { return std::isfinite(v) && v > 0.0; }
}  // namespace

std::optional<ResizeFactors> ResizeFactors::create(double x, double y, double z) {
    if (!isValidFactor(x) || !isValidFactor(y) || !isValidFactor(z)) return std::nullopt;
    return ResizeFactors(x, y, z);
}

}  // namespace semanticad::core::command::resize
