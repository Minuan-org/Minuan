#include "utils.hpp"
#include <iostream>

namespace Utils {

    void log(const std::string& msg) {
        std::cout << "[Minuan] " << msg << std::endl;
    }

    void logWarning(const std::string& msg) {
        std::cout << "[Minuan:Warning] " << msg << std::endl;
    }

    void logError(const std::string& msg) {
        std::cerr << "[Minuan:Error] " << msg << std::endl;
    }

}
