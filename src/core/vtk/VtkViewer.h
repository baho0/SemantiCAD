#pragma once

// The VTK visualization layer. It holds the render scene (renderer, actor and a
// cumulative vtkTransform) and reacts to CommandEvents from the EventBus by
// transforming the loaded object. It never references the NLP layer — the only
// shared types are the EventBus and CommandEvent (in core), keeping the layers
// decoupled.

#include "core/command/CommandDispatcher.h"
#include "core/eventbus/Events.h"

#include <vtkSmartPointer.h>

class vtkPolyData;
class vtkPolyDataMapper;
class vtkActor;
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

    // --- Embedding API (used by the Qt UI) -------------------------------
    // Attach the viewer's renderer to an externally owned render window (e.g.
    // the one created by Qt's QVTKOpenGLNativeWidget), then frame and render.
    // The viewer does not take exclusive ownership: the host widget keeps the
    // window alive. Commands and re-loads render into this window automatically.
    void setRenderWindow(vtkRenderWindow* window);

    // Frame the current object in the camera, then render.
    void resetCamera();

    // Request a redraw (no-op when not attached to any window).
    void render();

private:
    void onCommand(const core::CommandEvent& event);

    vtkSmartPointer<vtkPolyData> data_;
    vtkSmartPointer<vtkPolyDataMapper> mapper_;
    vtkSmartPointer<vtkActor> actor_;
    vtkSmartPointer<vtkRenderer> renderer_;
    vtkSmartPointer<vtkRenderWindow> window_;
    vtkSmartPointer<vtkRenderWindowInteractor> interactor_;

    // Routes CommandEvents to the matching command. Pre-populated with the
    // built-in commands (resize, translate, rotate, mirror).
    core::command::CommandDispatcher dispatcher_;
};

}  // namespace semanticad::viz
