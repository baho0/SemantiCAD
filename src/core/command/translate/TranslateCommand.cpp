#include "core/command/translate/TranslateCommand.h"

#include "core/command/translate/TranslateService.h"
#include "core/command/translate/Translation.h"

namespace semanticad::core::command::translate {

CommandResult TranslateCommand::execute(const nlohmann::ordered_json& params,
                                        CadObject& object) const {
    double dx = 0.0, dy = 0.0, dz = 0.0;
    try {
        dx = params.value("dx", 0.0);
        dy = params.value("dy", 0.0);
        dz = params.value("dz", 0.0);
    } catch (const nlohmann::json::exception&) {
        return CommandResult::failure("translate: dx, dy, dz sayisal olmali");
    }

    const auto translation = Translation::create(dx, dy, dz);
    if (!translation) {
        return CommandResult::failure("translate: dx, dy, dz sonlu olmali");
    }

    TranslateService::apply(*translation, object);
    return CommandResult::success("translate uygulandi");
}

}  // namespace semanticad::core::command::translate
