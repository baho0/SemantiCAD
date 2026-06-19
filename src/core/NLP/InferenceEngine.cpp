#include "InferenceEngine.h"

#include "DebugLog.h"
#include "NLPManager.h"

#include "common.h"
#include "llama.h"
#include "sampling.h"

#include <stdexcept>
#include <vector>

namespace semanticad::nlp {

InferenceEngine::InferenceEngine(NLPManager& manager) : manager_(manager) {}

std::string InferenceEngine::generate(const std::string& prompt,
                                      const std::string& grammar) {
    std::lock_guard<std::mutex> lock(mutex_);

    llama_model* model = manager_.model();
    llama_context* ctx = manager_.context();
    if (model == nullptr || ctx == nullptr)
        throw std::runtime_error("InferenceEngine: model/context not available");

    const llama_vocab* vocab = llama_model_get_vocab(model);

    // Independent instruction -> fresh KV cache.
    llama_memory_clear(llama_get_memory(ctx), /*data=*/true);

    // Deterministic (greedy), grammar-constrained sampler.
    common_params_sampling sp;
    sp.temp = 0.0f;
    if (!grammar.empty())
        sp.grammar = common_grammar(COMMON_GRAMMAR_TYPE_USER, grammar);

    common_sampler* smpl = common_sampler_init(model, sp);
    if (smpl == nullptr)
        throw std::runtime_error("InferenceEngine: failed to init sampler");

    std::vector<llama_token> tokens =
        common_tokenize(ctx, prompt, /*add_special=*/true, /*parse_special=*/true);
    if (tokens.empty()) {
        common_sampler_free(smpl);
        throw std::runtime_error("InferenceEngine: empty prompt tokenization");
    }

    llama_batch batch = llama_batch_get_one(tokens.data(), static_cast<int32_t>(tokens.size()));

    std::string out;
    llama_token id = 0;
    bool decodeFailed = false;
    for (int i = 0; i < maxNewTokens_; ++i) {
        if (llama_decode(ctx, batch) != 0) {
            DEBUG_LOG("[InferenceEngine] llama_decode failed at step %d\n", i);
            decodeFailed = true;
            break;
        }
        id = common_sampler_sample(smpl, ctx, -1);
        common_sampler_accept(smpl, id, /*is_generated=*/true);
        if (llama_vocab_is_eog(vocab, id)) break;
        out += common_token_to_piece(ctx, id, /*special=*/false);
        batch = llama_batch_get_one(&id, 1);
    }

    common_sampler_free(smpl);

    if (decodeFailed && out.empty())
        throw std::runtime_error("InferenceEngine: decode failed");
    return out;
}

}  // namespace semanticad::nlp
