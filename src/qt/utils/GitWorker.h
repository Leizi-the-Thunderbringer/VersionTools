#pragma once

#include <QObject>
#include <QThread>
#include <QString>

class GitManager;

class GitWorker : public QObject
{
    Q_OBJECT

public:
    explicit GitWorker(GitManager *gitManager, QObject *parent = nullptr);

public slots:
    void openRepository(const QString &path);
    void refreshStatus();
    void stageFiles(const QStringList &files);
    void unstageFiles(const QStringList &files);
    void commitChanges(const QString &message);
    void fetchRepository();
    void pullRepository();
    void pushRepository();

signals:
    void repositoryOpened(const QString &path);
    void statusChanged();
    void operationStarted(const QString &operation);
    void operationFinished(const QString &operation, bool success);
    void errorOccurred(const QString &error);

private:
    GitManager *m_gitManager;
};