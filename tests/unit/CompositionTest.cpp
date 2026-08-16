#include "core/application/ApplicationBootstrap.hpp"
#include <iostream>

using namespace fvm::core::application;

int main() {
    auto manager = ApplicationBootstrap::createVmManager();
    if (!manager) {
        std::cerr << "Fail: Bootstrap failed\n";
        return 1;
    }
    std::cout << "CompositionTest PASS\n";
    return 0;
}
