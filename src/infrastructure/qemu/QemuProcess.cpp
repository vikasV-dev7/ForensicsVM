#include "QemuProcess.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

#include <string>
#include <vector>

namespace fvm::infrastructure::qemu {

#ifdef _WIN32

std::string WindowsQemuProcess::escapeWindowsArg(const std::string& arg) {
    if (arg.empty()) {
        return "\"\"";
    }

    bool needsQuotes = arg.find_first_of(" \t\n\v\"") != std::string::npos;
    if (!needsQuotes) {
        return arg;
    }

    std::string escaped = "\"";
    int backslashCount = 0;

    for (char c : arg) {
        if (c == '\\') {
            backslashCount++;
        } else if (c == '"') {
            escaped.append(backslashCount * 2 + 1, '\\');
            escaped.push_back('"');
            backslashCount = 0;
        } else {
            escaped.append(backslashCount, '\\');
            escaped.push_back(c);
            backslashCount = 0;
        }
    }

    escaped.append(backslashCount * 2, '\\');
    escaped.push_back('"');

    return escaped;
}

std::string WindowsQemuProcess::buildWindowsCommandLine(const std::string& exePath, const std::vector<std::string>& args) {
    std::string cmdLine = escapeWindowsArg(exePath);
    for (const auto& arg : args) {
        cmdLine += " " + escapeWindowsArg(arg);
    }
    return cmdLine;
}

struct WindowsQemuProcess::Impl {
    HANDLE hProcess = nullptr;
    HANDLE hThread = nullptr;
    HANDLE hJob = nullptr;
    DWORD processId = 0;

    ~Impl() {
        closeHandles();
    }

    void closeHandles() {
        if (hProcess) {
            CloseHandle(hProcess);
            hProcess = nullptr;
        }
        if (hThread) {
            CloseHandle(hThread);
            hThread = nullptr;
        }
        if (hJob) {
            CloseHandle(hJob);
            hJob = nullptr;
        }
    }
};

WindowsQemuProcess::WindowsQemuProcess() : impl_(std::make_unique<Impl>()) {}
WindowsQemuProcess::~WindowsQemuProcess() {
    if (isRunning()) {
        terminate(true);
    }
}

std::expected<void, QemuProcess::Error> WindowsQemuProcess::start(const QemuLaunchSpec& spec) {
    if (isRunning()) {
        return std::unexpected(Error::AlreadyRunning);
    }

    impl_->closeHandles();

    impl_->hJob = CreateJobObjectA(nullptr, nullptr);
    if (!impl_->hJob) {
        return std::unexpected(Error::LaunchFailed);
    }

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli;
    ZeroMemory(&jeli, sizeof(jeli));
    jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(impl_->hJob, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli))) {
        impl_->closeHandles();
        return std::unexpected(Error::LaunchFailed);
    }

    std::string commandLine = buildWindowsCommandLine(spec.executablePath, spec.arguments);

    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    std::vector<char> cmdBuffer(commandLine.begin(), commandLine.end());
    cmdBuffer.push_back('\0');

    // Create process suspended to prevent race condition during assignment
    if (!CreateProcessA(
            spec.executablePath.c_str(),
            cmdBuffer.data(),
            nullptr,
            nullptr,
            FALSE,
            CREATE_NO_WINDOW | CREATE_SUSPENDED,
            nullptr,
            nullptr,
            &si,
            &pi)) {
        impl_->closeHandles();
        return std::unexpected(Error::LaunchFailed);
    }

    impl_->hProcess = pi.hProcess;
    impl_->hThread = pi.hThread;
    impl_->processId = pi.dwProcessId;

    if (!AssignProcessToJobObject(impl_->hJob, impl_->hProcess)) {
        TerminateProcess(impl_->hProcess, 1);
        impl_->closeHandles();
        return std::unexpected(Error::LaunchFailed);
    }

    ResumeThread(impl_->hThread);

    return {};
}

std::expected<void, QemuProcess::Error> WindowsQemuProcess::terminate(bool /*force*/) {
    if (!isRunning()) {
        return std::unexpected(Error::NotRunning);
    }
    
    // For Phase 2B, we use forced termination as requested because graceful QMP is deferred.
    TerminateProcess(impl_->hProcess, 1);
    WaitForSingleObject(impl_->hProcess, INFINITE);
    
    return {};
}

bool WindowsQemuProcess::isRunning() const {
    if (!impl_->hProcess) return false;
    DWORD exitCode = 0;
    if (GetExitCodeProcess(impl_->hProcess, &exitCode)) {
        return exitCode == STILL_ACTIVE;
    }
    return false;
}

std::optional<int> WindowsQemuProcess::getExitCode() const {
    if (!impl_->hProcess) return std::nullopt;
    DWORD exitCode = 0;
    if (GetExitCodeProcess(impl_->hProcess, &exitCode)) {
        if (exitCode != STILL_ACTIVE) {
            return static_cast<int>(exitCode);
        }
    }
    return std::nullopt;
}

#else
// Fallback for non-Windows platforms (compilation fallback only)
struct WindowsQemuProcess::Impl {};
WindowsQemuProcess::WindowsQemuProcess() : impl_(std::make_unique<Impl>()) {}
WindowsQemuProcess::~WindowsQemuProcess() = default;
std::expected<void, QemuProcess::Error> WindowsQemuProcess::start(const QemuLaunchSpec&) { return std::unexpected(Error::LaunchFailed); }
std::expected<void, QemuProcess::Error> WindowsQemuProcess::terminate(bool) { return std::unexpected(Error::NotRunning); }
bool WindowsQemuProcess::isRunning() const { return false; }
std::optional<int> WindowsQemuProcess::getExitCode() const { return std::nullopt; }
#endif

} // namespace fvm::infrastructure::qemu
