#include "touhoufm.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    // Create the application object
    QApplication a(argc, argv);
    // Set the global application details like company and app name
    a.setOrganizationName("TouHou.FM");
    a.setOrganizationDomain("touhou.fm");
    a.setApplicationName("TouHou.FM Player");
    a.setApplicationVersion("0.9");

    // Create the main window
    TouHouFM w;
    w.show(); // and show

    return a.exec(); // run the event loop
}
