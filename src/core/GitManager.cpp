#include "GitManager.h"
#include "SystemCommand.h"
#include "GitUtils.h"
#include <sstream>
#include <regex>
#include <filesystem>
#include <future>
#include <thread>

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
    
    fs::path repoPath(path);
    if (!fs::exists(repoPath)) {
        return false;
    }
    
    // Check for .git directory or file
    fs::path gitPath = repoPath / ".git";
    if (fs::exists(gitPath)) {
        return true;
    }
    
    // Check if this is a bare repository
    fs::path headPath = repoPath / "HEAD";
    fs::path objectsPath = repoPath / "objects";
    fs::path refsPath = repoPath / "refs";
    
    return fs::exists(headPath) && fs::exists(objectsPath) && fs::exists(refsPath);
}

GitRepository GitManager::getRepositoryInfo() const {
    GitRepository repo;
    repo.path = pImpl->repositoryPath;
    repo.workingDirectory = pImpl->repositoryPath;
    
    namespace fs = std::filesystem;
    fs::path gitDir = fs::path(pImpl->repositoryPath) / ".git";
    
    if (fs::exists(gitDir)) {
        if (fs::is_directory(gitDir)) {
            repo.gitDirectory = gitDir.string();
        } else {
            // Handle git worktrees and submodules
            std::ifstream gitFile(gitDir);
            std::string line;
            if (std::getline(gitFile, line) && line.starts_with("gitdir: ")) {
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
    
    if (!lines.empty() && lines[0].starts_with("##")) {
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
            
            if (change.status != FileStatus::Untracked) {
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
    std::vector<std::string> args = {"log", "--pretty=format:%H|%h|%an|%ae|%s|%B|%ct|%P"};
    
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
    auto commitBlocks = GitUtils::split(result.output, "\n\n");
    
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
                                               ProgressCallback progressCallback) const {
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
    auto lines = GitUtils::split(commitData, "\n");
    if (lines.empty()) {
        return {};
    }
    
    auto parts = GitUtils::split(lines[0], "|");
    if (parts.size() < 8) {
        return {};
    }
    
    GitCommit commit;
    commit.hash = parts[0];
    commit.shortHash = parts[1];
    commit.author = parts[2];
    commit.email = parts[3];
    commit.shortMessage = parts[4];
    commit.message = parts[5];
    
    // Parse timestamp
    try {
        auto timestamp = std::chrono::seconds(std::stoll(parts[6]));
        commit.timestamp = std::chrono::system_clock::time_point(timestamp);
    } catch (...) {
        commit.timestamp = std::chrono::system_clock::now();
    }
    
    // Parse parent hashes
    if (!parts[7].empty()) {
        commit.parentHashes = GitUtils::split(parts[7], " ");
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
    if (stagedFlag == 'A') {
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
    } else if (statusLine.starts_with("??")) {
        change.status = FileStatus::Untracked;
        change.isStaged = false;
    } else if (statusLine.starts_with("!!")) {
        change.status = FileStatus::Ignored;
        change.isStaged = false;
    } else if (unstagedFlag == 'U' || stagedFlag == 'U') {
        change.status = FileStatus::Conflicted;
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

}