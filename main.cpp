#include "touhoufm.h"
#include <QApplication>
#include <QTranslator>

int main(int argc, char *argv[])
{
    // Create the application object
    QApplication a(argc, argv);

    QString locale = QLocale::system().name();

    QTranslator translator;
    translator.load(QString("touhoufm_") + locale);
    a.installTranslator(&translator);

    // Set the global application details like company and app name
    a.setOrganizationName("TouHou.FM");
    a.setOrganizationDomain("touhou.fm");
    a.setApplicationName("TouHou.FM Player");
    a.setApplicationVersion("1.0");

    // Create the main window
    TouHouFM w;
    w.show(); // and show

    return a.exec(); // run the event loop
}
