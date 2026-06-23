#pragma once

// Service: the geometry of "resize". Builds the scale transform from validated
// factors and bakes it into the CadObject. The service owns the VTK transform
// construction; the command above it stays free of VTK details.

#include "core/command/resize/ResizeFactors.h"

namespace semanticad::core::command {
class CadObject;
}

namespace semanticad::core::command::resize {

class ResizeService {
public:
    static void apply(const ResizeFactors& factors, CadObject& object);
};

}  // namespace semanticad::core::command::resize
