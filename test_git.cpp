#include <iostream>
#include <string>
#include <vector>
#include "src/core/GitManager.h"
#include "src/core/SystemCommand.h"

using namespace VersionTools;

int main() {
    // Test repository path
    std::string repoPath = "/Users/logos/fleet/VersionTools";

    std::cout << "Testing Git operations on: " << repoPath << std::endl;

    // Test SystemCommand directly
    SystemCommand cmd;
    auto result = cmd.execute("git", {"status", "--porcelain=v1", "-b"}, repoPath);

    std::cout << "\n=== Git Status Command ===" << std::endl;
    std::cout << "Exit code: " << result.exitCode << std::endl;
    std::cout << "Success: " << (result.exitCode == 0 ? "Yes" : "No") << std::endl;
    std::cout << "Output length: " << result.output.length() << std::endl;
    std::cout << "Output:\n" << result.output << std::endl;
    std::cout << "Error:\n" << result.error << std::endl;

    // Test GitManager
    GitManager gitManager(repoPath);

    std::cout << "\n=== GitManager Tests ===" << std::endl;

    // Test getCurrentBranch
    std::string branch = gitManager.getCurrentBranch();
    std::cout << "Current branch: " << branch << std::endl;

    // Test getStatus
    auto status = gitManager.getStatus();
    std::cout << "Status - Current branch: " << status.currentBranch << std::endl;
    std::cout << "Status - Has uncommitted changes: " << status.hasUncommittedChanges << std::endl;
    std::cout << "Status - Number of changes: " << status.changes.size() << std::endl;

    for (const auto& change : status.changes) {
        std::cout << "  File: " << change.filePath << " Status: " << (int)change.status << " Staged: " << change.isStaged << std::endl;
    }

    // Test getCommitHistory
    std::cout << "\n=== Debug getCommitHistory ===" << std::endl;

    // Call the command directly to see raw output
    SystemCommand cmd2;
    auto rawResult = cmd2.execute("git", {"log", "--pretty=format:%H|%h|%an|%ae|%s|%ct|%P", "-z", "-5"}, repoPath);
    std::cout << "Raw output length: " << rawResult.output.length() << std::endl;

    // Count null characters
    int nullCount = 0;
    for (char c : rawResult.output) {
        if (c == '\0') nullCount++;
    }
    std::cout << "Null character count: " << nullCount << std::endl;

    auto commits = gitManager.getCommitHistory(50);  // Get more commits
    std::cout << "\n=== Recent Commits ===" << std::endl;
    std::cout << "Number of commits: " << commits.size() << std::endl;

    for (size_t i = 0; i < commits.size() && i < 10; ++i) {
        const auto& commit = commits[i];
        std::cout << "  " << commit.shortHash << " - " << commit.shortMessage << std::endl;
    }

    if (commits.size() > 10) {
        std::cout << "  ... and " << (commits.size() - 10) << " more commits" << std::endl;
    }

    // Test getBranches
    auto branches = gitManager.getBranches(false);
    std::cout << "\n=== Branches ===" << std::endl;
    std::cout << "Number of branches: " << branches.size() << std::endl;

    for (const auto& branch : branches) {
        std::cout << "  " << branch.name << " (current: " << branch.isCurrent << ")" << std::endl;
    }

    return 0;
}