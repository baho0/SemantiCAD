#pragma once

// The command catalog is the single source of truth that drives, from the same
// registered CommandSchema objects:
//   (a) promptCatalog()       -> the system message describing every command
//   (b) envelopeJsonSchema()  -> the {command, params} schema (oneOf per command)
//   (c) grammar()             -> a GBNF grammar derived from (b) that constrains
//                                model generation to valid, schema-conforming JSON
// Lookup + decode at parse time also go through the same schemas, so the prompt,
// the generation constraint, and the validator can never drift apart.
//
// Commands are loaded at runtime from JSON definition files (loadFromDirectory),
// so adding a command needs no source change or recompile.

#include "scheme/CommandSchema.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace semanticad::nlp {

// Outcome of loading command definitions from disk.
struct CommandLoadReport {
    std::vector<std::string> loaded;  // names of successfully registered commands
    std::vector<std::string> errors;  // "<path>: <message>" for each failure
};

class CommandCatalog {
public:
    void registerSchema(CommandSchemaPtr schema);
    void clear();

    // Loads one command definition file. Returns false and sets `error` on
    // failure (unreadable / invalid JSON / invalid definition).
    bool loadFromFile(const std::string& path, std::string& error);

    // Loads every "*.json" file in `dir` (sorted for deterministic order).
    // Never throws; per-file problems are collected in the report.
    CommandLoadReport loadFromDirectory(const std::string& dir);

    const CommandSchema* lookup(const std::string& name) const;
    std::vector<const CommandSchema*> all() const;  // in registration order
    bool empty() const { return order_.empty(); }

    // Human/LLM-readable command listing for the system prompt.
    std::string promptCatalog() const;

    // Full {command, params} JSON schema: a top-level oneOf with one branch
    // per command (command const + that command's paramsSchema()).
    json envelopeJsonSchema() const;

    // GBNF grammar derived from envelopeJsonSchema() (computed once, cached).
    const std::string& grammar() const;

private:
    std::unordered_map<std::string, CommandSchemaPtr> schemas_;
    std::vector<std::string> order_;    // preserves registration order
    mutable std::string grammarCache_;  // invalidated on registerSchema()/clear()
};

}  // namespace semanticad::nlp
