#$Id$
#DESCRIPTION: Dinotrace Lisp: Makefile for installing LISP sources

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

maintainer-copy::
	cp -f *.el $(SL)/..
