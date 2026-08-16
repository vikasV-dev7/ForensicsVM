#include "infrastructure/qemu/image/QemuImgLocator.hpp"
#include <iostream>

using namespace fvm::infrastructure::qemu::image;

int main() {
    int failed = 0;

    {
        DefaultQemuImgLocator locator;
        auto res = locator.discover();
        if (!res.has_value() || res->find("qemu-img") == std::string::npos) {
            std::cerr << "Fail: FindValidExecutable\n";
            failed++;
        }
    }

    {
        DefaultQemuImgLocator locator;
        auto res = locator.discover("C:\\Windows\\System32\\hostname.exe");
        if (res.has_value() || (res.error() != QemuImgLocator::Error::InvalidVersion && res.error() != QemuImgLocator::Error::ExecutionFailed)) {
            std::cerr << "Fail: InvalidExplicitPath\n";
            failed++;
        }
    }

    if (failed == 0) {
        std::cout << "QemuImgLocatorTest PASS\n";
        return 0;
    }
    return 1;
}
