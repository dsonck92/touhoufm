#-------------------------------------------------
#
# Project created by QtCreator 2015-01-24T14:23:02
#
#-------------------------------------------------

QT       += core gui
QT       += websockets multimedia
QT       += svg
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = TouHouFM

isEmpty(PREFIX) {
  PREFIX="/usr/local"
}

target.path = $$PREFIX/bin

desktop.path = $$PREFIX/share/applications
desktop.files = TouHouFM.desktop

icons.path = $$PREFIX/share/icons/hicolor/
icons.files = hicolor/*

TEMPLATE = app


SOURCES += main.cpp\
        touhoufm.cpp \
    report.cpp \
    touhoufmsocket.cpp

HEADERS  += touhoufm.h \
    report.h \
    touhoufmsocket.h

FORMS    += \
    report.ui

RESOURCES += \
    resource.qrc


INSTALLS += target \
            desktop \
            icons

DISTFILES += \
    TouHouFM.desktop
