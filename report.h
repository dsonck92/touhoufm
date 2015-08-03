#ifndef REPORT_H
#define REPORT_H

#include <QDialog>

namespace Ui {
class Report;
}

class Report : public QDialog
{
    Q_OBJECT

public:
    // Create a new Report dialog
    explicit Report(QString title, QString reason, QStringList whats, QWidget *parent = 0);
    ~Report();

    // Get the reason for reporting
    QString getWhat();
    // Get the details the user specified
    QString getDetail();

private:
    Ui::Report *ui;
};

#endif // REPORT_H
