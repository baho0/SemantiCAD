#include "core/command/translate/Translation.h"

#include <cmath>

namespace semanticad::core::command::translate {

std::optional<Translation> Translation::create(double dx, double dy, double dz) {
    if (!std::isfinite(dx) || !std::isfinite(dy) || !std::isfinite(dz)) {
        return std::nullopt;
    }
    return Translation(dx, dy, dz);
}

}  // namespace semanticad::core::command::translate
