#include "infrastructure/qemu/QemuLocator.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>

using namespace fvm::infrastructure::qemu;

int main() {
    int failed = 0;
    DefaultQemuLocator locator;

    // Test missing executable
    auto res = locator.discover("C:\\Invalid\\Path\\qemu.exe");
    if (res.has_value() || res.error() != QemuLocator::Error::NotFound) {
        std::cerr << "Fail: Should not find invalid path\n";
        failed++;
    }

    // Test explicit valid path containing spaces (using ping to fake success? No, validation expects qemu output)
    // We will test shell injection prevention instead.
    std::string maliciousPath = "malicious_file.exe\" & ping 127.0.0.1 -n 3 & REM \"";
    
    // Create the dummy file so std::filesystem::exists passes
    std::ofstream ofs(maliciousPath);
    ofs << "dummy";
    ofs.close();

    // If shell injection was possible, this would hang for 3 seconds executing ping
    auto start = std::chrono::steady_clock::now();
    res = locator.discover(maliciousPath);
    auto end = std::chrono::steady_clock::now();
    
    std::filesystem::remove(maliciousPath);

    if (res.has_value()) {
        std::cerr << "Fail: Malicious path should not be valid\n";
        failed++;
    }

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    if (duration > 1500) {
        std::cerr << "Fail: Shell injection likely occurred (took " << duration << " ms)\n";
        failed++;
    }

    if (failed == 0) {
        std::cout << "QemuLocatorTest PASS\n";
        return 0;
    }
    return 1;
}
