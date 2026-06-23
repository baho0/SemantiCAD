#include "core/command/mirror/MirrorPlane.h"

namespace semanticad::core::command::mirror {

std::optional<MirrorPlane> MirrorPlane::create(const std::string& plane) {
    if (plane == "xy") return MirrorPlane(Plane::XY);
    if (plane == "yz") return MirrorPlane(Plane::YZ);
    if (plane == "xz") return MirrorPlane(Plane::XZ);
    return std::nullopt;
}

}  // namespace semanticad::core::command::mirror
