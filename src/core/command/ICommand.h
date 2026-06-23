#pragma once

// The interface every CAD command implements. A command is a small, stateless
// object with an identity (its name() — the dispatcher key) that turns a JSON
// params object into a validated value object and applies it to the CadObject it
// is given. Parsing/validation lives here; the geometry lives in the command's
// Service; the validated data lives in the command's value object; the object
// being mutated is the CadObject entity.

#include "core/command/CommandResult.h"

#include <nlohmann/json.hpp>

#include <string>

namespace semanticad::core::command {

class CadObject;

class ICommand {
public:
    virtual ~ICommand() = default;

    // Canonical command name, e.g. "resize". Stable identity used for dispatch.
    virtual std::string name() const = 0;

    // Parse & validate `params`, then apply the operation to `object`. Must not
    // throw on malformed input — return CommandResult::failure(...) instead.
    virtual CommandResult execute(const nlohmann::ordered_json& params,
                                  CadObject& object) const = 0;
};

}  // namespace semanticad::core::command
