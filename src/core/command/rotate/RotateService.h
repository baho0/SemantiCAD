#pragma once

// Service: the geometry of "rotate" — builds a per-axis rotation transform and
// bakes it into the CadObject.

#include "core/command/rotate/Rotation.h"

namespace semanticad::core::command {
class CadObject;
}

namespace semanticad::core::command::rotate {

class RotateService {
public:
    static void apply(const Rotation& rotation, CadObject& object);
};

}  // namespace semanticad::core::command::rotate
