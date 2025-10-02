#include "VersionToolsMainWindow.h"
#include "widgets/SidebarWidget.h"
#include "widgets/ChangesWidget.h"
#include "widgets/HistoryWidget.h"
#include "widgets/BranchesWidget.h"
#include "utils/GitWorker.h"
#include "dialogs/SettingsDialog.h"
#include "core/GitManager.h"

#include <QApplication>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QSplitter>
#include <QStackedWidget>
#include <QLabel>
#include <QProgressBar>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>
#include <QTimer>

using namespace VersionTools;

VersionToolsMainWindow::VersionToolsMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_centralSplitter(nullptr)
    , m_contentStack(nullptr)
    , m_sidebarWidget(nullptr)
    , m_changesWidget(nullptr)
    , m_historyWidget(nullptr)
    , m_branchesWidget(nullptr)
    , m_gitManager(std::make_unique<VersionTools::GitManager>())
    , m_gitWorker(nullptr)
{
    setWindowTitle("Version Tools");
    setMinimumSize(1000, 700);
    resize(1400, 900);
    
    // Create Git worker thread
    m_gitWorker = new GitWorker(m_gitManager.get(), this);
    
    createActions();
    createMenus();
    createToolBars();
    createStatusBar();
    setupCentralWidget();
    connectSignals();
    
    loadSettings();
    
    // Auto-open last repository if available
    QSettings settings;
    QString lastRepo = settings.value("lastRepository").toString();
    if (!lastRepo.isEmpty() && QDir(lastRepo).exists()) {
        QTimer::singleShot(100, [this, lastRepo]() {
            onRepositoryOpened(lastRepo);
        });
    }
}

VersionToolsMainWindow::~VersionToolsMainWindow()
{
    saveSettings();
}

void VersionToolsMainWindow::createActions()
{
    // File menu actions
    m_openAction = new QAction(QIcon(":/icons/folder-open.svg"), tr("&Open Repository..."), this);
    m_openAction->setShortcut(QKeySequence::Open);
    m_openAction->setStatusTip(tr("Open an existing Git repository"));
    connect(m_openAction, &QAction::triggered, this, &VersionToolsMainWindow::openRepository);
    
    m_cloneAction = new QAction(QIcon(":/icons/download.svg"), tr("&Clone Repository..."), this);
    m_cloneAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O));
    m_cloneAction->setStatusTip(tr("Clone a repository from a remote URL"));
    connect(m_cloneAction, &QAction::triggered, this, &VersionToolsMainWindow::cloneRepository);
    
    m_refreshAction = new QAction(QIcon(":/icons/refresh.svg"), tr("&Refresh"), this);
    m_refreshAction->setShortcut(QKeySequence::Refresh);
    m_refreshAction->setStatusTip(tr("Refresh repository status"));
    connect(m_refreshAction, &QAction::triggered, this, &VersionToolsMainWindow::refreshRepository);
    
    m_settingsAction = new QAction(QIcon(":/icons/settings.svg"), tr("&Settings..."), this);
    m_settingsAction->setShortcut(QKeySequence::Preferences);
    m_settingsAction->setStatusTip(tr("Configure application settings"));
    connect(m_settingsAction, &QAction::triggered, this, &VersionToolsMainWindow::showSettings);
    
    m_exitAction = new QAction(tr("E&xit"), this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    m_exitAction->setStatusTip(tr("Exit the application"));
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);
    
    // Help menu actions
    m_aboutAction = new QAction(tr("&About Version Tools"), this);
    m_aboutAction->setStatusTip(tr("Show information about Version Tools"));
    connect(m_aboutAction, &QAction::triggered, this, &VersionToolsMainWindow::showAbout);
    
    m_aboutQtAction = new QAction(tr("About &Qt"), this);
    m_aboutQtAction->setStatusTip(tr("Show information about Qt"));
    connect(m_aboutQtAction, &QAction::triggered, qApp, &QApplication::aboutQt);
}

void VersionToolsMainWindow::createMenus()
{
    m_fileMenu = menuBar()->addMenu(tr("&File"));
    m_fileMenu->addAction(m_openAction);
    m_fileMenu->addAction(m_cloneAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_refreshAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_settingsAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_exitAction);
    
    m_viewMenu = menuBar()->addMenu(tr("&View"));
    // View menu items will be added by widgets
    
    m_gitMenu = menuBar()->addMenu(tr("&Git"));
    // Git menu items will be added by widgets
    
    m_helpMenu = menuBar()->addMenu(tr("&Help"));
    m_helpMenu->addAction(m_aboutAction);
    m_helpMenu->addAction(m_aboutQtAction);
}

void VersionToolsMainWindow::createToolBars()
{
    m_mainToolBar = addToolBar(tr("Main"));
    m_mainToolBar->setMovable(false);
    m_mainToolBar->addAction(m_openAction);
    m_mainToolBar->addAction(m_cloneAction);
    m_mainToolBar->addSeparator();
    m_mainToolBar->addAction(m_refreshAction);
    m_mainToolBar->addSeparator();
    m_mainToolBar->addAction(m_settingsAction);
}

void VersionToolsMainWindow::createStatusBar()
{
    m_repositoryLabel = new QLabel(tr("No repository opened"));
    m_repositoryLabel->setMinimumWidth(200);
    statusBar()->addWidget(m_repositoryLabel);
    
    statusBar()->addPermanentWidget(new QLabel("|"));
    
    m_branchLabel = new QLabel("");
    m_branchLabel->setMinimumWidth(100);
    statusBar()->addPermanentWidget(m_branchLabel);
    
    statusBar()->addPermanentWidget(new QLabel("|"));
    
    m_statusLabel = new QLabel(tr("Ready"));
    statusBar()->addPermanentWidget(m_statusLabel);
    
    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    m_progressBar->setMaximumWidth(200);
    statusBar()->addPermanentWidget(m_progressBar);
}

void VersionToolsMainWindow::setupCentralWidget()
{
    m_centralSplitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(m_centralSplitter);
    
    // Create sidebar
    m_sidebarWidget = new SidebarWidget(this);
    m_sidebarWidget->setFixedWidth(250);
    m_centralSplitter->addWidget(m_sidebarWidget);
    
    // Create content stack
    m_contentStack = new QStackedWidget(this);
    m_centralSplitter->addWidget(m_contentStack);
    
    // Create content widgets
    m_changesWidget = new ChangesWidget(m_gitManager.get(), this);
    m_historyWidget = new HistoryWidget(m_gitManager.get(), this);
    m_branchesWidget = new BranchesWidget(m_gitManager.get(), this);
    
    m_contentStack->addWidget(m_changesWidget);
    m_contentStack->addWidget(m_historyWidget);
    m_contentStack->addWidget(m_branchesWidget);
    
    // Set splitter proportions
    m_centralSplitter->setSizes({250, 1150});
    m_centralSplitter->setStretchFactor(0, 0);
    m_centralSplitter->setStretchFactor(1, 1);
}

void VersionToolsMainWindow::connectSignals()
{
    connect(m_sidebarWidget, &SidebarWidget::selectionChanged,
            this, &VersionToolsMainWindow::onSidebarSelectionChanged);
    
    connect(m_gitWorker, &GitWorker::repositoryOpened,
            this, &VersionToolsMainWindow::onRepositoryOpened);
    connect(m_gitWorker, &GitWorker::statusChanged,
            this, &VersionToolsMainWindow::onRepositoryStatusChanged);
    connect(m_gitWorker, &GitWorker::operationStarted,
            this, &VersionToolsMainWindow::onGitOperationStarted);
    connect(m_gitWorker, &GitWorker::operationFinished,
            this, &VersionToolsMainWindow::onGitOperationFinished);
}

void VersionToolsMainWindow::openRepository()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        tr("Open Git Repository"),
        QDir::homePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    
    if (!dir.isEmpty()) {
        m_gitWorker->openRepository(dir);
    }
}

void VersionToolsMainWindow::cloneRepository()
{
    // TODO: Implement clone dialog
    QMessageBox::information(this, tr("Clone Repository"), 
                           tr("Clone repository functionality will be implemented soon."));
}

void VersionToolsMainWindow::refreshRepository()
{
    if (!m_currentRepositoryPath.isEmpty()) {
        m_gitWorker->refreshStatus();
    }
}

void VersionToolsMainWindow::showSettings()
{
    SettingsDialog dialog(this);
    dialog.exec();
}

void VersionToolsMainWindow::showAbout()
{
    QMessageBox::about(this, tr("About Version Tools"),
        tr("<h2>Version Tools 1.0.0</h2>"
           "<p>A modern Git GUI application built with Qt and C++.</p>"
           "<p>Copyright © 2024 VersionTools Project</p>"
           "<p>Built with Qt %1</p>").arg(QT_VERSION_STR));
}

void VersionToolsMainWindow::onSidebarSelectionChanged(int index)
{
    if (index >= 0 && index < m_contentStack->count()) {
        m_contentStack->setCurrentIndex(index);
        
        // Update window title based on selection
        QStringList titles = {tr("Changes"), tr("History"), tr("Branches")};
        if (index < titles.size()) {
            updateWindowTitle();
        }
    }
}

void VersionToolsMainWindow::onRepositoryOpened(const QString &path)
{
    m_currentRepositoryPath = path;
    updateWindowTitle();
    updateStatusBar();
    
    // Save last repository
    QSettings settings;
    settings.setValue("lastRepository", path);
    
    // Update sidebar with repository info
    m_sidebarWidget->setRepositoryPath(path);
    
    // Refresh all widgets
    m_changesWidget->setRepository(path);
    m_historyWidget->setRepository(path);
    m_branchesWidget->setRepository(path);
    
    // Get current branch
    m_gitWorker->refreshStatus();
}

void VersionToolsMainWindow::onRepositoryStatusChanged()
{
    m_currentBranch = QString::fromStdString(m_gitManager->getCurrentBranch());
    updateStatusBar();
    
    // Update sidebar status
    auto status = m_gitManager->getStatus();
    m_sidebarWidget->updateStatus(status);
}

void VersionToolsMainWindow::onGitOperationStarted(const QString &operation)
{
    m_statusLabel->setText(operation);
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, 0); // Indeterminate progress
}

void VersionToolsMainWindow::onGitOperationFinished(const QString &operation, bool success)
{
    Q_UNUSED(operation)
    
    m_progressBar->setVisible(false);
    if (success) {
        m_statusLabel->setText(tr("Ready"));
    } else {
        m_statusLabel->setText(tr("Operation failed"));
        QTimer::singleShot(3000, [this]() {
            m_statusLabel->setText(tr("Ready"));
        });
    }
}

void VersionToolsMainWindow::updateWindowTitle()
{
    QString title = "Version Tools";
    
    if (!m_currentRepositoryPath.isEmpty()) {
        QDir dir(m_currentRepositoryPath);
        title += " - " + dir.dirName();
        
        if (!m_currentBranch.isEmpty()) {
            title += " (" + m_currentBranch + ")";
        }
    }
    
    setWindowTitle(title);
}

void VersionToolsMainWindow::updateStatusBar()
{
    if (m_currentRepositoryPath.isEmpty()) {
        m_repositoryLabel->setText(tr("No repository opened"));
        m_branchLabel->setText("");
    } else {
        QDir dir(m_currentRepositoryPath);
        m_repositoryLabel->setText(dir.dirName());
        m_branchLabel->setText(m_currentBranch.isEmpty() ? tr("No branch") : m_currentBranch);
    }
}

void VersionToolsMainWindow::loadSettings()
{
    QSettings settings;
    settings.beginGroup("MainWindow");
    
    // Restore window geometry
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("state").toByteArray());
    
    // Restore splitter state
    if (m_centralSplitter) {
        m_centralSplitter->restoreState(settings.value("splitterState").toByteArray());
    }
    
    settings.endGroup();
}

void VersionToolsMainWindow::saveSettings()
{
    QSettings settings;
    settings.beginGroup("MainWindow");
    
    // Save window geometry
    settings.setValue("geometry", saveGeometry());
    settings.setValue("state", saveState());
    
    // Save splitter state
    if (m_centralSplitter) {
        settings.setValue("splitterState", m_centralSplitter->saveState());
    }
    
    settings.endGroup();
}

void VersionToolsMainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    event->accept();
}

#include "VersionToolsMainWindow.moc"