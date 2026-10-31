#include "dom_processor.hpp"
#include "html6_engine.hpp"
#include "html6_validator.hpp"

#include <iostream>
#include <string>

namespace {

bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }
    return true;
}

} // namespace

int main() {
    const std::string source =
        "<!DOCTYPE>\n"
        "<html>"
        "<hd>"
        "<mt nm=\"title\" thng=\"Minuan Test\"/>"
        "<styl v=\"next\" src=\"site.css\"/>"
        "<scrpt v=\"2.0\" src=\"site.js\"/>"
        "<scrpt>if (1 < 2) { console.log(\"raw script\"); }</scrpt>"
        "</hd>"
        "<bdy><dv><bttn>Open</bttn><txt placeholder=\"Name\"/></dv></bdy>"
        "</html>";

    Minuan::HTML6::HTML6Engine engine;
    if (!expect(engine.parseDocument(source), "HTML6 parser accepts the sample")) return 1;

    const auto* ast = engine.getAST();
    if (!expect(ast && ast->valid, "AST is marked valid")) return 1;
    if (!expect(ast->head.metadata.size() == 1, "metadata is extracted")) return 1;
    if (!expect(ast->head.styles.size() == 1, "stylesheet is extracted")) return 1;
    if (!expect(ast->head.scripts.size() == 2, "script is extracted")) return 1;

    Minuan::HTML6::HTML6Validator validator;
    if (!expect(validator.validateDoctype(source), "doctype is recognised")) return 1;

    Minuan::HTML6::DOMProcessor processor;
    const std::string html = processor.generateHTML(ast);
    if (!expect(!html.empty(), "HTML conversion returns a document")) return 1;
    if (!expect(html.find("<head>") != std::string::npos, "hd becomes head")) return 1;
    if (!expect(html.find("<body>") != std::string::npos, "bdy becomes body")) return 1;
    if (!expect(html.find("<meta name=\"title\" content=\"Minuan Test\">") != std::string::npos,
                "mt becomes meta")) return 1;
    if (!expect(html.find("<link rel=\"stylesheet\" href=\"site.css\">") != std::string::npos,
                "external styl becomes a stylesheet link")) return 1;
    if (!expect(html.find("<script") != std::string::npos &&
                html.find("</script>") != std::string::npos,
                "scrpt becomes a valid script element")) return 1;
    if (!expect(html.find("if (1 < 2)") != std::string::npos,
                "inline script source is preserved without tag rewriting")) return 1;
    if (!expect(html.find("<button>Open</button>") != std::string::npos,
                "bttn becomes button")) return 1;
    if (!expect(html.find("<input") != std::string::npos &&
                html.find("</input>") == std::string::npos,
                "txt becomes a void input element")) return 1;
    if (!expect(html.find("<!doctype html>") != std::string::npos,
                "HTML output has a browser-compatible doctype")) return 1;

    std::cout << "Minuan HTML6 pipeline checks passed.\n";
    return 0;
}
