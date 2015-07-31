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
    /*!
     * \brief Report
     * \param title The title of the dialog
     * \param reason The main text of the dialog
     * \param whats A list of possible "what is wrong" values
     * \param parent Owner of the dialog
     */
    explicit Report(QString title, QString reason, QStringList whats, QWidget *parent = 0);
    ~Report();

    /*!
     * \brief getWhat
     * \return
     */
    QString getWhat();
    /*!
     * \brief getDetail
     * \return
     */
    QString getDetail();

private:
    Ui::Report *ui;
};

#endif // REPORT_H
