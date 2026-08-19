#include "infrastructure/qemu/QemuLocator.hpp"
#include "infrastructure/qemu/QemuProcess.hpp"
#include "infrastructure/qemu/QemuCommandBuilder.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace fvm::infrastructure::qemu;
using namespace fvm::domain;

int main() {
    int failed = 0;

    DefaultQemuLocator locator;
    auto exeRes = locator.discover();
    if (!exeRes) {
        std::cout << "SKIP: QEMU not found on host.\n";
        return 0; // Skip if QEMU isn't installed
    }

    VmConfig config{
        VmId("integration-test-vm"),
        "IntegrationTest",
        "",
        CpuConfig{CpuCount(1), 1, 1, 1},
        MemoryConfig{Megabytes(512)},
        {}, {}, FirmwareConfig{}, DisplayConfig{16, 0, false}
    };

    DefaultQemuCommandBuilder builder;
    auto spec = builder.build(config.id, config, exeRes.value(), {}, {});

    WindowsQemuProcess process;
    auto startRes = process.start(spec);
    
    if (!startRes) {
        std::cerr << "Fail: QEMU failed to start\n";
        failed++;
    } else {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        if (!process.isRunning()) {
            std::cerr << "Fail: QEMU exited prematurely\n";
            failed++;
        }
        
        auto termRes = process.terminate(true);
        if (!termRes) {
            std::cerr << "Fail: QEMU failed to terminate\n";
            failed++;
        }
    }

    if (failed == 0) {
        std::cout << "QemuIntegrationTest PASS\n";
        return 0;
    }
    return 1;
}
