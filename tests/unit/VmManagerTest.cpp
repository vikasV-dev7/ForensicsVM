#include "vm/management/VmManager.hpp"
#include "infrastructure/inmemory/InMemoryRepository.hpp"
#include "infrastructure/inmemory/InMemoryBackend.hpp"
#include <iostream>
#include <memory>
#include <cassert>

using namespace fvm::domain;
using namespace fvm::management;
using namespace fvm::infrastructure::inmemory;

int main() {
    int failed = 0;

    auto repo = std::make_unique<InMemoryRepository>();
    auto backend = std::make_unique<InMemoryBackend>();
    
    VmManager manager(std::move(repo), std::move(backend));

    VmConfig config{
        VmId("test-1"),
        "Test VM",
        "Desc",
        CpuConfig{CpuCount(2), 1, 2, 1},
        MemoryConfig{Megabytes(1024)},
        {},
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

    // Test Start
    auto startRes = manager.start(VmId("test-1"));
    if (!startRes) { std::cerr << "Fail: Start\n"; failed++; }

    // Test Invalid Transition (Remove while running)
    auto remRunRes = manager.removeVm(VmId("test-1"));
    if (remRunRes || remRunRes.error() != VmError::InvalidLifecycleTransition) { std::cerr << "Fail: Rem run\n"; failed++; }

    // Test Shutdown & Remove
    manager.shutdown(VmId("test-1"));
    auto remRes = manager.removeVm(VmId("test-1"));
    if (!remRes) { std::cerr << "Fail: Rem\n"; failed++; }

    if (failed == 0) {
        std::cout << "VmManagerTest PASS\n";
        return 0;
    }
    return 1;
}
