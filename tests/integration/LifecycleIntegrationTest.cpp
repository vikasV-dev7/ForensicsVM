#include "infrastructure/qemu/QemuBackend.hpp"
#include "infrastructure/qemu/QemuLocator.hpp"
#include "infrastructure/qemu/QemuCommandBuilder.hpp"
#include "infrastructure/qemu/image/QemuImageTool.hpp"
#include "infrastructure/qemu/image/QemuImgLocator.hpp"
#include "vm/domain/VmConfig.hpp"
#include "vm/domain/VmId.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>

using namespace fvm::infrastructure::qemu;
using namespace fvm::domain;

void testLifecycleReconciliation() {
    auto locator = std::make_unique<DefaultQemuLocator>();
    auto builder = std::make_unique<DefaultQemuCommandBuilder>();
    auto imageTool = std::make_unique<image::QemuImageTool>(std::make_unique<image::DefaultQemuImgLocator>());

    QemuBackend backend(std::move(locator), std::move(builder), std::move(imageTool));

    std::filesystem::path tempDir = std::filesystem::canonical(std::filesystem::temp_directory_path());
    std::filesystem::path dummyEvidencePath = tempDir / "lifecycle_dummy_evidence.raw";

    std::ofstream dummyFile(dummyEvidencePath, std::ios::binary);
    std::string dummyData(1024 * 1024, '\0');
    dummyFile.write(dummyData.c_str(), dummyData.size());
    dummyFile.close();

    VmConfig config{
        VmId("lifecycle-vm"),
        "Lifecycle VM",
        "",
        CpuConfig{CpuCount(1), 1, 1, 1},
        MemoryConfig{Megabytes(512)},
        std::vector<StorageAttachment>{
            StorageAttachment{
                "disk0",
                EvidenceId("lifecycle-ev-id"),
                AccessMode::Overlay,
                BusType::VirtIO,
                true
            }
        },
        std::vector<NetworkConfig>{},
        FirmwareConfig{FirmwareType::BIOS, false, false},
        DisplayConfig{16, 1, false}
    };

    auto createRes = backend.createVm(config);
    if (!createRes) {
        std::cerr << "Fail: createVm\n";
        exit(1);
    }

    std::vector<EvidenceRecord> resolvedEvidence;
    resolvedEvidence.push_back(EvidenceRecord(EvidenceId("lifecycle-ev-id"), dummyEvidencePath, DiskFormat::Raw, 1024 * 1024));
    resolvedEvidence[0].setVerified("deadbeef");

    auto startRes = backend.startVm(config.id, resolvedEvidence);
    if (!startRes) {
        std::cerr << "Fail: startVm failed\n";
        exit(1);
    }

    // Must be running
    auto query1 = backend.queryState(config.id);
    if (!query1 || query1->state != VmState::Running) {
        std::cerr << "Fail: VM not running\n";
        exit(1);
    }

    // Forcefully crash QEMU
    std::cout << "Simulating QEMU crash...\n";
    std::system("taskkill /F /IM qemu-system-x86_64.exe > nul 2>&1");
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Reconcile
    auto query2 = backend.queryState(config.id);
    if (!query2) {
        std::cerr << "Fail: queryState failed after crash\n";
        exit(1);
    }

    if (query2->state != VmState::Failed) {
        std::cerr << "Fail: State should be Failed, got " << static_cast<int>(query2->state) << "\n";
        exit(1);
    }

    if (query2->reason != TerminationReason::ProcessCrashed) {
        std::cerr << "Fail: Reason should be ProcessCrashed, got " << static_cast<int>(query2->reason) << "\n";
        exit(1);
    }

    // Check overlay cleanup
    std::string expectedOverlay = (tempDir / ("fvm-overlay-" + std::to_string(std::hash<std::string>{}("lifecycle-vm")) + "-" + std::to_string(std::hash<std::string>{}("disk0")) + ".qcow2")).string();
    
    if (std::filesystem::exists(expectedOverlay)) {
        std::cerr << "Fail: Overlay leaked after crash reconciliation!\n";
        exit(1);
    }

    std::filesystem::remove(dummyEvidencePath);
}

int main() {
    try {
        testLifecycleReconciliation();
        std::cout << "LifecycleIntegrationTest passed!\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << "\n";
        return 1;
    }
}
