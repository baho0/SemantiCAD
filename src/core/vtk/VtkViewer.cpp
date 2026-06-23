#include "core/vtk/VtkViewer.h"

#include "core/eventbus/EventBus.h"

#include <vtkActor.h>
#include <vtkCubeSource.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTransform.h>

#include <iostream>

namespace semanticad::viz {

VtkViewer::VtkViewer(core::EventBus& bus) {
    bus.subscribe<core::CommandEvent>(
        [this](const core::CommandEvent& e) { onCommand(e); });
}

VtkViewer::~VtkViewer() = default;

void VtkViewer::loadDefaultCube() {
    vtkNew<vtkCubeSource> cube;  // unit cube centred at the origin
    cube->Update();
    auto data = vtkSmartPointer<vtkPolyData>::New();
    data->DeepCopy(cube->GetOutput());
    loadPolyData(data);
}

void VtkViewer::loadPolyData(vtkSmartPointer<vtkPolyData> data) {
    data_ = data;

    mapper_ = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper_->SetInputData(data_);

    transform_ = vtkSmartPointer<vtkTransform>::New();
    transform_->PostMultiply();  // each command composes in world space
    transform_->Identity();

    actor_ = vtkSmartPointer<vtkActor>::New();
    actor_->SetMapper(mapper_);
    actor_->SetUserTransform(transform_);

    renderer_ = vtkSmartPointer<vtkRenderer>::New();
    renderer_->AddActor(actor_);
    renderer_->SetBackground(0.10, 0.10, 0.12);
}

void VtkViewer::onCommand(const core::CommandEvent& event) {
    if (!transform_) return;
    const auto& p = event.params;

    if (event.command == "resize") {
        transform_->Scale(p.value("x", 1.0), p.value("y", 1.0), p.value("z", 1.0));
    } else if (event.command == "translate") {
        transform_->Translate(p.value("dx", 0.0), p.value("dy", 0.0), p.value("dz", 0.0));
    } else if (event.command == "rotate") {
        const std::string axis = p.value("axis", std::string("z"));
        const double angle = p.value("angle_deg", 0.0);
        if (axis == "x") transform_->RotateX(angle);
        else if (axis == "y") transform_->RotateY(angle);
        else transform_->RotateZ(angle);
    } else if (event.command == "mirror") {
        const std::string plane = p.value("plane", std::string("xy"));
        if (plane == "yz") transform_->Scale(-1.0, 1.0, 1.0);
        else if (plane == "xz") transform_->Scale(1.0, -1.0, 1.0);
        else transform_->Scale(1.0, 1.0, -1.0);  // xy
    } else {
        return;  // "none" / "error" / unknown -> nothing to visualise
    }

    if (actor_) actor_->Modified();
    if (window_) window_->Render();
    std::cout << "[VtkViewer] '" << event.command << "' uygulandi." << std::endl;
}

void VtkViewer::getBounds(double out[6]) const {
    if (actor_) {
        actor_->GetBounds(out);  // includes the user transform; no rendering needed
    } else {
        for (int i = 0; i < 6; ++i) out[i] = 0.0;
    }
}

void VtkViewer::start(bool openWindow) {
    if (!renderer_) return;
    if (!openWindow) return;  // headless: do not create a window / touch OpenGL

    window_ = vtkSmartPointer<vtkRenderWindow>::New();
    window_->AddRenderer(renderer_);
    window_->SetSize(900, 700);
    window_->SetWindowName("SemantiCAD");

    interactor_ = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    interactor_->SetRenderWindow(window_);
    vtkNew<vtkInteractorStyleTrackballCamera> style;
    interactor_->SetInteractorStyle(style);

    renderer_->ResetCamera();
    window_->Render();
    interactor_->Start();  // blocks until the user closes the window
}

}  // namespace semanticad::viz
