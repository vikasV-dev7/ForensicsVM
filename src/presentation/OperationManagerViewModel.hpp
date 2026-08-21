#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QTimer>
#include <memory>
#include <vector>
#include "core/application/contracts/IForensicApplication.hpp"
#include "core/application/domain/OperationId.hpp"

namespace fvm::presentation {

class OperationManagerViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList activeOperations READ activeOperations NOTIFY activeOperationsChanged)

public:
    explicit OperationManagerViewModel(std::shared_ptr<fvm::core::application::contracts::IForensicApplication> backend, QObject* parent = nullptr);
    ~OperationManagerViewModel() override;

    QVariantList activeOperations() const;

    void trackOperation(const fvm::core::application::domain::OperationId& opId);

    Q_INVOKABLE void cancelOperation(const QString& opIdStr);

signals:
    void activeOperationsChanged();
    void operationCompleted(const QString& opIdStr, bool success, const QString& message);

private slots:
    void onPollTimer();

private:
    std::shared_ptr<fvm::core::application::contracts::IForensicApplication> m_backend;
    QTimer* m_pollTimer;
    std::vector<fvm::core::application::domain::OperationId> m_trackedOperations;
    QVariantList m_activeOperationsData;

    void updateOperations();
};

} // namespace fvm::presentation
