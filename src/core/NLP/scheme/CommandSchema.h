#pragma once

// Base types for the SemantiCAD command catalog.
//
// A CommandSchema is the single source of truth for ONE command type: it
// produces the prompt documentation, the JSON-schema fragment used to build
// the generation grammar, and the decode/validate logic that turns a raw
// params object into a typed Command. Adding a new command = one new header
// describing its schema + one registration line (see Builtins.cpp).

#include <nlohmann/json.hpp>

#include <memory>
#include <string>
#include <vector>

namespace semanticad::nlp {

// Order-preserving JSON so prompt/grammar output is stable across runs.
using json = nlohmann::ordered_json;

// A decoded, typed command instance. Concrete commands live under commands/.
struct Command {
    std::string name;
    virtual ~Command() = default;

    // Canonical "params" object for this command (round-trips with decode()).
    virtual json toJson() const = 0;
};
using CommandPtr = std::unique_ptr<Command>;

// A single field-level validation problem.
struct ValidationError {
    std::string field;
    std::string message;
};

// Result of decoding a params object into a typed Command.
struct DecodeResult {
    CommandPtr command;                    // null on failure
    std::vector<ValidationError> errors;
    bool ok() const { return command != nullptr && errors.empty(); }
};

// One parameter description, used only to build the human/LLM system prompt.
struct ParamDoc {
    std::string name;
    std::string type;   // "number", "string", ...
    std::string note;   // unit / meaning / default
};

// Describes ONE command type. All members are pure: identity, prompt docs,
// JSON schema, and decode+validate.
class CommandSchema {
public:
    virtual ~CommandSchema() = default;

    virtual std::string name() const = 0;
    virtual std::string description() const = 0;
    virtual std::vector<ParamDoc> params() const = 0;
    virtual std::vector<std::string> examples() const { return {}; }

    // JSON-schema fragment describing this command's "params" object.
    virtual json paramsSchema() const = 0;

    // Decode a params object into a typed Command (applies defaults + range
    // checks). Collects ALL validation errors rather than stopping at the first.
    virtual DecodeResult decode(const json& params) const = 0;
};
using CommandSchemaPtr = std::unique_ptr<CommandSchema>;

}  // namespace semanticad::nlp
