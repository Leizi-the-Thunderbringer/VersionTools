#pragma once

#include <string>
#include <vector>
#include <functional>

namespace VersionTools {

struct SystemCommandResult {
    int exitCode;
    std::string output;
    std::string error;
    bool success() const { return exitCode == 0; }
};

using OutputCallback = std::function<void(const std::string&)>;

class SystemCommand {
public:
    SystemCommand();
    ~SystemCommand();
    
    // Execute command synchronously
    SystemCommandResult execute(const std::string& command, 
                               const std::vector<std::string>& args = {},
                               const std::string& workingDirectory = "");
    
    // Execute command with real-time output callback
    SystemCommandResult executeWithCallback(const std::string& command,
                                           const std::vector<std::string>& args,
                                           OutputCallback outputCallback,
                                           const std::string& workingDirectory = "");
    
    // Execute command asynchronously
    void executeAsync(const std::string& command,
                     const std::vector<std::string>& args,
                     std::function<void(const SystemCommandResult&)> callback,
                     const std::string& workingDirectory = "");
    
    // Cancel running command
    void cancel();
    
    // Set environment variables
    void setEnvironmentVariable(const std::string& name, const std::string& value);
    void clearEnvironmentVariables();
    
    // Set timeout (in milliseconds)
    void setTimeout(int timeoutMs);
    
    // Check if command is available
    static bool isCommandAvailable(const std::string& command);
    
    // Get system command for git
    static std::string getGitCommand();
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;

#ifdef _WIN32
    SystemCommandResult executeWindows(const std::string& command,
                                       const std::vector<std::string>& args,
                                       const std::string& workingDirectory);
#else
    SystemCommandResult executeUnix(const std::string& command,
                                    const std::vector<std::string>& args,
                                    const std::string& workingDirectory);
#endif
};

}