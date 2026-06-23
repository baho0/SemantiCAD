#pragma once

// Mesh file I/O lives in the infrastructure layer (the layer that talks to the
// outside world: file system, formats, persistence). Reads a polygonal mesh
// from disk into a VTK polydata object, choosing the reader by file extension.

#include <vtkSmartPointer.h>

#include <string>

class vtkPolyData;

namespace semanticad::infra {

// Loads a mesh from `path`. Supported extensions: .stl, .obj, .ply, .vtp.
// Returns the polydata on success, or nullptr with `error` set (file missing,
// unsupported extension, empty/invalid mesh).
vtkSmartPointer<vtkPolyData> loadMesh(const std::string& path, std::string& error);

}  // namespace semanticad::infra
