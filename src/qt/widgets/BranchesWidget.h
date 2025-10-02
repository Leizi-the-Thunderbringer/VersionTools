#pragma once

#include <QWidget>

namespace VersionTools {
class GitManager;
}

class BranchesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BranchesWidget(VersionTools::GitManager *gitManager, QWidget *parent = nullptr);
    ~BranchesWidget() override = default;

    void setRepository(const QString &path);

signals:
    void branchChanged(const QString &branchName);

private:
    VersionTools::GitManager *m_gitManager;
};
