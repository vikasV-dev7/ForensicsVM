#pragma once

#include <string>
#include <vector>
#include <expected>
#include <chrono>
#include <mutex>
#include <windows.h>
#include <nlohmann/json.hpp>
#include "QmpError.hpp"

namespace fvm::infrastructure::qemu {

class QmpClient {
public:
    explicit QmpClient(const std::string& pipeName);
    ~QmpClient();

    // Prevent copying and moving to keep handle management safe
    QmpClient(const QmpClient&) = delete;
    QmpClient& operator=(const QmpClient&) = delete;
    QmpClient(QmpClient&&) = delete;
    QmpClient& operator=(QmpClient&&) = delete;

    std::expected<void, QmpError> connect(std::chrono::milliseconds timeout = std::chrono::seconds(5));
    
    std::expected<nlohmann::json, QmpError> execute(
        const std::string& command, 
        const nlohmann::json& arguments = nullptr,
        std::chrono::milliseconds timeout = std::chrono::seconds(5)
    );

    std::vector<nlohmann::json> pollEvents();
    
    void disconnect();

private:
    std::string pipeName_;
    HANDLE hPipe_;
    std::mutex mutex_;
    std::vector<nlohmann::json> pendingEvents_;
    std::string receiveBuffer_;

    std::expected<nlohmann::json, QmpError> readMessage(std::chrono::milliseconds timeout);
    bool writeMessage(const nlohmann::json& msg, std::chrono::milliseconds timeout);
};

} // namespace fvm::infrastructure::qemu
