#include "AppViewModel.hpp"
#include <QDir>
#include <QDebug>
#include <filesystem>
#include "core/application/domain/CaseMetadata.hpp"
#include "core/application/domain/ApplicationError.hpp"

namespace fvm::presentation {

AppViewModel::AppViewModel(std::shared_ptr<fvm::core::application::contracts::IForensicApplication> backend, QObject* parent)
    : QObject(parent), m_backend(std::move(backend)), m_dashboard(new CaseDashboardViewModel(m_backend, this)) {
}

AppViewModel::~AppViewModel() {
    if (m_backend && m_backend->isCaseOpen()) {
        m_backend->closeCase();
    }
}

bool AppViewModel::isCaseOpen() const {
    return m_backend && m_backend->isCaseOpen();
}

QString AppViewModel::errorMessage() const {
    return m_errorMessage;
}

CaseDashboardViewModel* AppViewModel::dashboard() const {
    return m_dashboard;
}

void AppViewModel::setErrorMessage(const QString& msg) {
    if (m_errorMessage != msg) {
        m_errorMessage = msg;
        emit errorMessageChanged();
        if (!msg.isEmpty()) {
            qWarning() << "AppViewModel Error:" << msg;
        }
    }
}

void AppViewModel::createCase(const QString& caseRoot, const QString& caseName, const QString& investigator) {
    if (!m_backend) return;
    setErrorMessage("");
    
    // In Qt 6, toStdString handles UTF-8 automatically
    std::filesystem::path root(caseRoot.toStdString());
    
    fvm::core::application::domain::CaseMetadata meta;
    meta.name = caseName.toStdString();
    meta.investigator = investigator.toStdString();
    
    auto result = m_backend->createCase(root, meta);
    if (!result.has_value()) {
        setErrorMessage(QString::fromStdString(result.error().what()));
        return;
    }
    
    openCase(caseRoot); // Auto-open after create
    
    emit caseOpenChanged();
    m_dashboard->refresh();
}

void AppViewModel::openCase(const QString& caseRoot) {
    if (!m_backend) return;
    setErrorMessage("");
    
    std::filesystem::path root(caseRoot.toStdString());
    auto result = m_backend->openCase(root);
    if (!result.has_value()) {
        setErrorMessage(QString::fromStdString(result.error().what()));
        return;
    }
    
    emit caseOpenChanged();
    m_dashboard->refresh();
}

void AppViewModel::closeCase() {
    if (!m_backend) return;
    m_backend->closeCase();
    emit caseOpenChanged();
}

} // namespace fvm::presentation
