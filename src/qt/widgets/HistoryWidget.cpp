#include "HistoryWidget.h"
#include "core/GitManager.h"
#include <QVBoxLayout>
#include <QLabel>

HistoryWidget::HistoryWidget(VersionTools::GitManager *gitManager, QWidget *parent)
    : QWidget(parent)
    , m_gitManager(gitManager)
{
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("History Widget - Coming Soon"), this));
}

void HistoryWidget::setRepository(const QString &path)
{
    m_repositoryPath = path;
    // TODO: Load history
}
