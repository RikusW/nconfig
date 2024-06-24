######################################################################
# Automatically generated by qmake (1.03a) Tue Apr 6 22:42:12 2004
######################################################################

# run qmake -makefile kkonfig.pro to generate the Makefile
# or just simply qmake

TEMPLATE = app

# Input
HEADERS += kkonfig.h ../nodes.h
SOURCES += kkonfig.cpp ../nodes.cpp
CONFIG += qt x11 release
QT += widgets

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

