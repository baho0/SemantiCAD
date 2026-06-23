#include "core/command/entity/CadObject.h"

#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

#include <utility>

namespace semanticad::core::command {

CadObject::CadObject(vtkSmartPointer<vtkPolyData> data) : data_(std::move(data)) {}

void CadObject::applyTransform(vtkTransform* transform) {
    if (!data_ || !transform) return;

    vtkNew<vtkTransformPolyDataFilter> filter;
    filter->SetInputData(data_);
    filter->SetTransform(transform);
    filter->Update();

    // DeepCopy back into the held instance so external references (e.g. the
    // viewer's mapper) keep pointing at the same, now-updated, polydata.
    data_->DeepCopy(filter->GetOutput());
}

vtkPolyData* CadObject::polyData() const { return data_.Get(); }

bool CadObject::valid() const { return data_ != nullptr; }

}  // namespace semanticad::core::command
