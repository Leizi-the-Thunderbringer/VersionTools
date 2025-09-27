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

}