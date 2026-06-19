#include "scheme/CommandCatalog.h"

#include "scheme/DynamicCommandSchema.h"

#include "json-schema-to-grammar.h"  // from llama-common

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace semanticad::nlp {

void CommandCatalog::registerSchema(CommandSchemaPtr schema) {
    if (!schema) return;
    const std::string n = schema->name();
    if (schemas_.find(n) == schemas_.end()) order_.push_back(n);
    schemas_[n] = std::move(schema);
    grammarCache_.clear();  // invalidate derived grammar
}

void CommandCatalog::clear() {
    schemas_.clear();
    order_.clear();
    grammarCache_.clear();
}

bool CommandCatalog::loadFromFile(const std::string& path, std::string& error) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        error = "cannot open file";
        return false;
    }
    std::ostringstream ss;
    ss << in.rdbuf();

    json def = json::parse(ss.str(), /*cb=*/nullptr, /*allow_exceptions=*/false);
    if (def.is_discarded()) {
        error = "invalid JSON";
        return false;
    }

    CommandSchemaPtr schema = parseCommandDefinition(def, error);
    if (!schema) return false;

    registerSchema(std::move(schema));
    return true;
}

CommandLoadReport CommandCatalog::loadFromDirectory(const std::string& dir) {
    namespace fs = std::filesystem;
    CommandLoadReport report;

    std::error_code ec;
    if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) {
        report.errors.push_back(dir + ": directory not found");
        return report;
    }

    std::vector<std::string> files;
    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json")
            files.push_back(entry.path().string());
    }
    std::sort(files.begin(), files.end());  // deterministic registration order

    for (const auto& f : files) {
        std::ifstream in(f, std::ios::binary);
        if (!in) {
            report.errors.push_back(f + ": cannot open file");
            continue;
        }
        std::ostringstream ss;
        ss << in.rdbuf();

        json def = json::parse(ss.str(), /*cb=*/nullptr, /*allow_exceptions=*/false);
        if (def.is_discarded()) {
            report.errors.push_back(f + ": invalid JSON");
            continue;
        }

        std::string err;
        CommandSchemaPtr schema = parseCommandDefinition(def, err);
        if (!schema) {
            report.errors.push_back(f + ": " + err);
            continue;
        }

        const std::string nm = schema->name();
        registerSchema(std::move(schema));
        report.loaded.push_back(nm);  // record the command name, not the path
    }
    return report;
}

const CommandSchema* CommandCatalog::lookup(const std::string& name) const {
    auto it = schemas_.find(name);
    return it == schemas_.end() ? nullptr : it->second.get();
}

std::vector<const CommandSchema*> CommandCatalog::all() const {
    std::vector<const CommandSchema*> out;
    out.reserve(order_.size());
    for (const auto& n : order_) {
        auto it = schemas_.find(n);
        if (it != schemas_.end()) out.push_back(it->second.get());
    }
    return out;
}

std::string CommandCatalog::promptCatalog() const {
    std::ostringstream os;
    for (const auto* s : all()) {
        os << "- " << s->name() << ": " << s->description() << "\n";
        for (const auto& pd : s->params())
            os << "    * " << pd.name << " (" << pd.type << "): " << pd.note << "\n";
    }
    return os.str();
}

json CommandCatalog::envelopeJsonSchema() const {
    json branches = json::array();
    for (const auto* s : all()) {
        branches.push_back(json{
            {"type", "object"},
            {"properties", json{
                {"command", json{{"const", s->name()}}},
                {"params", s->paramsSchema()},
            }},
            {"required", json::array({"command", "params"})},
            {"additionalProperties", false},
        });
    }
    return json{{"oneOf", std::move(branches)}};
}

const std::string& CommandCatalog::grammar() const {
    if (grammarCache_.empty())
        grammarCache_ = json_schema_to_grammar(envelopeJsonSchema());
    return grammarCache_;
}

}  // namespace semanticad::nlp
