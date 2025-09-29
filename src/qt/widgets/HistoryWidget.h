#pragma once

#include <QWidget>

class QListWidget;
class QTextEdit;

class HistoryWidget : public QWidget {
    Q_OBJECT

public:
    explicit HistoryWidget(QWidget *parent = nullptr);

signals:
    void commitSelected(const QString& commitHash);

private:
    QListWidget* m_commitList;
    QTextEdit* m_commitDetails;
};