#pragma once

// ICommand adapter for "mirror": reads the plane (default "xy"), builds a
// validated MirrorPlane value object, and applies it to the CadObject.

#include "core/command/ICommand.h"

namespace semanticad::core::command::mirror {

class MirrorCommand : public ICommand {
public:
    std::string name() const override { return "mirror"; }

    CommandResult execute(const nlohmann::ordered_json& params,
                          CadObject& object) const override;
};

}  // namespace semanticad::core::command::mirror
