#include "infrastructure/qemu/QemuCommandBuilder.hpp"
#include <iostream>
#include <algorithm>

using namespace fvm::infrastructure::qemu;
using namespace fvm::domain;

int main() {
    int failed = 0;
    DefaultQemuCommandBuilder builder;

    VmConfig config{
        VmId("test-vm"),
        "MyTestVM",
        "",
        CpuConfig{CpuCount(2), 1, 2, 1},
        MemoryConfig{Megabytes(2048)},
        {}, {}, FirmwareConfig{}, DisplayConfig{16, 0, false}
    };

    auto spec = builder.build(VmId("test-vm"), config, "C:\\qemu\\qemu-system-x86_64.exe");

    if (spec.executablePath != "C:\\qemu\\qemu-system-x86_64.exe") {
        std::cerr << "Fail: Executable path mismatch\n";
        failed++;
    }

    auto hasArg = [&](const std::string& arg) {
        return std::find(spec.arguments.begin(), spec.arguments.end(), arg) != spec.arguments.end();
    };

    if (!hasArg("-accel") || !hasArg("whpx")) {
        std::cerr << "Fail: Missing WHPX acceleration\n";
        failed++;
    }

    if (!hasArg("-nodefaults")) {
        std::cerr << "Fail: Missing nodefaults\n";
        failed++;
    }
    
    if (!hasArg("-display") || !hasArg("none")) {
        std::cerr << "Fail: Missing display none\n";
        failed++;
    }

    // Add storage test
    VmConfig configWithStorage = config;
    configWithStorage.storage.push_back(StorageAttachment{
        "disk1",
        EvidenceSource("C:\\ev1.raw", DiskFormat::Raw),
        AccessMode::ReadOnly,
        BusType::VirtIO,
        true
    });
    configWithStorage.storage.push_back(StorageAttachment{
        "disk2",
        EvidenceSource("C:\\ev2.qcow2", DiskFormat::Qcow2),
        AccessMode::Overlay,
        BusType::VirtIO,
        false
    });

    std::map<std::string, std::string> overlayPaths = {
        {"disk2", "C:\\overlays\\disk2.qcow2"}
    };

    auto specStorage = builder.build(VmId("test-vm"), configWithStorage, "C:\\qemu\\qemu-system-x86_64.exe", overlayPaths);
    
    auto hasArgStorage = [&](const std::string& arg) {
        return std::find(specStorage.arguments.begin(), specStorage.arguments.end(), arg) != specStorage.arguments.end();
    };

    if (!hasArgStorage("file=C:\\ev1.raw,format=raw,media=disk,readonly=on")) {
        std::cerr << "Fail: Missing read-only storage argument\n";
        failed++;
    }

    if (!hasArgStorage("file=C:\\overlays\\disk2.qcow2,format=qcow2,media=disk")) {
        std::cerr << "Fail: Missing overlay storage argument\n";
        failed++;
    }

    if (failed == 0) {
        std::cout << "QemuCommandBuilderTest PASS\n";
        return 0;
    }
    return 1;
}
