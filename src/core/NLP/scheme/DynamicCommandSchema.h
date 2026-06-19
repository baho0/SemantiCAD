#pragma once

// Data-driven command schema. Instead of one compiled C++ class per command,
// a command is described declaratively in a JSON definition file and loaded at
// runtime (see CommandCatalog::loadFromDirectory). One DynamicCommandSchema
// instance is built per definition; it produces the prompt docs, the JSON
// schema (which drives the generation grammar) and the decode/validate logic
// generically. Adding a command therefore needs NO source change or recompile.

#include "scheme/CommandSchema.h"

#include <string>
#include <vector>

namespace semanticad::nlp {

// A generic command instance: carries its validated params object.
struct DynamicCommand : Command {
    json params;
    explicit DynamicCommand(std::string n) { name = std::move(n); }
    json toJson() const override { return params; }
};

// Declarative description of one parameter, parsed from a definition file.
struct ParamSpec {
    std::string name;
    std::string type = "number";  // number | integer | string | boolean
    std::string note;
    bool required = false;
    bool hasDefault = false;
    json defaultValue;
    bool hasMin = false;
    double minVal = 0.0;
    bool exclusiveMin = false;
    bool hasMax = false;
    double maxVal = 0.0;
    bool exclusiveMax = false;
    std::vector<std::string> enumValues;  // allowed values for string params
};

class DynamicCommandSchema : public CommandSchema {
public:
    DynamicCommandSchema(std::string name, std::string description,
                         std::vector<std::string> examples,
                         std::vector<ParamSpec> specs);

    std::string name() const override { return name_; }
    std::string description() const override { return description_; }
    std::vector<ParamDoc> params() const override;
    std::vector<std::string> examples() const override { return examples_; }
    json paramsSchema() const override;
    DecodeResult decode(const json& params) const override;

private:
    std::string name_;
    std::string description_;
    std::vector<std::string> examples_;
    std::vector<ParamSpec> specs_;
};

// Parses one command definition (a JSON object) into a schema. On failure
// returns nullptr and sets `error` to a human-readable message, so a malformed
// definition file produces a clear diagnostic instead of a crash.
CommandSchemaPtr parseCommandDefinition(const json& def, std::string& error);

}  // namespace semanticad::nlp
