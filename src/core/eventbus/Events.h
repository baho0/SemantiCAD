#pragma once

// Event types carried over the EventBus. These are the neutral contract between
// layers: the NLP layer produces a CommandEvent, the VTK layer consumes it, and
// neither includes the other's headers. The params object is the same canonical
// JSON the NLP pipeline validates, so there is a single shared representation.

#include <nlohmann/json.hpp>

#include <string>

namespace semanticad::core {

// A CAD command that has been understood and should be applied to the scene.
struct CommandEvent {
    std::string command;            // "resize" | "translate" | "rotate" | "mirror" | ...
    nlohmann::ordered_json params;  // command-specific parameters
};

}  // namespace semanticad::core
