#include "application/ApplicationBootstrap.hpp"
#include <iostream>
#include <exception>

int main() {
    try {
        std::cout << "ForensicVM Starting...\n";
        
        // Composition Root: Assemble application
        auto vmManager = fvm::core::application::ApplicationBootstrap::createVmManager();
        
        std::cout << "ForensicVM core assembled. Shutting down.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred.\n";
        return 1;
    }
}
