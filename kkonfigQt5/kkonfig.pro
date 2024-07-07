#Qt5.15

TEMPLATE = app

# Input
#TODO fix these later
QMAKE_CXXFLAGS += -Wno-unused-parameter -Wno-write-strings -Wno-implicit-fallthrough -Wno-unused-result
QMAKE_CXXFLAGS += -g -Og
HEADERS += kkonfig.h kktextbrowser.h ../nodes.h
SOURCES += kkonfig.cpp kktextbrowser.cpp ../nodes.cpp
OBJECTS += icons32aa.o
CONFIG += qt x11 release
QT += widgets gui

!exists(icons32aa.png) {
    error("icons32aa.png is missing")
}

icons.target = icons32aa.o
icons.commands = ld -o icons32aa.o -r -b binary icons32aa.png
icons.depends = icons32aa.png
QMAKE_EXTRA_TARGETS += icons
system("ld -o icons32aa.o -r -b binary icons32aa.png")

exists( ../kconfig/zconf.tab.c ) {
    message("Enabling Kernel 2.6.x support for kkonfig.")
    DEFINES += LKC26
    LIBS += ../kconfig/zconf.tab.o
# add zconf to LIBS !not! OBJECTS because Qt will then clean it as well....
}
!exists( ../kconfig/zconf.tab.c ) {
    message("Disabling Kernel 2.6.x support for kkonfig.")
    message("Add a symlink ../kconfig to the 2.6.x kernel first.")
}

#INCLUDEPATH += /opt/kde3/include
#LIBS += -L/opt/kde3/lib

