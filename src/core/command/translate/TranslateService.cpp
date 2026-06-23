#include "core/command/translate/TranslateService.h"

#include "core/command/entity/CadObject.h"

#include <vtkNew.h>
#include <vtkTransform.h>

namespace semanticad::core::command::translate {

void TranslateService::apply(const Translation& translation, CadObject& object) {
    vtkNew<vtkTransform> transform;
    transform->Translate(translation.dx(), translation.dy(), translation.dz());
    object.applyTransform(transform);
}

}  // namespace semanticad::core::command::translate
