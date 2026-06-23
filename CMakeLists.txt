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
        CommonTransforms
        FiltersSources
        FiltersGeneral
        IOGeometry
        IOPLY
        IOXML
        RenderingCore
        RenderingOpenGL2
        InteractionStyle)
    message(STATUS "--- SemantiCAD: VTK ${VTK_VERSION} bulundu ---")
endif()

# --- nlohmann/json (header-only, vendored by llama.cpp) -------------------
# The command layer and its tests need only the JSON header, so we expose the
# vendor include root directly instead of linking all of llama-common.
set(SEMANTICAD_JSON_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/llama.cpp/vendor)

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

# --- VTK visualization layer + command layer ------------------------------
# Commands bake their transforms directly into the object's vtkPolyData (via the
# CadObject entity), so the command layer depends on VTK. It stays a separate
# library — linked into the app and the tests — so adding a command is: drop
# files in its subfolder and register one line in CommandDispatcher.
if(SEMANTICAD_USE_VTK)
    add_library(semanticad_command STATIC
        src/core/command/CommandDispatcher.cpp
        src/core/command/entity/CadObject.cpp
        src/core/command/resize/ResizeFactors.cpp
        src/core/command/resize/ResizeService.cpp
        src/core/command/resize/ResizeCommand.cpp
        src/core/command/translate/Translation.cpp
        src/core/command/translate/TranslateService.cpp
        src/core/command/translate/TranslateCommand.cpp
        src/core/command/rotate/Rotation.cpp
        src/core/command/rotate/RotateService.cpp
        src/core/command/rotate/RotateCommand.cpp
        src/core/command/mirror/MirrorPlane.cpp
        src/core/command/mirror/MirrorService.cpp
        src/core/command/mirror/MirrorCommand.cpp
    )
    target_include_directories(semanticad_command PUBLIC
        src
        ${SEMANTICAD_JSON_INCLUDE_DIR}
    )
    target_link_libraries(semanticad_command PUBLIC ${VTK_LIBRARIES})

    target_sources(semanticad PRIVATE
        src/infrastructure/MeshLoader.cpp
        src/core/vtk/VtkViewer.cpp
    )
    target_link_libraries(semanticad PRIVATE ${VTK_LIBRARIES} semanticad_command)
    target_compile_definitions(semanticad PRIVATE SEMANTICAD_HAS_VTK)
    # Required so VTK's object factories (OpenGL2 render window, interactor
    # styles, ...) self-register at startup.
    vtk_module_autoinit(TARGETS semanticad MODULES ${VTK_LIBRARIES})

    # --- Unit tests (command layer) ---------------------------------------
    # Commands run against a real (but tiny) vtkPolyData through CadObject — no
    # window, model, or GPU. src/ + json include and VTK come transitively from
    # semanticad_command. Run with `ctest` from the build dir.
    option(SEMANTICAD_BUILD_TESTS "Build the command-layer unit tests" ON)
    if(SEMANTICAD_BUILD_TESTS)
        find_package(GTest REQUIRED)
        enable_testing()
        add_executable(semanticad_tests
            tests/CommandDispatcherTest.cpp
            tests/ResizeCommandTest.cpp
            tests/TranslateCommandTest.cpp
            tests/RotateCommandTest.cpp
            tests/MirrorCommandTest.cpp
        )
        target_include_directories(semanticad_tests PRIVATE tests)
        target_link_libraries(semanticad_tests PRIVATE
            semanticad_command
            GTest::gtest
            GTest::gtest_main
        )
        include(GoogleTest)
        gtest_discover_tests(semanticad_tests)
    endif()
endif()
