#pragma once

#include <QWidget>
#include <QSplitter>
#include <QListWidget>
#include <QTextEdit>
#include <QLabel>
#include <QToolBar>
#include <QPushButton>
#include "../core/GitTypes.h"

QT_BEGIN_NAMESPACE
class QVBoxLayout;
class QHBoxLayout;
class QListWidgetItem;
class QTextEdit;
class QLineEdit;
QT_END_NAMESPACE

namespace VersionTools {
class GitManager;
}

class FileStatusItem;

class ChangesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChangesWidget(VersionTools::GitManager *gitManager, QWidget *parent = nullptr);

    void setRepository(const QString &path);
    void refreshChanges();

private slots:
    void onStageAllClicked();
    void onUnstageAllClicked();
    void onCommitClicked();
    void onFileItemChanged(QListWidgetItem *item);
    void onFileSelectionChanged();
    void showCommitDialog();

private:
    void setupUI();
    void updateFileList();
    void updateCommitButton();
    void stageFile(const QString &filePath);
    void unstageFile(const QString &filePath);

    VersionTools::GitManager *m_gitManager;
    QString m_repositoryPath;

    // UI Components
    QVBoxLayout *m_layout;
    QToolBar *m_toolbar;
    QSplitter *m_splitter;
    
    // Left panel - file list
    QWidget *m_fileListPanel;
    QListWidget *m_stagedList;
    QListWidget *m_unstagedList;
    
    // Right panel - diff view
    QWidget *m_diffPanel;
    QTextEdit *m_diffView;
    
    // Actions
    QPushButton *m_stageAllButton;
    QPushButton *m_unstageAllButton;
    QPushButton *m_commitButton;
    
    VersionTools::GitStatus m_currentStatus;
};