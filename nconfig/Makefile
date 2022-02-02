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

all: nconfig tconfig

nconfig: nconfig.o ../nodes.o
	g++ $(ZFLAGS) -lncurses -o nconfig nconfig.o ../nodes.o $(ZCONF)
	
tconfig: tconfig.o ../nodes.o
	g++ $(ZFLAGS) -o tconfig tconfig.o ../nodes.o $(ZCONF)

nconfig.o: nconfig.cpp ../nodes.h
	g++ $(ZFLAGS) -c -Wall -o nconfig.o nconfig.cpp

tconfig.o: tconfig.cpp ../nodes.h
	g++ $(ZFLAGS) -c -Wall -o tconfig.o tconfig.cpp
	
../nodes.o: ../nodes.h ../nodes.cpp
	@cd ..; $(MAKE) nodes.o

clean:
	-rm -f nconfig.o
	-rm -f tconfig.o

distclean: clean
	-rm -f *~
	-rm -f nconfig
	-rm -f tconfig
	
