#include "core/command/resize/ResizeCommand.h"

#include "core/command/resize/ResizeFactors.h"
#include "core/command/resize/ResizeService.h"

namespace semanticad::core::command::resize {

CommandResult ResizeCommand::execute(const nlohmann::ordered_json& params,
                                     CadObject& object) const {
    double x = 1.0, y = 1.0, z = 1.0;
    try {
        x = params.value("x", 1.0);
        y = params.value("y", 1.0);
        z = params.value("z", 1.0);
    } catch (const nlohmann::json::exception&) {
        return CommandResult::failure("resize: x, y, z sayisal olmali");
    }

    const auto factors = ResizeFactors::create(x, y, z);
    if (!factors) {
        return CommandResult::failure("resize: olcek carpani > 0 ve sonlu olmali");
    }

    ResizeService::apply(*factors, object);
    return CommandResult::success("resize uygulandi");
}

}  // namespace semanticad::core::command::resize
