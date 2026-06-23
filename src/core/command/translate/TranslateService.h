#pragma once

// Service: the geometry of "translate". Builds the translation transform and
// bakes it into the CadObject.

#include "core/command/translate/Translation.h"

namespace semanticad::core::command {
class CadObject;
}

namespace semanticad::core::command::translate {

class TranslateService {
public:
    static void apply(const Translation& translation, CadObject& object);
};

}  // namespace semanticad::core::command::translate
