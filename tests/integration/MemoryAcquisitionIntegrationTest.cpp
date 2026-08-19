#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include "vm/management/VmManager.hpp"
#include "vm/management/EvidenceRegistry.hpp"
#include "infrastructure/inmemory/InMemoryEvidenceRepository.hpp"
#include "infrastructure/crypto/NativeHashCalculator.hpp"
#include "infrastructure/qemu/QemuBackend.hpp"
#include "infrastructure/qemu/QemuLocator.hpp"
#include "infrastructure/qemu/QemuCommandBuilder.hpp"
#include "infrastructure/qemu/QemuImgLocator.hpp"
#include "infrastructure/qemu/QemuImageTool.hpp"

using namespace fvm::domain;
using namespace fvm::management;
using namespace fvm::infrastructure::inmemory;
using namespace fvm::infrastructure::qemu;
using namespace fvm::infrastructure::crypto;

int main() {
    int failed = 0;

    auto repo = std::make_unique<InMemoryEvidenceRepository>();
    auto hashCalc = std::make_unique<NativeHashCalculator>();
    auto registry = std::make_shared<EvidenceRegistry>(std::move(repo), std::move(hashCalc));

    auto locator = std::make_unique<QemuLocator>();
    auto builder = std::make_unique<QemuCommandBuilder>();
    auto imgLocator = std::make_unique<QemuImgLocator>();
    auto imgTool = std::make_unique<QemuImageTool>(std::move(imgLocator));
    
    auto backend = std::make_unique<QemuBackend>(std::move(locator), std::move(builder), std::move(imgTool));
    
    VmManager manager(std::make_unique<InMemoryVmRepository>(), std::move(backend), registry);

    VmConfig config{
        VmId("test-mem-vm"),
        CpuConfig{CpuCount(1), 1, 1, 1},
        MemoryConfig{Megabytes(256), Megabytes(256), Megabytes(256)},
        {}
    };

    if (!manager.createVm(config)) {
        std::cerr << "Fail: Create VM\n";
        return 1;
    }

    if (!manager.start(config.id)) {
        std::cerr << "Fail: Start VM\n";
        return 1;
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));

    auto acquireRes = manager.acquireMemory(config.id);
    if (!acquireRes) {
        std::cerr << "Fail: Memory acquisition failed\n";
        failed++;
    } else {
        auto evId = acquireRes.value();
        auto evRecord = registry->getEvidence(evId);
        if (!evRecord) {
            std::cerr << "Fail: Artifact not found in registry\n";
            failed++;
        } else {
            if (evRecord->status() != EvidenceStatus::Verified) {
                std::cerr << "Fail: Artifact not Verified\n";
                failed++;
            }
            if (evRecord->format() != DiskFormat::Elf) {
                std::cerr << "Fail: Artifact format is not Elf\n";
                failed++;
            }

            // Verify ELF magic
            std::ifstream file(evRecord->path(), std::ios::binary);
            if (!file) {
                std::cerr << "Fail: Could not open artifact file\n";
                failed++;
            } else {
                char magic[4];
                file.read(magic, 4);
                if (magic[0] != 0x7f || magic[1] != 'E' || magic[2] != 'L' || magic[3] != 'F') {
                    std::cerr << "Fail: Artifact is not an ELF file\n";
                    failed++;
                }
            }

            // Verify Independent SHA256 match
            NativeHashCalculator indepCalc;
            auto indepHashRes = indepCalc.calculateSha256(evRecord->path());
            if (!indepHashRes || indepHashRes.value() != evRecord->sha256()) {
                std::cerr << "Fail: SHA256 independent verification mismatch\n";
                failed++;
            }
        }
    }

    manager.powerOff(config.id);
    manager.removeVm(config.id);

    if (failed == 0) {
        std::cout << "MemoryAcquisitionIntegrationTest PASS\n";
        return 0;
    }
    return 1;
}
