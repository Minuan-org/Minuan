#ifndef MINUAN_HTML6_ENGINE_HPP
#define MINUAN_HTML6_ENGINE_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace Minuan {
namespace HTML6 {

// ============================================================================
// Data Structures
// ============================================================================

/// HTML6 Metadata Entry
struct MetadataEntry {
    std::string name;          // nm attribute
    std::string value;         // thng, content, or pblc value
    std::string type;          // "title", "url", "public", etc.
};

/// CSS Next Style Definition
struct StyleDefinition {
    std::string version;       // v attribute (e.g. "next")
    std::string src;           // src attribute (file path)
};

/// JS² v2.0 Script Definition
struct ScriptDefinition {
    std::string version;       // v attribute (e.g. "2.0")
    std::string src;           // src attribute (file path)
};

/// Request Handler inside <srv>
struct RequestHandler {
    std::string path;          // pth attribute
    std::string response;      // <nrq> content (JSON-like)
};

/// Server Definition
struct ServerDefinition {
    std::string id;            // srvd attribute
    bool cloud;                // cld attribute
    bool multiplayer;          // mltplyr attribute
    std::vector<RequestHandler> handlers;
    
    /// Validate server rules
    bool isValid() const {
        // Local servers cannot have multiplayer
        if (!cloud && multiplayer) {
            return false;
        }
        return true;
    }
};

/// HTML Element representation
struct Element {
    std::string tag_name;
    std::map<std::string, std::string> attributes;
    std::string content;
    std::vector<std::shared_ptr<Element>> children;
};

/// HTML6 AST (Abstract Syntax Tree)
struct HTML6AST {
    struct Head {
        std::vector<MetadataEntry> metadata;
        std::vector<StyleDefinition> styles;
        std::vector<ScriptDefinition> scripts;
        std::vector<ServerDefinition> servers;
    };
    
    struct Body {
        std::string raw_html;  // Original HTML
        std::shared_ptr<Element> root_element;
    };
    
    Head head;
    Body body;
    bool valid = false;
};

// ============================================================================
// HTML6 Engine
// ============================================================================

/// Main HTML6 Parser and Runtime Engine
class HTML6Engine {
public:
    HTML6Engine();
    ~HTML6Engine();
    
    // Parsing
    bool parseDocument(const std::string& html);
    
    // Runtime execution
    bool executeRuntime();
    
    // Server management
    void registerServer(const ServerDefinition& server);
    const std::vector<ServerDefinition>& getServers() const { return servers_; }
    
    // AST access
    const HTML6AST* getAST() const { return &ast_; }
    
    // Query
    ServerDefinition* findServer(const std::string& id);
    RequestHandler* findHandler(const std::string& server_id, const std::string& path);
    
private:
    HTML6AST ast_;
    std::vector<ServerDefinition> servers_;
    
    // Parsing helpers
    std::shared_ptr<Element> parseElement(const std::string& html, size_t& pos);
    void parseHead(std::shared_ptr<Element> head);
    void parseBody(std::shared_ptr<Element> body);
    std::string extractAttribute(const std::string& html, const std::string& attr_name);
    std::string extractTagContent(const std::string& html, const std::string& tag_name);
};

}  // namespace HTML6
}  // namespace Minuan

#endif  // MINUAN_HTML6_ENGINE_HPP