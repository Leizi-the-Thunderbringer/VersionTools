#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <iomanip>

namespace VersionTools {

class GitUtils {
public:
    // String utilities
    static std::string trim(const std::string& str);
    static std::string trimLeft(const std::string& str);
    static std::string trimRight(const std::string& str);
    static std::vector<std::string> split(const std::string& str, const std::string& delimiter);
    static std::string join(const std::vector<std::string>& parts, const std::string& delimiter);
    static bool startsWith(const std::string& str, const std::string& prefix);
    static bool endsWith(const std::string& str, const std::string& suffix);
    static std::string toLower(const std::string& str);
    static std::string toUpper(const std::string& str);
    
    // Path utilities
    static std::string normalizePath(const std::string& path);
    static std::string getFileName(const std::string& path);
    static std::string getFileExtension(const std::string& path);
    static std::string getDirectory(const std::string& path);
    static std::string joinPaths(const std::string& path1, const std::string& path2);
    static bool isAbsolutePath(const std::string& path);
    static std::string makeRelativePath(const std::string& from, const std::string& to);
    
    // Git-specific utilities
    static std::string shortenHash(const std::string& hash, int length = 7);
    static bool isValidHash(const std::string& hash);
    static std::string formatCommitMessage(const std::string& message, int maxLength = 50);
    static std::string formatAuthor(const std::string& name, const std::string& email);
    static std::string formatTimestamp(const std::chrono::system_clock::time_point& timestamp);
    static std::string formatRelativeTime(const std::chrono::system_clock::time_point& timestamp);
    static std::string formatFileSize(size_t bytes);
    
    // Branch name utilities
    static bool isValidBranchName(const std::string& name);
    static std::string sanitizeBranchName(const std::string& name);
    static std::string getShortBranchName(const std::string& fullName);
    static bool isRemoteBranch(const std::string& branchName);
    static std::string getRemoteFromBranch(const std::string& branchName);
    
    // URL utilities
    static bool isValidGitUrl(const std::string& url);
    static std::string extractRepoNameFromUrl(const std::string& url);
    static std::string normalizeGitUrl(const std::string& url);
    static bool isHttpsUrl(const std::string& url);
    static bool isSshUrl(const std::string& url);
    
    // Diff utilities
    static std::string colorizeGitDiff(const std::string& diff);
    static int countLinesAdded(const std::string& diff);
    static int countLinesRemoved(const std::string& diff);
    static std::string extractHunkHeader(const std::string& line);
    
    // Configuration utilities
    static std::string getGitConfigPath(bool global = false);
    static std::string escapeShellArgument(const std::string& arg);
    static std::vector<std::string> parseGitConfigLine(const std::string& line);
    
    // Validation utilities
    static bool isValidEmail(const std::string& email);
    static bool isValidCommitMessage(const std::string& message);
    static bool isBinaryFile(const std::string& filePath);
    static std::string detectFileEncoding(const std::string& filePath);
    
    // Progress and status utilities
    static std::string formatProgress(int current, int total, const std::string& operation = "");
    static std::string formatTransferSpeed(size_t bytesPerSecond);
    static std::string formatDuration(const std::chrono::milliseconds& duration);
    
private:
    static const std::string WHITESPACE_CHARS;
    static const std::vector<std::string> INVALID_BRANCH_CHARS;
    static const std::vector<std::string> BINARY_EXTENSIONS;
};

}