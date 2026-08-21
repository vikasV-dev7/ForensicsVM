#include "OperationManagerViewModel.hpp"
#include <QVariantMap>
#include <QDebug>
#include "core/application/domain/OperationRecord.hpp"
#include "core/application/domain/ApplicationError.hpp"

namespace fvm::presentation {

OperationManagerViewModel::OperationManagerViewModel(std::shared_ptr<fvm::core::application::contracts::IForensicApplication> backend, QObject* parent)
    : QObject(parent), m_backend(std::move(backend)), m_pollTimer(new QTimer(this)) {
    
    // Poll every 250ms for Operation Status
    m_pollTimer->setInterval(250);
    connect(m_pollTimer, &QTimer::timeout, this, &OperationManagerViewModel::onPollTimer);
    m_pollTimer->start();
}

OperationManagerViewModel::~OperationManagerViewModel() {
    m_pollTimer->stop();
}

QVariantList OperationManagerViewModel::activeOperations() const {
    return m_activeOperationsData;
}

void OperationManagerViewModel::trackOperation(const fvm::core::application::domain::OperationId& opId) {
    m_trackedOperations.push_back(opId);
    updateOperations();
}

void OperationManagerViewModel::cancelOperation(const QString& opIdStr) {
    if (!m_backend) return;
    auto opId = fvm::core::application::domain::OperationId(opIdStr.toStdString());
    auto result = m_backend->cancelOperation(opId);
    if (!result.has_value()) {
        qWarning() << "Failed to cancel operation" << opIdStr << ":" 
                   << QString::fromStdString(result.error().what());
    }
}

void OperationManagerViewModel::onPollTimer() {
    updateOperations();
}

void OperationManagerViewModel::updateOperations() {
    if (!m_backend) return;
    
    QVariantList newOperationsData;
    std::vector<fvm::core::application::domain::OperationId> activeTracked;

    bool changed = false;

    for (const auto& opId : m_trackedOperations) {
        auto statusResult = m_backend->getOperationStatus(opId);
        if (statusResult.has_value()) {
            const auto& record = statusResult.value();
            
            QVariantMap opMap;
            opMap["id"] = QString::fromStdString(record.id.value());
            opMap["type"] = static_cast<int>(record.type);
            opMap["state"] = static_cast<int>(record.state);
            opMap["message"] = QString::fromStdString(record.message);
            opMap["error"] = QString::fromStdString(record.error);
            
            newOperationsData.append(opMap);

            if (record.state == fvm::core::application::domain::OperationState::Running ||
                record.state == fvm::core::application::domain::OperationState::Queued ||
                record.state == fvm::core::application::domain::OperationState::Cancelling) {
                activeTracked.push_back(opId);
            } else {
                // Completed, Failed, or Cancelled
                emit operationCompleted(QString::fromStdString(record.id.value()), 
                                        record.state == fvm::core::application::domain::OperationState::Completed,
                                        QString::fromStdString(record.message));
                changed = true; // Size of active list will change
            }
        } else {
            // Failed to fetch status, might be missing, just drop tracking
            qWarning() << "Failed to get operation status for" << QString::fromStdString(opId.value());
            changed = true;
        }
    }

    if (changed || newOperationsData.size() != m_activeOperationsData.size() || !m_trackedOperations.empty()) {
        m_activeOperationsData = newOperationsData;
        m_trackedOperations = activeTracked;
        emit activeOperationsChanged();
    }
}

} // namespace fvm::presentation
