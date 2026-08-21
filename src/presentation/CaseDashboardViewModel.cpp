#include "CaseDashboardViewModel.hpp"
#include <QVariantMap>
#include <QDebug>
#include <filesystem>
#include "core/application/domain/Case.hpp"
#include "core/application/domain/ApplicationError.hpp"
#include "vm/domain/EvidenceId.hpp"
#include "vm/domain/VmId.hpp"

namespace fvm::presentation {

CaseDashboardViewModel::CaseDashboardViewModel(std::shared_ptr<fvm::core::application::contracts::IForensicApplication> backend, QObject* parent)
    : QObject(parent), m_backend(std::move(backend)), m_operationManager(new OperationManagerViewModel(m_backend, this)) {
}

void CaseDashboardViewModel::refresh() {
    if (!m_backend || !m_backend->isCaseOpen()) {
        m_caseId.clear();
        m_caseName.clear();
        m_evidenceList.clear();
        emit caseDetailsChanged();
        emit evidenceListChanged();
        return;
    }

    auto activeCase = m_backend->getActiveCase();
    if (activeCase.has_value()) {
        m_caseId = QString::fromStdString(activeCase->getId().value());
        m_caseName = QString::fromStdString(activeCase->getMetadata().name);
    } else {
        m_caseId.clear();
        m_caseName.clear();
    }
    emit caseDetailsChanged();

    auto evidenceResult = m_backend->listEvidence();
    m_evidenceList.clear();
    if (evidenceResult.has_value()) {
        for (const auto& evId : evidenceResult.value()) {
            QVariantMap evMap;
            evMap["id"] = QString::fromStdString(evId.value());
            m_evidenceList.append(evMap);
        }
    }
    emit evidenceListChanged();
}

QString CaseDashboardViewModel::caseId() const { return m_caseId; }
QString CaseDashboardViewModel::caseName() const { return m_caseName; }
QVariantList CaseDashboardViewModel::evidenceList() const { return m_evidenceList; }
OperationManagerViewModel* CaseDashboardViewModel::operationManager() const { return m_operationManager; }

void CaseDashboardViewModel::importEvidence(const QString& sourcePath) {
    if (!m_backend) return;
    std::filesystem::path src(sourcePath.toStdString());
    auto result = m_backend->importEvidence(src);
    if (result.has_value()) {
        m_operationManager->trackOperation(result.value());
    } else {
        emit errorOccurred(QString::fromStdString(result.error().what()));
    }
}

void CaseDashboardViewModel::launchSession(const QString& vmIdStr) {
    if (!m_backend) return;
    fvm::domain::VmConfig config { 
        fvm::domain::VmId(vmIdStr.toStdString()), // id
        "Temp VM", // name
        "", // description
        fvm::domain::CpuConfig { fvm::domain::CpuCount(1) }, // cpu
        fvm::domain::MemoryConfig { fvm::domain::Megabytes(1024) }, // memory
        {}, // storage
        {}, // network
        fvm::domain::FirmwareConfig { fvm::domain::FirmwareType::BIOS }, // firmware
        fvm::domain::DisplayConfig {} // display
    };
    
    // In a full implementation, config would be populated from UI inputs or a saved config.
    // We are just calling launchSession.
    auto result = m_backend->launchSession(config);
    if (result.has_value()) {
        m_operationManager->trackOperation(result.value());
    } else {
        emit errorOccurred(QString::fromStdString(result.error().what()));
    }
}

void CaseDashboardViewModel::stopSession(const QString& vmIdStr) {
    if (!m_backend) return;
    auto vmId = fvm::domain::VmId(vmIdStr.toStdString());
    auto result = m_backend->stopSession(vmId);
    if (result.has_value()) {
        m_operationManager->trackOperation(result.value());
    } else {
        emit errorOccurred(QString::fromStdString(result.error().what()));
    }
}

void CaseDashboardViewModel::acquireMemory(const QString& vmIdStr) {
    if (!m_backend) return;
    auto vmId = fvm::domain::VmId(vmIdStr.toStdString());
    auto result = m_backend->acquireMemory(vmId);
    if (result.has_value()) {
        m_operationManager->trackOperation(result.value());
    } else {
        emit errorOccurred(QString::fromStdString(result.error().what()));
    }
}

void CaseDashboardViewModel::acquireDiskDelta(const QString& vmIdStr) {
    if (!m_backend) return;
    auto vmId = fvm::domain::VmId(vmIdStr.toStdString());
    auto result = m_backend->acquireDiskDelta(vmId);
    if (result.has_value()) {
        m_operationManager->trackOperation(result.value());
    } else {
        emit errorOccurred(QString::fromStdString(result.error().what()));
    }
}

} // namespace fvm::presentation
