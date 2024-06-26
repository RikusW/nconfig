
LASTLKC26 = $(shell if [ -f .lkc ]; then cat .lkc; else echo ""; fi)
LKC26 = $(shell if [ -f kconfig/zconf.tab.o ]; then echo "-DLKC26"; else echo ""; fi)

NCFLAGS = -g
NCFLAGS += $(LKC26)
NCFLAGS += -Wno-write-strings

nodefiles = nodes.cpp nodes.h
ifneq ($(LKC26),$(LASTLKC26))
	nodefiles += zconf
endif

#-----

#all: nodes.o nconfig tconfig gconfig kkonfig
all: nodes.o nconfig tconfig gconfig

#-----

.PHONY: zconf
zconf:
	@if [ -f kconfig/zconf.tab.c ]; then \
	    $(MAKE) -s -C kconfig zconf.tab.o; \
	fi; \
	if [ "$(LASTLKC26)" != "$(LKC26)" ]; then \
	    echo $(LKC26) > .lkc; \
	    rm -f nodes.o; \
	fi; \

#	    rm -f kkonfigQt3/Makefile; \
#-----

nodes.o: $(nodefiles)
	@if [ "$(LKC26)" = "-DLKC26" ]; then \
	    echo "2.6.x Support Enabled"; \
	else \
	    echo "2.6.x Support Disabled"; \
	    echo "To enable: ln -s /kernel-2.6.x/scripts/kconfig kconfig"; \
	fi;
	g++ -c $(NCFLAGS) -Wall -o nodes.o nodes.cpp

#-----

.PHONY: tconfig
tconfig: nodes.o
	@$(MAKE) -C nconfig tconfig

.PHONY: nconfig
nconfig: nodes.o
	@$(MAKE) -C nconfig nconfig

#-----

./kkonfigQt3/Makefile: ./kkonfigQt3/kkonfig.pro
	@cd kkonfigQt3; . ./setqt; qmake

.PHONY: kkonfig
kkonfig: nodes.o kkonfigQt3/Makefile
	@. kkonfigQt3/setqt > /dev/null; $(MAKE) -C kkonfigQt3 kkonfig

#-----

.PHONY: gconfig
gconfig: nodes.o
	@$(MAKE) -C gconfig gconfig

#-----

bzfiles =\
 nconfig-0.4/Makefile nconfig-0.4/nodes.h nconfig-0.4/nodes.cpp nconfig-0.4/DOC nconfig-0.4/README\
 nconfig-0.4/TODO nconfig-0.4/kconfig nconfig-0.4/qconf.patch\
 nconfig-0.4/gconfig/Makefile nconfig-0.4/gconfig/gconfig.cpp nconfig-0.4/gconfig/gconfig1.cpp\
 nconfig-0.4/nconfig/Makefile nconfig-0.4/nconfig/nconfig.cpp nconfig-0.4/nconfig/tconfig.cpp\
 nconfig-0.4/kkonfigQt3/kkonfig.pro nconfig-0.4/kkonfigQt3/kkonfig.cpp nconfig-0.4/kkonfigQt3/kkonfig.h\
 nconfig-0.4/kkonfigQt3/setqt nconfig-0.4/kkonfigQt3/Makefile

.PHONY: $(bzfiles)

nconfig.tbz: $(bzfiles)
	@cd ..; tar -cjvf nconfig-0.4.tbz $(bzfiles);  # mv nconfig.tbz nconfig-0.4/

.PHONY: bz
bz: nconfig.tbz

nconfig9.tbz: $(bzfiles)
	@cd ..; tar -cvf nconfig.tar $(bzfiles); bzip2 -9 nconfig.tar; mv nconfig.tar.bz nconfig/nconfig.tbz

.PHONY: bz9
bz9: nconfig.tbz


clean:
	-rm -f nodes.o
	$(MAKE) -C nconfig clean
	$(MAKE) -C gconfig clean
	. kkonfigQt3/setqt; $(MAKE) -C kkonfigQt3 clean

distclean:
	-rm -f *~
	-rm -f .lkc
	-rm -f nodes.o
	$(MAKE) -C nconfig distclean
	$(MAKE) -C kkonfigQt3 distclean
	$(MAKE) -C gconfig distclean


