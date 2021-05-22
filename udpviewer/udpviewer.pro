TARGET=udpviewer 

QT+=widgets multimediawidgets network 
CONFIG+=c++17
INCLUDEPATH+=qkeycode/include

SOURCES=RenderArea.cpp SpectrumRenderArea.cpp udpviewer.cpp MainWindow.cpp \
	UDPListener.cpp \
        VCDWriter.cpp \
        ProgressBar.cpp \
        LineBuffer.cpp \
        keycapturer.cpp \
        qkeycode/src/qkeycode/qkeycode.cpp \
        qkeycode/src/qkeycode/chromium/keycode_converter.cc

HEADERS=RenderArea.h \
	MainWindow.h \
        VCDWriter.h \
        ProgressBar.h \
        SpectrumRenderArea.h \
	ScreenDrawer.h \
        UDPListener.h \
        LineBuffer.h  \
        keycapturer.h

