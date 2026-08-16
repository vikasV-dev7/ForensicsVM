#pragma once
#include "QemuLaunchSpec.hpp"
#include <expected>
#include <optional>
#include <memory>

namespace fvm::infrastructure::qemu {

class QemuProcess {
public:
    enum class Error {
        LaunchFailed,
        AlreadyRunning,
        NotRunning
    };

    virtual ~QemuProcess() = default;

    virtual std::expected<void, Error> start(const QemuLaunchSpec& spec) = 0;
    virtual std::expected<void, Error> terminate(bool force = false) = 0;
    virtual bool isRunning() const = 0;
    virtual std::optional<int> getExitCode() const = 0;
};

class WindowsQemuProcess : public QemuProcess {
public:
    // Exposed for testing
    static std::string escapeWindowsArg(const std::string& arg);
    static std::string buildWindowsCommandLine(const std::string& exePath, const std::vector<std::string>& args);

    WindowsQemuProcess();
    ~WindowsQemuProcess() override;

    std::expected<void, Error> start(const QemuLaunchSpec& spec) override;
    std::expected<void, Error> terminate(bool force = false) override;
    bool isRunning() const override;
    std::optional<int> getExitCode() const override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace fvm::infrastructure::qemu
