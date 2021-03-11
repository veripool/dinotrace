================
Dinotrace README
================

|Logo|

Dinotrace is a tool designed to aid in viewing Verilog Value Change Dump,
ASCII, Verilator, Tempest CCLI, COSMOS, Chango and Decsim Binary simulation
traces. It is optimized for rapid design debugging using X-Windows
Mosaic. A interface allows signal information to be annotated into source
code using Emacs.

Dinotrace is no longer under development, and has a somewhat outdated Motif
interface. It is however solid and reliable, and portability or other
serious issues are fixed when requested.

|license LGPLv3|


Screen Shots
============

Example screen shots:

|Example1| |Example2|


Linux Installation
==================

Build using the GNU configuration process.

::

   # Prerequisites
   sudo apt-get install perl
   # Obtain distribution kit
   git clone https://github.com/veripool/dinotrace.git
   # Build
   cd dinotrace
   git pull
   autoconf
   ./configure
   make
   # Test
   #export DISPLAY=:0.0  # If needed
   ./dinotrace traces/ascii.tra
   # Install
   make install

Dinotrace requires the Motif Widget set. If your system doesn't include
Motif, or OpenMotif, a public domain version, LessTif, is
available. Version 0.93.36 was known to work for this release. Be sure to
also have the LessTif and X11 development files (header files) installed.


Supported Systems
-----------------

Previous versions of Dinotrace were once built, and probably will still
build on:

- sparc-sun-solaris2.5.1
- i386-pc-linux
- i386-pc-cygwin32 (under Windows-2000 & Windows-XP)
- x86_64-suse-linux
- alpha-dec-osf4.0
- alpha-dec-vms6.0 (see the special section on VMS installation.)
- mips-dec-ultrix
- sparc-sun-sunos4.1.4
- vax-dec-vms6.0 (see the special section on VMS installation.)


Windows Installation
--------------------

Dinotrace can be built for Windows under the Cygwin environment.

Dinotrace is still a X11 program, even under Windows. Thus you must add
several packages to Cygwin.

- Install `Cygwin32 <http://sourceware.cygnus.com/cygwin/:>`__. You'll need
  the gcc-g++, lesstif, make, and XFree96-prog (headers) packages.
  Generally these aren't installed by default, use the Cygwin setup
  executable to get them.

- Obtain a X11 server. You can use XFree86 that came with cygwin.  Another
  alternative is the commercial eXcursion or exceed programs. If using
  XFree86, you can start the server with ``xwin -multiwindow``.

- Make and install dinotrace using the instructions in the Linux section.

There are known problems in LessTif from looking at network drives in the
Dinotrace File Open requestor. You may want to specify trace files on the
command line instead of using the requestor.


VMS Installation
================

It's unlikely anyone is using VMS, but this is provided for historical
reference/entertainment.

The ``configure`` program does not support Dinotrace, thus several command
files are supplied to build Dinotrace.

::

   # Change to the download directory of dinotrace.
   $ set default WHERE_DOWNLOADED
   # Build Dinotrace with the make.com file.
   $ @@make.com
   # If all was successful, you should be able to invoke the Dinotrace executable.
   $ run dinotrace.exe

Create a logical to point to Dinotrace, and make a symbol to invoke
Dinotrace into the background. To do this, put in your group's login.com:
(Substituting in the appropriate directory for somedisk$.)

::

   $! Dinotrace
   $ define/group/nolog DINODISK somedisk$:[DINOTRACE]
   $ arch_bin_v == "_VV"
   $ if f$getsyi("NODE_HWTYPE") .eqs. "ALPH" then arch_bin_v == "_VA"
   $ Dinotraceexe :== "$dinodisk:dinotrace''arch_bin_v'"
   $ Dinotrace :== "spawn/nowait/nolog/input=nl:/output=nl: dinotraceexe"

Note a hazard with this definition of dinotrace: The display, etc. must be
set correctly for Dinotrace to start up. When running under VMS, if
Dinotrace has an error message when starting, it will not be seen due to
the spawn. To see error messages, users should be told to type:

::

   $ dinotraceexe


Documentation
=============

See the documentation in \`dinotrace.texi' or the equivalent `Dinotrace
Manual PDF <https://www.veripool.org/ftp/dinotrace.pdf>`__.

Also see the `Dinotrace FAQ <docs/FAQ.rst>`__.


License
=======

This package is Copyright 1992-2021 by Digital Equipment Corporation, and
Wilson Snyder <wsnyder@wsnyder.org>.

Dinotrace is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

Code created before 1998 was Copyrighted by Digital Equipment
Corporation. Their Copyright also disclaims any warranties, and disallows
reproduction for sale. Observing the GNU Copyleft should also cover this
Copyright for any software distributed not for a fee.

.. |Logo| image:: https://www.veripool.org/img/dinotrace_200x200.png
.. |license LGPLv3| image:: https://img.shields.io/badge/License-LGPL%20v3-blue.svg
.. |Example1| image:: https://www.veripool.org/img/dinotrace_example.png
.. |Example2| image:: https://www.veripool.org/img/dinomode_example.png
