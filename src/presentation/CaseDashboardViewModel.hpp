#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <memory>
#include "core/application/contracts/IForensicApplication.hpp"
#include "OperationManagerViewModel.hpp"

namespace fvm::presentation {

class CaseDashboardViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString caseId READ caseId NOTIFY caseDetailsChanged)
    Q_PROPERTY(QString caseName READ caseName NOTIFY caseDetailsChanged)
    Q_PROPERTY(QVariantList evidenceList READ evidenceList NOTIFY evidenceListChanged)
    Q_PROPERTY(OperationManagerViewModel* operationManager READ operationManager CONSTANT)

public:
    explicit CaseDashboardViewModel(std::shared_ptr<fvm::core::application::contracts::IForensicApplication> backend, QObject* parent = nullptr);

    void refresh();

    QString caseId() const;
    QString caseName() const;
    QVariantList evidenceList() const;
    OperationManagerViewModel* operationManager() const;

    Q_INVOKABLE void importEvidence(const QString& sourcePath);
    Q_INVOKABLE void launchSession(const QString& vmIdStr);
    Q_INVOKABLE void stopSession(const QString& vmIdStr);
    Q_INVOKABLE void acquireMemory(const QString& vmIdStr);
    Q_INVOKABLE void acquireDiskDelta(const QString& vmIdStr);

signals:
    void caseDetailsChanged();
    void evidenceListChanged();
    void errorOccurred(const QString& message);

private:
    std::shared_ptr<fvm::core::application::contracts::IForensicApplication> m_backend;
    OperationManagerViewModel* m_operationManager;

    QString m_caseId;
    QString m_caseName;
    QVariantList m_evidenceList;
};

} // namespace fvm::presentation
