#include "process/SemanticIO.h"

#include "process/PromptBuilder.h"

#include <cctype>
#include <utility>

namespace semanticad::nlp {

namespace {
bool isBlank(const std::string& s) {
    for (unsigned char c : s)
        if (!std::isspace(c)) return false;
    return true;
}
}  // namespace

SemanticIO::SemanticIO(std::string modelPath, std::string commandsDir)
    : manager_(std::move(modelPath)),
      catalog_(),
      engine_(manager_),
      parser_(catalog_),
      commandsDir_(std::move(commandsDir)) {
    loadReport_ = catalog_.loadFromDirectory(commandsDir_);
    systemPrompt_ = buildSystemPrompt(catalog_);
}

SemanticIO::~SemanticIO() = default;

const CommandLoadReport& SemanticIO::reloadCommands() {
    catalog_.clear();
    loadReport_ = catalog_.loadFromDirectory(commandsDir_);
    systemPrompt_ = buildSystemPrompt(catalog_);  // grammar is rebuilt lazily by the catalog
    return loadReport_;
}

CommandResult SemanticIO::process(const std::string& input) {
    if (isBlank(input))
        return makeError("empty", "empty input");
    if (!manager_.isLoaded())
        return makeError("engine_error", "model is not loaded");
    if (catalog_.empty())
        return makeError("engine_error", "no commands are loaded");

    const std::string prompt = manager_.applyChatTemplate(systemPrompt_, input);

    std::string raw;
    try {
        raw = engine_.generate(prompt, catalog_.grammar());
    } catch (const std::exception& e) {
        return makeError("engine_error", e.what());
    }

    return parser_.parse(raw);
}

void SemanticIO::processInput(const std::string& input) {
    output_ = process(input).envelope.dump(2);
}

}  // namespace semanticad::nlp
