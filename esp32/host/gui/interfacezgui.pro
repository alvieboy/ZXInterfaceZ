TARGET = interfacez_gui
QT += widgets network

INCLUDEPATH=.. ../QtSpecem ../../main/
SOURCES=gui.cpp ansi2html.c
HEADERS=LogEmitter.h
                                       
LIBS+=-L.. -L../QtSpecem -linterfacez -lQtSpecem 
