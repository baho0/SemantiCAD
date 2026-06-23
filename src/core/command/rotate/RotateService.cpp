#include "core/command/rotate/RotateService.h"

#include "core/command/entity/CadObject.h"

#include <vtkNew.h>
#include <vtkTransform.h>

namespace semanticad::core::command::rotate {

void RotateService::apply(const Rotation& rotation, CadObject& object) {
    vtkNew<vtkTransform> transform;
    switch (rotation.axis()) {
        case Axis::X:
            transform->RotateX(rotation.angleDeg());
            break;
        case Axis::Y:
            transform->RotateY(rotation.angleDeg());
            break;
        case Axis::Z:
            transform->RotateZ(rotation.angleDeg());
            break;
    }
    object.applyTransform(transform);
}

}  // namespace semanticad::core::command::rotate
