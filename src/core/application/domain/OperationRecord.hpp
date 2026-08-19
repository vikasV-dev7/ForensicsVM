#pragma once
#include "OperationId.hpp"
#include "OperationType.hpp"
#include "OperationState.hpp"
#include <string>
#include <cstdint>

namespace fvm::core::application::domain {

struct OperationRecord {
    OperationId id;
    OperationType type;
    OperationState state;
    
    int64_t createdAtUnixSeconds{0};
    int64_t startedAtUnixSeconds{0};
    int64_t completedAtUnixSeconds{0};
    
    std::string message;
    std::string error;
    std::string resultReference; // e.g. VmId, EvidenceId as string
    
    OperationRecord() : type(OperationType::Unknown), state(OperationState::Queued) {}
    
    OperationRecord(OperationId id_val, OperationType type_val, OperationState state_val, int64_t created, int64_t started, int64_t completed, std::string msg, std::string err, std::string result)
        : id(std::move(id_val)), type(type_val), state(state_val), createdAtUnixSeconds(created), startedAtUnixSeconds(started), completedAtUnixSeconds(completed), message(std::move(msg)), error(std::move(err)), resultReference(std::move(result)) {}
};

} // namespace fvm::core::application::domain
