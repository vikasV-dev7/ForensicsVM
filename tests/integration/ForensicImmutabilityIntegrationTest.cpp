#include "infrastructure/qemu/QemuBackend.hpp"
#include "infrastructure/qemu/QemuLocator.hpp"
#include "infrastructure/qemu/QemuCommandBuilder.hpp"
#include "infrastructure/qemu/image/QemuImageTool.hpp"
#include "vm/domain/VmConfig.hpp"
#include "vm/domain/VmId.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#endif

std::string calculateSHA256(const std::string& filepath) {
#ifdef _WIN32
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    std::string result = "";
    
    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) return "";
    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0);
        return "";
    }
    
    std::ifstream file(filepath, std::ios::binary);
    if (!file) return "";
    
    std::vector<char> buffer(8192);
    while (file.read(buffer.data(), buffer.size()) || file.gcount() > 0) {
        CryptHashData(hHash, reinterpret_cast<const BYTE*>(buffer.data()), static_cast<DWORD>(file.gcount()), 0);
    }
    
    DWORD cbHashSize = 0;
    DWORD dwCount = sizeof(DWORD);
    if (CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&cbHashSize, &dwCount, 0)) {
        std::vector<BYTE> hashValue(cbHashSize);
        if (CryptGetHashParam(hHash, HP_HASHVAL, hashValue.data(), &cbHashSize, 0)) {
            char hex[3];
            for (DWORD i = 0; i < cbHashSize; i++) {
                snprintf(hex, sizeof(hex), "%02x", hashValue[i]);
                result += hex;
            }
        }
    }
    
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    return result;
#else
    return "NOT_IMPLEMENTED";
#endif
}

using namespace fvm::infrastructure::qemu;
using namespace fvm::domain;

int main() {
    int failed = 0;

    // Create a dummy raw evidence file
    std::filesystem::path dummyEvidence = std::filesystem::current_path() / "dummy_evidence.raw";
    {
        std::ofstream ofs(dummyEvidence, std::ios::binary);
        for (int i = 0; i < 1024 * 1024; ++i) {
            ofs.put('\0'); // 1MB zeroed file
        }
    }

    auto hashBefore = calculateSHA256(dummyEvidence.string());

    auto locator = std::make_unique<DefaultQemuLocator>();
    auto cmdBuilder = std::make_unique<DefaultQemuCommandBuilder>();
    auto imgLocator = std::make_unique<image::DefaultQemuImgLocator>();
    auto imgTool = std::make_unique<image::QemuImageTool>(std::move(imgLocator));
    
    QemuBackend backend(std::move(locator), std::move(cmdBuilder), std::move(imgTool));

    VmId id("forensic-immutability-test");
    VmConfig config{
        id,
        "ImmutabilityTestVM",
        "",
        CpuConfig{CpuCount(1), 1, 1, 1},
        MemoryConfig{Megabytes(512)},
        {
            StorageAttachment{
                "disk1",
                EvidenceSource(dummyEvidence.string(), DiskFormat::Raw),
                AccessMode::Overlay,
                BusType::VirtIO,
                false
            }
        }, 
        {}, FirmwareConfig{}, DisplayConfig{0, 0, false}
    };

    if (!backend.createVm(config)) {
        std::cerr << "Fail: createVm\n";
        failed++;
        goto cleanup;
    }

    if (!backend.startVm(id)) {
        std::cerr << "Fail: startVm\n";
        failed++;
        goto cleanup;
    }

    // Wait briefly and verify the overlay was created
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // We expect the overlay to be in the temp directory
    {
        std::filesystem::path tempDir = std::filesystem::temp_directory_path();
        std::string safeId = std::to_string(std::hash<std::string>{}(id.value()));
        std::string safeDisk = std::to_string(std::hash<std::string>{}("disk1"));
        std::string overlayName = "fvm-overlay-" + safeId + "-" + safeDisk + ".qcow2";
        std::filesystem::path expectedOverlay = tempDir / overlayName;

        if (!std::filesystem::exists(expectedOverlay)) {
            std::cerr << "Fail: Overlay file was not created: " << expectedOverlay << "\n";
            failed++;
        }
    }

    // Verify VM is running
    {
        auto state = backend.queryState(id);
        if (!state || *state != VmState::Running) {
            std::cerr << "Fail: VM should be running\n";
            failed++;
        }
    }

    // Power off the VM
    if (!backend.powerOffVm(id)) {
        std::cerr << "Fail: powerOffVm\n";
        failed++;
    }

    // After power off, the overlay should be deleted
    {
        std::filesystem::path tempDir = std::filesystem::temp_directory_path();
        std::string safeId = std::to_string(std::hash<std::string>{}(id.value()));
        std::string safeDisk = std::to_string(std::hash<std::string>{}("disk1"));
        std::string overlayName = "fvm-overlay-" + safeId + "-" + safeDisk + ".qcow2";
        std::filesystem::path expectedOverlay = tempDir / overlayName;

        if (std::filesystem::exists(expectedOverlay)) {
            std::cerr << "Fail: Overlay file was not cleaned up after powerOffVm: " << expectedOverlay << "\n";
            failed++;
        }
    }

    // Finally destroy the VM
    if (!backend.destroyVm(id)) {
        std::cerr << "Fail: destroyVm\n";
        failed++;
    }

    // Verify original evidence was NOT modified
    {
        auto hashAfter = calculateSHA256(dummyEvidence.string());
        if (hashBefore != hashAfter) {
            std::cerr << "Fail: Original evidence file was modified! Hash mismatch.\n";
            failed++;
        }
    }

cleanup:
    std::error_code ec;
    std::filesystem::remove(dummyEvidence, ec);

    if (failed == 0) {
        std::cout << "ForensicImmutabilityIntegrationTest PASS\n";
        return 0;
    }
    return 1;
}
