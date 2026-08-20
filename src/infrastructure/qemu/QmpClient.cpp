#include "QmpClient.hpp"
#include <iostream>
#include <thread>
#include <thread>

namespace fvm::infrastructure::qemu {

QmpClient::QmpClient(const std::string& pipeName)
    : pipeName_("\\\\.\\pipe\\" + pipeName), hPipe_(INVALID_HANDLE_VALUE) {}

QmpClient::~QmpClient() {
    disconnect();
}

void QmpClient::disconnect() {
    if (hPipe_ != INVALID_HANDLE_VALUE) {
        CloseHandle(hPipe_);
        hPipe_ = INVALID_HANDLE_VALUE;
    }
}

std::expected<void, QmpError> QmpClient::connect(std::chrono::milliseconds timeout) {
    auto start = std::chrono::steady_clock::now();
    
    while (true) {
        hPipe_ = CreateFileA(
            pipeName_.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED,
            nullptr
        );

        if (hPipe_ != INVALID_HANDLE_VALUE) {
            break;
        }

        if (GetLastError() != ERROR_PIPE_BUSY && GetLastError() != ERROR_FILE_NOT_FOUND) {
            return std::unexpected(QmpError::ConnectionFailed);
        }

        auto now = std::chrono::steady_clock::now();
        if (now - start > timeout) {
            return std::unexpected(QmpError::Timeout);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Read greeting
    auto greeting = readMessage(timeout);
    if (!greeting) {
        disconnect();
        return std::unexpected(greeting.error());
    }

    if (!greeting->contains("QMP")) {
        disconnect();
        return std::unexpected(QmpError::ProtocolError);
    }

    // Execute capabilities
    auto caps = execute("qmp_capabilities", nullptr, timeout);
    if (!caps) {
        disconnect();
        return std::unexpected(caps.error());
    }

    return {};
}

std::expected<nlohmann::json, QmpError> QmpClient::execute(
    const std::string& command, 
    const nlohmann::json& arguments,
    std::chrono::milliseconds timeout,
    std::stop_token stoken) 
{
    std::lock_guard<std::mutex> lock(mutex_);

    nlohmann::json cmd;
    cmd["execute"] = command;
    if (arguments != nullptr && !arguments.is_null()) {
        cmd["arguments"] = arguments;
    }

    std::cout << "[QMP SEND] " << cmd.dump() << "\n";

    if (!writeMessage(cmd, timeout, stoken)) {
        return std::unexpected(QmpError::ConnectionLost);
    }

    auto start = std::chrono::steady_clock::now();
    while (true) {
        if (stoken.stop_requested()) return std::unexpected(QmpError::CommandFailed);
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
        if (elapsed > timeout) {
            return std::unexpected(QmpError::Timeout);
        }

        auto msg = readMessage(timeout - elapsed, stoken);
        if (!msg) {
            if (msg.error() == QmpError::Timeout && stoken.stop_requested()) {
                return std::unexpected(QmpError::CommandFailed);
            }
            std::cout << "[QMP RECV ERROR] " << static_cast<int>(msg.error()) << "\n";
            return std::unexpected(msg.error());
        }

        std::cout << "[QMP RECV] " << msg->dump() << "\n";

        if (msg->contains("return")) {
            return *msg;
        } else if (msg->contains("error")) {
            return std::unexpected(QmpError::CommandFailed);
        } else if (msg->contains("event")) {
            pendingEvents_.push_back(*msg);
        } else {
            return std::unexpected(QmpError::InvalidResponse);
        }
    }
}

std::vector<nlohmann::json> QmpClient::pollEvents() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<nlohmann::json> events = std::move(pendingEvents_);
    pendingEvents_.clear();
    return events;
}

bool QmpClient::writeMessage(const nlohmann::json& msg, std::chrono::milliseconds timeout, std::stop_token stoken) {
    std::string payload = msg.dump() + "\n";
    OVERLAPPED overlapped{};
    overlapped.hEvent = CreateEventA(nullptr, TRUE, FALSE, nullptr);
    if (!overlapped.hEvent) {
        return false;
    }

    DWORD bytesWritten = 0;
    bool result = WriteFile(hPipe_, payload.c_str(), static_cast<DWORD>(payload.length()), &bytesWritten, &overlapped);
    if (!result && GetLastError() == ERROR_IO_PENDING) {
        DWORD waitTime = static_cast<DWORD>(timeout.count());
        DWORD totalWait = 0;
        DWORD waitRes = WAIT_TIMEOUT;
        while (totalWait < waitTime) {
            if (stoken.stop_requested()) {
                CancelIo(hPipe_);
                GetOverlappedResult(hPipe_, &overlapped, &bytesWritten, TRUE);
                CloseHandle(overlapped.hEvent);
                return false;
            }
            DWORD chunk = std::min<DWORD>(50, waitTime - totalWait);
            waitRes = WaitForSingleObject(overlapped.hEvent, chunk);
            if (waitRes == WAIT_OBJECT_0) {
                break;
            }
            totalWait += chunk;
        }
        
        if (waitRes == WAIT_OBJECT_0) {
            result = GetOverlappedResult(hPipe_, &overlapped, &bytesWritten, FALSE);
        } else {
            CancelIo(hPipe_);
            GetOverlappedResult(hPipe_, &overlapped, &bytesWritten, TRUE);
        }
    }

    CloseHandle(overlapped.hEvent);
    return result && bytesWritten == payload.length();
}

std::expected<nlohmann::json, QmpError> QmpClient::readMessage(std::chrono::milliseconds timeout, std::stop_token stoken) {
    auto start = std::chrono::steady_clock::now();

    char buffer[4096];
    while (true) {
        size_t newlinePos = receiveBuffer_.find('\n');
        if (newlinePos != std::string::npos) {
            std::string line = receiveBuffer_.substr(0, newlinePos);
            receiveBuffer_.erase(0, newlinePos + 1);
            try {
                return nlohmann::json::parse(line);
            } catch (...) {
                return std::unexpected(QmpError::InvalidResponse);
            }
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
        if (elapsed >= timeout) {
            return std::unexpected(QmpError::Timeout);
        }

        OVERLAPPED overlapped{};
        overlapped.hEvent = CreateEventA(nullptr, TRUE, FALSE, nullptr);
        if (!overlapped.hEvent) {
            return std::unexpected(QmpError::ConnectionFailed);
        }

        DWORD bytesRead = 0;
        bool result = ReadFile(hPipe_, buffer, sizeof(buffer), &bytesRead, &overlapped);
        if (!result) {
            if (GetLastError() == ERROR_IO_PENDING) {
                DWORD waitTime = static_cast<DWORD>((timeout - elapsed).count());
                DWORD totalWait = 0;
                DWORD waitRes = WAIT_TIMEOUT;
                while (totalWait < waitTime) {
                    if (stoken.stop_requested()) {
                        CancelIo(hPipe_);
                        GetOverlappedResult(hPipe_, &overlapped, &bytesRead, TRUE);
                        CloseHandle(overlapped.hEvent);
                        return std::unexpected(QmpError::Timeout);
                    }
                    DWORD chunk = std::min<DWORD>(50, waitTime - totalWait);
                    waitRes = WaitForSingleObject(overlapped.hEvent, chunk);
                    if (waitRes == WAIT_OBJECT_0) {
                        break;
                    }
                    totalWait += chunk;
                }
                
                if (waitRes == WAIT_OBJECT_0) {
                    if (!GetOverlappedResult(hPipe_, &overlapped, &bytesRead, FALSE)) {
                        CloseHandle(overlapped.hEvent);
                        return std::unexpected(QmpError::ConnectionLost);
                    }
                } else {
                    CancelIo(hPipe_);
                    GetOverlappedResult(hPipe_, &overlapped, &bytesRead, TRUE);
                    CloseHandle(overlapped.hEvent);
                    return std::unexpected(QmpError::Timeout);
                }
            } else {
                CloseHandle(overlapped.hEvent);
                return std::unexpected(QmpError::ConnectionLost);
            }
        }

        CloseHandle(overlapped.hEvent);
        if (bytesRead > 0) {
            receiveBuffer_.append(buffer, bytesRead);
        } else {
            return std::unexpected(QmpError::ConnectionLost);
        }
    }
}

} // namespace fvm::infrastructure::qemu
