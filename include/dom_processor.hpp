#ifndef MINUAN_DOM_PROCESSOR_HPP
#define MINUAN_DOM_PROCESSOR_HPP

#include "html6_engine.hpp"

#include <memory>
#include <string>

namespace Minuan {
namespace HTML6 {

class DOMProcessor {
public:
    DOMProcessor();

    void processHead(std::shared_ptr<Element> head, HTML6Engine* engine);
    void processMetadata(std::shared_ptr<Element> metadata);
    void processStyle(std::shared_ptr<Element> style);
    void processScript(std::shared_ptr<Element> script);
    void processServer(std::shared_ptr<Element> server);

    void processBody(std::shared_ptr<Element> body);
    void transformCustomTags(std::shared_ptr<Element> element);
    std::string generateHTML(const HTML6AST* ast);

private:
    void processChildElements(std::shared_ptr<Element> parent);
};

} // namespace HTML6
} // namespace Minuan

#endif
