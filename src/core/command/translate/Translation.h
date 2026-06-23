#pragma once

// Value object: a validated translation delta (millimetres) for the "translate"
// command. dx/dy/dz are signed distances (0 = no movement on that axis); the
// only invariant is that each component is finite.

#include <optional>

namespace semanticad::core::command::translate {

class Translation {
public:
    static std::optional<Translation> create(double dx, double dy, double dz);

    double dx() const { return dx_; }
    double dy() const { return dy_; }
    double dz() const { return dz_; }

    bool operator==(const Translation& o) const {
        return dx_ == o.dx_ && dy_ == o.dy_ && dz_ == o.dz_;
    }

private:
    Translation(double dx, double dy, double dz) : dx_(dx), dy_(dy), dz_(dz) {}

    double dx_;
    double dy_;
    double dz_;
};

}  // namespace semanticad::core::command::translate
