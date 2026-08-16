#include "infrastructure/qemu/QemuProcess.hpp"
#include <iostream>

using namespace fvm::infrastructure::qemu;

void assertEqual(const std::string& expected, const std::string& actual, const std::string& name, int& failed) {
    if (expected != actual) {
        std::cerr << "Fail: " << name << "\nExpected: " << expected << "\nActual:   " << actual << "\n";
        failed++;
    }
}

int main() {
    int failed = 0;

    assertEqual("\"\"", WindowsQemuProcess::escapeWindowsArg(""), "Empty string", failed);
    assertEqual("hello", WindowsQemuProcess::escapeWindowsArg("hello"), "Simple string", failed);
    assertEqual("\"hello world\"", WindowsQemuProcess::escapeWindowsArg("hello world"), "String with space", failed);
    assertEqual("\"hello\\\"world\"", WindowsQemuProcess::escapeWindowsArg("hello\"world"), "Embedded quote", failed);
    assertEqual("\"hello\\\\\\\"world\"", WindowsQemuProcess::escapeWindowsArg("hello\\\"world"), "Backslash and quote", failed);
    assertEqual("C:\\path\\to\\file", WindowsQemuProcess::escapeWindowsArg("C:\\path\\to\\file"), "Windows path without spaces", failed);
    assertEqual("\"C:\\path to\\file\"", WindowsQemuProcess::escapeWindowsArg("C:\\path to\\file"), "Windows path with spaces", failed);
    assertEqual("C:\\path\\", WindowsQemuProcess::escapeWindowsArg("C:\\path\\"), "Trailing backslash without quotes", failed);

    std::vector<std::string> args = {"arg1", "arg 2", "arg\"3"};
    std::string cmd = WindowsQemuProcess::buildWindowsCommandLine("C:\\exe path.exe", args);
    assertEqual("\"C:\\exe path.exe\" arg1 \"arg 2\" \"arg\\\"3\"", cmd, "Full command line", failed);

    if (failed == 0) {
        std::cout << "QemuQuotingTest PASS\n";
        return 0;
    }
    return 1;
}
