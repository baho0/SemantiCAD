// GUI entry point for SemantiCAD. The composition root proper lives in
// MainWindow; this only sets the OpenGL surface format VTK needs, spins up the
// Qt application, and optionally preloads a model file given on the command line.
//
//   ./semanticad_gui [model-file.stl]
//
// The CLI executable (semanticad) is unchanged and still used for headless /
// scripted runs (--apply, --no-window) and the test suite.

#include "ui/MainWindow.h"

#include <QVTKOpenGLNativeWidget.h>

#include <QApplication>
#include <QStringList>
#include <QSurfaceFormat>

int main(int argc, char** argv) {
    // Must be set before any OpenGL context / QVTKOpenGLNativeWidget is created.
    QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("SemantiCAD"));
    app.setApplicationDisplayName(QStringLiteral("SemantiCAD"));

    semanticad::ui::MainWindow window;

    const QStringList args = app.arguments();
    if (args.size() > 1)  // optional positional: a 3D model to open at startup
        window.openFile(args.at(1));

    window.show();
    return app.exec();
}
