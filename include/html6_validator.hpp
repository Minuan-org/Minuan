#ifndef MINUAN_HTML6_VALIDATOR_HPP
#define MINUAN_HTML6_VALIDATOR_HPP

#include "html6_engine.hpp"
#include <string>
#include <vector>

namespace Minuan {
namespace HTML6 {

/// Validation error information
struct ValidationError {
    std::string message;
    std::string location;  // Line/position info
    std::string severity;  // "error", "warning"
};

/// HTML6 Document Validator
class HTML6Validator {
public:
    HTML6Validator();
    
    // Validation methods
    bool validateDoctype(const std::string& html);
    bool validateTagName(const std::string& tag);
    bool validateAttributeName(const std::string& attr_name);
    bool validateServerRules(const ServerDefinition& server);
    bool validateDocumentStructure(const HTML6AST* ast);
    bool validateElement(const Element& element);
    
    // Error reporting
    const std::vector<ValidationError>& getErrors() const { return errors_; }
    void clearErrors() { errors_.clear(); }
    int getErrorCount() const;
    int getWarningCount() const;
    
private:
    std::vector<ValidationError> errors_;
    
    // Helper methods
    bool hasVowels(const std::string& str) const;
    bool isValidTagName(const std::string& tag) const;
    bool isValidAttributeName(const std::string& attr) const;
    void addError(const std::string& message, const std::string& location = "");
    void addWarning(const std::string& message, const std::string& location = "");
};

}  // namespace HTML6
}  // namespace Minuan

#endif  // MINUAN_HTML6_VALIDATOR_HPP