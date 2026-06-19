#pragma once

#include <string>

namespace semanticad::nlp {

class CommandCatalog;

// Builds the system prompt that instructs the model to translate one Turkish or
// English instruction into exactly one JSON command drawn from the catalog.
// The command list and few-shot examples are sourced from the catalog, so the
// prompt can never drift from the schemas used for validation.
std::string buildSystemPrompt(const CommandCatalog& catalog);

}  // namespace semanticad::nlp
