#include "core/command/resize/ResizeService.h"

#include "core/command/entity/CadObject.h"

#include <vtkNew.h>
#include <vtkTransform.h>

namespace semanticad::core::command::resize {

void ResizeService::apply(const ResizeFactors& factors, CadObject& object) {
    vtkNew<vtkTransform> transform;
    transform->Scale(factors.x(), factors.y(), factors.z());
    object.applyTransform(transform);
}

}  // namespace semanticad::core::command::resize
