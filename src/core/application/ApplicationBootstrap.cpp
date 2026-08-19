#include "ApplicationBootstrap.hpp"
#include "infrastructure/inmemory/InMemoryRepository.hpp"
#include "infrastructure/inmemory/InMemoryBackend.hpp"
#include "infrastructure/inmemory/InMemoryEvidenceRepository.hpp"
#include "infrastructure/crypto/NativeHashCalculator.hpp"

namespace fvm::core::application {

std::unique_ptr<fvm::management::VmManager> ApplicationBootstrap::createVmManager() {
    auto repo = std::make_unique<fvm::infrastructure::inmemory::InMemoryRepository>();
    auto backend = std::make_unique<fvm::infrastructure::inmemory::InMemoryBackend>();
    auto evRepo = std::make_unique<fvm::infrastructure::inmemory::InMemoryEvidenceRepository>();
    auto hasher = std::make_unique<fvm::infrastructure::crypto::NativeHashCalculator>();
    
    auto registry = std::make_shared<fvm::management::EvidenceRegistry>(std::move(evRepo), std::move(hasher));
    
    return std::make_unique<fvm::management::VmManager>(std::move(repo), std::move(backend), registry);
}

} // namespace fvm::core::application
