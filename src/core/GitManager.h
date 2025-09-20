#pragma once

#include "GitTypes.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <future>

namespace VersionTools {

enum class GitCommandResult {
    Success,
    Failed,
    NotFound,
    InvalidRepository,
    NetworkError,
    PermissionDenied,
    Cancelled
};

struct GitOperationResult {
    GitCommandResult result;
    std::string output;
    std::string error;
    int exitCode = 0;
    
    bool isSuccess() const { return result == GitCommandResult::Success; }
    bool hasError() const { return !error.empty() || exitCode != 0; }
};

using ProgressCallback = std::function<void(const std::string& operation, int current, int total)>;
using LogCallback = std::function<void(const std::string& message)>;

class GitManager {
public:
    explicit GitManager(const std::string& repositoryPath = "");
    ~GitManager();

    // Repository operations
    GitOperationResult initRepository(const std::string& path, bool bare = false);
    GitOperationResult cloneRepository(const std::string& url, const std::string& path, 
                                     ProgressCallback progressCallback = nullptr);
    GitOperationResult openRepository(const std::string& path);
    bool isValidRepository(const std::string& path) const;
    
    // Repository info
    GitRepository getRepositoryInfo() const;
    GitStatus getStatus() const;
    std::string getCurrentBranch() const;
    std::string getRepositoryPath() const;
    
    // Commit operations
    GitOperationResult addFiles(const std::vector<std::string>& files);
    GitOperationResult addAllFiles();
    GitOperationResult removeFiles(const std::vector<std::string>& files, bool cached = false);
    GitOperationResult resetFiles(const std::vector<std::string>& files);
    GitOperationResult resetHard(const std::string& commitHash = "HEAD");
    GitOperationResult commit(const std::string& message, bool amend = false);
    GitOperationResult commitWithFiles(const std::string& message, 
                                     const std::vector<std::string>& files);
    
    // History and log
    std::vector<GitCommit> getCommitHistory(int maxCount = 100, 
                                          GitLogOptions options = GitLogOptions::None,
                                          const std::string& branch = "",
                                          const std::string& filePath = "") const;
    std::optional<GitCommit> getCommit(const std::string& hash) const;
    std::vector<GitCommit> getCommitRange(const std::string& fromHash, 
                                        const std::string& toHash) const;
    
    // Branch operations
    std::vector<GitBranch> getBranches(bool includeRemote = true) const;
    GitOperationResult createBranch(const std::string& name, 
                                   const std::string& startPoint = "HEAD");
    GitOperationResult deleteBranch(const std::string& name, bool force = false);
    GitOperationResult renameBranch(const std::string& oldName, const std::string& newName);
    GitOperationResult checkoutBranch(const std::string& name);
    GitOperationResult mergeBranch(const std::string& branchName, bool noFastForward = false);
    GitOperationResult rebaseBranch(const std::string& branchName);
    
    // Remote operations
    std::vector<GitRemote> getRemotes() const;
    GitOperationResult addRemote(const std::string& name, const std::string& url);
    GitOperationResult removeRemote(const std::string& name);
    GitOperationResult renameRemote(const std::string& oldName, const std::string& newName);
    GitOperationResult fetch(const std::string& remote = "origin", 
                           ProgressCallback progressCallback = nullptr);
    GitOperationResult pull(const std::string& remote = "origin", 
                          const std::string& branch = "",
                          ProgressCallback progressCallback = nullptr);
    GitOperationResult push(const std::string& remote = "origin", 
                          const std::string& branch = "",
                          bool force = false,
                          ProgressCallback progressCallback = nullptr);
    
    // Diff operations
    GitDiff getDiff(const std::string& filePath, bool staged = false) const;
    std::vector<GitDiff> getDiffAll(bool staged = false) const;
    GitDiff getCommitDiff(const std::string& commitHash) const;
    std::vector<GitDiff> getCommitDiffAll(const std::string& commitHash) const;
    GitDiff getDiffBetweenCommits(const std::string& fromHash, 
                                const std::string& toHash,
                                const std::string& filePath = "") const;
    
    // Tag operations
    std::vector<GitTag> getTags() const;
    GitOperationResult createTag(const std::string& name, const std::string& message = "",
                               const std::string& commitHash = "HEAD");
    GitOperationResult deleteTag(const std::string& name);
    GitOperationResult pushTags(const std::string& remote = "origin");
    
    // Stash operations
    std::vector<GitStash> getStashes() const;
    GitOperationResult stash(const std::string& message = "", bool includeUntracked = false);
    GitOperationResult stashPop(int index = 0);
    GitOperationResult stashApply(int index = 0);
    GitOperationResult stashDrop(int index = 0);
    GitOperationResult stashClear();
    
    // Configuration
    GitOperationResult setConfig(const std::string& key, const std::string& value, 
                               bool global = false);
    std::string getConfig(const std::string& key, bool global = false) const;
    GitOperationResult setUserInfo(const std::string& name, const std::string& email,
                                 bool global = false);
    
    // Utility methods
    bool hasUncommittedChanges() const;
    bool hasUnstagedChanges() const;
    bool hasStagedChanges() const;
    std::string getLastError() const;
    
    // Async operations
    std::future<GitOperationResult> cloneRepositoryAsync(const std::string& url, 
                                                       const std::string& path,
                                                       ProgressCallback progressCallback = nullptr);
    std::future<GitOperationResult> fetchAsync(const std::string& remote = "origin",
                                             ProgressCallback progressCallback = nullptr);
    std::future<GitOperationResult> pullAsync(const std::string& remote = "origin",
                                            const std::string& branch = "",
                                            ProgressCallback progressCallback = nullptr);
    std::future<GitOperationResult> pushAsync(const std::string& remote = "origin",
                                            const std::string& branch = "",
                                            bool force = false,
                                            ProgressCallback progressCallback = nullptr);
    
    // Event callbacks
    void setLogCallback(LogCallback callback);
    void setProgressCallback(ProgressCallback callback);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
    
    GitOperationResult executeGitCommand(const std::vector<std::string>& args,
                                       const std::string& workingDir = "",
                                       ProgressCallback progressCallback = nullptr) const;
    
    std::vector<std::string> parseGitOutput(const std::string& output, 
                                          const std::string& delimiter = "\n") const;
    GitCommit parseCommit(const std::string& commitData) const;
    GitBranch parseBranch(const std::string& branchData) const;
    GitFileChange parseFileChange(const std::string& statusLine) const;
    GitDiff parseDiff(const std::string& diffOutput, const std::string& filePath = "") const;
};

}