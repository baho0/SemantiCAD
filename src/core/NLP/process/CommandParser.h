#pragma once

#include "scheme/CommandSchema.h"  // Command, CommandPtr, json

#include <string>

namespace semanticad::nlp {

class CommandCatalog;

// Outcome of running one instruction through the NLP pipeline.
struct CommandResult {
    bool ok = false;
    std::string kind;        // "ok" | "parse_error" | "unknown_command" |
                             // "validation_error" | "engine_error" | "empty"
    std::string message;
    CommandPtr command;      // typed command on success (may be a NoneCommand)
    json envelope;           // canonical {"command",...,"params",...} or error envelope
};

// Builds a structured error result and its error envelope:
//   {"command":"error","params":{"kind":...,"message":...,"raw":...}}
CommandResult makeError(const std::string& kind, const std::string& message,
                        const std::string& raw = "");

// Parses raw model output into a typed command, dispatching through the catalog.
class CommandParser {
public:
    explicit CommandParser(const CommandCatalog& catalog) : catalog_(catalog) {}
    CommandResult parse(const std::string& rawModelOutput) const;

private:
    const CommandCatalog& catalog_;
};

}  // namespace semanticad::nlp
