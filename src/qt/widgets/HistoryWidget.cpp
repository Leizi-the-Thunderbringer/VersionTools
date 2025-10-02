#include "HistoryWidget.h"
#include <QVBoxLayout>
#include <QLabel>

HistoryWidget::HistoryWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("History Widget - Coming Soon"), this));
}
