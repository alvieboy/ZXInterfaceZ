TARGET=udpviewer 

QT+=widgets multimediawidgets network 

SOURCES=RenderArea.cpp SpectrumRenderArea.cpp udpviewer.cpp MainWindow.cpp \
	UDPListener.cpp \
        VCDWriter.cpp \
        ProgressBar.cpp \
        LineBuffer.cpp

HEADERS=RenderArea.h \
	MainWindow.h \
        VCDWriter.h \
        ProgressBar.h \
        SpectrumRenderArea.h \
	ScreenDrawer.h \
        UDPListener.h \
        LineBuffer.h
