#include "ChangesWidget.h"
#include <QVBoxLayout>
#include <QLabel>

ChangesWidget::ChangesWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("Changes Widget - Coming Soon"), this));
}
