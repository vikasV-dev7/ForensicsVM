#pragma once
#include <stdexcept>
#include <string>

namespace fvm::core::application::domain {

class ApplicationError : public std::runtime_error {
public:
    explicit ApplicationError(const std::string& message)
        : std::runtime_error(message) {}
};

class PathSecurityError : public ApplicationError {
public:
    explicit PathSecurityError(const std::string& message)
        : ApplicationError(message) {}
};

class CaseNotOpenError : public ApplicationError {
public:
    CaseNotOpenError()
        : ApplicationError("No case is currently open") {}
};

class InvalidOperationError : public ApplicationError {
public:
    explicit InvalidOperationError(const std::string& message)
        : ApplicationError(message) {}
};

} // namespace fvm::core::application::domain
