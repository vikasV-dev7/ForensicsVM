#include "QemuCommandBuilder.hpp"

namespace fvm::infrastructure::qemu {

QemuLaunchSpec DefaultQemuCommandBuilder::build(const domain::VmId& id, const domain::VmConfig& config, const std::string& executablePath, const std::vector<domain::EvidenceRecord>& resolvedEvidence, const std::map<std::string, std::string>& overlayPaths) const {
    QemuLaunchSpec spec;
    spec.executablePath = executablePath;
    spec.qmpPipeName = "fvm-qmp-" + id.value();
    
    // Base configuration (safe defaults)
    spec.arguments = {
        "-accel", "whpx",
        "-nodefaults",
        "-name", config.name,
        "-m", std::to_string(config.memory.assignedMemory.value()),
        "-smp", std::to_string(config.cpu.vcpus.value()),
        "-chardev", "pipe,id=qmp0,path=" + spec.qmpPipeName,
        "-mon", "chardev=qmp0,mode=control"
    };
    
    // Display configuration
    if (config.display.displayCount == 0) {
        spec.arguments.push_back("-display");
        spec.arguments.push_back("none");
    } else {
        spec.arguments.push_back("-display");
        spec.arguments.push_back("sdl");
        spec.arguments.push_back("-vga");
        spec.arguments.push_back("std");
    }
    
    // Storage
    for (const auto& storage : config.storage) {
        spec.arguments.push_back("-drive");
        
        std::string path;
        std::string format;
        bool readOnly = false;
        
        // Find corresponding evidence record
        auto itEv = std::find_if(resolvedEvidence.begin(), resolvedEvidence.end(),
            [&storage](const domain::EvidenceRecord& r) { return r.id() == storage.evidenceId; });
        
        if (itEv == resolvedEvidence.end()) {
            continue; // Should not happen if validation passed
        }
        
        if (storage.access == domain::AccessMode::Overlay) {
            auto it = overlayPaths.find(storage.diskId);
            if (it == overlayPaths.end()) {
                path = itEv->path().string();
                format = "raw"; // Fallback safe format
                readOnly = true; 
            } else {
                path = it->second;
                format = "qcow2";
                readOnly = false;
            }
        } else {
            path = itEv->path().string();
            switch (itEv->format()) {
                case domain::DiskFormat::Raw: format = "raw"; break;
                case domain::DiskFormat::Qcow2: format = "qcow2"; break;
                case domain::DiskFormat::Vhdx: format = "vhdx"; break;
                case domain::DiskFormat::Vmdk: format = "vmdk"; break;
                default: format = "raw"; break;
            }
            readOnly = true; // ReadOnly mode always enforces readonly=on
        }

        std::string driveArg = "file=" + path + ",format=" + format + ",media=disk,id=drive-" + storage.diskId + ",node-name=node-" + storage.diskId;
        if (readOnly) {
            driveArg += ",readonly=on";
        }
        spec.arguments.push_back(driveArg);
    }
    
    return spec;
}

} // namespace fvm::infrastructure::qemu
