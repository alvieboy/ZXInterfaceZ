TARGET = interfacez_gui
QT += widgets network

INCLUDEPATH=.. ../QtSpecem
SOURCES=gui.cpp
HEADERS=LogEmitter.h

LIBS+=-L.. -L../QtSpecem -linterfacez -lQtSpecem
