#pragma once

// Tiny VTK fixtures for the command tests. Commands now mutate real geometry
// (via CadObject), so tests assert on the polydata's bounds before/after. A unit
// cube is enough: it has a known, symmetric bounding box that makes scale,
// translate, rotate and mirror results easy to predict.

#include <vtkCubeSource.h>
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

#include <array>

namespace semanticad::test {

// A unit cube (side 1) centred at (cx, cy, cz): bounds [cx-0.5, cx+0.5], etc.
inline vtkSmartPointer<vtkPolyData> makeCube(double cx = 0.0, double cy = 0.0, double cz = 0.0) {
    vtkNew<vtkCubeSource> cube;
    cube->SetCenter(cx, cy, cz);
    cube->Update();
    auto data = vtkSmartPointer<vtkPolyData>::New();
    data->DeepCopy(cube->GetOutput());
    return data;
}

// [xmin, xmax, ymin, ymax, zmin, zmax]
inline std::array<double, 6> boundsOf(vtkPolyData* data) {
    std::array<double, 6> b{};
    data->GetBounds(b.data());
    return b;
}

}  // namespace semanticad::test
