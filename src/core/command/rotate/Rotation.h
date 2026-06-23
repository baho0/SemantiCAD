#pragma once

// Value object: a validated rotation for the "rotate" command — an axis (x, y or
// z) plus an angle in degrees. The axis is a closed set, so the value object owns
// its parsing; the angle must be finite. Positive angles are counter-clockwise.

#include <optional>
#include <string>

namespace semanticad::core::command::rotate {

enum class Axis { X, Y, Z };

class Rotation {
public:
    static std::optional<Axis> parseAxis(const std::string& axis);
    static std::optional<Rotation> create(Axis axis, double angleDeg);

    Axis axis() const { return axis_; }
    double angleDeg() const { return angleDeg_; }

    bool operator==(const Rotation& o) const {
        return axis_ == o.axis_ && angleDeg_ == o.angleDeg_;
    }

private:
    Rotation(Axis axis, double angleDeg) : axis_(axis), angleDeg_(angleDeg) {}

    Axis axis_;
    double angleDeg_;
};

}  // namespace semanticad::core::command::rotate
