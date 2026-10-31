#include "dom_processor.hpp"
#include "utils.hpp"

#include <algorithm>
#include <cctype>
#include <map>
#include <sstream>
#include <string>

namespace Minuan {
namespace HTML6 {
namespace {

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) {
                       return static_cast<char>(std::tolower(ch));
                   });
    return value;
}

bool space(char ch) {
    return std::isspace(static_cast<unsigned char>(ch)) != 0;
}

std::size_t findClosingTag(const std::string& loweredInput,
                           const std::string& tagName,
                           std::size_t start) {
    const std::string candidate = "</" + tagName;
    std::size_t pos = loweredInput.find(candidate, start);
    while (pos != std::string::npos) {
        const std::size_t boundary = pos + candidate.size();
        if (boundary == loweredInput.size() || loweredInput[boundary] == '>' ||
            loweredInput[boundary] == '/' || space(loweredInput[boundary])) {
            return pos;
        }
        pos = loweredInput.find(candidate, pos + 2);
    }
    return std::string::npos;
}

std::map<std::string, std::string> readAttributes(const std::string& text) {
    std::map<std::string, std::string> result;
    std::size_t pos = 0;
    while (pos < text.size()) {
        while (pos < text.size() && (space(text[pos]) || text[pos] == '/')) ++pos;
        const std::size_t start = pos;
        while (pos < text.size() && !space(text[pos]) &&
               text[pos] != '=' && text[pos] != '/') ++pos;
        if (start == pos) {
            if (pos < text.size()) ++pos;
            continue;
        }

        const std::string name = lower(text.substr(start, pos - start));
        while (pos < text.size() && space(text[pos])) ++pos;
        std::string value;
        if (pos < text.size() && text[pos] == '=') {
            ++pos;
            while (pos < text.size() && space(text[pos])) ++pos;
            if (pos < text.size() && (text[pos] == '"' || text[pos] == '\'')) {
                const char quote = text[pos++];
                const std::size_t valueStart = pos;
                while (pos < text.size() && text[pos] != quote) ++pos;
                value = text.substr(valueStart, pos - valueStart);
                if (pos < text.size()) ++pos;
            } else {
                const std::size_t valueStart = pos;
                while (pos < text.size() && !space(text[pos]) && text[pos] != '>') ++pos;
                value = text.substr(valueStart, pos - valueStart);
            }
        }
        result[name] = value;
    }
    return result;
}

std::string escapeAttribute(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (char ch : value) {
        switch (ch) {
            case '&': escaped += "&amp;"; break;
            case '"': escaped += "&quot;"; break;
            case '<': escaped += "&lt;"; break;
            default: escaped += ch; break;
        }
    }
    return escaped;
}

std::string escapeText(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (char ch : value) {
        switch (ch) {
            case '&': escaped += "&amp;"; break;
            case '<': escaped += "&lt;"; break;
            case '>': escaped += "&gt;"; break;
            default: escaped += ch; break;
        }
    }
    return escaped;
}

std::string attributesToString(const std::map<std::string, std::string>& attrs) {
    std::ostringstream out;
    for (const auto& item : attrs) {
        out << ' ' << item.first << "=\"" << escapeAttribute(item.second) << "\"";
    }
    return out.str();
}

std::string mappedTag(const std::string& name) {
    static const std::map<std::string, std::string> aliases = {
        {"hd", "head"}, {"bdy", "body"}, {"bttn", "button"},
        {"dv", "div"}, {"nv", "nav"}, {"frm", "form"},
        {"lbl", "label"}, {"txt", "input"}, {"scrpt", "script"},
        {"styl", "style"}
    };
    const auto it = aliases.find(name);
    return it == aliases.end() ? name : it->second;
}

std::string convertMarkup(const std::string& input) {
    std::string output;
    output.reserve(input.size() + 64);
    std::size_t pos = 0;

    while (pos < input.size()) {
        const std::size_t tagStart = input.find('<', pos);
        if (tagStart == std::string::npos) {
            output.append(input, pos, std::string::npos);
            break;
        }
        output.append(input, pos, tagStart - pos);

        if (input.compare(tagStart, 4, "<!--") == 0) {
            const std::size_t commentEnd = input.find("-->", tagStart + 4);
            const std::size_t end = commentEnd == std::string::npos
                                        ? input.size() : commentEnd + 3;
            output.append(input, tagStart, end - tagStart);
            pos = end;
            continue;
        }

        std::size_t tagEnd = tagStart + 1;
        char quote = 0;
        for (; tagEnd < input.size(); ++tagEnd) {
            const char ch = input[tagEnd];
            if (quote != 0) {
                if (ch == quote) quote = 0;
            } else if (ch == '"' || ch == '\'') {
                quote = ch;
            } else if (ch == '>') {
                break;
            }
        }
        if (tagEnd >= input.size()) {
            output.append(input, tagStart, std::string::npos);
            break;
        }

        const std::string token = input.substr(tagStart + 1,
                                               tagEnd - tagStart - 1);
        if (token.empty() || token[0] == '!' || token[0] == '?') {
            const std::string declaration = input.substr(
                tagStart, tagEnd - tagStart + 1);
            if (lower(declaration) == "<!doctype>") {
                output += "<!doctype html>";
            } else {
                output += declaration;
            }
            pos = tagEnd + 1;
            continue;
        }

        std::size_t cursor = 0;
        while (cursor < token.size() && space(token[cursor])) ++cursor;
        const bool closing = cursor < token.size() && token[cursor] == '/';
        if (closing) ++cursor;
        const std::size_t nameStart = cursor;
        while (cursor < token.size() && !space(token[cursor]) &&
               token[cursor] != '/') ++cursor;
        const std::string originalName = lower(token.substr(nameStart, cursor - nameStart));
        const std::string attrText = token.substr(cursor);
        const auto attrs = readAttributes(attrText);
        std::size_t tokenEnd = token.size();
        while (tokenEnd > 0 && space(token[tokenEnd - 1])) --tokenEnd;
        const bool selfClosing = tokenEnd > 0 && token[tokenEnd - 1] == '/';

        if (originalName == "txt") {
            if (!closing) {
                auto inputAttrs = attrs;
                if (inputAttrs.count("type") == 0) inputAttrs["type"] = "text";
                output += "<input" + attributesToString(inputAttrs) + ">";
            }
            pos = tagEnd + 1;
            continue;
        }

        if (originalName == "mt") {
            if (!closing) {
                const std::string name = attrs.count("nm") ? attrs.at("nm")
                    : attrs.count("name") ? attrs.at("name") : "";
                const std::string value = attrs.count("thng") ? attrs.at("thng")
                    : attrs.count("content") ? attrs.at("content")
                    : attrs.count("pblc") ? attrs.at("pblc") : "";
                const std::string normalizedName = lower(name);
                if ((normalizedName == "title" || normalizedName == "<title>") &&
                    !value.empty()) {
                    // HTML6 uses mt title metadata for the browser tab title.
                    output += "<title>" + escapeText(value) + "</title>";
                } else {
                    output += "<meta";
                    if (!name.empty()) output += " name=\"" + escapeAttribute(name) + "\"";
                    if (!value.empty()) output += " content=\"" + escapeAttribute(value) + "\"";
                    output += ">";
                }
            }
            pos = tagEnd + 1;
            continue;
        }

        if (originalName == "styl" && attrs.count("src") != 0) {
            if (!closing) {
                output += "<link rel=\"stylesheet\" href=\"" +
                          escapeAttribute(attrs.at("src")) + "\"";
                if (attrs.count("v")) {
                    output += " data-css-version=\"" +
                              escapeAttribute(attrs.at("v")) + "\"";
                }
                output += ">";
            }
            pos = tagEnd + 1;
            if (!closing && !selfClosing) {
                const std::string lowered = lower(input);
                const std::size_t close = findClosingTag(lowered, "styl", pos);
                if (close != std::string::npos) {
                    const std::size_t closeEnd = input.find('>', close);
                    pos = closeEnd == std::string::npos ? input.size() : closeEnd + 1;
                }
            }
            continue;
        }

        if (!closing && !selfClosing && originalName == "bttn" &&
            attrs.count("s_ss") != 0 && attrs.at("s_ss") == "true" &&
            attrs.count("ss") != 0 && !attrs.at("ss").empty()) {
            const std::string loweredInput = lower(input);
            const std::size_t closeStart =
                findClosingTag(loweredInput, "bttn", tagEnd + 1);
            output += "<button" + attributesToString(attrs) + ">";
            if (closeStart != std::string::npos) {
                const std::string label = input.substr(
                    tagEnd + 1, closeStart - tagEnd - 1);
                output += label;
                if (!label.empty() &&
                    !std::isspace(static_cast<unsigned char>(label.back()))) {
                    output += " ";
                }
                output += escapeText(attrs.at("ss"));
                output += "</button>";
                const std::size_t closeEnd = input.find('>', closeStart);
                pos = closeEnd == std::string::npos ? input.size() : closeEnd + 1;
            } else {
                output.append(input, tagEnd + 1, std::string::npos);
                output += " " + escapeText(attrs.at("ss")) + "</button>";
                pos = input.size();
            }
            continue;
        }

        if (closing) {
            output += "</" + mappedTag(originalName) + ">";
        } else {
            output += "<" + mappedTag(originalName);
            auto mappedAttrs = attrs;
            if (originalName == "txt" && mappedAttrs.count("type") == 0) {
                mappedAttrs["type"] = "text";
            }
            if (originalName == "scrpt" && mappedAttrs.count("v")) {
                mappedAttrs["data-js2-version"] = mappedAttrs["v"];
                mappedAttrs.erase("v");
            }
            if (originalName == "styl" && mappedAttrs.count("v")) {
                mappedAttrs["data-css-version"] = mappedAttrs["v"];
                mappedAttrs.erase("v");
            }
            output += attributesToString(mappedAttrs);
            if (originalName == "scrpt" && selfClosing) {
                output += "></script>";
            } else {
                output += selfClosing ? " />" : ">";
            }
        }

        if (!closing && !selfClosing &&
            (originalName == "scrpt" || originalName == "script" ||
             originalName == "styl" || originalName == "style")) {
            const std::string loweredInput = lower(input);
            const std::size_t closeStart =
                findClosingTag(loweredInput, originalName, tagEnd + 1);
            if (closeStart != std::string::npos) {
                output.append(input, tagEnd + 1, closeStart - tagEnd - 1);
                output += "</" + mappedTag(originalName) + ">";
                const std::size_t closeEnd = input.find('>', closeStart);
                pos = closeEnd == std::string::npos ? input.size() : closeEnd + 1;
            } else {
                // Raw script/style text must never be re-parsed as HTML.
                output.append(input, tagEnd + 1, std::string::npos);
                output += "</" + mappedTag(originalName) + ">";
                pos = input.size();
            }
            continue;
        }
        pos = tagEnd + 1;
    }

    const std::string lowered = lower(output);
    const bool hasDoctype = lowered.find("<!doctype") != std::string::npos;
    const bool hasHtml = lowered.find("<html") != std::string::npos;
    if (!hasHtml) {
        output = std::string(hasDoctype ? "" : "<!doctype html>\n") +
                 "<html>\n" + output + "\n</html>\n";
    } else if (!hasDoctype) {
        output.insert(0, "<!doctype html>\n");
    }
    return output;
}

} // namespace

DOMProcessor::DOMProcessor() {
    Utils::log("DOMProcessor instance created");
}

void DOMProcessor::processHead(std::shared_ptr<Element> head,
                               HTML6Engine* engine) {
    if (!head || !engine) return;

    for (const auto& child : head->children) {
        if (!child) continue;
        if (child->tag_name == "mt") {
            processMetadata(child);
        } else if (child->tag_name == "styl") {
            processStyle(child);
        } else if (child->tag_name == "scrpt") {
            processScript(child);
        } else if (child->tag_name == "srv") {
            processServer(child);
            ServerDefinition server;
            const auto id = child->attributes.find("srvd");
            if (id != child->attributes.end()) server.id = id->second;
            const auto cloud = child->attributes.find("cld");
            server.cloud = cloud != child->attributes.end() && cloud->second == "true";
            const auto multiplayer = child->attributes.find("mltplyr");
            server.multiplayer = multiplayer != child->attributes.end() &&
                                 multiplayer->second == "true";
            for (const auto& handler : child->children) {
                if (!handler || handler->tag_name != "nrq") continue;
                const auto path = handler->attributes.find("pth");
                server.handlers.push_back({
                    path == handler->attributes.end() ? "" : path->second,
                    handler->content
                });
            }
            engine->registerServer(server);
        }
    }
}

void DOMProcessor::processMetadata(std::shared_ptr<Element> metadata) {
    if (metadata) Utils::log("HTML6 metadata: " + metadata->attributes["nm"]);
}

void DOMProcessor::processStyle(std::shared_ptr<Element> style) {
    if (style) Utils::log("HTML6 stylesheet: " + style->attributes["src"]);
}

void DOMProcessor::processScript(std::shared_ptr<Element> script) {
    if (script) Utils::log("HTML6 script: " + script->attributes["src"]);
}

void DOMProcessor::processServer(std::shared_ptr<Element> server) {
    if (server) Utils::log("HTML6 server: " + server->attributes["srvd"]);
}

void DOMProcessor::processBody(std::shared_ptr<Element> body) {
    if (body) transformCustomTags(body);
}

void DOMProcessor::transformCustomTags(std::shared_ptr<Element> element) {
    if (!element) return;
    const std::string originalName = lower(element->tag_name);
    element->tag_name = mappedTag(originalName);
    if (originalName == "txt" && element->attributes.count("type") == 0) {
        element->attributes["type"] = "text";
    }
    for (const auto& child : element->children) {
        transformCustomTags(child);
    }
}

void DOMProcessor::processChildElements(std::shared_ptr<Element> parent) {
    if (!parent) return;
    for (const auto& child : parent->children) {
        transformCustomTags(child);
        processChildElements(child);
    }
}

std::string DOMProcessor::generateHTML(const HTML6AST* ast) {
    if (!ast || !ast->valid || ast->body.raw_html.empty()) return {};
    return convertMarkup(ast->body.raw_html);
}

} // namespace HTML6
} // namespace Minuan
