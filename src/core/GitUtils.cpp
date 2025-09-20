#include "GitUtils.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <regex>
#include <iomanip>

namespace VersionTools {

const std::string GitUtils::WHITESPACE_CHARS = " \t\n\r\f\v";
const std::vector<std::string> GitUtils::INVALID_BRANCH_CHARS = {
    " ", "~", "^", ":", "?", "*", "[", "\\", "..", "@{", "//"
};
const std::vector<std::string> GitUtils::BINARY_EXTENSIONS = {
    ".jpg", ".jpeg", ".png", ".gif", ".bmp", ".tiff", ".ico",
    ".exe", ".dll", ".so", ".dylib", ".zip", ".tar", ".gz", ".rar",
    ".pdf", ".doc", ".docx", ".xls", ".xlsx", ".ppt", ".pptx"
};

// String utilities
std::string GitUtils::trim(const std::string& str) {
    return trimLeft(trimRight(str));
}

std::string GitUtils::trimLeft(const std::string& str) {
    size_t start = str.find_first_not_of(WHITESPACE_CHARS);
    return (start == std::string::npos) ? "" : str.substr(start);
}

std::string GitUtils::trimRight(const std::string& str) {
    size_t end = str.find_last_not_of(WHITESPACE_CHARS);
    return (end == std::string::npos) ? "" : str.substr(0, end + 1);
}

std::vector<std::string> GitUtils::split(const std::string& str, const std::string& delimiter) {
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end = str.find(delimiter);
    
    while (end != std::string::npos) {
        tokens.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
        end = str.find(delimiter, start);
    }
    
    tokens.push_back(str.substr(start));
    return tokens;
}

std::string GitUtils::join(const std::vector<std::string>& parts, const std::string& delimiter) {
    if (parts.empty()) return "";
    
    std::string result = parts[0];
    for (size_t i = 1; i < parts.size(); ++i) {
        result += delimiter + parts[i];
    }
    return result;
}

bool GitUtils::startsWith(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && 
           str.compare(0, prefix.size(), prefix) == 0;
}

bool GitUtils::endsWith(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() && 
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string GitUtils::toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string GitUtils::toUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

// Path utilities
std::string GitUtils::normalizePath(const std::string& path) {
    namespace fs = std::filesystem;
    try {
        return fs::weakly_canonical(fs::path(path)).string();
    } catch (const std::exception&) {
        return path;
    }
}

std::string GitUtils::getFileName(const std::string& path) {
    namespace fs = std::filesystem;
    return fs::path(path).filename().string();
}

std::string GitUtils::getFileExtension(const std::string& path) {
    namespace fs = std::filesystem;
    return fs::path(path).extension().string();
}

std::string GitUtils::getDirectory(const std::string& path) {
    namespace fs = std::filesystem;
    return fs::path(path).parent_path().string();
}

std::string GitUtils::joinPaths(const std::string& path1, const std::string& path2) {
    namespace fs = std::filesystem;
    return (fs::path(path1) / fs::path(path2)).string();
}

bool GitUtils::isAbsolutePath(const std::string& path) {
    namespace fs = std::filesystem;
    return fs::path(path).is_absolute();
}

std::string GitUtils::makeRelativePath(const std::string& from, const std::string& to) {
    namespace fs = std::filesystem;
    try {
        return fs::relative(fs::path(to), fs::path(from)).string();
    } catch (const std::exception&) {
        return to;
    }
}

// Git-specific utilities
std::string GitUtils::shortenHash(const std::string& hash, int length) {
    if (hash.length() <= static_cast<size_t>(length)) {
        return hash;
    }
    return hash.substr(0, length);
}

bool GitUtils::isValidHash(const std::string& hash) {
    if (hash.empty() || hash.length() < 4 || hash.length() > 40) {
        return false;
    }
    
    return std::all_of(hash.begin(), hash.end(), [](char c) {
        return std::isxdigit(c);
    });
}

std::string GitUtils::formatCommitMessage(const std::string& message, int maxLength) {
    if (message.length() <= static_cast<size_t>(maxLength)) {
        return message;
    }
    
    auto lines = split(message, "\n");
    if (lines.empty()) return "";
    
    std::string firstLine = lines[0];
    if (firstLine.length() <= static_cast<size_t>(maxLength)) {
        return firstLine;
    }
    
    return firstLine.substr(0, maxLength - 3) + "...";
}

std::string GitUtils::formatAuthor(const std::string& name, const std::string& email) {
    if (name.empty() && email.empty()) {
        return "Unknown";
    }
    if (name.empty()) {
        return email;
    }
    if (email.empty()) {
        return name;
    }
    return name + " <" + email + ">";
}

std::string GitUtils::formatTimestamp(const std::chrono::system_clock::time_point& timestamp) {
    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string GitUtils::formatRelativeTime(const std::chrono::system_clock::time_point& timestamp) {
    auto now = std::chrono::system_clock::now();
    auto duration = now - timestamp;
    
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    auto minutes = seconds / 60;
    auto hours = minutes / 60;
    auto days = hours / 24;
    auto weeks = days / 7;
    auto months = days / 30;
    auto years = days / 365;
    
    if (years > 0) {
        return std::to_string(years) + (years == 1 ? " year ago" : " years ago");
    } else if (months > 0) {
        return std::to_string(months) + (months == 1 ? " month ago" : " months ago");
    } else if (weeks > 0) {
        return std::to_string(weeks) + (weeks == 1 ? " week ago" : " weeks ago");
    } else if (days > 0) {
        return std::to_string(days) + (days == 1 ? " day ago" : " days ago");
    } else if (hours > 0) {
        return std::to_string(hours) + (hours == 1 ? " hour ago" : " hours ago");
    } else if (minutes > 0) {
        return std::to_string(minutes) + (minutes == 1 ? " minute ago" : " minutes ago");
    } else {
        return "just now";
    }
}

std::string GitUtils::formatFileSize(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    double size = static_cast<double>(bytes);
    int unit = 0;
    
    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        unit++;
    }
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << size << " " << units[unit];
    return ss.str();
}

// Branch name utilities
bool GitUtils::isValidBranchName(const std::string& name) {
    if (name.empty() || name.front() == '.' || name.back() == '.' ||
        name.front() == '/' || name.back() == '/') {
        return false;
    }
    
    for (const auto& invalid : INVALID_BRANCH_CHARS) {
        if (name.find(invalid) != std::string::npos) {
            return false;
        }
    }
    
    return true;
}

std::string GitUtils::sanitizeBranchName(const std::string& name) {
    std::string result = name;
    
    // Replace invalid characters with dashes
    for (const auto& invalid : INVALID_BRANCH_CHARS) {
        size_t pos = 0;
        while ((pos = result.find(invalid, pos)) != std::string::npos) {
            result.replace(pos, invalid.length(), "-");
            pos += 1;
        }
    }
    
    // Remove leading/trailing dots and slashes
    while (!result.empty() && (result.front() == '.' || result.front() == '/')) {
        result.erase(0, 1);
    }
    while (!result.empty() && (result.back() == '.' || result.back() == '/')) {
        result.pop_back();
    }
    
    return result;
}

std::string GitUtils::getShortBranchName(const std::string& fullName) {
    if (startsWith(fullName, "refs/heads/")) {
        return fullName.substr(11);
    }
    if (startsWith(fullName, "refs/remotes/")) {
        return fullName.substr(13);
    }
    if (startsWith(fullName, "origin/")) {
        return fullName.substr(7);
    }
    return fullName;
}

bool GitUtils::isRemoteBranch(const std::string& branchName) {
    return startsWith(branchName, "refs/remotes/") || 
           branchName.find('/') != std::string::npos;
}

std::string GitUtils::getRemoteFromBranch(const std::string& branchName) {
    if (startsWith(branchName, "refs/remotes/")) {
        auto parts = split(branchName.substr(13), "/");
        return parts.empty() ? "" : parts[0];
    }
    
    auto slashPos = branchName.find('/');
    if (slashPos != std::string::npos) {
        return branchName.substr(0, slashPos);
    }
    
    return "";
}

// URL utilities
bool GitUtils::isValidGitUrl(const std::string& url) {
    if (url.empty()) return false;
    
    // HTTP/HTTPS URLs
    if (startsWith(url, "http://") || startsWith(url, "https://")) {
        return url.find(".git") != std::string::npos || 
               url.find("github.com") != std::string::npos ||
               url.find("gitlab.com") != std::string::npos ||
               url.find("bitbucket.org") != std::string::npos;
    }
    
    // SSH URLs
    if (startsWith(url, "git@") || startsWith(url, "ssh://")) {
        return true;
    }
    
    // File URLs
    if (startsWith(url, "file://") || startsWith(url, "/")) {
        return true;
    }
    
    return false;
}

std::string GitUtils::extractRepoNameFromUrl(const std::string& url) {
    std::string result = url;
    
    // Remove protocol
    size_t protocolEnd = result.find("://");
    if (protocolEnd != std::string::npos) {
        result = result.substr(protocolEnd + 3);
    }
    
    // Remove user@host part for SSH
    size_t atPos = result.find('@');
    if (atPos != std::string::npos) {
        size_t colonPos = result.find(':', atPos);
        if (colonPos != std::string::npos) {
            result = result.substr(colonPos + 1);
        }
    }
    
    // Remove host part for HTTP
    size_t slashPos = result.find('/');
    if (slashPos != std::string::npos) {
        result = result.substr(slashPos + 1);
    }
    
    // Remove .git suffix
    if (endsWith(result, ".git")) {
        result = result.substr(0, result.length() - 4);
    }
    
    // Get just the repository name
    slashPos = result.find_last_of('/');
    if (slashPos != std::string::npos) {
        result = result.substr(slashPos + 1);
    }
    
    return result;
}

std::string GitUtils::normalizeGitUrl(const std::string& url) {
    std::string result = trim(url);
    
    // Convert SSH to HTTPS for GitHub, GitLab, Bitbucket
    if (startsWith(result, "git@github.com:")) {
        result = "https://github.com/" + result.substr(15);
    } else if (startsWith(result, "git@gitlab.com:")) {
        result = "https://gitlab.com/" + result.substr(15);
    } else if (startsWith(result, "git@bitbucket.org:")) {
        result = "https://bitbucket.org/" + result.substr(18);
    }
    
    // Ensure .git suffix for HTTP URLs
    if ((startsWith(result, "http://") || startsWith(result, "https://")) &&
        !endsWith(result, ".git")) {
        result += ".git";
    }
    
    return result;
}

bool GitUtils::isHttpsUrl(const std::string& url) {
    return startsWith(url, "https://");
}

bool GitUtils::isSshUrl(const std::string& url) {
    return startsWith(url, "ssh://") || startsWith(url, "git@");
}

// Validation utilities
bool GitUtils::isValidEmail(const std::string& email) {
    std::regex emailRegex(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
    return std::regex_match(email, emailRegex);
}

bool GitUtils::isValidCommitMessage(const std::string& message) {
    if (message.empty() || trim(message).empty()) {
        return false;
    }
    
    // Check for reasonable length (first line should be under 50 chars ideally)
    auto lines = split(message, "\n");
    if (!lines.empty() && lines[0].length() > 72) {
        return false;  // Too long for first line
    }
    
    return true;
}

bool GitUtils::isBinaryFile(const std::string& filePath) {
    std::string ext = toLower(getFileExtension(filePath));
    return std::find(BINARY_EXTENSIONS.begin(), BINARY_EXTENSIONS.end(), ext) 
           != BINARY_EXTENSIONS.end();
}

std::string GitUtils::detectFileEncoding(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return "unknown";
    }
    
    // Read first few bytes to detect encoding
    std::vector<unsigned char> buffer(4);
    file.read(reinterpret_cast<char*>(buffer.data()), 4);
    size_t bytesRead = file.gcount();
    
    if (bytesRead >= 3) {
        // UTF-8 BOM
        if (buffer[0] == 0xEF && buffer[1] == 0xBB && buffer[2] == 0xBF) {
            return "utf-8-bom";
        }
    }
    
    if (bytesRead >= 2) {
        // UTF-16 BOM
        if ((buffer[0] == 0xFF && buffer[1] == 0xFE) ||
            (buffer[0] == 0xFE && buffer[1] == 0xFF)) {
            return "utf-16";
        }
    }
    
    // Check for binary content
    file.seekg(0);
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    
    for (char c : content) {
        if (c == 0) {
            return "binary";
        }
    }
    
    return "utf-8";
}

// Diff utilities
std::string GitUtils::colorizeGitDiff(const std::string& diff) {
    auto lines = split(diff, "\n");
    std::string result;
    
    for (const auto& line : lines) {
        if (startsWith(line, "+")) {
            result += "\033[32m" + line + "\033[0m\n";  // Green for additions
        } else if (startsWith(line, "-")) {
            result += "\033[31m" + line + "\033[0m\n";  // Red for deletions
        } else if (startsWith(line, "@@")) {
            result += "\033[36m" + line + "\033[0m\n";  // Cyan for hunk headers
        } else {
            result += line + "\n";
        }
    }
    
    return result;
}

int GitUtils::countLinesAdded(const std::string& diff) {
    auto lines = split(diff, "\n");
    int count = 0;
    
    for (const auto& line : lines) {
        if (startsWith(line, "+") && !startsWith(line, "+++")) {
            count++;
        }
    }
    
    return count;
}

int GitUtils::countLinesRemoved(const std::string& diff) {
    auto lines = split(diff, "\n");
    int count = 0;
    
    for (const auto& line : lines) {
        if (startsWith(line, "-") && !startsWith(line, "---")) {
            count++;
        }
    }
    
    return count;
}

std::string GitUtils::extractHunkHeader(const std::string& line) {
    if (startsWith(line, "@@")) {
        size_t end = line.find("@@", 2);
        if (end != std::string::npos) {
            return line.substr(0, end + 2);
        }
    }
    return line;
}

// Progress and status utilities
std::string GitUtils::formatProgress(int current, int total, const std::string& operation) {
    if (total <= 0) {
        return operation.empty() ? "Working..." : operation + "...";
    }
    
    int percentage = (current * 100) / total;
    std::string result = std::to_string(percentage) + "% (" + 
                        std::to_string(current) + "/" + std::to_string(total) + ")";
    
    if (!operation.empty()) {
        result = operation + ": " + result;
    }
    
    return result;
}

std::string GitUtils::formatTransferSpeed(size_t bytesPerSecond) {
    return formatFileSize(bytesPerSecond) + "/s";
}

std::string GitUtils::formatDuration(const std::chrono::milliseconds& duration) {
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    auto minutes = seconds / 60;
    auto hours = minutes / 60;
    
    if (hours > 0) {
        return std::to_string(hours) + "h " + 
               std::to_string(minutes % 60) + "m " + 
               std::to_string(seconds % 60) + "s";
    } else if (minutes > 0) {
        return std::to_string(minutes) + "m " + 
               std::to_string(seconds % 60) + "s";
    } else {
        return std::to_string(seconds) + "s";
    }
}

}