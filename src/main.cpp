#include <iostream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "core/NLP/process/SemanticIO.h"
#include "core/eventbus/EventBus.h"
#include "core/eventbus/Events.h"

#ifdef SEMANTICAD_HAS_VTK
#include "core/vtk/VtkViewer.h"
#include "infrastructure/MeshLoader.h"
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#endif

#ifdef SEMANTICAD_HAS_GUI
#include "ui/MainWindow.h"

#include <QVTKOpenGLNativeWidget.h>

#include <QApplication>
#include <QString>
#include <QSurfaceFormat>
#endif

namespace {

const char* kDefaultModel = "models/Qwen2.5-3B-Instruct-Q4_K_M.gguf";

void printBounds(const char* label, const double b[6]) {
    std::cout << "[SemantiCAD] " << label << ": x[" << b[0] << ", " << b[1] << "] y["
              << b[2] << ", " << b[3] << "] z[" << b[4] << ", " << b[5] << "]" << std::endl;
}

// Builds a CommandEvent from a {"command":..,"params":..} envelope.
semanticad::core::CommandEvent toEvent(const nlohmann::ordered_json& envelope) {
    return semanticad::core::CommandEvent{
        envelope.value("command", std::string{}),
        envelope.contains("params") ? envelope["params"] : nlohmann::ordered_json::object()};
}

#ifdef SEMANTICAD_HAS_GUI
// Default mode: the Qt desktop UI (VTK viewport + chat). An optional --object
// is preloaded and an optional natural-language instruction is auto-run once the
// model finishes loading.
int runGui(int argc, char** argv, const std::string& commandsDir,
           const std::string& objectPath, const std::string& instruction) {
    // Must be set before any OpenGL context / QVTKOpenGLNativeWidget is created.
    QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("SemantiCAD"));
    app.setApplicationDisplayName(QStringLiteral("SemantiCAD"));

    semanticad::ui::MainWindow window(QString::fromUtf8(kDefaultModel),
                                      QString::fromStdString(commandsDir));
    if (!objectPath.empty()) window.openFile(QString::fromStdString(objectPath));
    if (!instruction.empty()) window.queuePrompt(QString::fromStdString(instruction));
    window.show();
    return app.exec();
}
#endif

// Headless / no-GUI-build mode: drives the scene without Qt. Used for scripting
// and automated verification (--no-window, --apply) and as the fallback when the
// GUI is not compiled in.
int runHeadless(const std::string& commandsDir, [[maybe_unused]] const std::string& objectPath,
                const std::string& applyJson, const std::string& instruction,
                [[maybe_unused]] bool openWindow) {
    semanticad::core::EventBus bus;

#ifdef SEMANTICAD_HAS_VTK
    // VTK layer subscribes to CommandEvents — it never sees the NLP layer.
    semanticad::viz::VtkViewer viewer(bus);
    if (!objectPath.empty()) {
        std::string err;
        auto mesh = semanticad::infra::loadMesh(objectPath, err);
        if (!mesh) {
            std::cerr << "[SemantiCAD] Obje yuklenemedi: " << err << std::endl;
            return 1;
        }
        viewer.loadPolyData(mesh);
        std::cout << "[SemantiCAD] Obje yuklendi: " << objectPath << std::endl;
    } else {
        viewer.loadDefaultCube();
        std::cout << "[SemantiCAD] Obje verilmedi; varsayilan kup yuklendi "
                     "(--object <dosya> ile verebilirsiniz)." << std::endl;
    }
    if (!openWindow) {
        double b[6];
        viewer.getBounds(b);
        printBounds("Baslangic sinirlari", b);
    }
#endif

    // Command source: a direct --apply envelope, or an NL instruction via the model.
    if (!applyJson.empty()) {
        auto j = nlohmann::ordered_json::parse(applyJson, nullptr, /*allow_exceptions=*/false);
        if (j.is_discarded() || !j.is_object() || !j.contains("command")) {
            std::cerr << "[SemantiCAD] --apply gecersiz JSON zarfi." << std::endl;
        } else {
            std::cout << "[SemantiCAD] Komut uygulaniyor: " << j.dump() << std::endl;
            bus.publish(toEvent(j));
        }
    } else if (!instruction.empty()) {
        std::cout << "[SemantiCAD] NLP katmani baslatiliyor..." << std::endl;
        semanticad::nlp::SemanticIO io(kDefaultModel, commandsDir);

        const auto& report = io.commandLoadReport();
        std::cout << "[SemantiCAD] Yuklenen komutlar (" << report.loaded.size() << "): ";
        for (size_t i = 0; i < report.loaded.size(); ++i)
            std::cout << (i ? ", " : "") << report.loaded[i];
        std::cout << std::endl;
        for (const auto& e : report.errors)
            std::cerr << "[SemantiCAD] Komut yukleme hatasi: " << e << std::endl;

        if (!io.ready()) {
            std::cerr << "[SemantiCAD] NLP hazir degil (model yuklenemedi veya komut tanimi yok)."
                      << std::endl;
        } else {
            const auto result = io.process(instruction);
            std::cout << "> " << instruction << "\n" << result.envelope.dump(2) << std::endl;
            if (result.ok) bus.publish(toEvent(result.envelope));
        }
    }

#ifdef SEMANTICAD_HAS_VTK
    if (!openWindow) {
        double b[6];
        viewer.getBounds(b);
        printBounds("Sonuc sinirlari", b);
    }
    viewer.start(openWindow);
#endif

    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    // CLI:
    //   (no args)           launch the Qt desktop UI (default)
    //   --object <file>     3D model to load (.stl/.obj/.ply/.vtp)
    //   --commands <dir>    command-definition directory (default: commands)
    //   --no-window         headless: no UI/window (scripting / CI)
    //   --apply <json>      apply a {"command":..,"params":..} envelope directly
    //                       (headless; bypasses the model)
    //   <rest...>           natural-language instruction (auto-run in the UI, or
    //                       run once headless with --no-window)
    std::string commandsDir = "commands";
    std::string objectPath;
    std::string applyJson;
    bool openWindow = true;
    std::vector<std::string> rest;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--commands" && i + 1 < argc) commandsDir = argv[++i];
        else if (a == "--object" && i + 1 < argc) objectPath = argv[++i];
        else if (a == "--apply" && i + 1 < argc) applyJson = argv[++i];
        else if (a == "--no-window") openWindow = false;
        else rest.push_back(a);
    }
    std::string instruction;
    for (const auto& s : rest) {
        if (!instruction.empty()) instruction += ' ';
        instruction += s;
    }

#ifdef SEMANTICAD_HAS_GUI
    // The GUI is the default; --no-window (or --apply) drops to headless.
    if (openWindow && applyJson.empty())
        return runGui(argc, argv, commandsDir, objectPath, instruction);
#endif

    return runHeadless(commandsDir, objectPath, applyJson, instruction, openWindow);
}
