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
