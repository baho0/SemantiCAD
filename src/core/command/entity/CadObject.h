#pragma once

// Entity: the CAD object that commands act on. It owns the geometry (a
// vtkPolyData) and is the single thing every command receives and mutates. It
// knows how to bake a transform into its own points, so commands and their
// services never touch VTK's transform plumbing directly — they just hand the
// object a vtkTransform via applyTransform().
//
// The geometry is mutated in place (DeepCopy of the transformed output back into
// the held polydata), so any mapper already pointing at polyData() stays valid.

#include <vtkSmartPointer.h>

class vtkPolyData;
class vtkTransform;

namespace semanticad::core::command {

class CadObject {
public:
    explicit CadObject(vtkSmartPointer<vtkPolyData> data);

    // Bake `transform` into the object's points. No-op if the object is empty.
    void applyTransform(vtkTransform* transform);

    vtkPolyData* polyData() const;
    bool valid() const;

private:
    vtkSmartPointer<vtkPolyData> data_;
};

}  // namespace semanticad::core::command
