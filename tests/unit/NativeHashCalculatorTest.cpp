#include <gtest/gtest.h>
#include "infrastructure/crypto/NativeHashCalculator.hpp"
#include <fstream>
#include <filesystem>

using namespace fvm::infrastructure::crypto;

class NativeHashCalculatorTest : public ::testing::Test {
protected:
    std::filesystem::path tempFile_;

    void SetUp() override {
        tempFile_ = std::filesystem::temp_directory_path() / "fvm_hash_test.dat";
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove(tempFile_, ec);
    }
    
    void createTestFile(const std::string& content) {
        std::ofstream out(tempFile_, std::ios::binary);
        out.write(content.c_str(), content.size());
    }
};

TEST_F(NativeHashCalculatorTest, HashEmptyFile) {
    createTestFile("");
    NativeHashCalculator calc;
    auto res = calc.calculateSha256(tempFile_);
    ASSERT_TRUE(res.has_value());
    // SHA256 of empty string
    EXPECT_EQ(res.value(), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST_F(NativeHashCalculatorTest, HashKnownContent) {
    createTestFile("hello world");
    NativeHashCalculator calc;
    auto res = calc.calculateSha256(tempFile_);
    ASSERT_TRUE(res.has_value());
    // SHA256 of "hello world"
    EXPECT_EQ(res.value(), "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9");
}

TEST_F(NativeHashCalculatorTest, HashFileNotFound) {
    NativeHashCalculator calc;
    auto res = calc.calculateSha256("C:\\nonexistent_fvm_test_file.dat");
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), fvm::contracts::HashError::FileNotFound);
}
