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
        "a", "abbr", "address", "area", "article", "aside", "audio", "b",
        "base", "bdi", "bdo", "blockquote", "body", "br", "button", "canvas",
        "caption", "cite", "code", "col", "colgroup", "data", "datalist", "dd",
        "del", "details", "dfn", "dialog", "div", "dl", "dt", "em", "embed",
        "fieldset", "figcaption", "figure", "footer", "form", "h1", "h2", "h3",
        "h4", "h5", "h6", "head", "header", "hgroup", "hr", "html", "i",
        "iframe", "img", "input", "ins", "kbd", "label", "legend", "li",
        "link", "main", "map", "mark", "menu", "meta", "meter", "nav",
        "noscript", "object", "ol", "optgroup", "option", "output", "p", "param",
        "picture", "pre", "progress", "q", "rp", "rt", "ruby", "s", "samp",
        "script", "search", "section", "select", "slot", "small", "source",
        "span", "strong", "style", "sub", "summary", "sup", "svg", "path",
        "table", "tbody", "td", "template", "textarea", "tfoot", "th",
        "thead", "time", "title", "tr", "track", "u", "ul", "var", "video",
        "wbr"
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
        "value", "placeholder", "disabled", "required", "checked", "selected",
        "readonly", "for", "action", "method", "target", "rel", "style", "role",
        "width", "height", "tabindex", "autocomplete", "download", "accept",
        "multiple", "min", "max", "step", "rows", "cols", "content", "charset",
        "lang", "dir", "hidden", "draggable", "spellcheck", "colspan", "rowspan",
        "scope", "open", "controls", "autoplay", "muted", "loop", "poster",
        "preload", "crossorigin", "defer", "async", "media", "datetime",
        "loading", "decoding", "srcset", "sizes", "data", "autofocus", "form",
        "formaction", "formenctype", "formmethod", "formnovalidate", "formtarget",
        "pattern", "size", "maxlength", "minlength", "inputmode", "enterkeyhint",
        "contenteditable", "accesskey", "translate", "inert", "slot", "popover",
        "popovertarget", "popovertargetaction", "cite", "start", "reversed",
        "list", "kind", "srclang", "label", "playsinline", "usemap", "ismap",
        "shape", "coords", "hreflang", "ping", "integrity", "nonce", "nomodule",
        "referrerpolicy", "fetchpriority", "http-equiv", "xmlns", "viewbox",
        "d", "fill", "stroke", "stroke-width", "stroke-linecap", "stroke-linejoin",
        "cx", "cy", "r", "x", "y", "x1", "y1", "x2", "y2", "transform",
        "preserveaspectratio"
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
    bool valid = true;
    if (server.id.empty()) {
        addError("HTML6 server is missing its srvd identifier");
        valid = false;
    }
    if (!server.cloud && server.multiplayer) {
        addError("Server '" + server.id +
                 "' is local (cld=false) but has multiplayer enabled");
        valid = false;
    }
    for (const auto& handler : server.handlers) {
        if (handler.path.empty()) {
            addError("Server '" + server.id +
                     "' contains an nrq handler without a pth path");
            valid = false;
        }
    }
    return valid;
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

    const auto document = ast->document_root_element
                              ? ast->document_root_element
                              : ast->body.root_element;
    bool usesHtml6 = false;
    std::function<void(const std::shared_ptr<Element>&)> detectHtml6 =
        [&](const std::shared_ptr<Element>& element) {
            if (!element) return;
            const std::string& tag = element->tag_name;
            if (tag == "hd" || tag == "bdy" || tag == "mt" ||
                tag == "styl" || tag == "scrpt" || tag == "srv" ||
                tag == "nrq") {
                usesHtml6 = true;
            }
            for (const auto& child : element->children) detectHtml6(child);
        };
    detectHtml6(document);

    // HTML6 documents use one root element with the system section before the UI.
    if (usesHtml6) {
        std::vector<std::shared_ptr<Element>> roots;
        if (document) {
            for (const auto& child : document->children) {
                if (child && (child->tag_name == "html" ||
                              child->tag_name == "html6")) {
                    roots.push_back(child);
                }
            }
        }

        if (roots.size() != 1) {
            addError("HTML6 documents must have exactly one top-level <html> root");
            valid = false;
        } else {
            const auto& root = roots.front();
            std::size_t headCount = 0;
            std::size_t bodyCount = 0;
            std::size_t headPosition = root->children.size();
            std::size_t bodyPosition = root->children.size();

            for (std::size_t i = 0; i < root->children.size(); ++i) {
                const auto& child = root->children[i];
                if (!child) continue;
                if (child->tag_name == "hd" || child->tag_name == "head") {
                    ++headCount;
                    headPosition = i;
                } else if (child->tag_name == "bdy" ||
                           child->tag_name == "body") {
                    ++bodyCount;
                    bodyPosition = i;
                }
            }

            if (headCount != 1) {
                addError("HTML6 documents must have exactly one direct <hd> section");
                valid = false;
            }
            if (bodyCount != 1) {
                addError("HTML6 documents must have exactly one direct <bdy> section");
                valid = false;
            }
            if (headCount == 1 && bodyCount == 1 &&
                headPosition >= bodyPosition) {
                addError("The HTML6 <hd> section must appear before <bdy>");
                valid = false;
            }
        }
    }

    std::unordered_set<std::string> serverIds;
    std::function<void(const std::shared_ptr<Element>&, const std::string&, bool)> validateTree =
        [&](const std::shared_ptr<Element>& element,
            const std::string& parentTag, bool insideBody) {
            if (!element) return;

            const std::string& tag = element->tag_name;
            if (tag != "#document" && !validateElement(*element)) {
                valid = false;
            }

            const bool bodyContext = insideBody || tag == "bdy" || tag == "body";
            if (insideBody &&
                (tag == "srv" || tag == "styl" || tag == "scrpt" ||
                 tag == "mt" || tag == "nrq")) {
                addError("HTML6 system tag <" + tag +
                         "> must not appear inside <bdy>");
                valid = false;
            }

            if ((tag == "hd" || tag == "bdy") &&
                parentTag != "html" && parentTag != "html6") {
                addError("HTML6 <" + tag +
                         "> must be a direct child of the document root");
                valid = false;
            }

            if (tag == "srv") {
                if (parentTag != "hd" && parentTag != "head") {
                    addError("HTML6 <srv> definitions must be direct children of <hd>");
                    valid = false;
                }

                const auto id = element->attributes.find("srvd");
                if (id == element->attributes.end() || id->second.empty()) {
                    addError("HTML6 <srv> is missing its srvd identifier");
                    valid = false;
                } else if (!serverIds.insert(id->second).second) {
                    addError("HTML6 server identifier '" + id->second +
                             "' is declared more than once");
                    valid = false;
                }

                const auto cloud = element->attributes.find("cld");
                if (cloud == element->attributes.end() ||
                    (cloud->second != "true" && cloud->second != "false")) {
                    addError("HTML6 <srv> must set cld to either true or false");
                    valid = false;
                }

                const auto multiplayer = element->attributes.find("mltplyr");
                if (cloud != element->attributes.end() &&
                    cloud->second == "false" &&
                    multiplayer != element->attributes.end()) {
                    addError("A local HTML6 server (cld=false) must not have mltplyr");
                    valid = false;
                } else if (multiplayer != element->attributes.end() &&
                           multiplayer->second != "true" &&
                           multiplayer->second != "false") {
                    addError("HTML6 srv mltplyr must be either true or false");
                    valid = false;
                } else if (cloud != element->attributes.end() &&
                           cloud->second == "true" &&
                           multiplayer != element->attributes.end() &&
                           multiplayer->second == "true") {
                    addWarning("Server '" +
                               (id != element->attributes.end() ? id->second : "(unnamed)") +
                               "' requests Chrono multiplayer; Minuan currently stores "
                               "the definition but does not run Chrono");
                }
            }

            if (tag == "nrq") {
                if (parentTag != "srv") {
                    addError("HTML6 <nrq> handlers must be direct children of <srv>");
                    valid = false;
                }
                const auto path = element->attributes.find("pth");
                if (path == element->attributes.end() || path->second.empty()) {
                    addError("HTML6 <nrq> is missing its pth request path");
                    valid = false;
                }
            }

            for (const auto& child : element->children) {
                validateTree(child, tag, bodyContext);
            }
        };
    validateTree(document, "", false);

    // Keep metadata names as values: names such as "title" and "public or not"
    // are explicitly valid in the HTML6 specification.
    for (const auto& style : ast->head.styles) {
        if (style.version.empty()) addWarning("Style missing version attribute");
        if (style.src.empty()) addWarning("Style missing src attribute");
    }

    for (const auto& script : ast->head.scripts) {
        if (script.version.empty()) addWarning("Script missing version attribute");
        if (script.src.empty()) addWarning("Script missing src attribute");
    }

    for (const auto& server : ast->head.servers) {
        if (!validateServerRules(server)) valid = false;
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
