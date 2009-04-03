#DESCRIPTION: Dinotrace Lisp: Makefile for installing LISP sources

all:
#	Nothing needs to be done; just install

install: dinotrace sim-log

dinotrace:
	@echo "Please copy dinotrace.el to your site-lisp directory, and byte-compile"

sim-log:
	@echo "Please copy sim-log.el to your site-lisp directory, and byte-compile"

clean mostlyclean distclean maintainer-clean::
	-rm *.elc

maintainer-clean::

maintainer-copy::
	cp -f *.el $(SL)/..
