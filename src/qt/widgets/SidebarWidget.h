#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QVBoxLayout>
#include "../core/GitTypes.h"

QT_BEGIN_NAMESPACE
class QListWidgetItem;
QT_END_NAMESPACE

class GitManager;

class SidebarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SidebarWidget(QWidget *parent = nullptr);

    void setRepositoryPath(const QString &path);
    void updateStatus(const VersionTools::GitStatus &status);

signals:
    void selectionChanged(int index);

private slots:
    void onItemClicked(QListWidgetItem *item);

private:
    void setupUI();
    void createNavigationItems();
    void updateStatusCounts();

    QVBoxLayout *m_layout;
    QLabel *m_repositoryLabel;
    QLabel *m_branchLabel;
    QListWidget *m_navigationList;
    QLabel *m_statusLabel;

    QString m_repositoryPath;
    VersionTools::GitStatus m_currentStatus;

    // Navigation items
    QListWidgetItem *m_changesItem;
    QListWidgetItem *m_historyItem;
    QListWidgetItem *m_branchesItem;
    QListWidgetItem *m_remotesItem;
    QListWidgetItem *m_tagsItem;
    QListWidgetItem *m_stashesItem;
};