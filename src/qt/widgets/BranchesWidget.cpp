#include "BranchesWidget.h"
#include "core/GitManager.h"
#include <QVBoxLayout>
#include <QLabel>

BranchesWidget::BranchesWidget(VersionTools::GitManager *gitManager, QWidget *parent)
    : QWidget(parent)
    , m_gitManager(gitManager)
{
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("Branches Widget - Coming Soon"), this));
}

void BranchesWidget::setRepository(const QString &path)
{
    (void)path;
    // TODO: Load branches
}
