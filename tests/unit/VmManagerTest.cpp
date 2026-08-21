#include "vm/management/VmManager.hpp"
#include "infrastructure/inmemory/InMemoryRepository.hpp"
#include "infrastructure/inmemory/InMemoryBackend.hpp"
#include "infrastructure/inmemory/InMemoryEvidenceRepository.hpp"
#include "vm/contracts/IHashCalculator.hpp"
#include <iostream>
#include <fstream>
#include <memory>
#include <cassert>

using namespace fvm::domain;
using namespace fvm::management;
using namespace fvm::infrastructure::inmemory;

class DummyHash : public fvm::contracts::IHashCalculator {
public:
    std::expected<std::string, fvm::contracts::HashError> calculateSha256(const std::filesystem::path&) override {
        return "dummy-hash";
    }
    std::expected<std::string, fvm::contracts::HashError> calculateSha256(const std::string&) override {
        return "dummy-hash";
    }
};

int main() {
    int failed = 0;

    auto repo = std::make_unique<InMemoryRepository>();
    auto backend = std::make_unique<InMemoryBackend>();
    
    auto evRepo = std::make_unique<InMemoryEvidenceRepository>();
    
    // Inject test evidence (use an existing file so getEvidence doesn't mark it missing)
    std::filesystem::path dummyPath = std::filesystem::current_path() / "dummy.qcow2";
    {
        std::ofstream dummy(dummyPath);
        dummy << "test";
    }
    EvidenceRecord record(EvidenceId("ev1"), dummyPath, DiskFormat::Qcow2, 1024);
    record.setVerified("mock-hash");
    evRepo->save(record);
    
    auto evHash = std::make_unique<DummyHash>();
    auto registry = std::make_shared<EvidenceRegistry>(std::move(evRepo), std::move(evHash));
    
    VmManager manager(std::move(repo), std::move(backend), registry);

    VmConfig config{
        VmId("test-1"),
        "Test VM",
        "Desc",
        CpuConfig{CpuCount(2), 1, 2, 1},
        MemoryConfig{Megabytes(1024)},
        { StorageAttachment{"disk1", EvidenceId("ev1"), AccessMode::Overlay, BusType::VirtIO, false} },
        {},
        FirmwareConfig{FirmwareType::BIOS, false, false},
        DisplayConfig{}
    };

    // Test Create
    auto createRes = manager.createVm(config);
    if (!createRes) { std::cerr << "Fail: Create\n"; failed++; }

    // Test Duplicate
    auto dupRes = manager.createVm(config);
    if (dupRes || dupRes.error() != VmError::DuplicateVm) { std::cerr << "Fail: Dup\n"; failed++; }

    // Test List
    auto listRes = manager.listVms();
    if (!listRes || listRes->size() != 1) { std::cerr << "Fail: List\n"; failed++; }

    auto ev = registry->getEvidence(EvidenceId("ev1"));
    if (!ev) {
        std::cerr << "Fail: ev1 not found!\n";
    } else if (ev->status() != EvidenceStatus::Verified) {
        std::cerr << "Fail: ev1 not verified! Status: " << static_cast<int>(ev->status()) << "\n";
    }

    // Test Start
    auto startRes = manager.start(VmId("test-1"));
    if (!startRes) { 
        std::cerr << "Fail: Start. Error code: " << static_cast<int>(startRes.error()) << "\n";
        failed++; 
    }

    // Test Invalid Transition (Remove while running)
    auto remRunRes = manager.removeVm(VmId("test-1"));
    if (remRunRes || remRunRes.error() != VmError::InvalidLifecycleTransition) { std::cerr << "Fail: Rem run\n"; failed++; }

    // Test Shutdown & Remove
    manager.shutdown(VmId("test-1"));
    auto remRes = manager.removeVm(VmId("test-1"));
    listRes = manager.listVms();
    if (listRes->size() != 0) {
        std::cerr << "Fail: List VMs should be empty after removal\n";
        failed++;
    }

    // --- Acquisition Tests ---
    // Recreate VM for acquisition test
    auto createRes2 = manager.createVm(config);
    manager.start(config.id);
    
    // Acquire memory
    auto acquireRes = manager.acquireMemory(config.id);
    if (!acquireRes) {
        std::cerr << "Fail: Memory acquisition should succeed\n";
        failed++;
    } else {
        auto evId = acquireRes.value();
        auto evRecord = registry->getEvidence(evId);
        if (!evRecord || evRecord->status() != EvidenceStatus::Verified) {
            std::cerr << "Fail: Acquired artifact should be verified in registry\n";
            failed++;
        }
    }
    
    // Acquire disk delta
    auto acquireDiskRes = manager.acquireDiskDelta(config.id, "disk1");
    if (!acquireDiskRes) {
        std::cerr << "Fail: Disk delta acquisition should succeed\n";
        failed++;
    } else {
        auto evId = acquireDiskRes.value();
        auto evRecord = registry->getEvidence(evId);
        if (!evRecord || evRecord->status() != EvidenceStatus::Verified) {
            std::cerr << "Fail: Acquired artifact should be verified in registry\n";
            failed++;
        }
    }
    
    // Test invalid state acquisition
    manager.shutdown(config.id);
    manager.powerOff(config.id);
    auto acquireResBad = manager.acquireMemory(config.id);
    if (acquireResBad) {
        std::cerr << "Fail: Memory acquisition should fail in Stopped state\n";
        failed++;
    }
    auto acquireDiskResBad = manager.acquireDiskDelta(config.id, "disk1");
    if (acquireDiskResBad) {
        std::cerr << "Fail: Disk delta acquisition should fail in Stopped state\n";
        failed++;
    }

    if (failed == 0) {
        std::cout << "VmManagerTest PASS\n";
        return 0;
    }
    return 1;
}
