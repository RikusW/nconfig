#
# is kernel 2.6.x available ?
# 
LASTZCONF = $(shell if [ -f ../.lkc ]; then cat ../.lkc; else echo ""; fi)
ZFLAGS = $(shell if [ -f ../kconfig/zconf.tab.o ]; then echo "-DLKC26"; else echo ""; fi)

ifneq ($(ZFLAGS),$(LASTZCONF))
.PHONY: ../nodes.o
endif

ZCONF =
ifeq ("$(ZFLAGS)", "-DLKC26")
  ZCONF = ../kconfig/zconf.tab.o
endif
#
# 
#

ZFLAGS += -g

all: gconfig

gconfig: gconfig.o ../nodes.o
	g++ $(ZFLAGS) -o gconfig ../nodes.o gconfig.o $(ZCONF) \
	    `pkg-config --libs gtk+-2.0 libgnomeui-2.0`
	
gconfig.o: gconfig.cpp
	g++ $(ZFLAGS) -c -o gconfig.o gconfig.cpp \
	    `pkg-config --cflags glib-2.0 gtk+-2.0 libgnomeui-2.0`

gconfig1: gconfig1.cpp ../nodes.o
	@echo
	@echo 'my first try using gnome 1, abandoned before even starting... (60 lines)'
	@echo 'only included for sake of interest.'
	@echo
	g++ -o gconfig1 gconfig1.cpp ../nodes.o $(ZFLAGS) $(ZCONF) \
	    `gnome-config --cflags --libs gnomeui gtk`

../nodes.o: ../nodes.cpp ../nodes.h
	$(MAKE) -C ../ nodes.o

clean:
	-rm -f gconfig.o

distclean: clean
	-rm -f *~
	-rm -f gconfig
	-rm -f gconfig1

