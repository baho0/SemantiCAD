#pragma once

// ICommand adapter for "resize": reads x/y/z multipliers from the JSON params
// (each defaulting to 1.0 = unchanged), builds a validated ResizeFactors value
// object, and applies it to the CadObject through ResizeService.

#include "core/command/ICommand.h"

namespace semanticad::core::command::resize {

class ResizeCommand : public ICommand {
public:
    std::string name() const override { return "resize"; }

    CommandResult execute(const nlohmann::ordered_json& params,
                          CadObject& object) const override;
};

}  // namespace semanticad::core::command::resize
