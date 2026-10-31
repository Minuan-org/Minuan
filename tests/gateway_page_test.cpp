#include <array>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Expected the path to browser/home.html.\n";
        return 1;
    }

    std::ifstream input(argv[1], std::ios::binary);
    if (!input) {
        std::cerr << "Could not open the SimVerse gateway page.\n";
        return 1;
    }

    const std::string html(
        (std::istreambuf_iterator<char>(input)),
        std::istreambuf_iterator<char>());

    const std::array<std::string, 7> requiredContent{
        "<title>SimVerse | Minuan</title>",
        "href=\"https://github.com/Minuan-org/Minuan/blob/main/SIMVERSE_PETITION.md\"",
        "href=\"minuan://open-html6\"",
        "action=\"https://www.google.co.uk/search\"",
        "SimVerse App Store",
        "In development",
        "href=\"https://github.com/Minuan-org/Minuan\""
    };

    for (const std::string& item : requiredContent) {
        if (html.find(item) == std::string::npos) {
            std::cerr << "Gateway page is missing required content: "
                      << item << '\n';
            return 1;
        }
    }

    std::cout << "SimVerse gateway page checks passed.\n";
    return 0;
}
