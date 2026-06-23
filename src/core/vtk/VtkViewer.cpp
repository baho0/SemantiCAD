#include "core/vtk/VtkViewer.h"

#include "core/command/entity/CadObject.h"
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

    // Commands bake their transforms directly into the polydata (via CadObject),
    // so the actor needs no cumulative user transform — it just draws data_.
    actor_ = vtkSmartPointer<vtkActor>::New();
    actor_->SetMapper(mapper_);

    renderer_ = vtkSmartPointer<vtkRenderer>::New();
    renderer_->AddActor(actor_);
    renderer_->SetBackground(0.10, 0.10, 0.12);
}

void VtkViewer::onCommand(const core::CommandEvent& event) {
    if (!data_) return;

    // Wrap the loaded geometry as the CadObject entity and hand it, with the
    // command, to the dispatcher. The matching command bakes its transform into
    // the object's points. The viewer no longer knows what any command does —
    // only how to draw the result. "none" / "error" / unknown names and invalid
    // params come back as a failure and visualise nothing.
    core::command::CadObject object(data_);
    const auto result = dispatcher_.dispatch(event.command, event.params, object);
    if (!result.ok) return;

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
