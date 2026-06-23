#pragma once

// Value object: a validated mirror plane for the "mirror" command. The plane is
// a closed set (xy, yz, xz), so the value object owns its parsing — an unknown
// plane string simply yields std::nullopt.

#include <optional>
#include <string>

namespace semanticad::core::command::mirror {

enum class Plane { XY, YZ, XZ };

class MirrorPlane {
public:
    static std::optional<MirrorPlane> create(const std::string& plane);

    Plane plane() const { return plane_; }

    bool operator==(const MirrorPlane& o) const { return plane_ == o.plane_; }

private:
    explicit MirrorPlane(Plane plane) : plane_(plane) {}

    Plane plane_;
};

}  // namespace semanticad::core::command::mirror
