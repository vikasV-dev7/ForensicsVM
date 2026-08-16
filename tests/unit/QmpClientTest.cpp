#include "infrastructure/qemu/QmpClient.hpp"
#include <iostream>
#include <thread>
#include <future>
#include <windows.h>

using namespace fvm::infrastructure::qemu;

void dummyPipeServer(const std::string& pipeName, const std::vector<std::string>& responses, std::atomic<bool>& connected) {
    std::string fullPath = "\\\\.\\pipe\\" + pipeName;
    HANDLE hPipe = CreateNamedPipeA(
        fullPath.c_str(),
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1, 4096, 4096, 0, nullptr
    );

    if (hPipe == INVALID_HANDLE_VALUE) return;

    if (ConnectNamedPipe(hPipe, nullptr) || GetLastError() == ERROR_PIPE_CONNECTED) {
        connected = true;
        for (const auto& resp : responses) {
            DWORD written = 0;
            WriteFile(hPipe, resp.c_str(), static_cast<DWORD>(resp.length()), &written, nullptr);
            
            // Read next command
            char buffer[1024];
            DWORD read = 0;
            ReadFile(hPipe, buffer, sizeof(buffer), &read, nullptr);
        }
    }
    
    FlushFileBuffers(hPipe);
    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);
}

int main() {
    int failed = 0;
    
    std::string pipeName = "fvm-qmp-test-123";
    std::atomic<bool> connected = false;
    
    std::vector<std::string> responses = {
        R"({"QMP": {"version": {"qemu": {"micro": 0, "minor": 1, "major": 11}, "package": ""}, "capabilities": []}})" "\n",
        R"({"return": {}})" "\n", // response to qmp_capabilities
        R"({"event": "STOP", "timestamp": {"seconds": 12345, "microseconds": 6789}})" "\n" R"({"return": {"status": "paused"}})" "\n", // response to query-status + event
        R"({"error": {"class": "CommandNotFound", "desc": "The command does not exist"}})" "\n"
    };

    auto serverThread = std::thread(dummyPipeServer, pipeName, responses, std::ref(connected));

    // Wait a bit for pipe creation
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    QmpClient client(pipeName);
    auto res = client.connect(std::chrono::seconds(2));
    
    if (!res) {
        std::cerr << "Fail: QmpClient connect failed\n";
        failed++;
    } else {
        auto status = client.execute("query-status");
        if (!status) {
            std::cerr << "Fail: query-status failed\n";
            failed++;
        } else {
            auto events = client.pollEvents();
            if (events.size() != 1 || events[0]["event"] != "STOP") {
                std::cerr << "Fail: Event buffering failed\n";
                failed++;
            }
        }
        
        auto errorCmd = client.execute("invalid-cmd");
        if (errorCmd) {
            std::cerr << "Fail: Expected error but got success\n";
            failed++;
        }
    }

    client.disconnect();
    serverThread.join();

    if (failed == 0) {
        std::cout << "QmpClientTest PASS\n";
        return 0;
    }
    return 1;
}
