#ifndef MINUAN_DOM_PROCESSOR_HPP
#define MINUAN_DOM_PROCESSOR_HPP

#include "html6_engine.hpp"
#include <string>
#include <memory>

namespace Minuan {
namespace HTML6 {

/// Processes HTML6 DOM and transforms tags
class DOMProcessor {
public:
    DOMProcessor();
    
    // Head processing
    void processHead(std::shared_ptr<Element> hd, HTML6Engine* engine);
    void processMetadata(std::shared_ptr<Element> mt);
    void processStyle(std::shared_ptr<Element> styl);
    void processScript(std::shared_ptr<Element> scrpt);
    void processServer(std::shared_ptr<Element> srv);
    
    // Body processing
    void processBody(std::shared_ptr<Element> bdy);
    void transformCustomTags(std::shared_ptr<Element> element);
    
    // Transform individual tags
    std::shared_ptr<Element> transformButton(std::shared_ptr<Element> bttn);
    std::shared_ptr<Element> transformDiv(std::shared_ptr<Element> dv);
    std::shared_ptr<Element> transformNav(std::shared_ptr<Element> nv);
    std::shared_ptr<Element> transformForm(std::shared_ptr<Element> frm);
    std::shared_ptr<Element> transformLabel(std::shared_ptr<Element> lbl);
    std::shared_ptr<Element> transformInput(std::shared_ptr<Element> txt);
    
    // Generate output HTML
    std::string generateHTML(const HTML6AST* ast);
    
private:
    // Helper methods
    std::string elementToString(const std::shared_ptr<Element>& elem);
    void processChildElements(std::shared_ptr<Element> parent);
};

}  // namespace HTML6
}  // namespace Minuan

#endif  // MINUAN_DOM_PROCESSOR_HPP