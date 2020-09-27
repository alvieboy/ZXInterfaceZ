TARGET = interfacez_gui
QT += widgets network

INCLUDEPATH=.. ../QtSpecem
SOURCES=gui.cpp
HEADERS=LogEmitter.h
                                       
CONFIG += link_pkgconfig
PKGCONFIG += valgrind

LIBS+=-L.. -L../QtSpecem -linterfacez -lQtSpecem 
