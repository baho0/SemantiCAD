#pragma once

// ICommand adapter for "translate": reads dx/dy/dz (mm, each defaulting to 0),
// builds a validated Translation value object, and applies it to the CadObject.

#include "core/command/ICommand.h"

namespace semanticad::core::command::translate {

class TranslateCommand : public ICommand {
public:
    std::string name() const override { return "translate"; }

    CommandResult execute(const nlohmann::ordered_json& params,
                          CadObject& object) const override;
};

}  // namespace semanticad::core::command::translate
