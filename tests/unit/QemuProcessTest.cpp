#include "infrastructure/qemu/QemuProcess.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#endif

using namespace fvm::infrastructure::qemu;

bool isProcessRunning(const std::string& processName) {
#ifdef _WIN32
    bool exists = false;
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (Process32First(snapshot, &entry)) {
        while (Process32Next(snapshot, &entry)) {
            if (processName == entry.szExeFile) {
                exists = true;
                break;
            }
        }
    }
    CloseHandle(snapshot);
    return exists;
#else
    return false;
#endif
}

void runChildHelper() {
    WindowsQemuProcess process;
    QemuLaunchSpec spec;
    // Ping for 10 seconds (enough time for parent to check)
    spec.executablePath = "C:\\Windows\\System32\\ping.exe";
    spec.arguments = {"127.0.0.1", "-n", "10"};
    
    if (process.start(spec)) {
        // hard exit without running destructors
        _exit(0);
    }
    _exit(1);
}

int main(int argc, char* argv[]) {
    if (argc > 1 && std::string(argv[1]) == "child") {
        runChildHelper();
        return 0; // unreachable
    }

    int failed = 0;

    {
        WindowsQemuProcess process;
        QemuLaunchSpec spec;
        spec.executablePath = "C:\\Windows\\System32\\ping.exe";
        spec.arguments = {"127.0.0.1", "-n", "2"};

        auto res = process.start(spec);
        if (!res) {
            std::cerr << "Fail: Launch failed\n";
            failed++;
        } else {
            if (!process.isRunning()) {
                std::cerr << "Fail: Process should be running immediately after launch\n";
                failed++;
            }
            
            // Explicit termination works
            process.terminate(true);
            if (process.isRunning()) {
                std::cerr << "Fail: Process should not be running after termination\n";
                failed++;
            }
            
            // Repeated termination is safe
            auto res2 = process.terminate(true);
            if (res2) {
                std::cerr << "Fail: Repeated termination should return NotRunning\n";
                failed++;
            }
        }
    }

    // Test Job Object Containment using the helper
    {
        STARTUPINFOA si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        
        PROCESS_INFORMATION pi;
        ZeroMemory(&pi, sizeof(pi));
        std::string cmdLine = std::string(argv[0]) + " child";
        std::vector<char> cmdBuffer(cmdLine.begin(), cmdLine.end());
        cmdBuffer.push_back('\0');

        if (CreateProcessA(nullptr, cmdBuffer.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
            WaitForSingleObject(pi.hProcess, 5000);
            
            // wait a little bit for the OS to kill the ping process via job object
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            if (isProcessRunning("ping.exe")) {
                std::cerr << "Fail: ping.exe leaked out of the Job Object!\n";
                failed++;
                
                // cleanup just in case
                system("taskkill /f /im ping.exe >nul 2>&1");
            }
            
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        } else {
            std::cerr << "Fail: Could not start helper process\n";
            failed++;
        }
    }

    if (failed == 0) {
        std::cout << "QemuProcessTest PASS\n";
        return 0;
    }
    return 1;
}
