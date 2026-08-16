#include <iostream>
#include "vm/domain/ExecutionSession.hpp"
#include "vm/domain/TerminationReason.hpp"
#include "vm/domain/SessionEvidence.hpp"

using namespace fvm::domain;

void testExecutionSessionCreation() {
    ExecutionSession session{
        "sess-123-abc",
        VmId("vm-1"),
        {},
        std::chrono::system_clock::now(),
        std::nullopt,
        VmState::Starting,
        TerminationReason::NotTerminated
    };
    
    SessionEvidence ev;
    ev.diskId = "disk-1";
    ev.evidenceSha256 = "abc123hash";
    ev.access = AccessMode::Overlay;
    ev.overlayPath = "C:\\temp\\fvm-overlay.qcow2";
    
    session.evidence.push_back(ev);
    
    if (session.sessionId == session.vmId.value()) {
        std::cerr << "Fail: session ID must be distinct from VM ID\n";
        exit(1);
    }
    
    if (session.evidence.size() != 1) {
        std::cerr << "Fail: Expected 1 evidence associated\n";
        exit(1);
    }
}

void testTerminalStates() {
    ExecutionSession session{
        "sess-123",
        VmId("vm-1"),
        {},
        std::chrono::system_clock::now(),
        std::chrono::system_clock::now(),
        VmState::Failed,
        TerminationReason::ProcessCrashed
    };
    
    if (session.finalState != VmState::Failed || session.terminationReason != TerminationReason::ProcessCrashed) {
        std::cerr << "Fail: Terminal states not recorded properly\n";
        exit(1);
    }
}

int main() {
    try {
        testExecutionSessionCreation();
        testTerminalStates();
        std::cout << "ExecutionSessionTest passed!\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}
