#pragma once

// ICommand adapter for "rotate": reads the axis (default "z") and angle_deg
// (default 0), builds a validated Rotation value object, and applies it.

#include "core/command/ICommand.h"

namespace semanticad::core::command::rotate {

class RotateCommand : public ICommand {
public:
    std::string name() const override { return "rotate"; }

    CommandResult execute(const nlohmann::ordered_json& params,
                          CadObject& object) const override;
};

}  // namespace semanticad::core::command::rotate
