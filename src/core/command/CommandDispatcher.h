#pragma once

// The command core component. Owns the registry of ICommand objects keyed by
// name and routes an incoming (name, params) pair to the matching command,
// applying it to the given CadObject. This replaces the if/else chain that used
// to live in the VTK layer: the viewer now just hands every CommandEvent here
// together with the object to mutate.
//
// Adding a new command is: create its subfolder (entity / value object / service
// / ICommand impl) and register one line in registerBuiltins().

#include "core/command/CommandResult.h"

#include <nlohmann/json.hpp>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace semanticad::core::command {

class ICommand;
class CadObject;

class CommandDispatcher {
public:
    // Builds a dispatcher pre-populated with all built-in commands.
    CommandDispatcher();
    ~CommandDispatcher();

    CommandDispatcher(const CommandDispatcher&) = delete;
    CommandDispatcher& operator=(const CommandDispatcher&) = delete;

    // Add (or replace) a command. The dispatcher takes ownership.
    void registerCommand(std::unique_ptr<ICommand> command);

    bool has(const std::string& name) const;
    std::vector<std::string> names() const;  // sorted, for diagnostics/logging

    // Look up `name` and execute it against `object`. Unknown names (including
    // "none" / "error") yield a failure result; no exception is thrown.
    CommandResult dispatch(const std::string& name,
                           const nlohmann::ordered_json& params,
                           CadObject& object) const;

private:
    void registerBuiltins();

    std::unordered_map<std::string, std::unique_ptr<ICommand>> commands_;
};

}  // namespace semanticad::core::command
