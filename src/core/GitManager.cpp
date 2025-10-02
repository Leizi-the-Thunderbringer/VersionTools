#include "GitManager.h"
#include "SystemCommand.h"
#include "GitUtils.h"
#include <sstream>
#include <regex>
#include <filesystem>
#include <future>
#include <thread>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <set>

#ifdef USE_LIBGIT2
#include <git2.h>
#endif

namespace VersionTools {

class GitManager::Impl {
public:
    std::string repositoryPath;
    std::string lastError;
    LogCallback logCallback;
    ProgressCallback progressCallback;

    Impl(const std::string& repoPath) : repositoryPath(repoPath) {
#ifdef USE_LIBGIT2
        git_libgit2_init();
#endif
    }

    ~Impl() {
#ifdef USE_LIBGIT2
        git_libgit2_shutdown();
#endif
    }

    GitOperationResult executeGitCommand(const std::string& command) {
        GitOperationResult result;
        std::string fullCommand = "git " + command;

        SystemCommand cmd;
        auto cmdResult = cmd.execute(fullCommand, {}, repositoryPath);

        result.exitCode = cmdResult.exitCode;
        result.output = cmdResult.output;
        result.error = cmdResult.error;
        result.result = cmdResult.exitCode == 0 ? GitCommandResult::Success : GitCommandResult::Failed;

        if (!result.isSuccess()) {
            lastError = result.error.empty() ? result.output : result.error;
        }

        return result;
    }
};

GitManager::GitManager(const std::string& repositoryPath) 
    : pImpl(std::make_unique<Impl>(repositoryPath)) {
}

GitManager::~GitManager() = default;

GitOperationResult GitManager::initRepository(const std::string& path, bool bare) {
    std::vector<std::string> args = {"init"};
    if (bare) {
        args.push_back("--bare");
    }
    args.push_back(path);
    
    auto result = executeGitCommand(args);
    if (result.isSuccess()) {
        pImpl->repositoryPath = path;
    }
    return result;
}

GitOperationResult GitManager::cloneRepository(const std::string& url, const std::string& path,
                                             ProgressCallback progressCallback) {
    std::vector<std::string> args = {"clone", "--progress", url, path};
    
    auto result = executeGitCommand(args, "", progressCallback);
    if (result.isSuccess()) {
        pImpl->repositoryPath = path;
    }
    return result;
}

GitOperationResult GitManager::openRepository(const std::string& path) {
    if (!isValidRepository(path)) {
        return {GitCommandResult::InvalidRepository, "", "Not a valid git repository", 1};
    }
    
    pImpl->repositoryPath = path;
    return {GitCommandResult::Success, "", "", 0};
}

bool GitManager::isValidRepository(const std::string& path) const {
    namespace fs = std::filesystem;

    std::filesystem::path repoPath(path);
    if (!std::filesystem::exists(repoPath)) {
        return false;
    }

    // Check for .git directory or file
    std::filesystem::path gitPath = repoPath / ".git";
    if (std::filesystem::exists(gitPath)) {
        return true;
    }

    // Check if this is a bare repository
    std::filesystem::path headPath = repoPath / "HEAD";
    std::filesystem::path objectsPath = repoPath / "objects";
    std::filesystem::path refsPath = repoPath / "refs";

    return std::filesystem::exists(headPath) && std::filesystem::exists(objectsPath) && std::filesystem::exists(refsPath);
}

GitRepository GitManager::getRepositoryInfo() const {
    GitRepository repo;
    repo.path = pImpl->repositoryPath;
    repo.workingDirectory = pImpl->repositoryPath;

    std::filesystem::path gitDir = std::filesystem::path(pImpl->repositoryPath) / ".git";

    if (std::filesystem::exists(gitDir)) {
        if (std::filesystem::is_directory(gitDir)) {
            repo.gitDirectory = gitDir.string();
        } else {
            // Handle git worktrees and submodules
            std::ifstream gitFile(gitDir);
            std::string line;
            if (std::getline(gitFile, line) && line.substr(0, 8) == "gitdir: ") {
                repo.gitDirectory = line.substr(8);
            }
        }
    } else {
        // Bare repository
        repo.gitDirectory = pImpl->repositoryPath;
        repo.isBare = true;
    }

    repo.head = getCurrentBranch();
    repo.status = getStatus();

    return repo;
}

GitStatus GitManager::getStatus() const {
    auto result = executeGitCommand({"status", "--porcelain=v1", "-b"});
    if (!result.isSuccess()) {
        return {};
    }
    
    GitStatus status;
    auto lines = parseGitOutput(result.output);
    
    if (!lines.empty() && lines[0].substr(0, 2) == "##") {
        std::string branchLine = lines[0].substr(3);
        
        // Parse branch information
        std::regex branchRegex(R"(([^.]+)(?:\.\.\.([^[\s]+))?\s*(?:\[([^\]]+)\])?)");
        std::smatch matches;
        
        if (std::regex_search(branchLine, matches, branchRegex)) {
            status.currentBranch = matches[1].str();
            if (matches[2].matched) {
                status.upstreamBranch = matches[2].str();
            }
            
            if (matches[3].matched) {
                std::string trackInfo = matches[3].str();
                std::regex trackRegex(R"(ahead (\d+)|behind (\d+))");
                std::sregex_iterator iter(trackInfo.begin(), trackInfo.end(), trackRegex);
                std::sregex_iterator end;
                
                for (; iter != end; ++iter) {
                    if ((*iter)[1].matched) {
                        status.aheadCount = std::stoi((*iter)[1].str());
                    } else if ((*iter)[2].matched) {
                        status.behindCount = std::stoi((*iter)[2].str());
                    }
                }
            }
        }
    }
    
    // Parse file changes
    for (size_t i = 1; i < lines.size(); ++i) {
        if (lines[i].length() >= 3) {
            GitFileChange change = parseFileChange(lines[i]);
            status.changes.push_back(change);

            // Any change (including untracked files) means we have uncommitted changes
            if (change.status != FileStatus::Ignored) {
                status.hasUncommittedChanges = true;
            }
            if (change.isStaged) {
                status.hasStagedChanges = true;
            } else if (change.status != FileStatus::Untracked) {
                status.hasUnstagedChanges = true;
            }
        }
    }
    
    return status;
}

std::string GitManager::getCurrentBranch() const {
    auto result = executeGitCommand({"branch", "--show-current"});
    if (result.isSuccess() && !result.output.empty()) {
        return GitUtils::trim(result.output);
    }
    
    // Fallback: try symbolic-ref
    result = executeGitCommand({"symbolic-ref", "--short", "HEAD"});
    if (result.isSuccess() && !result.output.empty()) {
        return GitUtils::trim(result.output);
    }
    
    // Fallback: detached HEAD
    result = executeGitCommand({"rev-parse", "--short", "HEAD"});
    if (result.isSuccess() && !result.output.empty()) {
        return "HEAD detached at " + GitUtils::trim(result.output);
    }
    
    return "unknown";
}

std::string GitManager::getRepositoryPath() const {
    return pImpl->repositoryPath;
}

GitOperationResult GitManager::addFiles(const std::vector<std::string>& files) {
    if (files.empty()) {
        return {GitCommandResult::Success, "", "", 0};
    }
    
    std::vector<std::string> args = {"add"};
    args.insert(args.end(), files.begin(), files.end());
    
    return executeGitCommand(args);
}

GitOperationResult GitManager::addAllFiles() {
    return executeGitCommand({"add", "."});
}

GitOperationResult GitManager::removeFiles(const std::vector<std::string>& files, bool cached) {
    if (files.empty()) {
        return {GitCommandResult::Success, "", "", 0};
    }
    
    std::vector<std::string> args = {"rm"};
    if (cached) {
        args.push_back("--cached");
    }
    args.insert(args.end(), files.begin(), files.end());
    
    return executeGitCommand(args);
}

GitOperationResult GitManager::resetFiles(const std::vector<std::string>& files) {
    if (files.empty()) {
        return executeGitCommand({"reset", "HEAD"});
    }
    
    std::vector<std::string> args = {"reset", "HEAD"};
    args.insert(args.end(), files.begin(), files.end());
    
    return executeGitCommand(args);
}

GitOperationResult GitManager::resetHard(const std::string& commitHash) {
    return executeGitCommand({"reset", "--hard", commitHash});
}

GitOperationResult GitManager::commit(const std::string& message, bool amend) {
    std::vector<std::string> args = {"commit", "-m", message};
    if (amend) {
        args.insert(args.begin() + 1, "--amend");
    }
    
    return executeGitCommand(args);
}

GitOperationResult GitManager::commitWithFiles(const std::string& message, 
                                              const std::vector<std::string>& files) {
    // First add the files
    auto addResult = addFiles(files);
    if (!addResult.isSuccess()) {
        return addResult;
    }
    
    // Then commit
    return commit(message);
}

std::vector<GitCommit> GitManager::getCommitHistory(int maxCount,
                                                   GitLogOptions options,
                                                   const std::string& branch,
                                                   const std::string& filePath) const {
    std::vector<std::string> args = {"log", "--pretty=format:%H|%h|%an|%ae|%s|%ct|%P", "-z"};
    
    if (maxCount > 0) {
        args.push_back("-" + std::to_string(maxCount));
    }
    
    if ((options & GitLogOptions::FirstParentOnly) != GitLogOptions::None) {
        args.push_back("--first-parent");
    }
    
    if ((options & GitLogOptions::ShowMerges) == GitLogOptions::None) {
        args.push_back("--no-merges");
    }
    
    if ((options & GitLogOptions::FollowRenames) != GitLogOptions::None && !filePath.empty()) {
        args.push_back("--follow");
    }
    
    if (!branch.empty()) {
        args.push_back(branch);
    }
    
    if (!filePath.empty()) {
        args.push_back("--");
        args.push_back(filePath);
    }
    
    auto result = executeGitCommand(args);
    if (!result.isSuccess()) {
        return {};
    }

    std::vector<GitCommit> commits;
    // Split by null character since we used -z flag
    auto commitBlocks = GitUtils::split(result.output, std::string(1, '\0'));

    for (const auto& block : commitBlocks) {
        if (!block.empty()) {
            commits.push_back(parseCommit(block));
        }
    }

    return commits;
}

std::optional<GitCommit> GitManager::getCommit(const std::string& hash) const {
    auto result = executeGitCommand({"show", "--pretty=format:%H|%h|%an|%ae|%s|%B|%ct|%P", 
                                   "--no-patch", hash});
    if (!result.isSuccess() || result.output.empty()) {
        return std::nullopt;
    }
    
    return parseCommit(result.output);
}

GitOperationResult GitManager::executeGitCommand(const std::vector<std::string>& args,
                                               const std::string& workingDir,
                                               ProgressCallback /*progressCallback*/) const {
    std::string gitCommand = "git";
    std::string dir = workingDir.empty() ? pImpl->repositoryPath : workingDir;
    
    SystemCommand cmd;
    auto result = cmd.execute(gitCommand, args, dir);
    
    GitCommandResult gitResult = GitCommandResult::Success;
    if (result.exitCode != 0) {
        if (result.output.find("not a git repository") != std::string::npos) {
            gitResult = GitCommandResult::InvalidRepository;
        } else if (result.exitCode == 128) {
            gitResult = GitCommandResult::Failed;
        } else {
            gitResult = GitCommandResult::Failed;
        }
    }
    
    return {gitResult, result.output, result.error, result.exitCode};
}

std::vector<std::string> GitManager::parseGitOutput(const std::string& output, 
                                                   const std::string& delimiter) const {
    return GitUtils::split(output, delimiter);
}

GitCommit GitManager::parseCommit(const std::string& commitData) const {
    auto parts = GitUtils::split(commitData, "|");
    if (parts.size() < 7) {
        return {};
    }

    GitCommit commit;
    commit.hash = parts[0];
    commit.shortHash = parts[1];
    commit.author = parts[2];
    commit.email = parts[3];
    commit.shortMessage = parts[4];
    commit.message = parts[4];  // Use the subject as the full message

    // Parse timestamp
    try {
        auto timestamp = std::chrono::seconds(std::stoll(parts[5]));
        commit.timestamp = std::chrono::system_clock::time_point(timestamp);
    } catch (...) {
        commit.timestamp = std::chrono::system_clock::now();
    }

    // Parse parent hashes
    if (!parts[6].empty()) {
        commit.parentHashes = GitUtils::split(parts[6], " ");
    }

    return commit;
}

GitFileChange GitManager::parseFileChange(const std::string& statusLine) const {
    if (statusLine.length() < 3) {
        return {};
    }

    GitFileChange change;
    char stagedFlag = statusLine[0];
    char unstagedFlag = statusLine[1];
    change.filePath = statusLine.substr(3);

    // Handle renames and copies
    if (change.filePath.find(" -> ") != std::string::npos) {
        auto parts = GitUtils::split(change.filePath, " -> ");
        if (parts.size() == 2) {
            change.oldPath = parts[0];
            change.filePath = parts[1];
        }
    }

    // Determine status based on flags
    if (statusLine.substr(0, 2) == "??") {
        change.status = FileStatus::Untracked;
        change.isStaged = false;
    } else if (statusLine.substr(0, 2) == "!!") {
        change.status = FileStatus::Ignored;
        change.isStaged = false;
    } else if (stagedFlag == 'A') {
        change.status = FileStatus::Added;
        change.isStaged = true;
    } else if (stagedFlag == 'M') {
        change.status = FileStatus::Modified;
        change.isStaged = true;
    } else if (stagedFlag == 'D') {
        change.status = FileStatus::Deleted;
        change.isStaged = true;
    } else if (stagedFlag == 'R') {
        change.status = FileStatus::Renamed;
        change.isStaged = true;
    } else if (stagedFlag == 'C') {
        change.status = FileStatus::Copied;
        change.isStaged = true;
    } else if (unstagedFlag == 'M') {
        change.status = FileStatus::Modified;
        change.isStaged = false;
    } else if (unstagedFlag == 'D') {
        change.status = FileStatus::Deleted;
        change.isStaged = false;
    } else if (unstagedFlag == 'U' || stagedFlag == 'U') {
        change.status = FileStatus::Conflicted;
        change.isStaged = false;
    } else if (unstagedFlag == 'A') {
        // Added but not staged (shouldn't happen in normal workflow)
        change.status = FileStatus::Added;
        change.isStaged = false;
    }

    return change;
}

std::string GitManager::getLastError() const {
    return pImpl->lastError;
}

bool GitManager::hasUncommittedChanges() const {
    return getStatus().hasUncommittedChanges;
}

bool GitManager::hasUnstagedChanges() const {
    return getStatus().hasUnstagedChanges;
}

bool GitManager::hasStagedChanges() const {
    return getStatus().hasStagedChanges;
}

void GitManager::setLogCallback(LogCallback callback) {
    pImpl->logCallback = callback;
}

void GitManager::setProgressCallback(ProgressCallback callback) {
    pImpl->progressCallback = callback;
}

std::future<GitOperationResult> GitManager::cloneRepositoryAsync(const std::string& url, 
                                                               const std::string& path,
                                                               ProgressCallback progressCallback) {
    return std::async(std::launch::async, [this, url, path, progressCallback]() {
        return cloneRepository(url, path, progressCallback);
    });
}

// Additional method implementations would continue here...
// For brevity, I'm showing the core structure and key methods

// Branch operations
std::vector<GitBranch> GitManager::getBranches(bool includeRemote) const {
    std::vector<GitBranch> branches;

    // First, get current branch
    std::string currentBranch = getCurrentBranch();

    // Get all branches using for-each-ref (more reliable than branch command)
    std::vector<std::string> args = {"for-each-ref", "--format=%(refname:short)|%(objectname:short)|%(committerdate:iso)|%(upstream:short)|%(upstream:track)|%(subject)"};

    if (includeRemote) {
        args.push_back("refs/heads");
        args.push_back("refs/remotes");
    } else {
        args.push_back("refs/heads");
    }

    auto result = executeGitCommand(args);
    if (!result.isSuccess()) {
        return branches;
    }

    auto lines = parseGitOutput(result.output, "\n");
    for (const auto& line : lines) {
        if (line.empty()) continue;

        auto parts = GitUtils::split(line, "|");
        if (parts.size() < 6) continue;

        GitBranch branch;
        branch.name = parts[0];
        branch.fullName = parts[0];

        // Check if it's a remote branch
        if (branch.name.find("origin/") == 0 || branch.name.find("remotes/") == 0) {
            branch.isRemote = true;
            // Remove remote prefix
            if (branch.name.find("remotes/") == 0) {
                branch.name = branch.name.substr(8);
            }
        } else {
            branch.isRemote = false;
        }

        // Check if current branch
        branch.isCurrent = (branch.name == currentBranch || branch.fullName == currentBranch);

        // Parse upstream branch
        if (!parts[3].empty()) {
            branch.upstreamBranch = parts[3];
        }

        // Parse ahead/behind counts from tracking info
        if (!parts[4].empty()) {
            std::regex aheadRegex("ahead (\\d+)");
            std::regex behindRegex("behind (\\d+)");
            std::smatch matches;

            if (std::regex_search(parts[4], matches, aheadRegex)) {
                branch.aheadCount = std::stoi(matches[1]);
            }
            if (std::regex_search(parts[4], matches, behindRegex)) {
                branch.behindCount = std::stoi(matches[1]);
            }
        }

        // Create minimal commit info from available data
        if (!parts[1].empty()) {
            GitCommit commit;
            commit.shortHash = parts[1];
            commit.hash = parts[1]; // We only have short hash for now
            commit.shortMessage = parts[5];

            // Parse date
            if (!parts[2].empty()) {
                try {
                    // Parse ISO date format
                    std::tm tm = {};
                    std::istringstream ss(parts[2]);
                    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
                    commit.timestamp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
                } catch (...) {
                    commit.timestamp = std::chrono::system_clock::now();
                }
            }
            branch.lastCommit = commit;
        }

        branches.push_back(branch);
    }

    return branches;
}

GitOperationResult GitManager::createBranch(const std::string& name, const std::string& startPoint) {
    std::vector<std::string> args = {"branch", name};

    // Add start point if provided
    if (!startPoint.empty()) {
        args.push_back(startPoint);
    }

    return executeGitCommand(args);
}

GitOperationResult GitManager::deleteBranch(const std::string& name, bool force) {
    std::vector<std::string> args = {"branch"};

    // Use -d for safe delete or -D for force delete
    args.push_back(force ? "-D" : "-d");
    args.push_back(name);

    return executeGitCommand(args);
}

GitOperationResult GitManager::checkoutBranch(const std::string& name) {
    return executeGitCommand({"checkout", name});
}

// Stash operations
std::vector<GitStash> GitManager::getStashes() const {
    std::vector<GitStash> stashes;

    // Get stash list with more detailed information
    auto result = executeGitCommand({"stash", "list", "--format=%gd|%s|%cn|%ct"});
    if (!result.isSuccess() || result.output.empty()) {
        return stashes;
    }

    auto lines = parseGitOutput(result.output, "\n");
    int index = 0;

    for (const auto& line : lines) {
        if (line.empty()) continue;

        auto parts = GitUtils::split(line, "|");
        if (parts.size() < 4) continue;

        GitStash stash;
        stash.name = parts[0];
        stash.message = parts[1];
        stash.index = index++;

        // Extract branch name from the message if present
        std::regex branchRegex("On ([^:]+):");
        std::smatch matches;
        if (std::regex_search(stash.message, matches, branchRegex)) {
            stash.branch = matches[1];
        }

        // Parse timestamp
        try {
            auto timestamp = std::chrono::seconds(std::stoll(parts[3]));
            stash.timestamp = std::chrono::system_clock::time_point(timestamp);
        } catch (...) {
            stash.timestamp = std::chrono::system_clock::now();
        }

        stashes.push_back(stash);
    }

    return stashes;
}

// Stash operations
GitOperationResult GitManager::stash(const std::string& message, bool includeUntracked) {
    std::vector<std::string> args = {"stash", "push"};

    if (!message.empty()) {
        args.push_back("-m");
        args.push_back(message);
    }

    if (includeUntracked) {
        args.push_back("--include-untracked");
    }

    return executeGitCommand(args);
}

GitOperationResult GitManager::stashPop(int index) {
    std::vector<std::string> args = {"stash", "pop"};

    if (index > 0) {
        args.push_back("stash@{" + std::to_string(index) + "}");
    }

    return executeGitCommand(args);
}

GitOperationResult GitManager::stashApply(int index) {
    std::vector<std::string> args = {"stash", "apply"};

    if (index >= 0) {
        args.push_back("stash@{" + std::to_string(index) + "}");
    }

    return executeGitCommand(args);
}

GitOperationResult GitManager::stashDrop(int index) {
    std::vector<std::string> args = {"stash", "drop"};

    if (index >= 0) {
        args.push_back("stash@{" + std::to_string(index) + "}");
    }

    return executeGitCommand(args);
}

GitOperationResult GitManager::stashClear() {
    return executeGitCommand({"stash", "clear"});
}

// Diff operations
GitDiff GitManager::getCommitDiff(const std::string& commitHash) const {
    GitDiff diff;

    // Get diff for the first changed file in a commit
    auto filesResult = executeGitCommand({"diff-tree", "--no-commit-id", "--name-status", "-r", commitHash});
    if (!filesResult.isSuccess() || filesResult.output.empty()) {
        return diff;
    }

    auto files = parseGitOutput(filesResult.output, "\n");
    if (files.empty()) {
        return diff;
    }

    // Get the first file's diff
    auto fileParts = GitUtils::split(files[0], "\t");
    if (fileParts.size() < 2) {
        return diff;
    }

    std::string fileName = fileParts[1];
    diff.filePath = fileName;

    // Determine file status
    if (fileParts[0] == "A") {
        diff.isNewFile = true;
    } else if (fileParts[0] == "D") {
        diff.isDeletedFile = true;
    }

    // Get the actual diff content
    // Use show for the commit to handle initial commits properly
    auto diffResult = executeGitCommand({"show", commitHash, "--", fileName});
    if (!diffResult.isSuccess()) {
        return diff;
    }

    // Parse diff hunks
    auto lines = parseGitOutput(diffResult.output, "\n");
    GitDiffHunk* currentHunk = nullptr;

    for (const auto& line : lines) {
        if (line.empty()) continue;

        // Check for hunk header
        if (line.substr(0, 2) == "@@") {
            std::regex hunkRegex("@@ -(\\d+)(?:,(\\d+))? \\+(\\d+)(?:,(\\d+))? @@");
            std::smatch matches;
            if (std::regex_search(line, matches, hunkRegex)) {
                GitDiffHunk hunk;
                hunk.header = line;
                hunk.oldStart = std::stoi(matches[1]);
                hunk.oldCount = matches[2].matched ? std::stoi(matches[2]) : 1;
                hunk.newStart = std::stoi(matches[3]);
                hunk.newCount = matches[4].matched ? std::stoi(matches[4]) : 1;
                diff.hunks.push_back(hunk);
                currentHunk = &diff.hunks.back();
            }
        } else if (currentHunk && !line.empty()) {
            GitDiffLine diffLine;
            if (line[0] == '+') {
                diffLine.type = GitDiffLine::Type::Addition;
                diffLine.content = line.substr(1);
            } else if (line[0] == '-') {
                diffLine.type = GitDiffLine::Type::Deletion;
                diffLine.content = line.substr(1);
            } else if (line[0] == ' ') {
                diffLine.type = GitDiffLine::Type::Context;
                diffLine.content = line.substr(1);
            } else if (line.substr(0, 4) == "diff" || line.substr(0, 5) == "index" ||
                      line.substr(0, 3) == "+++" || line.substr(0, 3) == "---") {
                diffLine.type = GitDiffLine::Type::Header;
                diffLine.content = line;
            } else {
                continue;
            }
            currentHunk->lines.push_back(diffLine);
        }
    }

    return diff;
}

std::vector<GitDiff> GitManager::getCommitDiffAll(const std::string& commitHash) const {
    std::vector<GitDiff> diffs;

    // Get all changed files in a commit
    auto filesResult = executeGitCommand({"diff-tree", "--no-commit-id", "--name-status", "-r", commitHash});
    if (!filesResult.isSuccess() || filesResult.output.empty()) {
        return diffs;
    }

    auto files = parseGitOutput(filesResult.output, "\n");

    for (const auto& file : files) {
        if (file.empty()) continue;

        auto fileParts = GitUtils::split(file, "\t");
        if (fileParts.size() < 2) continue;

        std::string fileName = fileParts[1];
        GitDiff diff;
        diff.filePath = fileName;

        // Determine file status
        if (fileParts[0] == "A") {
            diff.isNewFile = true;
        } else if (fileParts[0] == "D") {
            diff.isDeletedFile = true;
        } else if (fileParts[0].find("R") != std::string::npos && fileParts.size() >= 3) {
            // Handle renames
            diff.oldPath = fileParts[1];
            diff.filePath = fileParts[2];
        }

        // Get the actual diff content for this file
        // Use show for the commit to handle initial commits properly
        auto diffResult = executeGitCommand({"show", commitHash, "--", fileName});
        if (!diffResult.isSuccess()) {
            continue;
        }

        // Check if binary
        if (diffResult.output.find("Binary files") != std::string::npos) {
            diff.isBinary = true;
            diffs.push_back(diff);
            continue;
        }

        // Parse diff hunks
        auto lines = parseGitOutput(diffResult.output, "\n");
        GitDiffHunk* currentHunk = nullptr;
        int oldLineNum = 0, newLineNum = 0;

        for (const auto& line : lines) {
            if (line.empty()) continue;

            // Check for hunk header
            if (line.substr(0, 2) == "@@") {
                std::regex hunkRegex("@@ -(\\d+)(?:,(\\d+))? \\+(\\d+)(?:,(\\d+))? @@");
                std::smatch matches;
                if (std::regex_search(line, matches, hunkRegex)) {
                    GitDiffHunk hunk;
                    hunk.header = line;
                    hunk.oldStart = std::stoi(matches[1]);
                    hunk.oldCount = matches[2].matched ? std::stoi(matches[2]) : 1;
                    hunk.newStart = std::stoi(matches[3]);
                    hunk.newCount = matches[4].matched ? std::stoi(matches[4]) : 1;
                    diff.hunks.push_back(hunk);
                    currentHunk = &diff.hunks.back();
                    oldLineNum = hunk.oldStart;
                    newLineNum = hunk.newStart;
                }
            } else if (currentHunk && !line.empty()) {
                GitDiffLine diffLine;
                if (line[0] == '+') {
                    diffLine.type = GitDiffLine::Type::Addition;
                    diffLine.content = line.substr(1);
                    diffLine.newLineNumber = newLineNum++;
                } else if (line[0] == '-') {
                    diffLine.type = GitDiffLine::Type::Deletion;
                    diffLine.content = line.substr(1);
                    diffLine.oldLineNumber = oldLineNum++;
                } else if (line[0] == ' ') {
                    diffLine.type = GitDiffLine::Type::Context;
                    diffLine.content = line.substr(1);
                    diffLine.oldLineNumber = oldLineNum++;
                    diffLine.newLineNumber = newLineNum++;
                } else if (line.substr(0, 4) == "diff" || line.substr(0, 5) == "index" ||
                          line.substr(0, 3) == "+++" || line.substr(0, 3) == "---") {
                    diffLine.type = GitDiffLine::Type::Header;
                    diffLine.content = line;
                } else {
                    continue;
                }
                currentHunk->lines.push_back(diffLine);
            }
        }

        diffs.push_back(diff);
    }

    return diffs;
}

// Remote operations
std::vector<GitRemote> GitManager::getRemotes() const {
    std::vector<GitRemote> remotes;

    auto result = pImpl->executeGitCommand("remote -v");
    if (result.isSuccess()) {
        std::istringstream stream(result.output);
        std::string line;
        std::set<std::string> seenNames;

        while (std::getline(stream, line)) {
            if (!line.empty()) {
                auto parts = GitUtils::split(line, "\t");
                if (parts.size() >= 2) {
                    std::string name = parts[0];

                    // Only add each remote once (git remote -v shows fetch and push URLs)
                    if (seenNames.find(name) == seenNames.end()) {
                        GitRemote remote;
                        remote.name = name;

                        // Parse URL and type from the second part
                        auto urlParts = GitUtils::split(parts[1], " ");
                        if (!urlParts.empty()) {
                            remote.url = urlParts[0];
                            remote.pushUrl = urlParts[0]; // Default to same as fetch
                        }

                        remotes.push_back(remote);
                        seenNames.insert(name);
                    }
                }
            }
        }

        // Get push URLs if different from fetch URLs
        for (auto& remote : remotes) {
            auto pushResult = pImpl->executeGitCommand("remote get-url --push " + remote.name);
            if (pushResult.isSuccess() && !pushResult.output.empty()) {
                remote.pushUrl = GitUtils::trim(pushResult.output);
            }
        }
    }

    return remotes;
}

GitOperationResult GitManager::addRemote(const std::string& name, const std::string& url) {
    if (name.empty() || url.empty()) {
        GitOperationResult result;
        result.result = GitCommandResult::Failed;
        result.error = "Remote name and URL cannot be empty";
        return result;
    }

    std::string command = "remote add " + name + " " + url;
    return pImpl->executeGitCommand(command);
}

GitOperationResult GitManager::removeRemote(const std::string& name) {
    if (name.empty()) {
        GitOperationResult result;
        result.result = GitCommandResult::Failed;
        result.error = "Remote name cannot be empty";
        return result;
    }

    std::string command = "remote remove " + name;
    return pImpl->executeGitCommand(command);
}

GitOperationResult GitManager::renameRemote(const std::string& oldName, const std::string& newName) {
    if (oldName.empty() || newName.empty()) {
        GitOperationResult result;
        result.result = GitCommandResult::Failed;
        result.error = "Remote names cannot be empty";
        return result;
    }

    std::string command = "remote rename " + oldName + " " + newName;
    return pImpl->executeGitCommand(command);
}

GitOperationResult GitManager::fetch(const std::string& remote, ProgressCallback progressCallback) {
    if (remote.empty()) {
        GitOperationResult result;
        result.result = GitCommandResult::Failed;
        result.error = "Remote name cannot be empty";
        return result;
    }

    std::vector<std::string> args = {"fetch", remote};
    return executeGitCommand(args, "", progressCallback);
}

GitOperationResult GitManager::pull(const std::string& remote, const std::string& branch, ProgressCallback progressCallback) {
    if (remote.empty()) {
        GitOperationResult result;
        result.result = GitCommandResult::Failed;
        result.error = "Remote name cannot be empty";
        return result;
    }

    std::vector<std::string> args = {"pull", remote};
    if (!branch.empty()) {
        args.push_back(branch);
    }

    return executeGitCommand(args, "", progressCallback);
}

GitOperationResult GitManager::push(const std::string& remote, const std::string& branch, bool force, ProgressCallback progressCallback) {
    if (remote.empty()) {
        GitOperationResult result;
        result.result = GitCommandResult::Failed;
        result.error = "Remote name cannot be empty";
        return result;
    }

    std::vector<std::string> args = {"push", remote};
    if (!branch.empty()) {
        args.push_back(branch);
    }
    if (force) {
        args.push_back("--force");
    }

    return executeGitCommand(args, "", progressCallback);
}

// Tag operations
std::vector<GitTag> GitManager::getTags() const {
    std::vector<GitTag> tags;

    // Get all tags with their details
    auto result = pImpl->executeGitCommand(
        "for-each-ref --format='%(refname:short)|%(objectname:short)|%(taggerdate:short)|%(subject)' refs/tags"
    );

    if (result.isSuccess()) {
        std::istringstream stream(result.output);
        std::string line;

        while (std::getline(stream, line)) {
            if (!line.empty()) {
                auto parts = GitUtils::split(line, "|");
                if (!parts.empty()) {
                    GitTag tag;
                    tag.name = parts[0];

                    if (parts.size() > 1) {
                        tag.commitHash = parts[1];
                    }

                    if (parts.size() > 2 && !parts[2].empty()) {
                        tag.date = parts[2];
                        tag.isAnnotated = true;
                    } else {
                        tag.isAnnotated = false;
                    }

                    if (parts.size() > 3) {
                        tag.message = parts[3];
                    }

                    tags.push_back(tag);
                }
            }
        }
    }

    return tags;
}

GitOperationResult GitManager::createTag(const std::string& name, const std::string& message,
                                       const std::string& commitHash) {
    if (name.empty()) {
        GitOperationResult result;
        result.result = GitCommandResult::Failed;
        result.error = "Tag name cannot be empty";
        return result;
    }

    std::string command = "tag";

    if (!message.empty()) {
        // Create annotated tag
        command += " -a " + name + " -m \"" + message + "\"";
    } else {
        // Create lightweight tag
        command += " " + name;
    }

    if (commitHash != "HEAD" && !commitHash.empty()) {
        command += " " + commitHash;
    }

    return pImpl->executeGitCommand(command);
}

GitOperationResult GitManager::deleteTag(const std::string& name) {
    if (name.empty()) {
        GitOperationResult result;
        result.result = GitCommandResult::Failed;
        result.error = "Tag name cannot be empty";
        return result;
    }

    std::string command = "tag -d " + name;
    return pImpl->executeGitCommand(command);
}

GitOperationResult GitManager::pushTags(const std::string& remote) {
    std::string command = "push " + remote + " --tags";
    return pImpl->executeGitCommand(command);
}

}

// 不会写啦，召唤夷陵老祖帮我写
// 话说莫玄羽献舍算未定义行为吗
// 其实我也不知道
// 风邪盘·····罗盘刻纹和指纹都甚是诡异，盘身堪堪可置于掌心。（现修真界通用的风邪盘是魏无羡所做的第一版，确实精密不足。魏无羡原本正在着手改进，谁教没改完老巢就被人捣了，也就只好委屈下大家，继续用精密不足的第一版了）
// 并非普通罗盘。不是用来指东南西北的，而是用来指凶邪妖煞的。
// 这可能不是罗盘，是nullptr
// 召阴旗
/*黑旗，上有纹饰咒文。
夷陵老祖出品，后被仙门百家效仿。
不同品级的召阴旗有不同的画法和威力。画法有误使用稍有不慎便会酿出大祸。*/
/*插在某个活人身上，将会把一定范围内的阴灵、冤魂、凶尸、邪祟都吸引过去，只攻击这名活人。由于被插旗者仿佛变成了活生生的靶子，所以又称“靶旗”。
也可以插在房子上，但房子里必须有活人，那么攻击范围就会扩大至屋子里的所有人。
因为插旗处附近一定阴气缭绕，仿佛黑风盘旋，也被叫做“黑风旗”。*/
// 夷陵老祖的东西，果然不是普通人能懂的
// 这是weak_ptr
// 当然也小心变成nullptr
// 以一人元神操控尸傀和恶灵。横笛一支吹彻长夜，纵鬼兵鬼将如千军万马，所向披靡，人挡杀人佛挡杀佛。笛声有如天人之音。这夷陵老祖的并发模型有点厉害
// 不会竞态条件吗
// 靠，这西门子S7-1200不停的死机
// 要去买个新的了
// 以及HwlloChen大佬的华硕天选4不要突然变成天选姬（不过加了AD域应该不会这样的，除非······夷陵老祖给我AD域控制器附体了）
// 以及我的MacBook Pro M1不要半夜自己malloc出1000000个0的heap然后里面存了个夷陵老祖的元神
// 还有，我的iPhone 14 Pro Max不要突然变成iPhone 14 Pro Max夷陵老祖版
// 以及，我的iPad Pro 12.9不要突然变成iPad Pro 12.9夷陵老祖版
// 以及我的ThinkPad X1 Carbon不要突然变成ThinkPad X1 Carbon夷陵老祖版