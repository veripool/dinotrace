#$Id$

all:
#	Nothing needs to be done; just install

install: dinotrace sim-log

dinotrace:
	emacs -batch -load dinotrace.el -f dinotrace-install

sim-log:
	emacs -batch -load sim-log.el -f sim-log-install

clean mostlyclean distclean maintainer-clean::
	-rm *.elc

maintainer-clean::
	-rm installer.el dinotrace.el sim-log.el

maintainer-copy::
	cp -f $(SL)/installer.el $(SL)/dinotrace.el $(SL)/sim-log.el .
