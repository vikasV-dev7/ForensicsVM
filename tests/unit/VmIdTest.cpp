#include "vm/domain/VmId.hpp"
#include <iostream>

using namespace fvm::domain;

int main() {
    int failed = 0;

    // Test valid construction
    VmId id1("uuid-1234");
    if (id1.value() != "uuid-1234") { std::cerr << "Fail: value mismatch\n"; failed++; }

    // Test equality
    VmId id2("uuid-1234");
    if (!(id1 == id2)) { std::cerr << "Fail: equality\n"; failed++; }

    // Test empty id
    VmId emptyId("");
    if (!emptyId.empty()) { std::cerr << "Fail: empty tracking\n"; failed++; }

    if (failed == 0) {
        std::cout << "VmIdTest PASS\n";
        return 0;
    }
    return 1;
}
