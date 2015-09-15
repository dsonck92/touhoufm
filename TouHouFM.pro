#-------------------------------------------------
#
# Project created by QtCreator 2015-01-24T14:23:02
#
#-------------------------------------------------

include(touhoufmsocket/touhoufmsocket.pri)

QT       += core gui
QT       += websockets multimedia
QT       += svg
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = TouHouFM

isEmpty(PREFIX) {
  PREFIX="/usr/local"
}

target.path = $$PREFIX/bin

TEMPLATE = app


SOURCES += main.cpp\
        touhoufm.cpp \
    report.cpp

HEADERS  += touhoufm.h \
    report.h

FORMS    += \
    report.ui

RESOURCES += \
    resource.qrc

TRANSLATIONS += \
    touhoufm_nl.ts \
    touhoufm_ja.ts

INSTALLS += target
