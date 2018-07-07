QT += core gui widgets

TARGET = zs-gdiff
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    main.cpp \
    ZSDiff.cpp \
    CodeView.cpp \
    GuiColorScheme.cpp

HEADERS += \
    ZSDiff.hpp \
    CodeView.hpp \
    GuiColorScheme.hpp

FORMS += \
    zsdiff.ui

LIBS += -L$$OUT/ -lzograscope
LIBS += -lboost_iostreams -lboost_program_options -lboost_filesystem
LIBS += -lboost_system

INCLUDEPATH += $$PWD/../../src
DEPENDPATH += $$PWD/../../src
INCLUDEPATH += $$PWD/../../third-party
DEPENDPATH += $$PWD/../../third-party

PRE_TARGETDEPS += $$OUT/libzograscope.a
