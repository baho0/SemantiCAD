#include "process/PromptBuilder.h"

#include "scheme/CommandCatalog.h"
#include "scheme/CommandSchema.h"

#include <sstream>

namespace semanticad::nlp {

std::string buildSystemPrompt(const CommandCatalog& catalog) {
    std::ostringstream os;
    os << "You are the command parser for SemantiCAD, a CAD tool.\n"
       << "Translate the user's single instruction (Turkish or English) into "
          "EXACTLY ONE JSON command.\n"
       << "Output ONLY one JSON object: no prose, no explanation, no markdown, "
          "no code fences.\n"
       << "The object must have this exact shape: "
          "{\"command\": <name>, \"params\": { ... }}.\n"
       << "Pick the command whose meaning best matches the instruction. If "
          "nothing matches or the request is unclear, use the \"none\" command.\n\n"
       << "Available commands:\n"
       << catalog.promptCatalog()
       << "\nExamples (input -> output):\n";

    for (const auto* s : catalog.all())
        for (const auto& ex : s->examples())
            os << "  " << ex << "\n";

    return os.str();
}

}  // namespace semanticad::nlp
