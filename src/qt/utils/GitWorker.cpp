#include "GitWorker.h"
#include "core/GitManager.h"
#include <QDebug>

using namespace VersionTools;

GitWorker::GitWorker(GitManager *gitManager, QObject *parent)
    : QObject(parent)
    , m_gitManager(gitManager)
{
}

void GitWorker::openRepository(const QString &path)
{
    emit operationStarted(tr("Opening repository..."));
    
    auto result = m_gitManager->openRepository(path.toStdString());
    
    if (result.isSuccess()) {
        emit repositoryOpened(path);
        emit operationFinished(tr("Repository opened"), true);
        
        // Refresh status after opening
        refreshStatus();
    } else {
        emit errorOccurred(QString::fromStdString(result.error));
        emit operationFinished(tr("Failed to open repository"), false);
    }
}

void GitWorker::refreshStatus()
{
    emit operationStarted(tr("Refreshing status..."));
    
    try {
        // Get repository status
        auto status = m_gitManager->getStatus();
        emit statusChanged();
        emit operationFinished(tr("Status refreshed"), true);
    } catch (const std::exception &e) {
        emit errorOccurred(QString::fromUtf8(e.what()));
        emit operationFinished(tr("Failed to refresh status"), false);
    }
}

void GitWorker::stageFiles(const QStringList &files)
{
    emit operationStarted(tr("Staging files..."));
    
    std::vector<std::string> fileList;
    for (const QString &file : files) {
        fileList.push_back(file.toStdString());
    }
    
    auto result = m_gitManager->addFiles(fileList);
    
    if (result.isSuccess()) {
        emit statusChanged();
        emit operationFinished(tr("Files staged"), true);
    } else {
        emit errorOccurred(QString::fromStdString(result.error));
        emit operationFinished(tr("Failed to stage files"), false);
    }
}

void GitWorker::unstageFiles(const QStringList &files)
{
    emit operationStarted(tr("Unstaging files..."));
    
    std::vector<std::string> fileList;
    for (const QString &file : files) {
        fileList.push_back(file.toStdString());
    }
    
    auto result = m_gitManager->resetFiles(fileList);
    
    if (result.isSuccess()) {
        emit statusChanged();
        emit operationFinished(tr("Files unstaged"), true);
    } else {
        emit errorOccurred(QString::fromStdString(result.error));
        emit operationFinished(tr("Failed to unstage files"), false);
    }
}

void GitWorker::commitChanges(const QString &message)
{
    emit operationStarted(tr("Creating commit..."));
    
    auto result = m_gitManager->commit(message.toStdString());
    
    if (result.isSuccess()) {
        emit statusChanged();
        emit operationFinished(tr("Commit created"), true);
    } else {
        emit errorOccurred(QString::fromStdString(result.error));
        emit operationFinished(tr("Failed to create commit"), false);
    }
}

void GitWorker::fetchRepository()
{
    emit operationStarted(tr("Fetching from remote..."));
    
    auto result = m_gitManager->fetch();
    
    if (result.isSuccess()) {
        emit statusChanged();
        emit operationFinished(tr("Fetch completed"), true);
    } else {
        emit errorOccurred(QString::fromStdString(result.error));
        emit operationFinished(tr("Failed to fetch"), false);
    }
}

void GitWorker::pullRepository()
{
    emit operationStarted(tr("Pulling from remote..."));
    
    auto result = m_gitManager->pull();
    
    if (result.isSuccess()) {
        emit statusChanged();
        emit operationFinished(tr("Pull completed"), true);
    } else {
        emit errorOccurred(QString::fromStdString(result.error));
        emit operationFinished(tr("Failed to pull"), false);
    }
}

void GitWorker::pushRepository()
{
    emit operationStarted(tr("Pushing to remote..."));
    
    auto result = m_gitManager->push();
    
    if (result.isSuccess()) {
        emit statusChanged();
        emit operationFinished(tr("Push completed"), true);
    } else {
        emit errorOccurred(QString::fromStdString(result.error));
        emit operationFinished(tr("Failed to push"), false);
    }
}

#include "GitWorker.moc"