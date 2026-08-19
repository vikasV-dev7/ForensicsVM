#include "QemuImageTool.hpp"
#include "../QemuProcess.hpp"
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fvm::infrastructure::qemu::image {

QemuImageTool::QemuImageTool(std::unique_ptr<QemuImgLocator> locator)
    : locator_(std::move(locator)), initStatus_(std::unexpected(Error::ToolNotFound)) {}

std::expected<void, IQemuImageTool::Error> QemuImageTool::initialize() const {
    std::call_once(initFlag_, [this]() {
        auto res = locator_->discover();
        if (res) {
            executablePath_ = res.value();
            initStatus_ = {};
        } else {
            initStatus_ = std::unexpected(Error::ToolNotFound);
        }
    });
    return initStatus_;
}

std::string QemuImageTool::formatToString(domain::DiskFormat format) const {
    switch (format) {
        case domain::DiskFormat::Raw: return "raw";
        case domain::DiskFormat::Qcow2: return "qcow2";
        case domain::DiskFormat::Vhdx: return "vhdx";
        case domain::DiskFormat::Vmdk: return "vmdk";
        case domain::DiskFormat::Elf: return "elf";
    }
    return "raw";
}

std::expected<void, IQemuImageTool::Error> QemuImageTool::createOverlay(
    const std::filesystem::path& evidencePath, 
    domain::DiskFormat evidenceFormat, 
    const std::filesystem::path& overlayPath) const 
{
    auto initRes = initialize();
    if (!initRes) return initRes;

    // Validate absolute paths
    if (!evidencePath.is_absolute() || !overlayPath.is_absolute()) {
        return std::unexpected(Error::InvalidPath);
    }
    
    // Prevent aliasing
    if (evidencePath == overlayPath) {
        return std::unexpected(Error::InvalidPath);
    }

#ifdef _WIN32
    std::vector<std::string> args = {
        "create",
        "-f", "qcow2",
        "-F", formatToString(evidenceFormat),
        "-b", evidencePath.string(),
        overlayPath.string()
    };

    std::string cmd = WindowsQemuProcess::buildWindowsCommandLine(executablePath_, args);
    std::vector<char> cmdBuffer(cmd.begin(), cmd.end());
    cmdBuffer.push_back('\0');

    // Create a Job Object for containment in case ForensicVM crashes during creation
    HANDLE hJob = CreateJobObjectA(nullptr, nullptr);
    if (!hJob) return std::unexpected(Error::ExecutionFailed);

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli;
    ZeroMemory(&jeli, sizeof(jeli));
    jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli));

    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    SECURITY_ATTRIBUTES saNul;
    ZeroMemory(&saNul, sizeof(saNul));
    saNul.nLength = sizeof(saNul);
    saNul.bInheritHandle = TRUE;

    // NUL redirection to prevent inherited handles from causing issues
    HANDLE hNul = CreateFileA("NUL", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &saNul, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hNul != INVALID_HANDLE_VALUE) {
        si.hStdOutput = hNul;
        si.hStdError = hNul;
        si.dwFlags |= STARTF_USESTDHANDLES;
    }

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessA(
            executablePath_.c_str(),
            cmdBuffer.data(),
            nullptr,
            nullptr,
            TRUE,
            CREATE_NO_WINDOW | CREATE_SUSPENDED,
            nullptr,
            nullptr,
            &si,
            &pi)) {
        if (hNul != INVALID_HANDLE_VALUE) CloseHandle(hNul);
        CloseHandle(hJob);
        return std::unexpected(Error::ExecutionFailed);
    }

    if (hNul != INVALID_HANDLE_VALUE) CloseHandle(hNul);

    AssignProcessToJobObject(hJob, pi.hProcess);
    ResumeThread(pi.hThread);

    // Wait for qemu-img to finish
    DWORD waitResult = WaitForSingleObject(pi.hProcess, 30000); // 30 sec timeout for overlay creation
    
    DWORD exitCode = 1;
    if (waitResult == WAIT_OBJECT_0) {
        GetExitCodeProcess(pi.hProcess, &exitCode);
    } else {
        // Timeout or error, forcefully terminate
        TerminateProcess(pi.hProcess, 1);
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hJob);

    if (exitCode != 0) {
        return std::unexpected(Error::ExecutionFailed);
    }

    return {};
#else
    return std::unexpected(Error::ExecutionFailed);
#endif
}

} // namespace fvm::infrastructure::qemu::image
