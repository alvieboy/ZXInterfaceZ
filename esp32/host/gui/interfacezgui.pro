
TARGET = interfacez_gui
QT += widgets network

CONFIG += link_pkgconfig debug

DEFINES+=FPGA_USE_SOCKET_PROTOCOL

INCLUDEPATH=.. ../QtSpecem ../../main/
SOURCES=gui.cpp ansi2html.c
HEADERS=LogEmitter.h gui.h
PKGCONFIG += libusb-1.0
                                       
LIBS+=-L.. -L../QtSpecem -linterfacez -lQtSpecem 

include(../QtGifImage/src/gifimage/qtgifimage.pri)
