#pragma once

#include "DebugLog.h"

#include "chat.h"     // common_chat_templates_ptr, common_chat_*
#include "common.h"   // common_init_result_ptr, common_params

#include <string>

struct llama_model;
struct llama_context;

namespace semanticad::nlp {

// Owns the llama.cpp backend, the loaded Qwen2.5 model + context, and the chat
// templates read from the GGUF. One instance per process (the llama backend is
// global state). The model/context are borrowed (not owned) by InferenceEngine.
class NLPManager {
public:
    explicit NLPManager(std::string modelPath);
    ~NLPManager();

    NLPManager(const NLPManager&) = delete;
    NLPManager& operator=(const NLPManager&) = delete;

    bool isLoaded() const { return loaded_; }

    llama_model* model() const;
    llama_context* context() const;

    // Renders (system, user) into a single prompt string using the model's
    // built-in Qwen2.5 chat template, adding the assistant generation prompt.
    std::string applyChatTemplate(const std::string& system,
                                  const std::string& user) const;

private:
    bool loaded_ = false;
    common_init_result_ptr init_;
    common_chat_templates_ptr templates_;
};

}  // namespace semanticad::nlp
