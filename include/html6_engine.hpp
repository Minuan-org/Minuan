#ifndef MINUAN_HTML6_ENGINE_HPP
#define MINUAN_HTML6_ENGINE_HPP

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Minuan {
namespace HTML6 {

struct MetadataEntry {
    std::string name;
    std::string value;
    std::string type;
};

struct StyleDefinition {
    std::string version;
    std::string src;
};

struct ScriptDefinition {
    std::string version;
    std::string src;
};

struct RequestHandler {
    std::string path;
    std::string response;
};

struct ServerDefinition {
    std::string id;
    bool cloud = false;
    bool multiplayer = false;
    std::vector<RequestHandler> handlers;

    bool isValid() const {
        return cloud || !multiplayer;
    }
};

struct Element {
    std::string tag_name;
    std::map<std::string, std::string> attributes;
    std::string content;
    std::vector<std::shared_ptr<Element>> children;
};

struct HTML6AST {
    struct Head {
        std::vector<MetadataEntry> metadata;
        std::vector<StyleDefinition> styles;
        std::vector<ScriptDefinition> scripts;
        std::vector<ServerDefinition> servers;
    };

    struct Body {
        std::string raw_html;
        std::shared_ptr<Element> root_element;
    };

    Head head;
    Body body;
    bool valid = false;
};

class HTML6Engine {
public:
    HTML6Engine();
    ~HTML6Engine();

    bool parseDocument(const std::string& html);
    bool executeRuntime();

    void registerServer(const ServerDefinition& server);
    const std::vector<ServerDefinition>& getServers() const { return servers_; }

    const HTML6AST* getAST() const { return &ast_; }

    ServerDefinition* findServer(const std::string& id);
    RequestHandler* findHandler(const std::string& server_id,
                                const std::string& path);

private:
    HTML6AST ast_;
    std::vector<ServerDefinition> servers_;

    std::shared_ptr<Element> parseElement(const std::string& html,
                                          std::size_t& pos);
    void parseHead(const std::shared_ptr<Element>& head);
    void parseBody(const std::shared_ptr<Element>& body);
    std::string extractAttribute(const std::string& html,
                                 const std::string& attr_name);
    std::string extractTagContent(const std::string& html,
                                  const std::string& tag_name);
};

} // namespace HTML6
} // namespace Minuan

#endif
