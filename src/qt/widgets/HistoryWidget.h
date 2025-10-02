#pragma once

#include <QWidget>

class QListWidget;
class QTextEdit;

namespace VersionTools {
class GitManager;
}

class HistoryWidget : public QWidget {
    Q_OBJECT

public:
    explicit HistoryWidget(VersionTools::GitManager *gitManager, QWidget *parent = nullptr);

    void setRepository(const QString &path);

signals:
    void commitSelected(const QString& commitHash);

private:
    VersionTools::GitManager *m_gitManager;
    QString m_repositoryPath;
    QListWidget* m_commitList;
    QTextEdit* m_commitDetails;
};