cmake_minimum_required(VERSION 3.14)
project(SemantiCAD C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(SEMANTICAD_USE_CUDA "Enable CUDA support via ggml" ON)
option(SEMANTICAD_USE_VTK  "Enable the VTK visualization layer" ON)

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

if(SEMANTICAD_USE_VTK)
    find_package(VTK 9 REQUIRED COMPONENTS
        CommonCore
        CommonDataModel
        FiltersSources
        IOGeometry
        IOPLY
        IOXML
        RenderingCore
        RenderingOpenGL2
        InteractionStyle)
    message(STATUS "--- SemantiCAD: VTK ${VTK_VERSION} bulundu ---")
endif()

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

# `src`           -> "core/...", "infrastructure/..." includes (eventbus, vtk, DebugLog)
# `src/core/NLP`  -> intra-NLP includes ("scheme/...", "process/...", etc.)
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

# --- VTK visualization layer (optional) -----------------------------------
if(SEMANTICAD_USE_VTK)
    target_sources(semanticad PRIVATE
        src/infrastructure/MeshLoader.cpp
        src/core/vtk/VtkViewer.cpp
    )
    target_link_libraries(semanticad PRIVATE ${VTK_LIBRARIES})
    target_compile_definitions(semanticad PRIVATE SEMANTICAD_HAS_VTK)
    # Required so VTK's object factories (OpenGL2 render window, interactor
    # styles, ...) self-register at startup.
    vtk_module_autoinit(TARGETS semanticad MODULES ${VTK_LIBRARIES})
endif()
