#include "infrastructure/MeshLoader.h"

#include <vtkOBJReader.h>
#include <vtkPLYReader.h>
#include <vtkPolyData.h>
#include <vtkSTLReader.h>
#include <vtkXMLPolyDataReader.h>

#include <algorithm>
#include <cctype>
#include <filesystem>

namespace semanticad::infra {

namespace {

std::string lowerExtension(const std::string& path) {
    std::string ext = std::filesystem::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

// Reads via the given VTK reader type and returns a detached deep copy.
template <class ReaderT>
vtkSmartPointer<vtkPolyData> readWith(const std::string& path) {
    vtkSmartPointer<ReaderT> reader = vtkSmartPointer<ReaderT>::New();
    reader->SetFileName(path.c_str());
    reader->Update();
    vtkPolyData* output = reader->GetOutput();
    if (output == nullptr || output->GetNumberOfPoints() == 0) return nullptr;
    auto data = vtkSmartPointer<vtkPolyData>::New();
    data->DeepCopy(output);  // detach from the reader's pipeline
    return data;
}

}  // namespace

vtkSmartPointer<vtkPolyData> loadMesh(const std::string& path, std::string& error) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        error = "file not found: " + path;
        return nullptr;
    }

    const std::string ext = lowerExtension(path);
    vtkSmartPointer<vtkPolyData> data;
    if (ext == ".stl")
        data = readWith<vtkSTLReader>(path);
    else if (ext == ".obj")
        data = readWith<vtkOBJReader>(path);
    else if (ext == ".ply")
        data = readWith<vtkPLYReader>(path);
    else if (ext == ".vtp")
        data = readWith<vtkXMLPolyDataReader>(path);
    else {
        error = "unsupported file extension '" + ext + "' (expected .stl/.obj/.ply/.vtp)";
        return nullptr;
    }

    if (!data) {
        error = "failed to read a non-empty mesh from: " + path;
        return nullptr;
    }
    return data;
}

}  // namespace semanticad::infra
