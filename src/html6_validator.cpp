#include "html6_validator.hpp"
#include "utils.hpp"

#include <algorithm>
#include <cctype>

using namespace Minuan::HTML6;

HTML6Validator::HTML6Validator() {
    Utils::log("HTML6Validator instance created");
}

bool HTML6Validator::validateDoctype(const std::string& html) {
    std::string lowered = html;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                   [](unsigned char ch) {
                       return static_cast<char>(std::tolower(ch));
                   });

    if (lowered.find("<!doctype") == std::string::npos) {
        addWarning("Missing <!DOCTYPE> declaration");
        return false;
    }
    return true;
}

bool HTML6Validator::validateTagName(const std::string& tag) {
    return !hasVowels(tag);
}

bool HTML6Validator::validateAttributeName(const std::string& attr) {
    return !hasVowels(attr);
}

bool HTML6Validator::validateServerRules(const ServerDefinition& server) {
    if (!server.cloud && server.multiplayer) {
        addError("Server '" + server.id +
                 "' is local (cld=false) but has multiplayer enabled");
        return false;
    }
    return true;
}

bool HTML6Validator::validateDocumentStructure(const HTML6AST* ast) {
    clearErrors();
    if (!ast) {
        addError("AST is null");
        return false;
    }

    bool valid = true;
    for (const auto& server : ast->head.servers) {
        if (!validateServerRules(server)) valid = false;
    }

    for (const auto& meta : ast->head.metadata) {
        if (hasVowels(meta.name)) {
            addWarning("Metadata name '" + meta.name + "' contains vowels");
        }
    }

    for (const auto& style : ast->head.styles) {
        if (style.version.empty()) addWarning("Style missing version attribute");
        if (style.src.empty()) addWarning("Style missing src attribute");
    }

    for (const auto& script : ast->head.scripts) {
        if (script.version.empty()) addWarning("Script missing version attribute");
        if (script.src.empty()) addWarning("Script missing src attribute");
    }

    return valid;
}

bool HTML6Validator::validateElement(const Element& element) {
    if (!validateTagName(element.tag_name)) {
        addError("Tag <" + element.tag_name + "> contains vowels");
        return false;
    }

    for (const auto& attr : element.attributes) {
        if (!validateAttributeName(attr.first)) {
            addError("Attribute '" + attr.first + "' contains vowels");
            return false;
        }
    }
    return true;
}

bool HTML6Validator::hasVowels(const std::string& value) const {
    for (unsigned char ch : value) {
        const char lowerCh = static_cast<char>(std::tolower(ch));
        if (lowerCh == 'a' || lowerCh == 'e' || lowerCh == 'i' ||
            lowerCh == 'o' || lowerCh == 'u') {
            return true;
        }
    }
    return false;
}

bool HTML6Validator::isValidTagName(const std::string& tag) const {
    return !hasVowels(tag);
}

bool HTML6Validator::isValidAttributeName(const std::string& attr) const {
    return !hasVowels(attr);
}

void HTML6Validator::addError(const std::string& message,
                              const std::string& location) {
    errors_.push_back({message, location, "error"});
    Utils::logError("[HTML6] " + message);
}

void HTML6Validator::addWarning(const std::string& message,
                                const std::string& location) {
    errors_.push_back({message, location, "warning"});
    Utils::logWarning("[HTML6] " + message);
}

int HTML6Validator::getErrorCount() const {
    return static_cast<int>(std::count_if(
        errors_.begin(), errors_.end(),
        [](const ValidationError& error) { return error.severity == "error"; }));
}

int HTML6Validator::getWarningCount() const {
    return static_cast<int>(std::count_if(
        errors_.begin(), errors_.end(),
        [](const ValidationError& error) { return error.severity == "warning"; }));
}
