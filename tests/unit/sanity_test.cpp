#include <iostream>

// A minimal test executable that returns 0 on success.
int main() {
    int expected = 42;
    int actual = 42;
    
    if (expected == actual) {
        std::cout << "[PASS] Sanity check passed.\n";
        return 0;
    } else {
        std::cerr << "[FAIL] Sanity check failed.\n";
        return 1;
    }
}
