#include "core/command/mirror/MirrorService.h"

#include "core/command/entity/CadObject.h"

#include <vtkNew.h>
#include <vtkTransform.h>

namespace semanticad::core::command::mirror {

void MirrorService::apply(const MirrorPlane& mirror, CadObject& object) {
    vtkNew<vtkTransform> transform;
    switch (mirror.plane()) {
        case Plane::YZ:
            transform->Scale(-1.0, 1.0, 1.0);  // reflect across yz -> flip x
            break;
        case Plane::XZ:
            transform->Scale(1.0, -1.0, 1.0);  // reflect across xz -> flip y
            break;
        case Plane::XY:
            transform->Scale(1.0, 1.0, -1.0);  // reflect across xy -> flip z
            break;
    }
    object.applyTransform(transform);
}

}  // namespace semanticad::core::command::mirror
