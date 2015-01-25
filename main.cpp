#include "touhoufm.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("TouHou.FM");
    a.setOrganizationDomain("touhou.fm");
    a.setApplicationName("TouHou.FM Player");
    a.setApplicationVersion("1.0");
    TouHouFM w;
    w.show();

    return a.exec();
}
