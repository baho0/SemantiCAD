#include "process/CommandParser.h"

#include "scheme/CommandCatalog.h"

namespace semanticad::nlp {

CommandResult makeError(const std::string& kind, const std::string& message,
                        const std::string& raw) {
    CommandResult r;
    r.ok = false;
    r.kind = kind;
    r.message = message;

    json params;
    params["kind"] = kind;
    params["message"] = message;
    if (!raw.empty()) params["raw"] = raw;

    r.envelope = json{{"command", "error"}, {"params", std::move(params)}};
    return r;
}

CommandResult CommandParser::parse(const std::string& raw) const {
    // No-throw parse: a grammar-constrained model should never produce invalid
    // JSON, but we guard anyway.
    json j = json::parse(raw, /*cb=*/nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded() || !j.is_object())
        return makeError("parse_error", "model output is not a valid JSON object", raw);

    if (!j.contains("command") || !j.at("command").is_string())
        return makeError("parse_error", "missing or non-string 'command' field", raw);

    const std::string name = j.at("command").get<std::string>();
    const CommandSchema* schema = catalog_.lookup(name);
    if (schema == nullptr)
        return makeError("unknown_command", "unknown command: " + name, raw);

    const json params = j.contains("params") ? j.at("params") : json::object();
    DecodeResult dr = schema->decode(params);
    if (!dr.ok()) {
        std::string msg = "invalid params for '" + name + "':";
        for (const auto& e : dr.errors)
            msg += " [" + e.field + ": " + e.message + "]";
        return makeError("validation_error", msg, raw);
    }

    CommandResult r;
    r.ok = true;
    r.kind = "ok";
    r.envelope = json{{"command", dr.command->name}, {"params", dr.command->toJson()}};
    r.command = std::move(dr.command);
    return r;
}

}  // namespace semanticad::nlp
