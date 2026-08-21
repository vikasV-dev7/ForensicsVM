#pragma once

#include <QObject>
#include <QString>
#include <memory>
#include "core/application/contracts/IForensicApplication.hpp"
#include "CaseDashboardViewModel.hpp"

namespace fvm::presentation {

class AppViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isCaseOpen READ isCaseOpen NOTIFY caseOpenChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(CaseDashboardViewModel* dashboard READ dashboard CONSTANT)

public:
    explicit AppViewModel(std::shared_ptr<fvm::core::application::contracts::IForensicApplication> backend, QObject* parent = nullptr);
    ~AppViewModel() override;

    bool isCaseOpen() const;
    QString errorMessage() const;
    CaseDashboardViewModel* dashboard() const;

    Q_INVOKABLE void createCase(const QString& caseRoot, const QString& caseName, const QString& investigator);
    Q_INVOKABLE void openCase(const QString& caseRoot);
    Q_INVOKABLE void closeCase();

signals:
    void caseOpenChanged();
    void errorMessageChanged();

private:
    void setErrorMessage(const QString& msg);

    std::shared_ptr<fvm::core::application::contracts::IForensicApplication> m_backend;
    CaseDashboardViewModel* m_dashboard;
    QString m_errorMessage;
};

} // namespace fvm::presentation
