#include "ApplicationBootstrap.hpp"
#include "infrastructure/inmemory/InMemoryRepository.hpp"
#include "infrastructure/inmemory/InMemoryEvidenceRepository.hpp"
#include "infrastructure/crypto/NativeHashCalculator.hpp"

#include "infrastructure/qemu/QemuBackend.hpp"
#include "infrastructure/qemu/QemuLocator.hpp"
#include "infrastructure/qemu/QemuCommandBuilder.hpp"
#include "infrastructure/qemu/image/QemuImageTool.hpp"
#include "infrastructure/qemu/image/QemuImgLocator.hpp"

#include "core/application/services/ForensicApplicationImpl.hpp"
#include "infrastructure/sqlite/SqliteCaseRepository.hpp"

namespace fvm::core::application {

std::unique_ptr<fvm::management::VmManager> ApplicationBootstrap::createVmManager() {
    auto repo = std::make_unique<fvm::infrastructure::inmemory::InMemoryRepository>();
    
    auto qemuLocator = std::make_unique<fvm::infrastructure::qemu::DefaultQemuLocator>();
    auto qemuImgLocator = std::make_unique<fvm::infrastructure::qemu::image::DefaultQemuImgLocator>();
    auto qemuImgTool = std::make_unique<fvm::infrastructure::qemu::image::QemuImageTool>(std::move(qemuImgLocator));
    auto qemuCommandBuilder = std::make_unique<fvm::infrastructure::qemu::DefaultQemuCommandBuilder>();
    auto backend = std::make_unique<fvm::infrastructure::qemu::QemuBackend>(
        std::move(qemuLocator),
        std::move(qemuCommandBuilder),
        std::move(qemuImgTool)
    );

    auto evRepo = std::make_unique<fvm::infrastructure::inmemory::InMemoryEvidenceRepository>();
    auto hasher = std::make_unique<fvm::infrastructure::crypto::NativeHashCalculator>();
    
    auto registry = std::make_shared<fvm::management::EvidenceRegistry>(std::move(evRepo), std::move(hasher));
    
    return std::make_unique<fvm::management::VmManager>(std::move(repo), std::move(backend), registry);
}

std::unique_ptr<fvm::core::application::contracts::IForensicApplication> ApplicationBootstrap::createApplication() {
    auto vmRepo = std::make_unique<fvm::infrastructure::inmemory::InMemoryRepository>();
    
    auto qemuLocator = std::make_unique<fvm::infrastructure::qemu::DefaultQemuLocator>();
    auto qemuImgLocator = std::make_unique<fvm::infrastructure::qemu::image::DefaultQemuImgLocator>();
    auto qemuImgTool = std::make_unique<fvm::infrastructure::qemu::image::QemuImageTool>(std::move(qemuImgLocator));
    auto qemuCommandBuilder = std::make_unique<fvm::infrastructure::qemu::DefaultQemuCommandBuilder>();
    auto backend = std::make_unique<fvm::infrastructure::qemu::QemuBackend>(
        std::move(qemuLocator),
        std::move(qemuCommandBuilder),
        std::move(qemuImgTool)
    );

    auto evRepo = std::make_unique<fvm::infrastructure::inmemory::InMemoryEvidenceRepository>();
    auto hasher = std::make_unique<fvm::infrastructure::crypto::NativeHashCalculator>();
    
    auto registry = std::make_shared<fvm::management::EvidenceRegistry>(std::move(evRepo), std::move(hasher));
    auto vmManager = std::make_shared<fvm::management::VmManager>(std::move(vmRepo), std::move(backend), registry);
    auto caseRepo = std::make_shared<fvm::infrastructure::sqlite::SqliteCaseRepository>();
    
    return std::make_unique<fvm::core::application::services::ForensicApplicationImpl>(std::move(caseRepo), std::move(vmManager), std::move(registry));
}

} // namespace fvm::core::application
