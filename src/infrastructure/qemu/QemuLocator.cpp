#include "QemuLocator.hpp"
#include <filesystem>
#include <array>
#include <memory>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fvm::infrastructure::qemu {

std::expected<std::string, QemuLocator::Error> DefaultQemuLocator::discover(const std::optional<std::string>& explicitPath) const {
    if (explicitPath) {
        if (std::filesystem::exists(*explicitPath)) {
            auto val = validate(*explicitPath);
            if (val) return *explicitPath;
            return std::unexpected(val.error());
        }
        return std::unexpected(Error::NotFound);
    }

    auto pathExec = findInPath();
    if (pathExec) {
        auto val = validate(*pathExec);
        if (val) return *pathExec;
    }
    
    return std::unexpected(Error::NotFound);
}

std::expected<void, QemuLocator::Error> DefaultQemuLocator::validate(const std::string& path) const {
#ifdef _WIN32
    HANDLE hReadPipe = nullptr;
    HANDLE hWritePipe = nullptr;

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        return std::unexpected(Error::ExecutionFailed);
    }

    // Ensure read handle is not inherited
    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdError = hWritePipe;
    si.hStdOutput = hWritePipe;
    si.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    // Proper quoting for CreateProcess
    std::string cmd = "\"" + path + "\" --version";
    std::vector<char> cmdBuffer(cmd.begin(), cmd.end());
    cmdBuffer.push_back('\0');

    if (!CreateProcessA(
            path.c_str(),
            cmdBuffer.data(),
            nullptr,
            nullptr,
            TRUE,
            CREATE_NO_WINDOW,
            nullptr,
            nullptr,
            &si,
            &pi)) {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return std::unexpected(Error::ExecutionFailed);
    }

    CloseHandle(hWritePipe); // Close our copy so we can read until EOF

    std::string result;
    std::array<char, 128> buffer;
    DWORD bytesRead;

    while (ReadFile(hReadPipe, buffer.data(), buffer.size() - 1, &bytesRead, nullptr) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        result += buffer.data();
    }

    CloseHandle(hReadPipe);
    WaitForSingleObject(pi.hProcess, 5000); // 5 sec timeout

    DWORD exitCode;
    if (!GetExitCodeProcess(pi.hProcess, &exitCode) || exitCode != 0) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return std::unexpected(Error::ExecutionFailed);
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (result.find("QEMU emulator version") == std::string::npos && 
        result.find("qemu-system-") == std::string::npos) {
        return std::unexpected(Error::InvalidVersion);
    }
    
    return {};
#else
    return std::unexpected(Error::ExecutionFailed);
#endif
}

std::optional<std::string> DefaultQemuLocator::findInPath() const {
#ifdef _WIN32
    char buffer[MAX_PATH];
    char* filePart;
    DWORD res = SearchPathA(nullptr, "qemu-system-x86_64.exe", nullptr, MAX_PATH, buffer, &filePart);
    if (res > 0 && res < MAX_PATH) {
        return std::string(buffer);
    }
    return std::nullopt;
#else
    return std::nullopt;
#endif
}

} // namespace fvm::infrastructure::qemu
