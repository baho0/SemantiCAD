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

namespace {

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

}  // namespace

int main(int argc, char** argv) {
    // CLI:
    //   --object <file>     3D model to load into VTK (.stl/.obj/.ply/.vtp)
    //   --commands <dir>    command-definition directory (default: commands)
    //   --apply <json>      apply a {"command":..,"params":..} envelope directly
    //                       (bypasses the model — handy for testing/scripting)
    //   --no-window         headless: do not open a render window
    //   <rest...>           natural-language instruction for the NLP layer
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
        semanticad::nlp::SemanticIO io("models/Qwen2.5-3B-Instruct-Q4_K_M.gguf", commandsDir);

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
