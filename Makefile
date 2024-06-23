
LASTLKC26 = $(shell if [ -f .lkc ]; then cat .lkc; else echo ""; fi)
LKC26 = $(shell if [ -f kconfig/zconf.tab.o ]; then echo "-DLKC26"; else echo ""; fi)

NCFLAGS = -g
NCFLAGS += $(LKC26)

nodefiles = nodes.cpp nodes.h
ifneq ($(LKC26),$(LASTLKC26))
	nodefiles += zconf
endif

#-----

#all: nodes.o nconfig tconfig gconfig kkonfig
all: nodes.o tconfig

#-----

.PHONY: zconf
zconf:
	@if [ -f kconfig/zconf.tab.c ]; then \
	    $(MAKE) -s -C kconfig zconf.tab.o; \
	fi; \
	if [ "$(LASTLKC26)" != "$(LKC26)" ]; then \
	    echo $(LKC26) > .lkc; \
	    rm -f kkonfig/Makefile; \
	    rm -f nodes.o; \
	fi; \

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

./kkonfig/Makefile: ./kkonfig/kkonfig.pro
	@cd kkonfig; . ./setqt; qmake

.PHONY: kkonfig
kkonfig: nodes.o kkonfig/Makefile
	@. kkonfig/setqt > /dev/null; $(MAKE) -C kkonfig kkonfig

#-----

.PHONY: gconfig
gconfig: nodes.o
	@$(MAKE) -C gconfig gconfig

#-----


.PHONY: gconfig1
gconfig1: nodes.o
	@$(MAKE) -C gconfig gconfig1

#-----

bzfiles =\
 nconfig-0.4/Makefile nconfig-0.4/nodes.h nconfig-0.4/nodes.cpp nconfig-0.4/DOC nconfig-0.4/README\
 nconfig-0.4/TODO nconfig-0.4/kconfig nconfig-0.4/qconf.patch\
 nconfig-0.4/gconfig/Makefile nconfig-0.4/gconfig/gconfig.cpp nconfig-0.4/gconfig/gconfig1.cpp\
 nconfig-0.4/nconfig/Makefile nconfig-0.4/nconfig/nconfig.cpp nconfig-0.4/nconfig/tconfig.cpp\
 nconfig-0.4/kkonfig/kkonfig.pro nconfig-0.4/kkonfig/kkonfig.cpp nconfig-0.4/kkonfig/kkonfig.h\
 nconfig-0.4/kkonfig/setqt nconfig-0.4/kkonfig/Makefile

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
	. kkonfig/setqt; $(MAKE) -C kkonfig clean
	$(MAKE) -C gconfig clean

distclean:
	-rm -f *~
	-rm -f .lkc
	-rm -f nodes.o
	$(MAKE) -C nconfig distclean
	$(MAKE) -C kkonfig distclean
	$(MAKE) -C gconfig distclean



