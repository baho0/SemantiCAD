#include <iostream>
#include <string>
#include <vector>

#include "core/NLP/process/SemanticIO.h"

int main(int argc, char** argv) {
    // Optional: --commands <dir> overrides where command definitions are read
    // from. Any remaining arguments (joined) are treated as one instruction.
    std::string commandsDir = "commands";
    std::vector<std::string> rest;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--commands" && i + 1 < argc)
            commandsDir = argv[++i];
        else
            rest.push_back(a);
    }

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
        std::cerr << "[SemantiCAD] Hazir degil: model yuklenemedi veya '" << commandsDir
                  << "/' altinda komut tanimi bulunamadi.\n"
                     "  - Model icin: ./download_model.sh\n"
                     "  - Komutlar icin: '" << commandsDir << "/*.json' dosyalarini kontrol edin."
                  << std::endl;
        return 1;
    }

    std::cout << "[SemantiCAD] Model hazir.\n" << std::endl;

    std::vector<std::string> inputs;
    if (!rest.empty()) {
        std::string joined;
        for (const auto& s : rest) {
            if (!joined.empty()) joined += ' ';
            joined += s;
        }
        inputs.push_back(joined);
    } else {
        inputs = {
            "objeyi yuzde 5 buyult",
            "10 mm saga kaydir",
            "rotate 90 degrees around z",
            "bugun hava nasil",
        };
    }

    for (const auto& in : inputs) {
        io.processInput(in);
        std::cout << "> " << in << "\n" << io.getOutput() << "\n" << std::endl;
    }
    return 0;
}
