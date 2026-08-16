#include "infrastructure/qemu/QemuBackend.hpp"
#include "infrastructure/qemu/QemuLocator.hpp"
#include "infrastructure/qemu/QemuCommandBuilder.hpp"
#include "infrastructure/qemu/image/QemuImageTool.hpp"
#include "vm/domain/VmConfig.hpp"
#include "vm/domain/VmId.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace fvm::infrastructure::qemu;
using namespace fvm::domain;

int main() {
    int failed = 0;

    auto locator = std::make_unique<DefaultQemuLocator>();
    auto cmdBuilder = std::make_unique<DefaultQemuCommandBuilder>();
    auto imgLocator = std::make_unique<image::DefaultQemuImgLocator>();
    auto imgTool = std::make_unique<image::QemuImageTool>(std::move(imgLocator));
    QemuBackend backend(std::move(locator), std::move(cmdBuilder), std::move(imgTool));

    VmId id("qmp-integration-test");
    VmConfig config{
        id,
        "TestVM",
        "",
        CpuConfig{CpuCount(1), 1, 1, 1},
        MemoryConfig{Megabytes(512)},
        {}, {}, FirmwareConfig{}, DisplayConfig{0, 0, false}
    };

    if (!backend.createVm(config)) {
        std::cerr << "Fail: createVm\n";
        return 1;
    }

    if (!backend.startVm(id)) {
        std::cerr << "Fail: startVm\n";
        return 1;
    }

    auto state = backend.queryState(id);
    if (!state || *state != VmState::Running) {
        std::cerr << "Fail: VM should be running\n";
        failed++;
    }

    if (!backend.pauseVm(id)) {
        std::cerr << "Fail: pauseVm\n";
        failed++;
    }

    auto res = backend.resumeVm(id);
    if (!res) {
        std::cerr << "Fail: resumeVm, error=" << static_cast<int>(res.error()) << "\n";
        failed++;
    }

    state = backend.queryState(id);
    if (!state || *state != VmState::Running) {
        std::cerr << "Fail: VM should be running again\n";
        failed++;
    }

    // Try graceful shutdown (which doesn't do anything because no OS, but shouldn't error QMP)
    auto sdRes = backend.shutdownVm(id);
    if (!sdRes) {
        std::cerr << "Fail: shutdownVm, error=" << static_cast<int>(sdRes.error()) << "\n";
        failed++;
    }
    
    // Immediate termination
    if (!backend.powerOffVm(id)) {
        std::cerr << "Fail: powerOffVm\n";
        failed++;
    }

    state = backend.queryState(id);
    if (!state || *state != VmState::Stopped) {
        std::cerr << "Fail: VM should be stopped\n";
        failed++;
    }

    if (!backend.destroyVm(id)) {
        std::cerr << "Fail: destroyVm\n";
        failed++;
    }

    if (failed == 0) {
        std::cout << "QmpIntegrationTest PASS\n";
        return 0;
    }
    return 1;
}
