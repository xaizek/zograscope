QT += core gui widgets

TARGET = zs-gdiff
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    main.cpp \
    ZSDiff.cpp \
    CodeView.cpp \
    GuiColorScheme.cpp \
    SynHi.cpp \
    BlankLineAttr.cpp \
    FoldTextAttr.cpp \
    DiffList.cpp \
    Repository.cpp

HEADERS += \
    ZSDiff.hpp \
    CodeView.hpp \
    GuiColorScheme.hpp \
    SynHi.hpp \
    BlankLineAttr.hpp \
    FoldTextAttr.hpp \
    DiffList.hpp \
    Repository.hpp

FORMS += \
    zsdiff.ui

LIBS += -L$$OUT/ -lzograscope
LIBS += -lboost_iostreams -lboost_program_options -lboost_filesystem
LIBS += -lboost_system -lgit2

INCLUDEPATH += $$PWD/../../src
DEPENDPATH += $$PWD/../../src
INCLUDEPATH += $$PWD/../../third-party
DEPENDPATH += $$PWD/../../third-party

PRE_TARGETDEPS += $$OUT/libzograscope.a

RESOURCES += \
    resources.qrc
