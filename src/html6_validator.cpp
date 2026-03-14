#include "html6_validator.hpp"
#include "utils.hpp"
#include <cctype>
#include <algorithm>

using namespace Minuan::HTML6;

HTML6Validator::HTML6Validator() {
    Utils::log("HTML6Validator instance created");
}

bool HTML6Validator::validateDoctype(const std::string& html) {
    bool has_doctype = html.find("<!DOCTYPE>") != std::string::npos;
    
    if (!has_doctype) {
        addWarning("Missing <!DOCTYPE> declaration");
        return false;
    }
    
    return true;
}

bool HTML6Validator::validateTagName(const std::string& tag) {
    // Tag names should NOT contain vowels
    return !hasVowels(tag);
}

bool HTML6Validator::validateAttributeName(const std::string& attr) {
    // Attribute names should NOT contain vowels
    // Exception: attribute names in body HTML5 elements are allowed
    return !hasVowels(attr);
}

bool HTML6Validator::validateServerRules(const ServerDefinition& server) {
    // Server validation rule: local servers cannot have multiplayer
    if (!server.cloud && server.multiplayer) {
        std::string msg = "Server '" + server.id + "' is local (cld=false) but has multiplayer enabled";
        addError(msg);
        return false;
    }
    
    return true;
}

bool HTML6Validator::validateDocumentStructure(const HTML6AST* ast) {
    if (!ast) {
        addError("AST is null");
        return false;
    }
    
    clearErrors();
    bool valid = true;
    
    // Validate all servers
    for (const auto& server : ast->head.servers) {
        if (!validateServerRules(server)) {
            valid = false;
        }
    }
    
    // Validate all metadata
    for (const auto& meta : ast->head.metadata) {
        if (hasVowels(meta.name)) {
            addWarning("Metadata name '" + meta.name + "' contains vowels");
        }
    }
    
    // Validate all styles
    for (const auto& style : ast->head.styles) {
        if (style.version.empty()) {
            addWarning("Style missing version attribute");
        }
        if (style.src.empty()) {
            addWarning("Style missing src attribute");
        }
    }
    
    // Validate all scripts
    for (const auto& script : ast->head.scripts) {
        if (script.version.empty()) {
            addWarning("Script missing version attribute");
        }
        if (script.src.empty()) {
            addWarning("Script missing src attribute");
        }
    }
    
    return valid;
}

bool HTML6Validator::validateElement(const Element& element) {
    if (!validateTagName(element.tag_name)) {
        addError("Tag <" + element.tag_name + "> contains vowels");
        return false;
    }
    
    for (const auto& [attr_name, attr_value] : element.attributes) {
        if (!validateAttributeName(attr_name)) {
            addError("Attribute '" + attr_name + "' contains vowels");
            return false;
        }
    }
    
    return true;
}

bool HTML6Validator::hasVowels(const std::string& str) const {
    for (char c : str) {
        char lower_c = std::tolower(c);
        if (lower_c == 'a' || lower_c == 'e' || lower_c == 'i' || 
            lower_c == 'o' || lower_c == 'u') {
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

void HTML6Validator::addError(const std::string& message, const std::string& location) {
    ValidationError error;
    error.message = message;
    error.location = location;
    error.severity = "error";
    errors_.push_back(error);
    Utils::logError("[HTML6] " + message);
}

void HTML6Validator::addWarning(const std::string& message, const std::string& location) {
    ValidationError error;
    error.message = message;
    error.location = location;
    error.severity = "warning";
    errors_.push_back(error);
    Utils::logWarning("[HTML6] " + message);
}

int HTML6Validator::getErrorCount() const {
    int count = 0;
    for (const auto& error : errors_) {
        if (error.severity == "error") count++;
    }
    return count;
}

int HTML6Validator::getWarningCount() const {
    int count = 0;
    for (const auto& error : errors_) {
        if (error.severity == "warning") count++;
    }
    return count;
}