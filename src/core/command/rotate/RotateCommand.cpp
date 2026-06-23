#include "core/command/rotate/RotateCommand.h"

#include "core/command/rotate/RotateService.h"
#include "core/command/rotate/Rotation.h"

namespace semanticad::core::command::rotate {

CommandResult RotateCommand::execute(const nlohmann::ordered_json& params,
                                     CadObject& object) const {
    std::string axisStr = "z";
    double angle = 0.0;
    try {
        axisStr = params.value("axis", std::string("z"));
        angle = params.value("angle_deg", 0.0);
    } catch (const nlohmann::json::exception&) {
        return CommandResult::failure("rotate: axis metin, angle_deg sayisal olmali");
    }

    const auto axis = Rotation::parseAxis(axisStr);
    if (!axis) {
        return CommandResult::failure("rotate: axis 'x', 'y' veya 'z' olmali");
    }

    const auto rotation = Rotation::create(*axis, angle);
    if (!rotation) {
        return CommandResult::failure("rotate: angle_deg sonlu olmali");
    }

    RotateService::apply(*rotation, object);
    return CommandResult::success("rotate uygulandi");
}

}  // namespace semanticad::core::command::rotate
