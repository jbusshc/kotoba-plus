#pragma once
#include <QWidget>

namespace Ui {
class SrsDashboard;
}

class SrsDashboard : public QWidget
{
    Q_OBJECT

public:
    explicit SrsDashboard(QWidget *parent = nullptr);
    ~SrsDashboard();

    void updateStats(uint32_t due,
                     uint32_t learning,
                     uint32_t newly,
                     uint32_t lapsed);

signals:
    void startRequested();

private:
    Ui::SrsDashboard *ui;
};
