#include "ApplicationBootstrap.hpp"
#include "infrastructure/inmemory/InMemoryRepository.hpp"
#include "infrastructure/inmemory/InMemoryBackend.hpp"

namespace fvm::core::application {

std::unique_ptr<fvm::management::VmManager> ApplicationBootstrap::createVmManager() {
    auto repo = std::make_unique<fvm::infrastructure::inmemory::InMemoryRepository>();
    auto backend = std::make_unique<fvm::infrastructure::inmemory::InMemoryBackend>();
    
    return std::make_unique<fvm::management::VmManager>(std::move(repo), std::move(backend));
}

} // namespace fvm::core::application
