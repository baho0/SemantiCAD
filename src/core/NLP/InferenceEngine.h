#pragma once

#include <mutex>
#include <string>

namespace semanticad::nlp {

class NLPManager;

// Runs the grammar-constrained decode loop. Borrows the model/context from
// NLPManager. Thread-safe: a single mutex serialises generation because a
// llama_context cannot be decoded concurrently (future UI thread safety).
class InferenceEngine {
public:
    explicit InferenceEngine(NLPManager& manager);

    // Generates raw model text for `prompt`, constrained to `grammar` (GBNF).
    // Returns the decoded string (expected to be a single JSON object).
    // Throws std::runtime_error on backend failure.
    std::string generate(const std::string& prompt, const std::string& grammar);

private:
    NLPManager& manager_;
    std::mutex mutex_;
    int maxNewTokens_ = 256;
};

}  // namespace semanticad::nlp
