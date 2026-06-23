#include "core/command/CommandDispatcher.h"

#include "core/command/ICommand.h"
#include "core/command/mirror/MirrorCommand.h"
#include "core/command/resize/ResizeCommand.h"
#include "core/command/rotate/RotateCommand.h"
#include "core/command/translate/TranslateCommand.h"

#include <algorithm>

namespace semanticad::core::command {

CommandDispatcher::CommandDispatcher() { registerBuiltins(); }

CommandDispatcher::~CommandDispatcher() = default;

void CommandDispatcher::registerBuiltins() {
    registerCommand(std::make_unique<resize::ResizeCommand>());
    registerCommand(std::make_unique<translate::TranslateCommand>());
    registerCommand(std::make_unique<rotate::RotateCommand>());
    registerCommand(std::make_unique<mirror::MirrorCommand>());
}

void CommandDispatcher::registerCommand(std::unique_ptr<ICommand> command) {
    if (!command) return;
    commands_[command->name()] = std::move(command);
}

bool CommandDispatcher::has(const std::string& name) const {
    return commands_.find(name) != commands_.end();
}

std::vector<std::string> CommandDispatcher::names() const {
    std::vector<std::string> out;
    out.reserve(commands_.size());
    for (const auto& [name, _] : commands_) out.push_back(name);
    std::sort(out.begin(), out.end());
    return out;
}

CommandResult CommandDispatcher::dispatch(const std::string& name,
                                          const nlohmann::ordered_json& params,
                                          CadObject& object) const {
    const auto it = commands_.find(name);
    if (it == commands_.end()) {
        return CommandResult::failure("bilinmeyen komut: '" + name + "'");
    }
    return it->second->execute(params, object);
}

}  // namespace semanticad::core::command
