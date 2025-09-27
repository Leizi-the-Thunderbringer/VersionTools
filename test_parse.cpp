#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "src/core/GitUtils.h"
#include "src/core/GitManager.h"

using namespace VersionTools;

int main() {
    // Read the git output from file
    std::ifstream file("/tmp/git_test.txt", std::ios::binary);
    std::string gitOutput((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
    file.close();

    std::cout << "Git output length: " << gitOutput.length() << std::endl;

    // Split by null character
    auto commitBlocks = GitUtils::split(gitOutput, std::string(1, '\0'));

    std::cout << "Number of commit blocks: " << commitBlocks.size() << std::endl;

    for (size_t i = 0; i < commitBlocks.size(); ++i) {
        std::cout << "\n=== Block " << i << " ===" << std::endl;
        std::cout << "Length: " << commitBlocks[i].length() << std::endl;
        if (!commitBlocks[i].empty()) {
            // Try to parse just the parts
            auto parts = GitUtils::split(commitBlocks[i], "|");
            std::cout << "Number of parts: " << parts.size() << std::endl;
            if (parts.size() >= 7) {
                std::cout << "  Hash: " << parts[0].substr(0, 8) << "..." << std::endl;
                std::cout << "  Short: " << parts[1] << std::endl;
                std::cout << "  Author: " << parts[2] << std::endl;
                std::cout << "  Subject: " << parts[4] << std::endl;
            }
        }
    }

    return 0;
}