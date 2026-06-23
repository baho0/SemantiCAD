#pragma once

// Service: the geometry of "mirror". A reflection across a plane is a scale of
// -1 along the axis normal to that plane; the service builds that transform and
// bakes it into the CadObject.

#include "core/command/mirror/MirrorPlane.h"

namespace semanticad::core::command {
class CadObject;
}

namespace semanticad::core::command::mirror {

class MirrorService {
public:
    static void apply(const MirrorPlane& mirror, CadObject& object);
};

}  // namespace semanticad::core::command::mirror
