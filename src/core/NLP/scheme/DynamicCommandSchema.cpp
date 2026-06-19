#include "scheme/DynamicCommandSchema.h"

#include <utility>

namespace semanticad::nlp {

DynamicCommandSchema::DynamicCommandSchema(std::string name, std::string description,
                                           std::vector<std::string> examples,
                                           std::vector<ParamSpec> specs)
    : name_(std::move(name)),
      description_(std::move(description)),
      examples_(std::move(examples)),
      specs_(std::move(specs)) {}

std::vector<ParamDoc> DynamicCommandSchema::params() const {
    std::vector<ParamDoc> out;
    out.reserve(specs_.size());
    for (const auto& s : specs_) out.push_back({s.name, s.type, s.note});
    return out;
}

json DynamicCommandSchema::paramsSchema() const {
    json props = json::object();
    json required = json::array();
    for (const auto& s : specs_) {
        json p = json::object();
        p["type"] = s.type;
        if (!s.enumValues.empty()) {
            json e = json::array();
            for (const auto& v : s.enumValues) e.push_back(v);
            p["enum"] = std::move(e);
        }
        props[s.name] = std::move(p);
        if (s.required) required.push_back(s.name);
    }
    // Numeric ranges are validated in decode() (a GBNF grammar can't enforce
    // arithmetic), so the schema only carries what the grammar can use: types,
    // string enums, required fields and a closed object.
    json schema = json{
        {"type", "object"},
        {"properties", std::move(props)},
        {"additionalProperties", false},
    };
    if (!required.empty()) schema["required"] = std::move(required);
    return schema;
}

namespace {

bool typeMatches(const std::string& type, const json& v) {
    if (type == "number") return v.is_number();
    if (type == "integer") return v.is_number_integer();
    if (type == "string") return v.is_string();
    if (type == "boolean") return v.is_boolean();
    return false;
}

const ParamSpec* findSpec(const std::vector<ParamSpec>& specs, const std::string& key) {
    for (const auto& s : specs)
        if (s.name == key) return &s;
    return nullptr;
}

}  // namespace

DecodeResult DynamicCommandSchema::decode(const json& params) const {
    DecodeResult r;
    auto cmd = std::make_unique<DynamicCommand>(name_);
    json out = json::object();

    // Reject unknown fields (mirrors "additionalProperties": false).
    if (params.is_object()) {
        for (auto it = params.begin(); it != params.end(); ++it)
            if (findSpec(specs_, it.key()) == nullptr)
                r.errors.push_back({it.key(), "unknown field"});
    }

    for (const auto& s : specs_) {
        const bool present = params.is_object() && params.contains(s.name);
        if (!present) {
            if (s.required) {
                r.errors.push_back({s.name, "is required"});
            } else if (s.hasDefault) {
                out[s.name] = s.defaultValue;
            }
            continue;
        }

        const json& v = params.at(s.name);
        if (!typeMatches(s.type, v)) {
            r.errors.push_back({s.name, "must be of type " + s.type});
            continue;
        }

        if (!s.enumValues.empty() && v.is_string()) {
            const std::string sv = v.get<std::string>();
            bool allowed = false;
            for (const auto& e : s.enumValues)
                if (e == sv) { allowed = true; break; }
            if (!allowed) {
                r.errors.push_back({s.name, "must be one of the allowed values"});
                continue;
            }
        }

        if (v.is_number()) {
            const double d = v.get<double>();
            if (s.hasMin && (s.exclusiveMin ? d <= s.minVal : d < s.minVal)) {
                r.errors.push_back({s.name, "value below allowed minimum"});
                continue;
            }
            if (s.hasMax && (s.exclusiveMax ? d >= s.maxVal : d > s.maxVal)) {
                r.errors.push_back({s.name, "value above allowed maximum"});
                continue;
            }
        }

        out[s.name] = v;
    }

    if (r.errors.empty()) {
        cmd->params = std::move(out);
        r.command = std::move(cmd);
    }
    return r;
}

namespace {

// Reads an optional boolean field; returns false (and sets err) on type error.
bool readOptionalBool(const json& obj, const char* key, bool& out, std::string& err,
                      const std::string& ctx) {
    if (!obj.contains(key)) return true;
    if (!obj.at(key).is_boolean()) { err = ctx + "'" + key + "' must be a boolean"; return false; }
    out = obj.at(key).get<bool>();
    return true;
}

bool readOptionalNumber(const json& obj, const char* key, bool& has, double& out,
                        std::string& err, const std::string& ctx) {
    if (!obj.contains(key)) return true;
    if (!obj.at(key).is_number()) { err = ctx + "'" + key + "' must be a number"; return false; }
    has = true;
    out = obj.at(key).get<double>();
    return true;
}

}  // namespace

CommandSchemaPtr parseCommandDefinition(const json& def, std::string& error) {
    auto fail = [&](const std::string& m) -> CommandSchemaPtr {
        error = m;
        return nullptr;
    };

    if (!def.is_object()) return fail("definition must be a JSON object");
    if (!def.contains("name") || !def.at("name").is_string() ||
        def.at("name").get<std::string>().empty())
        return fail("missing or empty string field 'name'");

    const std::string name = def.at("name").get<std::string>();

    std::string description;
    if (def.contains("description")) {
        if (!def.at("description").is_string()) return fail("'description' must be a string");
        description = def.at("description").get<std::string>();
    }

    std::vector<std::string> examples;
    if (def.contains("examples")) {
        if (!def.at("examples").is_array()) return fail("'examples' must be an array of strings");
        for (const auto& e : def.at("examples")) {
            if (!e.is_string()) return fail("each item in 'examples' must be a string");
            examples.push_back(e.get<std::string>());
        }
    }

    std::vector<ParamSpec> specs;
    if (def.contains("params")) {
        if (!def.at("params").is_array()) return fail("'params' must be an array");
        for (const auto& p : def.at("params")) {
            if (!p.is_object()) return fail("each 'params' item must be an object");
            if (!p.contains("name") || !p.at("name").is_string())
                return fail("each param needs a string 'name'");

            ParamSpec s;
            s.name = p.at("name").get<std::string>();
            const std::string ctx = "param '" + s.name + "': ";

            if (p.contains("type")) {
                if (!p.at("type").is_string()) return fail(ctx + "'type' must be a string");
                s.type = p.at("type").get<std::string>();
                if (s.type != "number" && s.type != "integer" && s.type != "string" &&
                    s.type != "boolean")
                    return fail(ctx + "type must be number|integer|string|boolean");
            }
            if (p.contains("note")) {
                if (!p.at("note").is_string()) return fail(ctx + "'note' must be a string");
                s.note = p.at("note").get<std::string>();
            }
            if (!readOptionalBool(p, "required", s.required, error, ctx)) return nullptr;
            if (p.contains("default")) {
                s.hasDefault = true;
                s.defaultValue = p.at("default");
            }
            if (!readOptionalNumber(p, "min", s.hasMin, s.minVal, error, ctx)) return nullptr;
            if (!readOptionalNumber(p, "max", s.hasMax, s.maxVal, error, ctx)) return nullptr;
            if (!readOptionalBool(p, "exclusiveMin", s.exclusiveMin, error, ctx)) return nullptr;
            if (!readOptionalBool(p, "exclusiveMax", s.exclusiveMax, error, ctx)) return nullptr;
            if (p.contains("enum")) {
                if (!p.at("enum").is_array()) return fail(ctx + "'enum' must be an array");
                for (const auto& e : p.at("enum")) {
                    if (!e.is_string()) return fail(ctx + "'enum' values must be strings");
                    s.enumValues.push_back(e.get<std::string>());
                }
            }
            specs.push_back(std::move(s));
        }
    }

    return std::make_unique<DynamicCommandSchema>(name, std::move(description),
                                                  std::move(examples), std::move(specs));
}

}  // namespace semanticad::nlp
