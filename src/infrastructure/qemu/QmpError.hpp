#pragma once

namespace fvm::infrastructure::qemu {

enum class QmpError {
    Timeout,
    ConnectionLost,
    ConnectionFailed,
    InvalidResponse,
    CommandFailed,
    ProtocolError
};

} // namespace fvm::infrastructure::qemu
