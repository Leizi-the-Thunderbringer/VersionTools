#include <iostream>
#include <string>
#include <vector>
#include "src/core/GitUtils.h"

using namespace VersionTools;

int main() {
    // Test with null-separated string
    std::string test = "one";
    test += '\0';
    test += "two";
    test += '\0';
    test += "three";

    std::cout << "Test string length: " << test.length() << std::endl;

    // Test split with null character
    auto parts = GitUtils::split(test, std::string(1, '\0'));

    std::cout << "Number of parts: " << parts.size() << std::endl;
    for (size_t i = 0; i < parts.size(); ++i) {
        std::cout << "Part " << i << ": '" << parts[i] << "' (length: " << parts[i].length() << ")" << std::endl;
    }

    return 0;
}