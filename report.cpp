#include "report.h"
#include "ui_report.h"

Report::Report(QString title, QString reason, QStringList whats, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Report)
{
    ui->setupUi(this);

    setWindowTitle(title);

    ui->lblReason->setText(reason);
    ui->cbWhat->addItems(whats);
}

Report::~Report()
{
    delete ui;
}

QString Report::getWhat()
{
    return ui->cbWhat->currentText();
}

QString Report::getDetail()
{
    return ui->leDetails->text();
}
