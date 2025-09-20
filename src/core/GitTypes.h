#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <optional>

namespace VersionTools {

enum class FileStatus {
    Untracked,
    Modified,
    Added,
    Deleted,
    Renamed,
    Copied,
    Conflicted,
    Ignored
};

enum class ChangeType {
    Addition,
    Deletion,
    Modification
};

struct GitCommit {
    std::string hash;
    std::string shortHash;
    std::string author;
    std::string email;
    std::string message;
    std::string shortMessage;
    std::chrono::system_clock::time_point timestamp;
    std::vector<std::string> parentHashes;
    bool isMerge() const { return parentHashes.size() > 1; }
};

struct GitBranch {
    std::string name;
    std::string fullName;
    bool isRemote;
    bool isCurrent;
    std::string upstreamBranch;
    int aheadCount = 0;
    int behindCount = 0;
    std::optional<GitCommit> lastCommit;
};

struct GitRemote {
    std::string name;
    std::string url;
    std::string pushUrl;
};

struct GitFileChange {
    std::string filePath;
    std::string oldPath;
    FileStatus status;
    bool isStaged;
    size_t linesAdded = 0;
    size_t linesDeleted = 0;
};

struct GitDiffLine {
    enum class Type { Context, Addition, Deletion, Header };
    Type type;
    std::string content;
    int oldLineNumber = -1;
    int newLineNumber = -1;
};

struct GitDiffHunk {
    std::string header;
    std::vector<GitDiffLine> lines;
    int oldStart;
    int oldCount;
    int newStart;
    int newCount;
};

struct GitDiff {
    std::string filePath;
    std::string oldPath;
    bool isBinary = false;
    bool isNewFile = false;
    bool isDeletedFile = false;
    std::vector<GitDiffHunk> hunks;
};

struct GitStatus {
    std::string currentBranch;
    std::string upstreamBranch;
    int aheadCount = 0;
    int behindCount = 0;
    bool hasUncommittedChanges = false;
    bool hasUnstagedChanges = false;
    bool hasStagedChanges = false;
    std::vector<GitFileChange> changes;
};

struct GitRepository {
    std::string path;
    std::string workingDirectory;
    std::string gitDirectory;
    bool isBare = false;
    bool isShallow = false;
    std::string head;
    GitStatus status;
};

struct GitTag {
    std::string name;
    std::string message;
    std::string commitHash;
    bool isAnnotated;
    std::chrono::system_clock::time_point timestamp;
};

struct GitStash {
    std::string name;
    std::string message;
    std::string branch;
    std::chrono::system_clock::time_point timestamp;
    int index;
};

enum class GitLogOptions {
    None = 0,
    ShowMerges = 1 << 0,
    FirstParentOnly = 1 << 1,
    FollowRenames = 1 << 2,
    SimplifyMerges = 1 << 3
};

inline GitLogOptions operator|(GitLogOptions a, GitLogOptions b) {
    return static_cast<GitLogOptions>(static_cast<int>(a) | static_cast<int>(b));
}

inline GitLogOptions operator&(GitLogOptions a, GitLogOptions b) {
    return static_cast<GitLogOptions>(static_cast<int>(a) & static_cast<int>(b));
}

}