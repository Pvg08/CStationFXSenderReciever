#-------------------------------------------------
#
# Project created by QtCreator 2016-06-19T19:47:45
#
#-------------------------------------------------

QT       += core gui
QT       += serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = CStationFXSender
TEMPLATE = app

SOURCES += main.cpp\
    mainwindow.cpp \
    serialfxwriter.cpp \
    datagenerator.cpp \
    crc16.cpp \
    datageneratorledscreen.cpp

HEADERS  += mainwindow.h \
    serialfxwriter.h \
    datagenerator.h \
    crc16.h \
    datageneratorledscreen.h

FORMS    += mainwindow.ui
