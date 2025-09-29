#include "SystemCommand.h"
#include <cstdlib>
#include <future>
#include <iostream>
#include <map>
#include <sstream>
#include <thread>

#ifdef _WIN32
#include <process.h>
#include <windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace VersionTools {

class SystemCommand::Impl {
  public:
    std::map<std::string, std::string> environmentVariables;
    int timeoutMs = 30000; // 30 seconds default
    bool cancelled = false;

#ifdef _WIN32
    HANDLE process = INVALID_HANDLE_VALUE;
    HANDLE thread = INVALID_HANDLE_VALUE;
#else
    pid_t childPid = -1;
#endif

    std::string buildCommandLine(const std::string& command, const std::vector<std::string>& args) {
        std::string cmdLine = command;
        for (const auto& arg : args) {
            cmdLine += " ";
            // Quote arguments that contain spaces
            if (arg.find(' ') != std::string::npos) {
                cmdLine += "\"" + arg + "\"";
            } else {
                cmdLine += arg;
            }
        }
        return cmdLine;
    }

    std::vector<std::string> buildArgVector(const std::string& command, const std::vector<std::string>& args) {
        std::vector<std::string> argv;
        argv.push_back(command);
        argv.insert(argv.end(), args.begin(), args.end());
        return argv;
    }
};

SystemCommand::SystemCommand() : pImpl(std::make_unique<Impl>()) {}

SystemCommand::~SystemCommand() {
    cancel();
}

SystemCommandResult SystemCommand::execute(const std::string& command, const std::vector<std::string>& args,
                                           const std::string& workingDirectory) {
    pImpl->cancelled = false;

#ifdef _WIN32
    return executeWindows(command, args, workingDirectory);
#else
    return executeUnix(command, args, workingDirectory);
#endif
}

#ifdef _WIN32
SystemCommandResult SystemCommand::executeWindows(const std::string& command, const std::vector<std::string>& args,
                                                  const std::string& workingDirectory) {
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    HANDLE hStdoutRead, hStdoutWrite;
    HANDLE hStderrRead, hStderrWrite;

    // Create pipes for stdout and stderr
    if (!CreatePipe(&hStdoutRead, &hStdoutWrite, &sa, 0) || !CreatePipe(&hStderrRead, &hStderrWrite, &sa, 0)) {
        return {-1, "", "Failed to create pipes", false};
    }

    // Make sure the read handles are not inherited
    SetHandleInformation(hStdoutRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hStderrRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdError = hStderrWrite;
    si.hStdOutput = hStdoutWrite;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.dwFlags |= STARTF_USESTDHANDLES;

    std::string cmdLine = pImpl->buildCommandLine(command, args);

    // Set environment variables
    std::string envBlock;
    if (!pImpl->environmentVariables.empty()) {
        for (const auto& [key, value] : pImpl->environmentVariables) {
            envBlock += key + "=" + value + '\0';
        }
        envBlock += '\0';
    }

    BOOL success = CreateProcess(NULL, const_cast<char*>(cmdLine.c_str()), NULL, NULL, TRUE, 0,
                                 envBlock.empty() ? NULL : const_cast<char*>(envBlock.c_str()),
                                 workingDirectory.empty() ? NULL : workingDirectory.c_str(), &si, &pi);

    CloseHandle(hStdoutWrite);
    CloseHandle(hStderrWrite);

    if (!success) {
        CloseHandle(hStdoutRead);
        CloseHandle(hStderrRead);
        return {-1, "", "Failed to create process", false};
    }

    pImpl->process = pi.hProcess;
    pImpl->thread = pi.hThread;

    // Read output
    std::string output, error;
    DWORD bytesRead;
    char buffer[4096];

    // Wait for process completion with timeout
    DWORD waitResult = WaitForSingleObject(pi.hProcess, pImpl->timeoutMs);

    if (waitResult == WAIT_TIMEOUT || pImpl->cancelled) {
        TerminateProcess(pi.hProcess, -1);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(hStdoutRead);
        CloseHandle(hStderrRead);
        return {-1, "", "Process timed out or was cancelled", false};
    }

    // Read stdout
    while (ReadFile(hStdoutRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        output += buffer;
    }

    // Read stderr
    while (ReadFile(hStderrRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        error += buffer;
    }

    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hStdoutRead);
    CloseHandle(hStderrRead);

    pImpl->process = INVALID_HANDLE_VALUE;
    pImpl->thread = INVALID_HANDLE_VALUE;

    return {static_cast<int>(exitCode), output, error};
}

#else // Unix/Linux/macOS

SystemCommandResult SystemCommand::executeUnix(const std::string& command, const std::vector<std::string>& args,
                                               const std::string& workingDirectory) {
    int pipeOut[2], pipeErr[2];

    if (pipe(pipeOut) == -1 || pipe(pipeErr) == -1) {
        return {-1, "", "Failed to create pipes"};
    }

    pid_t pid = fork();
    if (pid == -1) {
        close(pipeOut[0]);
        close(pipeOut[1]);
        close(pipeErr[0]);
        close(pipeErr[1]);
        return {-1, "", "Failed to fork process"};
    }

    if (pid == 0) {
        // Child process
        close(pipeOut[0]);
        close(pipeErr[0]);

        dup2(pipeOut[1], STDOUT_FILENO);
        dup2(pipeErr[1], STDERR_FILENO);

        close(pipeOut[1]);
        close(pipeErr[1]);

        // Change working directory if specified
        if (!workingDirectory.empty()) {
            if (chdir(workingDirectory.c_str()) != 0) {
                exit(EXIT_FAILURE);
            }
        }

        // Set environment variables
        for (const auto& [key, value] : pImpl->environmentVariables) {
            setenv(key.c_str(), value.c_str(), 1);
        }

        // Prepare arguments
        auto argv = pImpl->buildArgVector(command, args);
        std::vector<char*> cArgs;
        for (const auto& arg : argv) {
            cArgs.push_back(const_cast<char*>(arg.c_str()));
        }
        cArgs.push_back(nullptr);

        execvp(command.c_str(), cArgs.data());
        exit(EXIT_FAILURE);
    }

    // Parent process
    pImpl->childPid = pid;
    close(pipeOut[1]);
    close(pipeErr[1]);

    // Set non-blocking mode for reading
    fcntl(pipeOut[0], F_SETFL, O_NONBLOCK);
    fcntl(pipeErr[0], F_SETFL, O_NONBLOCK);

    std::string output, error;
    char buffer[4096];

    auto startTime = std::chrono::steady_clock::now();
    auto timeout = std::chrono::milliseconds(pImpl->timeoutMs);

    while (true) {
        if (pImpl->cancelled) {
            kill(pid, SIGTERM);
            break;
        }

        // Check timeout
        auto elapsed = std::chrono::steady_clock::now() - startTime;
        if (elapsed > timeout) {
            kill(pid, SIGTERM);
            waitpid(pid, nullptr, 0); // Clean up zombie
            close(pipeOut[0]);
            close(pipeErr[0]);
            pImpl->childPid = -1;
            return {-1, "", "Process timed out"};
        }

        // Check if process is still running
        int status;
        pid_t result = waitpid(pid, &status, WNOHANG);

        // Read from pipes
        ssize_t bytesRead;
        while ((bytesRead = read(pipeOut[0], buffer, sizeof(buffer))) > 0) {
            output.append(buffer, bytesRead);
        }

        while ((bytesRead = read(pipeErr[0], buffer, sizeof(buffer))) > 0) {
            error.append(buffer, bytesRead);
        }

        if (result == pid) {
            // Process has finished
            // Final read to get any remaining output
            ssize_t bytesRead;
            while ((bytesRead = read(pipeOut[0], buffer, sizeof(buffer))) > 0) {
                output.append(buffer, bytesRead);
            }

            while ((bytesRead = read(pipeErr[0], buffer, sizeof(buffer))) > 0) {
                error.append(buffer, bytesRead);
            }

            close(pipeOut[0]);
            close(pipeErr[0]);

            pImpl->childPid = -1;

            int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
            return {exitCode, output, error};
        }

        // Small delay to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Should not reach here, but handle cleanup if it does
    close(pipeOut[0]);
    close(pipeErr[0]);
    pImpl->childPid = -1;
    return {-1, output, error};
}
#endif

SystemCommandResult SystemCommand::executeWithCallback(const std::string& command, const std::vector<std::string>& args,
                                                       OutputCallback outputCallback,
                                                       const std::string& workingDirectory) {
    // For now, execute normally and call callback with full output
    // TODO: Implement real-time streaming
    auto result = execute(command, args, workingDirectory);
    if (outputCallback) {
        outputCallback(result.output);
        if (!result.error.empty()) {
            outputCallback(result.error);
        }
    }
    return result;
}

void SystemCommand::executeAsync(const std::string& command, const std::vector<std::string>& args,
                                 std::function<void(const SystemCommandResult&)> callback,
                                 const std::string& workingDirectory) {
    std::thread([this, command, args, callback, workingDirectory]() {
        auto result = execute(command, args, workingDirectory);
        if (callback) {
            callback(result);
        }
    }).detach();
}

void SystemCommand::cancel() {
    pImpl->cancelled = true;

#ifdef _WIN32
    if (pImpl->process != INVALID_HANDLE_VALUE) {
        TerminateProcess(pImpl->process, -1);
    }
#else
    if (pImpl->childPid != -1) {
        kill(pImpl->childPid, SIGTERM);
    }
#endif
}

void SystemCommand::setEnvironmentVariable(const std::string& name, const std::string& value) {
    pImpl->environmentVariables[name] = value;
}

void SystemCommand::clearEnvironmentVariables() {
    pImpl->environmentVariables.clear();
}

void SystemCommand::setTimeout(int timeoutMs) {
    pImpl->timeoutMs = timeoutMs;
}

bool SystemCommand::isCommandAvailable(const std::string& command) {
#ifdef _WIN32
    std::string cmd = "where " + command + " >nul 2>&1";
    return system(cmd.c_str()) == 0;
#else
    std::string cmd = "command -v " + command + " >/dev/null 2>&1";
    return system(cmd.c_str()) == 0;
#endif
}

std::string SystemCommand::getGitCommand() {
    if (isCommandAvailable("git")) {
        return "git";
    }

#ifdef _WIN32
    // Try common Git installation paths on Windows
    std::vector<std::string> paths = {"C:\\Program Files\\Git\\bin\\git.exe",
                                      "C:\\Program Files (x86)\\Git\\bin\\git.exe", "C:\\Git\\bin\\git.exe"};

    for (const auto& path : paths) {
        if (GetFileAttributes(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
            return path;
        }
    }
#endif

    return "git"; // Fallback
}

} // namespace VersionTools