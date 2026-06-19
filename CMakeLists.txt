cmake_minimum_required(VERSION 3.14)
project(SemantiCAD C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(SEMANTICAD_USE_CUDA "Enable CUDA support via ggml" ON)

if(SEMANTICAD_USE_CUDA)
    message(STATUS "--- SemantiCAD: CUDA donanim hizlandirmasi aktif ediliyor ---")
    set(GGML_CUDA ON CACHE BOOL "Enable CUDA in ggml" FORCE)
endif()

# llama.cpp is included as a submodule. When added via add_subdirectory it is
# NOT standalone, so the common utils library (llama-common) is off by default.
# We rely on it for nlohmann/json, json-schema-to-grammar, chat templates and
# the common sampler, so force it on.
set(LLAMA_BUILD_COMMON ON CACHE BOOL "Build llama.cpp common utils" FORCE)

add_subdirectory(external/llama.cpp)

add_executable(semanticad
    src/main.cpp
    src/core/NLP/NLPManager.cpp
    src/core/NLP/InferenceEngine.cpp
    src/core/NLP/process/SemanticIO.cpp
    src/core/NLP/process/PromptBuilder.cpp
    src/core/NLP/process/CommandParser.cpp
    src/core/NLP/scheme/CommandCatalog.cpp
    src/core/NLP/scheme/DynamicCommandSchema.cpp
)

# `src`           -> DebugLog.h and "core/NLP/..." includes from main.cpp
# `src/core/NLP`  -> intra-module includes ("scheme/...", "process/...", etc.)
target_include_directories(semanticad PRIVATE
    src
    src/core/NLP
)

# llama-common (PUBLIC) transitively provides the common/ and vendor/ include
# dirs (nlohmann/json, json-schema-to-grammar.h, common.h, sampling.h, chat.h).
target_link_libraries(semanticad PRIVATE llama ggml llama-common)

target_compile_definitions(semanticad PRIVATE
    $<$<CONFIG:Debug>:ENABLE_DEBUG_LOG>
)
