#include "core/command/mirror/MirrorCommand.h"

#include "core/command/mirror/MirrorPlane.h"
#include "core/command/mirror/MirrorService.h"

namespace semanticad::core::command::mirror {

CommandResult MirrorCommand::execute(const nlohmann::ordered_json& params,
                                     CadObject& object) const {
    std::string planeStr = "xy";
    try {
        planeStr = params.value("plane", std::string("xy"));
    } catch (const nlohmann::json::exception&) {
        return CommandResult::failure("mirror: plane metin olmali");
    }

    const auto mirror = MirrorPlane::create(planeStr);
    if (!mirror) {
        return CommandResult::failure("mirror: plane 'xy', 'yz' veya 'xz' olmali");
    }

    MirrorService::apply(*mirror, object);
    return CommandResult::success("mirror uygulandi");
}

}  // namespace semanticad::core::command::mirror
