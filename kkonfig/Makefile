#############################################################################
# Makefile for building: kkonfig
# Generated by qmake (1.07a) (Qt 3.3.7) on: Mon Mar 21 14:13:53 2011
# Project:  kkonfig.pro
# Template: app
# Command: $(QMAKE) -o Makefile kkonfig.pro
#############################################################################

####### Compiler, tools and options

CC       = gcc
CXX      = g++
LEX      = flex
YACC     = yacc
CFLAGS   = -pipe -Wall -W -O2 -D_REENTRANT  -DLKC26 -DQT_NO_DEBUG -DQT_THREAD_SUPPORT -DQT_SHARED -DQT_TABLET_SUPPORT
CXXFLAGS = -pipe -Wall -W -O2 -D_REENTRANT  -DLKC26 -DQT_NO_DEBUG -DQT_THREAD_SUPPORT -DQT_SHARED -DQT_TABLET_SUPPORT
LEXFLAGS = 
YACCFLAGS= -d
INCPATH  = -I/usr/share/qt3/mkspecs/linux-g++ -I. -I../../../include/qt3 -I/usr/X11R6/include
LINK     = g++
LFLAGS   = 
LIBS     = $(SUBLIBS) -L/usr/share/qt3/lib -L/usr/X11R6/lib ../kconfig/zconf.tab.o -lqt-mt -lXext -lX11 -lm -lpthread
AR       = ar cqs
RANLIB   = 
MOC      = /usr/share/qt3/bin/moc
UIC      = /usr/share/qt3/bin/uic
QMAKE    = qmake
TAR      = tar -cf
GZIP     = gzip -9f
COPY     = cp -f
COPY_FILE= $(COPY)
COPY_DIR = $(COPY) -r
INSTALL_FILE= $(COPY_FILE)
INSTALL_DIR = $(COPY_DIR)
DEL_FILE = rm -f
SYMLINK  = ln -sf
DEL_DIR  = rmdir
MOVE     = mv -f
CHK_DIR_EXISTS= test -d
MKDIR    = mkdir -p

####### Output directory

OBJECTS_DIR = ./

####### Files

HEADERS = kkonfig.h \
		../nodes.h
SOURCES = kkonfig.cpp \
		../nodes.cpp
OBJECTS = kkonfig.o \
		nodes.o
FORMS = 
UICDECLS = 
UICIMPLS = 
SRCMOC   = moc_kkonfig.cpp
OBJMOC = moc_kkonfig.o
DIST	   = kkonfig.pro
QMAKE_TARGET = kkonfig
DESTDIR  = 
TARGET   = kkonfig

first: all
####### Implicit rules

.SUFFIXES: .c .o .cpp .cc .cxx .C

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.cc.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.cxx.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.C.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.c.o:
	$(CC) -c $(CFLAGS) $(INCPATH) -o $@ $<

####### Build rules

all: Makefile $(TARGET)

$(TARGET):  $(UICDECLS) $(OBJECTS) $(OBJMOC)  
	$(LINK) $(LFLAGS) -o $(TARGET) $(OBJECTS) $(OBJMOC) $(OBJCOMP) $(LIBS)

mocables: $(SRCMOC)
uicables: $(UICDECLS) $(UICIMPLS)

$(MOC): 
	( cd $(QTDIR)/src/moc && $(MAKE) )

Makefile: kkonfig.pro  /usr/share/qt3/mkspecs/linux-g++/qmake.conf ../../../share/qt3/lib/libqt-mt.prl
	$(QMAKE) -o Makefile kkonfig.pro
qmake: 
	@$(QMAKE) -o Makefile kkonfig.pro

dist: 
	@mkdir -p .tmp/kkonfig && $(COPY_FILE) --parents $(SOURCES) $(HEADERS) $(FORMS) $(DIST) .tmp/kkonfig/ && ( cd `dirname .tmp/kkonfig` && $(TAR) kkonfig.tar kkonfig && $(GZIP) kkonfig.tar ) && $(MOVE) `dirname .tmp/kkonfig`/kkonfig.tar.gz . && $(DEL_FILE) -r .tmp/kkonfig

mocclean:
	-$(DEL_FILE) $(OBJMOC)
	-$(DEL_FILE) $(SRCMOC)

uiclean:

yaccclean:
lexclean:
clean: mocclean
	-$(DEL_FILE) $(OBJECTS)
	-$(DEL_FILE) *~ core *.core


####### Sub-libraries

distclean: clean
	-$(DEL_FILE) $(TARGET) $(TARGET)


FORCE:

####### Compile

kkonfig.o: kkonfig.cpp kkonfig.h \
		../nodes.h \
		../kconfig/lkc.h \
		../kconfig/expr.h \
		../kconfig/lkc_defs.h \
		../kconfig/lkc_proto.h

nodes.o: ../nodes.cpp ../nodes.h \
		../kconfig/lkc.h \
		../kconfig/expr.h \
		../kconfig/lkc_defs.h \
		../kconfig/lkc_proto.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o nodes.o ../nodes.cpp

moc_kkonfig.o: moc_kkonfig.cpp  kkonfig.h 

moc_kkonfig.cpp: $(MOC) kkonfig.h
	$(MOC) kkonfig.h -o moc_kkonfig.cpp

####### Install

install:  

uninstall:  

