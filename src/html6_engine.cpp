#include "html6_engine.hpp"

#include "utils.hpp"

#include <algorithm>
#include <cctype>
#include <functional>
#include <iterator>
#include <string>

namespace Minuan {
namespace HTML6 {
namespace {

std::string lowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) {
                       return static_cast<char>(std::tolower(ch));
                   });
    return value;
}

bool isSpace(char ch) {
    return std::isspace(static_cast<unsigned char>(ch)) != 0;
}

bool isVoidElement(const std::string& tag) {
    static const char* names[] = {
        "area", "base", "br", "col", "embed", "hr", "img", "input",
        "link", "meta", "param", "source", "track", "wbr", "mt", "txt"
    };
    return std::any_of(std::begin(names), std::end(names),
                       [&tag](const char* name) { return tag == name; });
}

std::size_t findClosingTag(const std::string& loweredHtml,
                           const std::string& tagName,
                           std::size_t start) {
    const std::string candidate = "</" + tagName;
    std::size_t pos = loweredHtml.find(candidate, start);
    while (pos != std::string::npos) {
        const std::size_t boundary = pos + candidate.size();
        if (boundary == loweredHtml.size() || loweredHtml[boundary] == '>' ||
            loweredHtml[boundary] == '/' || isSpace(loweredHtml[boundary])) {
            return pos;
        }
        pos = loweredHtml.find(candidate, pos + 2);
    }
    return std::string::npos;
}

std::string attributeValue(const Element& element,
                           const std::string& key,
                           const std::string& fallback = {}) {
    const auto it = element.attributes.find(key);
    return it == element.attributes.end() ? fallback : it->second;
}

} // namespace

HTML6Engine::HTML6Engine() = default;
HTML6Engine::~HTML6Engine() = default;

bool HTML6Engine::parseDocument(const std::string& html) {
    ast_ = HTML6AST{};
    servers_.clear();

    if (html.empty() || html.size() > 16 * 1024 * 1024 ||
        html.find('\0') != std::string::npos) {
        Utils::logError("HTML6 input is empty, too large, or contains a null byte");
        return false;
    }

    ast_.body.raw_html = html;
    auto document = std::make_shared<Element>();
    document->tag_name = "#document";

    std::size_t pos = 0;
    while (pos < html.size()) {
        const std::size_t nextTag = html.find('<', pos);
        if (nextTag == std::string::npos) break;
        pos = nextTag;

        if (html.compare(pos, 4, "<!--") == 0) {
            const std::size_t end = html.find("-->", pos + 4);
            pos = end == std::string::npos ? html.size() : end + 3;
            continue;
        }

        if (pos + 1 < html.size() &&
            (html[pos + 1] == '!' || html[pos + 1] == '?')) {
            const std::size_t end = html.find('>', pos + 2);
            pos = end == std::string::npos ? html.size() : end + 1;
            continue;
        }

        if (pos + 1 < html.size() && html[pos + 1] == '/') {
            const std::size_t end = html.find('>', pos + 2);
            pos = end == std::string::npos ? html.size() : end + 1;
            continue;
        }

        const std::size_t before = pos;
        auto element = parseElement(html, pos);
        if (element) {
            document->children.push_back(element);
        } else if (pos <= before) {
            ++pos;
        }
    }

    ast_.document_root_element = document;
    ast_.body.root_element.reset();
    std::function<void(const std::shared_ptr<Element>&)> collect =
        [&](const std::shared_ptr<Element>& element) {
            if (!element) return;
            const std::string tag = lowerCopy(element->tag_name);
            if (tag == "hd" || tag == "head") {
                parseHead(element);
            } else if (tag == "bdy" || tag == "body") {
                parseBody(element);
            }
            for (const auto& child : element->children) collect(child);
        };
    collect(document);

    ast_.valid = true;
    Utils::log("HTML6 document parsed");
    return true;
}

std::shared_ptr<Element> HTML6Engine::parseElement(const std::string& html,
                                                   std::size_t& pos) {
    if (pos >= html.size() || html[pos] != '<') return nullptr;

    ++pos;
    const std::size_t nameStart = pos;
    while (pos < html.size() &&
           !isSpace(html[pos]) && html[pos] != '>' && html[pos] != '/') {
        ++pos;
    }
    if (pos == nameStart) return nullptr;

    auto element = std::make_shared<Element>();
    element->tag_name = lowerCopy(html.substr(nameStart, pos - nameStart));

    bool selfClosing = false;
    while (pos < html.size()) {
        while (pos < html.size() && isSpace(html[pos])) ++pos;
        if (pos >= html.size()) break;
        if (html[pos] == '>') {
            ++pos;
            break;
        }
        if (html[pos] == '/') {
            selfClosing = true;
            ++pos;
            while (pos < html.size() && isSpace(html[pos])) ++pos;
            if (pos < html.size() && html[pos] == '>') ++pos;
            break;
        }

        const std::size_t attrStart = pos;
        while (pos < html.size() && !isSpace(html[pos]) &&
               html[pos] != '=' && html[pos] != '>' && html[pos] != '/') {
            ++pos;
        }
        if (pos == attrStart) {
            ++pos;
            continue;
        }

        const std::string attrName =
            lowerCopy(html.substr(attrStart, pos - attrStart));
        while (pos < html.size() && isSpace(html[pos])) ++pos;

        std::string value;
        if (pos < html.size() && html[pos] == '=') {
            ++pos;
            while (pos < html.size() && isSpace(html[pos])) ++pos;
            if (pos < html.size() &&
                (html[pos] == '"' || html[pos] == '\'')) {
                const char quote = html[pos++];
                const std::size_t valueStart = pos;
                while (pos < html.size() && html[pos] != quote) ++pos;
                value = html.substr(valueStart, pos - valueStart);
                if (pos < html.size()) ++pos;
            } else {
                const std::size_t valueStart = pos;
                while (pos < html.size() && !isSpace(html[pos]) &&
                       html[pos] != '>') ++pos;
                value = html.substr(valueStart, pos - valueStart);
            }
        }
        element->attributes[attrName] = value;
    }

    if (selfClosing || isVoidElement(element->tag_name)) return element;

    if (element->tag_name == "script" || element->tag_name == "scrpt" ||
        element->tag_name == "style" || element->tag_name == "styl") {
        const std::string lowered = lowerCopy(html);
        const std::size_t end =
            findClosingTag(lowered, element->tag_name, pos);
        if (end == std::string::npos) {
            element->content = html.substr(pos);
            pos = html.size();
        } else {
            element->content = html.substr(pos, end - pos);
            const std::size_t closeEnd = html.find('>', end);
            pos = closeEnd == std::string::npos ? html.size() : closeEnd + 1;
        }
        return element;
    }

    while (pos < html.size()) {
        if (html.compare(pos, 4, "<!--") == 0) {
            const std::size_t end = html.find("-->", pos + 4);
            pos = end == std::string::npos ? html.size() : end + 3;
            continue;
        }

        if (html.compare(pos, 2, "</") == 0) {
            const std::size_t end = html.find('>', pos + 2);
            if (end == std::string::npos) {
                pos = html.size();
                break;
            }
            std::size_t nameStart = pos + 2;
            while (nameStart < end && isSpace(html[nameStart])) {
                ++nameStart;
            }
            std::size_t nameEnd = nameStart;
            while (nameEnd < end && !isSpace(html[nameEnd]) &&
                   html[nameEnd] != '/') {
                ++nameEnd;
            }
            const std::string closingName =
                lowerCopy(html.substr(nameStart, nameEnd - nameStart));
            pos = end + 1;
            if (closingName == element->tag_name) break;
            continue;
        }

        if (html[pos] == '<') {
            const std::size_t before = pos;
            auto child = parseElement(html, pos);
            if (child) {
                element->children.push_back(child);
            } else if (pos <= before) {
                ++pos;
            }
        } else {
            const std::size_t end = html.find('<', pos);
            const std::size_t textEnd =
                end == std::string::npos ? html.size() : end;
            element->content.append(html, pos, textEnd - pos);
            pos = textEnd;
        }
    }

    return element;
}

void HTML6Engine::parseHead(const std::shared_ptr<Element>& head) {
    if (!head) return;

    for (const auto& child : head->children) {
        if (!child) continue;
        if (child->tag_name == "mt" || child->tag_name == "meta") {
            MetadataEntry entry;
            entry.name = attributeValue(*child, "nm",
                                        attributeValue(*child, "name"));
            entry.value = attributeValue(*child, "thng",
                                         attributeValue(*child, "content",
                                                        attributeValue(*child, "pblc")));
            if (child->attributes.count("thng") != 0) {
                entry.type = "thng";
            } else if (child->attributes.count("content") != 0) {
                entry.type = "content";
            } else if (child->attributes.count("pblc") != 0) {
                entry.type = "pblc";
            }
            ast_.head.metadata.push_back(entry);
        } else if (child->tag_name == "styl" || child->tag_name == "style") {
            ast_.head.styles.push_back({
                attributeValue(*child, "v", "next"),
                attributeValue(*child, "src")
            });
        } else if (child->tag_name == "scrpt" || child->tag_name == "script") {
            ast_.head.scripts.push_back({
                attributeValue(*child, "v", "2.0"),
                attributeValue(*child, "src")
            });
        } else if (child->tag_name == "srv") {
            ServerDefinition server;
            server.id = attributeValue(*child, "srvd");
            server.cloud = attributeValue(*child, "cld") == "true";
            server.multiplayer = attributeValue(*child, "mltplyr") == "true";
            for (const auto& handler : child->children) {
                if (handler && handler->tag_name == "nrq") {
                    server.handlers.push_back({
                        attributeValue(*handler, "pth"),
                        handler->content
                    });
                }
            }
            registerServer(server);
        }
    }
}

void HTML6Engine::parseBody(const std::shared_ptr<Element>& body) {
    ast_.body.root_element = body;
}

bool HTML6Engine::executeRuntime() {
    Utils::logWarning("The JS-squared runtime is not implemented yet.");
    return false;
}

void HTML6Engine::registerServer(const ServerDefinition& server) {
    // Preserve every declaration in the AST so validation can report invalid
    // definitions instead of silently losing them during parsing.
    ast_.head.servers.push_back(server);
    if (server.isValid()) {
        servers_.push_back(server);
    } else {
        Utils::logError("Rejected a local HTML6 server with multiplayer enabled");
    }
}

ServerDefinition* HTML6Engine::findServer(const std::string& id) {
    const auto it = std::find_if(servers_.begin(), servers_.end(),
        [&id](const ServerDefinition& server) { return server.id == id; });
    return it == servers_.end() ? nullptr : &*it;
}

RequestHandler* HTML6Engine::findHandler(const std::string& server_id,
                                         const std::string& path) {
    ServerDefinition* server = findServer(server_id);
    if (!server) return nullptr;
    const auto it = std::find_if(server->handlers.begin(), server->handlers.end(),
        [&path](const RequestHandler& handler) { return handler.path == path; });
    return it == server->handlers.end() ? nullptr : &*it;
}

std::string HTML6Engine::extractAttribute(const std::string& html,
                                          const std::string& attr_name) {
    const std::string lowered = lowerCopy(html);
    const std::string key = lowerCopy(attr_name);
    std::size_t pos = lowered.find(key);
    if (pos == std::string::npos) return {};

    pos += key.size();
    while (pos < html.size() && isSpace(html[pos])) ++pos;
    if (pos >= html.size() || html[pos] != '=') return {};
    ++pos;
    while (pos < html.size() && isSpace(html[pos])) ++pos;
    if (pos >= html.size()) return {};

    if (html[pos] == '"' || html[pos] == '\'') {
        const char quote = html[pos++];
        const std::size_t end = html.find(quote, pos);
        return end == std::string::npos ? std::string{} :
                                          html.substr(pos, end - pos);
    }
    const std::size_t end = html.find_first_of(" >\t\r\n", pos);
    return html.substr(pos, end == std::string::npos
                                 ? std::string::npos : end - pos);
}

std::string HTML6Engine::extractTagContent(const std::string& html,
                                           const std::string& tag_name) {
    const std::string lowered = lowerCopy(html);
    const std::string tag = "<" + lowerCopy(tag_name);
    const std::size_t open = lowered.find(tag);
    if (open == std::string::npos) return {};
    const std::size_t contentStart = lowered.find('>', open);
    if (contentStart == std::string::npos) return {};
    const std::string closeTag = "</" + lowerCopy(tag_name);
    const std::size_t close = lowered.find(closeTag, contentStart + 1);
    if (close == std::string::npos) return {};
    return html.substr(contentStart + 1, close - contentStart - 1);
}

} // namespace HTML6
} // namespace Minuan
