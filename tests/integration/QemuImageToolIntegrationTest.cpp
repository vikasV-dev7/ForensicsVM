#include "infrastructure/qemu/image/QemuImageTool.hpp"
#include <fstream>
#include <iostream>

using namespace fvm::infrastructure::qemu::image;
using namespace fvm::domain;

int main() {
    int failed = 0;
    
    std::filesystem::path dummyEvidencePath = std::filesystem::current_path() / "test_evidence.raw";
    std::filesystem::path overlayPath = std::filesystem::current_path() / "test_overlay.qcow2";

    // Create dummy evidence file
    std::ofstream ofs(dummyEvidencePath, std::ios::binary);
    for (int i = 0; i < 1024; ++i) { // 1MB dummy
        char buf[1024] = {0};
        ofs.write(buf, 1024);
    }
    ofs.close();

    {
        auto locator = std::make_unique<DefaultQemuImgLocator>();
        QemuImageTool tool(std::move(locator));

        auto res = tool.createOverlay(dummyEvidencePath, DiskFormat::Raw, overlayPath);
        if (!res.has_value()) {
            std::cerr << "Fail: QemuImageTool failed to create overlay\n";
            failed++;
        } else {
            // Verify overlay exists
            if (!std::filesystem::exists(overlayPath) || std::filesystem::file_size(overlayPath) == 0) {
                std::cerr << "Fail: Overlay file does not exist or is empty\n";
                failed++;
            }
        }
    }

    std::error_code ec;
    std::filesystem::remove(dummyEvidencePath, ec);
    std::filesystem::remove(overlayPath, ec);

    if (failed == 0) {
        std::cout << "QemuImageToolIntegrationTest PASS\n";
        return 0;
    }
    return 1;
}
