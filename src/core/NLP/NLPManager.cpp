#include "NLPManager.h"

#include "llama.h"

#include <cstdio>
#include <utility>

namespace semanticad::nlp {

namespace {
// Suppress llama/ggml backend log spam unless debug logging is enabled.
void quietLogCallback(ggml_log_level level, const char* text, void* /*user*/) {
#ifdef ENABLE_DEBUG_LOG
    (void)level;
    std::fputs(text, stderr);
#else
    if (level >= GGML_LOG_LEVEL_ERROR) std::fputs(text, stderr);
#endif
}
}  // namespace

NLPManager::NLPManager(std::string modelPath) {
    llama_log_set(quietLogCallback, nullptr);

    DEBUG_LOG("[NLPManager] Initializing llama backend...\n");
    llama_backend_init();

    common_params params;
    params.model.path = std::move(modelPath);
    params.n_ctx = 4096;          // bounded context: system prompt + one short turn
    params.n_gpu_layers = 99;     // offload all layers if CUDA is available (ignored on CPU build)
    params.sampling.temp = 0.0f;  // deterministic defaults

    DEBUG_LOG("[NLPManager] Loading model: %s\n", params.model.path.c_str());
    init_ = common_init_from_params(params);

    if (!init_ || init_->model() == nullptr || init_->context() == nullptr) {
        DEBUG_LOG("[NLPManager] Model load FAILED.\n");
        loaded_ = false;
        return;
    }

    templates_ = common_chat_templates_init(init_->model(), /*chat_template_override=*/"");
    loaded_ = (templates_ != nullptr);
    DEBUG_LOG("[NLPManager] Model load: %s\n", loaded_ ? "OK" : "chat template init failed");
}

NLPManager::~NLPManager() {
    DEBUG_LOG("[NLPManager] Releasing model and backend...\n");
    templates_.reset();
    init_.reset();  // free context + model before backend shutdown
    llama_backend_free();
}

llama_model* NLPManager::model() const {
    return init_ ? init_->model() : nullptr;
}

llama_context* NLPManager::context() const {
    return init_ ? init_->context() : nullptr;
}

std::string NLPManager::applyChatTemplate(const std::string& system,
                                          const std::string& user) const {
    if (!templates_) return user;

    common_chat_templates_inputs inputs;
    inputs.add_generation_prompt = true;

    common_chat_msg sys;
    sys.role = "system";
    sys.content = system;

    common_chat_msg usr;
    usr.role = "user";
    usr.content = user;

    inputs.messages = {sys, usr};

    common_chat_params out = common_chat_templates_apply(templates_.get(), inputs);
    return out.prompt;
}

}  // namespace semanticad::nlp
