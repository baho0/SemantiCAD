#pragma once

// Value object: validated per-axis scale multipliers for the "resize" command.
// Immutable and self-validating — a ResizeFactors can only exist if every factor
// is a finite, strictly positive multiplier (1.0 = unchanged). Construction goes
// through create(), which returns std::nullopt for any invalid factor.

#include <optional>

namespace semanticad::core::command::resize {

class ResizeFactors {
public:
    static std::optional<ResizeFactors> create(double x, double y, double z);

    double x() const { return x_; }
    double y() const { return y_; }
    double z() const { return z_; }

    bool operator==(const ResizeFactors& o) const {
        return x_ == o.x_ && y_ == o.y_ && z_ == o.z_;
    }

private:
    ResizeFactors(double x, double y, double z) : x_(x), y_(y), z_(z) {}

    double x_;
    double y_;
    double z_;
};

}  // namespace semanticad::core::command::resize
