#include "dom_processor.hpp"
#include "utils.hpp"

using namespace Minuan::HTML6;

DOMProcessor::DOMProcessor() {
    Utils::log("DOMProcessor instance created");
}

void DOMProcessor::processHead(std::shared_ptr<Element> hd, HTML6Engine* engine) {
    if (!hd || hd->tag_name != "hd") {
        Utils::logError("Invalid head element");
        return;
    }
    
    Utils::log("Processing <hd> section...");
    
    // Process metadata tags
    for (auto& child : hd->children) {
        if (child->tag_name == "mt") {
            processMetadata(child);
        } else if (child->tag_name == "styl") {
            processStyle(child);
        } else if (child->tag_name == "scrpt") {
            processScript(child);
        } else if (child->tag_name == "srv") {
            processServer(child);
        }
    }
    
    Utils::log("✓ Head processing complete");
}

void DOMProcessor::processBody(std::shared_ptr<Element> bdy) {
    if (!bdy || bdy->tag_name != "bdy") {
        Utils::logError("Invalid body element");
        return;
    }
    
    Utils::log("Processing <bdy> section...");
    
    // Transform custom tags
    transformCustomTags(bdy);
    
    // Process all child elements recursively
    processChildElements(bdy);
    
    Utils::log("✓ Body processing complete");
}

void DOMProcessor::transformCustomTags(std::shared_ptr<Element> element) {
    if (!element) return;
    
    // Transform <bttn> → <button>
    if (element->tag_name == "bttn") {
        element->tag_name = "button";
        Utils::log("  Transformed <bttn> to <button>");
    }
    
    // Transform <dv> → <div>
    else if (element->tag_name == "dv") {
        element->tag_name = "div";
        Utils::log("  Transformed <dv> to <div>");
    }
    
    // Transform <nv> → <nav>
    else if (element->tag_name == "nv") {
        element->tag_name = "nav";
        Utils::log("  Transformed <nv> to <nav>");
    }
    
    // Transform <frm> → <form>
    else if (element->tag_name == "frm") {
        element->tag_name = "form";
        Utils::log("  Transformed <frm> to <form>");
    }
    
    // Transform <lbl> → <label>
    else if (element->tag_name == "lbl") {
        element->tag_name = "label";
        Utils::log("  Transformed <lbl> to <label>");
    }
    
    // Transform <txt> → <input>
    else if (element->tag_name == "txt") {
        element->tag_name = "input";
        element->attributes["type"] = "text";
        Utils::log("  Transformed <txt> to <input type=\"text\">");
    }
    
    // Recursively transform children
    for (auto& child : element->children) {
        transformCustomTags(child);
    }
}

void DOMProcessor::processMetadata(std::shared_ptr<Element> mt) {
    std::string nm = mt->attributes["nm"];
    std::string value = !mt->attributes["thng"].empty() ? mt->attributes["thng"] : 
                       !mt->attributes["content"].empty() ? mt->attributes["content"] :
                       mt->attributes["pblc"];
    
    Utils::log("  Metadata: " + nm + " = " + value);
}

void DOMProcessor::processStyle(std::shared_ptr<Element> styl) {
    std::string v = styl->attributes["v"];
    std::string src = styl->attributes["src"];
    
    Utils::log("  Loading style: " + src + " (CSS " + v + ")");
    // TODO: Load CSS file
}

void DOMProcessor::processScript(std::shared_ptr<Element> scrpt) {
    std::string v = scrpt->attributes["v"];
    std::string src = scrpt->attributes["src"];
    
    Utils::log("  Loading script: " + src + " (JS² " + v + ")");
    // TODO: Load JS file
}

void DOMProcessor::processServer(std::shared_ptr<Element> srv) {
    std::string id = srv->attributes["srvd"];
    std::string cld = srv->attributes["cld"];
    std::string mltplyr = srv->attributes["mltplyr"];
    
    Utils::log("  Server: " + id + " (cloud=" + cld + ", multiplayer=" + mltplyr + ")");
    
    // Process request handlers
    for (auto& child : srv->children) {
        if (child->tag_name == "nrq") {
            std::string pth = child->attributes["pth"];
            Utils::log("    Handler: " + pth);
        }
    }
}

void DOMProcessor::processChildElements(std::shared_ptr<Element> parent) {
    for (auto& child : parent->children) {
        transformCustomTags(child);
        processChildElements(child);
    }
}

std::string DOMProcessor::generateHTML(const HTML6AST* ast) {
    if (!ast) return "";
    
    std::string result = "<!DOCTYPE>\n";
    result += "<html>\n";
    
    // Add head
    result += "  <head>\n";
    for (const auto& style : ast->head.styles) {
        result += "    <link rel=\"stylesheet\" href=\"" + style.src + "\">\n";
    }
    for (const auto& script : ast->head.scripts) {
        result += "    <script src=\"" + script.src + "\"></script>\n";
    }
    result += "  </head>\n";
    
    // Add body
    result += "  <body>\n";
    result += ast->body.raw_html;
    result += "  </body>\n";
    
    result += "</html>\n";
    
    return result;
}