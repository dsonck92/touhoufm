#-------------------------------------------------
#
# Project created by QtCreator 2015-01-24T14:23:02
#
#-------------------------------------------------

QMAKE_DIR_SEP		= /
QMAKE_COPY		= cp
QMAKE_COPY_DIR		= cp -r
QMAKE_MOVE		= mv
QMAKE_DEL_FILE		= rm -f
QMAKE_MKDIR		= mkdir -p
QMAKE_DEL_DIR		= rm -rf

QMAKE_ZIP		= zip -r -9

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


INSTALLS += target
