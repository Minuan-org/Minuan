#include "html6_validator.hpp"
#include "utils.hpp"

#include <algorithm>
#include <cctype>
#include <functional>
#include <unordered_set>

using namespace Minuan::HTML6;

namespace {

bool isStandardHtmlTag(const std::string& tag) {
    static const std::unordered_set<std::string> names{
        "html", "head", "body", "title", "base", "link", "meta", "style",
        "script", "noscript", "main", "header", "footer", "nav", "section",
        "article", "aside", "div", "span", "p", "a", "button", "input",
        "textarea", "select", "option", "label", "form", "fieldset",
        "legend", "ul", "ol", "li", "dl", "dt", "dd", "table", "caption",
        "thead", "tbody", "tfoot", "tr", "th", "td", "col", "colgroup",
        "h1", "h2", "h3", "h4", "h5", "h6", "img", "picture", "source",
        "audio", "video", "track", "canvas", "svg", "path", "br", "hr",
        "pre", "code", "blockquote", "strong", "em", "small", "sub",
        "sup", "details", "summary", "dialog", "progress", "meter",
        "time", "mark", "iframe", "embed", "object", "param", "wbr"
    };
    return names.find(tag) != names.end();
}

bool isStandardHtmlAttribute(const std::string& attribute) {
    if (attribute.rfind("data-", 0) == 0 ||
        attribute.rfind("aria-", 0) == 0) {
        return true;
    }

    static const std::unordered_set<std::string> names{
        "id", "class", "title", "href", "src", "alt", "type", "name",
        "value", "placeholder", "disabled", "required", "checked",
        "selected", "readonly", "readOnly", "for", "action", "method",
        "target", "rel", "style", "role", "width", "height", "tabindex",
        "autocomplete", "download", "accept", "multiple", "min", "max",
        "step", "rows", "cols", "content", "charset", "lang", "dir",
        "hidden", "draggable", "spellcheck", "colspan", "rowspan",
        "scope", "open", "controls", "autoplay", "muted", "loop",
        "poster", "preload", "crossorigin", "defer", "async", "media",
        "datetime", "loading", "decoding", "srcset", "sizes"
    };
    return names.find(attribute) != names.end();
}

} // namespace

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
    // A missing doctype is a warning, so it does not make the document unusable.
    validateDoctype(ast->body.raw_html);

    for (const auto& server : ast->head.servers) {
        if (!validateServerRules(server)) valid = false;
    }

    std::function<void(const std::shared_ptr<Element>&)> validateTree =
        [&](const std::shared_ptr<Element>& element) {
            if (!element) return;
            if (element->tag_name != "#document" &&
                !validateElement(*element)) {
                valid = false;
            }
            for (const auto& child : element->children) {
                validateTree(child);
            }
        };
    validateTree(ast->body.root_element);

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
    const std::string tag = element.tag_name;
    if (tag != "#document" && !isStandardHtmlTag(tag) &&
        !validateTagName(tag)) {
        addError("Custom HTML6 tag <" + tag +
                 "> must use the vowel-free HTML6 naming style");
        return false;
    }

    for (const auto& attr : element.attributes) {
        if (!isStandardHtmlAttribute(attr.first) &&
            !validateAttributeName(attr.first)) {
            addError("Custom HTML6 attribute '" + attr.first +
                     "' must use the vowel-free HTML6 naming style");
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
