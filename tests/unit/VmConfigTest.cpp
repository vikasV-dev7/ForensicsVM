#include "vm/domain/CpuConfig.hpp"
#include "vm/domain/MemoryConfig.hpp"
#include <iostream>

using namespace fvm::domain;

int main() {
    int failed = 0;

    // CPU config test
    CpuConfig cpuValid{CpuCount(4), 1, 4, 1};
    if (!cpuValid.isValid()) { std::cerr << "Fail: valid cpu\n"; failed++; }

    CpuConfig cpuInvalidTopo{CpuCount(4), 1, 2, 1}; // 1*2*1 != 4
    if (cpuInvalidTopo.isValid()) { std::cerr << "Fail: invalid topology\n"; failed++; }

    // Memory config test
    MemoryConfig memValid{Megabytes(1024), Megabytes(512), Megabytes(2048)};
    if (!memValid.isValid()) { std::cerr << "Fail: valid mem\n"; failed++; }

    MemoryConfig memZero{Megabytes(0)};
    if (memZero.isValid()) { std::cerr << "Fail: zero mem\n"; failed++; }

    MemoryConfig memOOB{Megabytes(1024), Megabytes(2048), Megabytes(4096)};
    if (memOOB.isValid()) { std::cerr << "Fail: out of bounds mem\n"; failed++; }

    if (failed == 0) {
        std::cout << "VmConfigTest PASS\n";
        return 0;
    }
    return 1;
}
