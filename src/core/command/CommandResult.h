#pragma once

// Value object describing the outcome of executing a command. Commands never
// throw on bad input — they report failure here, so the caller (the dispatcher
// and, above it, the VTK layer) can decide whether to re-render.

#include <string>
#include <utility>

namespace semanticad::core::command {

struct CommandResult {
    bool ok = false;
    std::string message;

    static CommandResult success(std::string message = {}) {
        return CommandResult{true, std::move(message)};
    }
    static CommandResult failure(std::string message) {
        return CommandResult{false, std::move(message)};
    }
};

}  // namespace semanticad::core::command
