#include "SidebarWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QDir>
#include <QFont>
#include <QIcon>

SidebarWidget::SidebarWidget(QWidget *parent)
    : QWidget(parent)
    , m_layout(nullptr)
    , m_repositoryLabel(nullptr)
    , m_branchLabel(nullptr)
    , m_navigationList(nullptr)
    , m_statusLabel(nullptr)
    , m_changesItem(nullptr)
    , m_historyItem(nullptr)
    , m_branchesItem(nullptr)
    , m_remotesItem(nullptr)
    , m_tagsItem(nullptr)
    , m_stashesItem(nullptr)
{
    setupUI();
    createNavigationItems();
}

void SidebarWidget::setupUI()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(8, 8, 8, 8);
    m_layout->setSpacing(8);

    // Repository section
    auto repositoryFrame = new QWidget;
    repositoryFrame->setObjectName("repositoryFrame");
    auto repositoryLayout = new QVBoxLayout(repositoryFrame);
    repositoryLayout->setContentsMargins(8, 8, 8, 8);

    m_repositoryLabel = new QLabel(tr("No Repository"));
    m_repositoryLabel->setObjectName("repositoryLabel");
    QFont repoFont = m_repositoryLabel->font();
    repoFont.setBold(true);
    repoFont.setPointSize(repoFont.pointSize() + 1);
    m_repositoryLabel->setFont(repoFont);
    repositoryLayout->addWidget(m_repositoryLabel);

    m_branchLabel = new QLabel("");
    m_branchLabel->setObjectName("branchLabel");
    m_branchLabel->setStyleSheet("color: #666;");
    repositoryLayout->addWidget(m_branchLabel);

    m_layout->addWidget(repositoryFrame);

    // Navigation list
    m_navigationList = new QListWidget;
    m_navigationList->setObjectName("navigationList");
    m_navigationList->setFrameStyle(QFrame::NoFrame);
    m_navigationList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_navigationList->setIconSize(QSize(20, 20));
    m_layout->addWidget(m_navigationList);

    // Status section
    m_statusLabel = new QLabel(tr("Ready"));
    m_statusLabel->setObjectName("statusLabel");
    m_statusLabel->setStyleSheet("color: #666; font-size: 11px;");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_layout->addWidget(m_statusLabel);

    m_layout->addStretch();

    // Apply styling
    setStyleSheet(R"(
        #repositoryFrame {
            background-color: rgba(0, 0, 0, 0.05);
            border-radius: 8px;
            border: 1px solid rgba(0, 0, 0, 0.1);
        }
        
        #repositoryLabel {
            color: #333;
        }
        
        #branchLabel {
            color: #666;
            font-size: 12px;
        }
        
        #navigationList {
            background: transparent;
            outline: none;
        }
        
        #navigationList::item {
            padding: 8px 12px;
            border-radius: 6px;
            margin: 2px 0px;
            border: none;
        }
        
        #navigationList::item:hover {
            background-color: rgba(0, 120, 215, 0.1);
        }
        
        #navigationList::item:selected {
            background-color: rgba(0, 120, 215, 0.15);
            color: #0078d4;
        }
        
        #statusLabel {
            color: #666;
            font-size: 11px;
        }
    )");

    connect(m_navigationList, &QListWidget::itemClicked,
            this, &SidebarWidget::onItemClicked);
}

void SidebarWidget::createNavigationItems()
{
    // Changes
    m_changesItem = new QListWidgetItem(QIcon(":/icons/file-text.svg"), tr("Changes"));
    m_changesItem->setData(Qt::UserRole, 0);
    m_navigationList->addItem(m_changesItem);

    // History
    m_historyItem = new QListWidgetItem(QIcon(":/icons/clock.svg"), tr("History"));
    m_historyItem->setData(Qt::UserRole, 1);
    m_navigationList->addItem(m_historyItem);

    // Branches
    m_branchesItem = new QListWidgetItem(QIcon(":/icons/git-branch.svg"), tr("Branches"));
    m_branchesItem->setData(Qt::UserRole, 2);
    m_navigationList->addItem(m_branchesItem);

    // Remotes
    m_remotesItem = new QListWidgetItem(QIcon(":/icons/globe.svg"), tr("Remotes"));
    m_remotesItem->setData(Qt::UserRole, 3);
    m_navigationList->addItem(m_remotesItem);

    // Tags
    m_tagsItem = new QListWidgetItem(QIcon(":/icons/tag.svg"), tr("Tags"));
    m_tagsItem->setData(Qt::UserRole, 4);
    m_navigationList->addItem(m_tagsItem);

    // Stashes
    m_stashesItem = new QListWidgetItem(QIcon(":/icons/archive.svg"), tr("Stashes"));
    m_stashesItem->setData(Qt::UserRole, 5);
    m_navigationList->addItem(m_stashesItem);

    // Select first item by default
    if (m_navigationList->count() > 0) {
        m_navigationList->setCurrentRow(0);
    }
}

void SidebarWidget::setRepositoryPath(const QString &path)
{
    m_repositoryPath = path;

    if (path.isEmpty()) {
        m_repositoryLabel->setText(tr("No Repository"));
        m_branchLabel->setText("");
    } else {
        QDir dir(path);
        m_repositoryLabel->setText(dir.dirName());
    }
}

void SidebarWidget::updateStatus(const VersionTools::GitStatus &status)
{
    m_currentStatus = status;

    // Update branch label
    if (!status.currentBranch.empty()) {
        QString branchText = QString::fromStdString(status.currentBranch);
        if (status.aheadCount > 0 || status.behindCount > 0) {
            branchText += QString(" (%1↑ %2↓)")
                         .arg(status.aheadCount)
                         .arg(status.behindCount);
        }
        m_branchLabel->setText(branchText);
    } else {
        m_branchLabel->setText("");
    }

    // Update status counts
    updateStatusCounts();
}

void SidebarWidget::updateStatusCounts()
{
    // Count changes by type
    int stagedCount = 0;
    int unstagedCount = 0;

    for (const auto &change : m_currentStatus.changes) {
        if (change.isStaged) {
            stagedCount++;
        } else {
            unstagedCount++;
        }
    }

    int totalChanges = stagedCount + unstagedCount;

    // Update changes item text
    if (totalChanges > 0) {
        m_changesItem->setText(tr("Changes (%1)").arg(totalChanges));
    } else {
        m_changesItem->setText(tr("Changes"));
    }

    // Update status label
    if (totalChanges > 0) {
        QString statusText;
        if (stagedCount > 0 && unstagedCount > 0) {
            statusText = tr("%1 staged, %2 unstaged").arg(stagedCount).arg(unstagedCount);
        } else if (stagedCount > 0) {
            statusText = tr("%1 staged").arg(stagedCount);
        } else {
            statusText = tr("%1 unstaged").arg(unstagedCount);
        }
        m_statusLabel->setText(statusText);
    } else {
        m_statusLabel->setText(tr("Working directory clean"));
    }
}

void SidebarWidget::onItemClicked(QListWidgetItem *item)
{
    if (item) {
        int index = item->data(Qt::UserRole).toInt();
        emit selectionChanged(index);
    }
}

#include "SidebarWidget.moc"