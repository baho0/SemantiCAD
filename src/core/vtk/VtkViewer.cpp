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

    // Build the scene pipeline once and reuse it on subsequent loads. Recreating
    // the renderer would orphan the one already attached to an embedded (Qt)
    // render window, so a freshly opened file would not appear. Instead we swap
    // the mapper's input and keep the same renderer/actor wired to the window.
    if (!mapper_) mapper_ = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper_->SetInputData(data_);

    // Commands bake their transforms directly into the polydata (via CadObject),
    // so the actor needs no cumulative user transform — it just draws data_.
    if (!actor_) {
        actor_ = vtkSmartPointer<vtkActor>::New();
        actor_->SetMapper(mapper_);
    }

    if (!renderer_) {
        renderer_ = vtkSmartPointer<vtkRenderer>::New();
        renderer_->SetBackground(0.10, 0.10, 0.12);
    }
    renderer_->AddActor(actor_);  // no-op if the actor is already present

    // When attached to a window (CLI start() or an embedded Qt widget), frame
    // the new object and show it immediately.
    if (window_) {
        renderer_->ResetCamera();
        window_->Render();
    }
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

void VtkViewer::setRenderWindow(vtkRenderWindow* window) {
    window_ = window;  // shared with the host widget; both keep it alive
    if (window_ && renderer_) {
        window_->AddRenderer(renderer_);
        renderer_->ResetCamera();
        window_->Render();
    }
}

void VtkViewer::resetCamera() {
    if (renderer_) renderer_->ResetCamera();
    if (window_) window_->Render();
}

void VtkViewer::render() {
    if (window_) window_->Render();
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
