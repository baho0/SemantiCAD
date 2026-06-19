#pragma once

#include "InferenceEngine.h"
#include "NLPManager.h"
#include "process/CommandParser.h"
#include "scheme/CommandCatalog.h"

#include <string>

namespace semanticad::nlp {

// Facade for the NLP layer: natural-language text in -> validated, typed command
// (and its canonical JSON envelope) out. Owns the model, the command catalog,
// the inference engine and the parser, and orchestrates the full pipeline:
// ingest -> prompt -> grammar-constrained generation -> parse -> decode/validate.
//
// Commands are loaded from `commandsDir` (JSON definition files) at construction
// and can be re-read at runtime with reloadCommands() — no recompile needed.
class SemanticIO {
public:
    explicit SemanticIO(
        std::string modelPath = "models/Qwen2.5-1.5B-Instruct-Q4_K_M.gguf",
        std::string commandsDir = "commands");
    ~SemanticIO();

    SemanticIO(const SemanticIO&) = delete;
    SemanticIO& operator=(const SemanticIO&) = delete;

    // True once the model + chat templates loaded AND at least one command exists.
    bool ready() const { return manager_.isLoaded() && !catalog_.empty(); }

    // Re-reads the command directory and rebuilds the prompt + grammar.
    const CommandLoadReport& reloadCommands();
    const CommandLoadReport& commandLoadReport() const { return loadReport_; }

    // Full pipeline; returns the typed result (success or structured error).
    CommandResult process(const std::string& input);

    // Convenience: runs process() and stores the canonical JSON envelope.
    void processInput(const std::string& input);
    std::string getOutput() const { return output_; }

private:
    NLPManager manager_;
    CommandCatalog catalog_;
    InferenceEngine engine_;
    CommandParser parser_;
    std::string commandsDir_;
    CommandLoadReport loadReport_;
    std::string systemPrompt_;
    std::string output_;
};

}  // namespace semanticad::nlp
