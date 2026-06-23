#pragma once

// The VTK visualization layer. It holds the render scene (renderer, actor and a
// cumulative vtkTransform) and reacts to CommandEvents from the EventBus by
// transforming the loaded object. It never references the NLP layer — the only
// shared types are the EventBus and CommandEvent (in core), keeping the layers
// decoupled.

#include "core/eventbus/Events.h"

#include <vtkSmartPointer.h>

class vtkPolyData;
class vtkPolyDataMapper;
class vtkActor;
class vtkTransform;
class vtkRenderer;
class vtkRenderWindow;
class vtkRenderWindowInteractor;

namespace semanticad::core {
class EventBus;
}

namespace semanticad::viz {

class VtkViewer {
public:
    explicit VtkViewer(core::EventBus& bus);
    ~VtkViewer();

    VtkViewer(const VtkViewer&) = delete;
    VtkViewer& operator=(const VtkViewer&) = delete;

    void loadPolyData(vtkSmartPointer<vtkPolyData> data);
    void loadDefaultCube();  // fallback when no object file is provided

    // World-space bounds of the (transformed) object: [xmin,xmax,ymin,ymax,zmin,zmax].
    void getBounds(double out[6]) const;

    // openWindow=true opens an interactive render window (blocks until closed).
    // openWindow=false is headless: no window/GL is created (useful for tests
    // and for running where there is no display).
    void start(bool openWindow);

private:
    void onCommand(const core::CommandEvent& event);

    vtkSmartPointer<vtkPolyData> data_;
    vtkSmartPointer<vtkPolyDataMapper> mapper_;
    vtkSmartPointer<vtkActor> actor_;
    vtkSmartPointer<vtkTransform> transform_;
    vtkSmartPointer<vtkRenderer> renderer_;
    vtkSmartPointer<vtkRenderWindow> window_;
    vtkSmartPointer<vtkRenderWindowInteractor> interactor_;
};

}  // namespace semanticad::viz
