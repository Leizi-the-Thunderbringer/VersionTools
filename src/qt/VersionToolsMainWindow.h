#pragma once

#include <QMainWindow>
#include <QSplitter>
#include <QStackedWidget>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QProgressBar>
#include <memory>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QLabel;
QT_END_NAMESPACE

class SidebarWidget;
class ChangesWidget;
class HistoryWidget;
class BranchesWidget;
class DiffViewerWidget;
class GitWorker;
class GitManager;

class VersionToolsMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    VersionToolsMainWindow(QWidget *parent = nullptr);
    ~VersionToolsMainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void openRepository();
    void cloneRepository();
    void refreshRepository();
    void showSettings();
    void showAbout();
    void onSidebarSelectionChanged(int index);
    void onRepositoryOpened(const QString &path);
    void onRepositoryStatusChanged();
    void onGitOperationStarted(const QString &operation);
    void onGitOperationFinished(const QString &operation, bool success);

private:
    void createActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void setupCentralWidget();
    void connectSignals();
    void updateWindowTitle();
    void updateStatusBar();
    void loadSettings();
    void saveSettings();

    // UI Components
    QSplitter *m_centralSplitter;
    QStackedWidget *m_contentStack;
    SidebarWidget *m_sidebarWidget;
    ChangesWidget *m_changesWidget;
    HistoryWidget *m_historyWidget;
    BranchesWidget *m_branchesWidget;
    
    // Menus and Actions
    QMenu *m_fileMenu;
    QMenu *m_viewMenu;
    QMenu *m_gitMenu;
    QMenu *m_helpMenu;
    
    QAction *m_openAction;
    QAction *m_cloneAction;
    QAction *m_refreshAction;
    QAction *m_settingsAction;
    QAction *m_exitAction;
    QAction *m_aboutAction;
    QAction *m_aboutQtAction;
    
    // Toolbars
    QToolBar *m_mainToolBar;
    
    // Status bar
    QLabel *m_repositoryLabel;
    QLabel *m_branchLabel;
    QLabel *m_statusLabel;
    QProgressBar *m_progressBar;
    
    // Git integration
    std::unique_ptr<GitManager> m_gitManager;
    GitWorker *m_gitWorker;

    QString m_currentRepositoryPath;
    QString m_currentBranch;
};