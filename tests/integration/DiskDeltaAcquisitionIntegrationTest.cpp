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
#include "infrastructure/qemu/image/QemuImgLocator.hpp"
#include "infrastructure/qemu/image/QemuImageTool.hpp"
#include "infrastructure/inmemory/InMemoryRepository.hpp"

using namespace fvm::domain;
using namespace fvm::management;
using namespace fvm::infrastructure::inmemory;
using namespace fvm::infrastructure::qemu;
using namespace fvm::infrastructure::qemu::image;
using namespace fvm::infrastructure::crypto;

int main() {
    int failed = 0;

    auto locator = std::make_unique<DefaultQemuLocator>();
    auto builder = std::make_unique<DefaultQemuCommandBuilder>();
    
    auto registry = std::make_shared<EvidenceRegistry>(
        std::make_unique<InMemoryEvidenceRepository>(),
        std::make_unique<NativeHashCalculator>()
    );

    auto imgLocator = std::make_unique<DefaultQemuImgLocator>();
    auto imgTool = std::make_unique<QemuImageTool>(std::move(imgLocator));
    
    auto backend = std::make_unique<QemuBackend>(std::move(locator), std::move(builder), std::move(imgTool));
    
    VmManager manager(std::make_unique<InMemoryRepository>(), std::move(backend), registry);

    std::filesystem::path dummyQcow2 = std::filesystem::current_path() / "dummy.qcow2";
    if (std::filesystem::exists(dummyQcow2)) {
        std::filesystem::remove(dummyQcow2);
    }
    
    // Create a real qcow2 file using system qemu-img
    std::string cmd = "qemu-img create -f qcow2 \"" + dummyQcow2.string() + "\" 1M > NUL 2>&1";
    int res = std::system(cmd.c_str());
    if (res != 0) {
        std::cerr << "Fail: Could not create dummy qcow2\n";
        return 1;
    }
    
    // Ingest the evidence
    auto ingestRes = registry->ingest(dummyQcow2, DiskFormat::Qcow2);
    if (!ingestRes) {
        std::cerr << "Fail: Ingest dummy disk\n";
        return 1;
    }
    auto evId = ingestRes.value();

    VmConfig config{
        VmId("test-disk-vm"),
        "Disk VM",
        "Desc",
        CpuConfig{CpuCount(1), 1, 1, 1},
        MemoryConfig{Megabytes(256), Megabytes(256), Megabytes(256)},
        { StorageAttachment{"disk1", evId, AccessMode::Overlay, BusType::VirtIO, false} },
        {}, FirmwareConfig{}, DisplayConfig{}
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

    auto acquireRes = manager.acquireDiskDelta(config.id, "disk1");
    if (!acquireRes) {
        std::cerr << "Fail: Disk delta acquisition failed\n";
        failed++;
    } else {
        auto outEvId = acquireRes.value();
        auto evRecord = registry->getEvidence(outEvId);
        if (!evRecord) {
            std::cerr << "Fail: Artifact not found in registry\n";
            failed++;
        } else {
            if (evRecord->status() != EvidenceStatus::Verified) {
                std::cerr << "Fail: Artifact not Verified\n";
                failed++;
            }
            if (evRecord->format() != DiskFormat::Qcow2) {
                std::cerr << "Fail: Artifact format is not Qcow2\n";
                failed++;
            }

            // Verify QCOW2 magic
            std::ifstream file(evRecord->path(), std::ios::binary);
            if (!file) {
                std::cerr << "Fail: Could not open artifact file\n";
                failed++;
            } else {
                char magic[4];
                file.read(magic, 4);
                if (magic[0] != 'Q' || magic[1] != 'F' || magic[2] != 'I' || magic[3] != '\xfb') {
                    std::cerr << "Fail: Artifact is not a QCOW2 file\n";
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
        std::cout << "DiskDeltaAcquisitionIntegrationTest PASS\n";
        return 0;
    }
    return 1;
}
