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
    if (!expect(validator.validateDocumentStructure(ast),
                "standard HTML tags and attributes pass document validation")) return 1;
    if (!expect(validator.getErrorCount() == 0,
                "valid HTML6 document has no validation errors")) return 1;

    Minuan::HTML6::Element invalidCustomElement;
    invalidCustomElement.tag_name = "container";
    validator.clearErrors();
    if (!expect(!validator.validateElement(invalidCustomElement),
                "custom HTML6 tags with vowels are rejected")) return 1;

    Minuan::HTML6::DOMProcessor processor;
    const std::string html = processor.generateHTML(ast);
    if (!expect(!html.empty(), "HTML conversion returns a document")) return 1;
    if (!expect(html.find("<head>") != std::string::npos, "hd becomes head")) return 1;
    if (!expect(html.find("<body>") != std::string::npos, "bdy becomes body")) return 1;
    if (!expect(html.find("<title>Minuan Test</title>") != std::string::npos,
                "HTML6 title metadata becomes the browser tab title")) return 1;
    if (!expect(html.find("<link rel=\"stylesheet\" href=\"site.css\"") != std::string::npos,
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

    const std::string rawBoundarySource =
        "<html><hd>"
        "<scrpt>const marker = '</scrptExtra>'; console.log('script still open');</scrpt>"
        "<styl>.sample::after { content: '</stylExtra>'; }</styl>"
        "</hd><bdy><dv>After raw tags</dv></bdy></html>";
    Minuan::HTML6::HTML6Engine rawBoundaryEngine;
    if (!expect(rawBoundaryEngine.parseDocument(rawBoundarySource),
                "raw script and style content with similar closing names parses")) return 1;
    const std::string rawBoundaryHtml =
        processor.generateHTML(rawBoundaryEngine.getAST());
    if (!expect(rawBoundaryHtml.find("</scrptExtra>") != std::string::npos,
                "script text is not cut off by a longer tag name")) return 1;
    if (!expect(rawBoundaryHtml.find("</stylExtra>") != std::string::npos,
                "style text is not cut off by a longer tag name")) return 1;
    if (!expect(rawBoundaryHtml.find("script still open") != std::string::npos &&
                rawBoundaryHtml.find("After raw tags") != std::string::npos,
                "content after raw tags remains in the document")) return 1;

    const std::string ordinaryHtml =
        "<!DOCTYPE html><html><hd><title>Project</title></hd>"
        "<bdy><main><figure class=\"card\" loading=\"lazy\" "
        "aria-label=\"Example card\" data-kind=\"demo\">"
        "<figcaption>Example</figcaption></figure>"
        "<object data=\"player.dat\"><param name=\"autoplay\" value=\"0\"></object>"
        "</main></bdy></html>";
    Minuan::HTML6::HTML6Engine ordinaryEngine;
    if (!expect(ordinaryEngine.parseDocument(ordinaryHtml),
                "ordinary HTML with common semantic elements parses")) return 1;
    Minuan::HTML6::HTML6Validator ordinaryValidator;
    if (!expect(ordinaryValidator.validateDocumentStructure(ordinaryEngine.getAST()),
                "common HTML tags and modern attributes pass validation")) return 1;
    if (!expect(ordinaryValidator.getErrorCount() == 0,
                "ordinary HTML produces no validation errors")) return 1;

    const std::string ssoSource =
        "<!DOCTYPE><html><hd></hd><bdy>"
        "<bttn s_ss=\"true\" ss=\"Google\">Sign in with </bttn>"
        "</bdy></html>";
    Minuan::HTML6::HTML6Engine ssoEngine;
    if (!expect(ssoEngine.parseDocument(ssoSource),
                "HTML6 SSO button sample parses")) return 1;
    Minuan::HTML6::DOMProcessor ssoProcessor;
    const std::string ssoHtml = ssoProcessor.generateHTML(ssoEngine.getAST());
    if (!expect(ssoHtml.find("Sign in with Google") != std::string::npos,
                "s_ss and ss combine the provider name with the button label")) return 1;

    const std::string serverSource =
        "<!DOCTYPE><html><hd>"
        "<srv srvd=\"cloud\" cld=\"true\" mltplyr=\"true\">"
        "<nrq pth=\"/api/status\">{\"ok\":true}</nrq>"
        "</srv></hd><bdy><p>Server example</p></bdy></html>";
    Minuan::HTML6::HTML6Engine serverEngine;
    if (!expect(serverEngine.parseDocument(serverSource),
                "HTML6 server declaration parses")) return 1;
    if (!expect(serverEngine.getAST()->head.servers.size() == 1,
                "server declarations are retained in the AST")) return 1;
    if (!expect(serverEngine.findHandler("cloud", "/api/status") != nullptr,
                "server request handlers are available to the native engine")) return 1;
    Minuan::HTML6::HTML6Validator serverValidator;
    if (!expect(serverValidator.validateDocumentStructure(serverEngine.getAST()),
                "valid cloud server declaration passes structural validation")) return 1;
    if (!expect(serverValidator.getErrorCount() == 0,
                "valid server declaration has no validation errors")) return 1;
    if (!expect(serverValidator.getWarningCount() > 0,
                "Chrono multiplayer support is clearly reported as unavailable")) return 1;

    const std::string invalidLocalServer =
        "<!DOCTYPE><html><hd>"
        "<srv srvd=\"local\" cld=\"false\" mltplyr=\"false\"></srv>"
        "</hd><bdy></bdy></html>";
    Minuan::HTML6::HTML6Engine invalidServerEngine;
    if (!expect(invalidServerEngine.parseDocument(invalidLocalServer),
                "invalid local server sample still parses for diagnostics")) return 1;
    Minuan::HTML6::HTML6Validator invalidServerValidator;
    if (!expect(!invalidServerValidator.validateDocumentStructure(
                    invalidServerEngine.getAST()),
                "local servers cannot declare mltplyr, even when false")) return 1;

    const std::string misplacedServer =
        "<!DOCTYPE><html><hd></hd><bdy>"
        "<srv srvd=\"misplaced\" cld=\"false\"></srv>"
        "</bdy></html>";
    Minuan::HTML6::HTML6Engine misplacedEngine;
    if (!expect(misplacedEngine.parseDocument(misplacedServer),
                "misplaced server sample parses for diagnostics")) return 1;
    Minuan::HTML6::HTML6Validator misplacedValidator;
    if (!expect(!misplacedValidator.validateDocumentStructure(misplacedEngine.getAST()),
                "server definitions inside the visible body are rejected")) return 1;

    const std::string misplacedScript =
        "<!DOCTYPE><html><hd></hd><bdy></bdy>"
        "<scrpt src=\"outside.js\"></scrpt></html>";
    Minuan::HTML6::HTML6Engine misplacedScriptEngine;
    if (!expect(misplacedScriptEngine.parseDocument(misplacedScript),
                "misplaced script sample parses for diagnostics")) return 1;
    Minuan::HTML6::HTML6Validator misplacedScriptValidator;
    if (!expect(!misplacedScriptValidator.validateDocumentStructure(
                    misplacedScriptEngine.getAST()),
                "HTML6 scripts must be direct children of the system section")) return 1;

    const std::string wrongOrder =
        "<!DOCTYPE><html><bdy></bdy><hd></hd></html>";
    Minuan::HTML6::HTML6Engine wrongOrderEngine;
    if (!expect(wrongOrderEngine.parseDocument(wrongOrder),
                "reordered HTML6 sections parse for diagnostics")) return 1;
    Minuan::HTML6::HTML6Validator wrongOrderValidator;
    if (!expect(!wrongOrderValidator.validateDocumentStructure(
                    wrongOrderEngine.getAST()),
                "the system section must precede the visible body")) return 1;

    std::cout << "Minuan HTML6 pipeline checks passed.\n";
    return 0;
}
