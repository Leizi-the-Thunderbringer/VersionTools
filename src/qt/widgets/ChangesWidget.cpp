#include "ChangesWidget.h"
#include "core/GitManager.h"
#include <QVBoxLayout>
#include <QLabel>

ChangesWidget::ChangesWidget(VersionTools::GitManager *gitManager, QWidget *parent)
    : QWidget(parent)
    , m_gitManager(gitManager)
{
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("Changes Widget - Coming Soon"), this));
}

void ChangesWidget::setRepository(const QString &path)
{
    m_repositoryPath = path;
    // TODO: Load changes
}

void ChangesWidget::refreshChanges()
{
    // TODO: Refresh changes
}

void ChangesWidget::onStageAllClicked() {}
void ChangesWidget::onUnstageAllClicked() {}
void ChangesWidget::onCommitClicked() {}
void ChangesWidget::onFileItemChanged(QListWidgetItem *) {}
void ChangesWidget::onFileSelectionChanged() {}
void ChangesWidget::showCommitDialog() {}
void ChangesWidget::setupUI() {}
void ChangesWidget::updateFileList() {}
void ChangesWidget::updateCommitButton() {}
void ChangesWidget::stageFile(const QString &) {}
void ChangesWidget::unstageFile(const QString &) {}
