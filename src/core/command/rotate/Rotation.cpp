#include "core/command/rotate/Rotation.h"

#include <cmath>

namespace semanticad::core::command::rotate {

std::optional<Axis> Rotation::parseAxis(const std::string& axis) {
    if (axis == "x") return Axis::X;
    if (axis == "y") return Axis::Y;
    if (axis == "z") return Axis::Z;
    return std::nullopt;
}

std::optional<Rotation> Rotation::create(Axis axis, double angleDeg) {
    if (!std::isfinite(angleDeg)) return std::nullopt;
    return Rotation(axis, angleDeg);
}

}  // namespace semanticad::core::command::rotate
