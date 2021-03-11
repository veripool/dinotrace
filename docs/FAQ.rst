=============
Dinotrace FAQ
=============

This is a list of frequently asked questions (FAQ) for Dinotrace users.

By Wilson Snyder <wsnyder@wsnyder.org>


General Questions
=================


Where can I get the latest version of Dinotrace?
------------------------------------------------

The latest version is available from the `Dinotrace
Website <https://www.veripool.org/dinotrace>`__.


Suggestions
===========


Can I copy Dinotrace?
---------------------

Dinotrace is covered under two Copyrights. The latter one is the GNU
Copyleft, which allows free use and modification, provided that you in turn
provide free use and modification to all others that request it.  This is
described in the ``COPYING`` file distributed with Dinotrace.

Code created before 1998 was Copyrighted by Digital Equipment
Corporation. Their Copyright also disclaims any warranties, and disallows
reproduction for sale. Observing the GNU Copyleft should also cover this
Copyright for any software distributed not for a fee. (The water is murky
if embedding Dinotrace.)


Where should I report bugs and other problems with Dinotrace?
-------------------------------------------------------------

Dinotrace is no longer under development. It is however solid and reliable,
and portability or other serious issues are fixed when requested.

See the issues under the `Dinotrace website
<https://www.veripool.org/dinotrace>`__.


What other public domain tools work with Dinotrace?
---------------------------------------------------

The primary tool you will want are GNU Emacs, with

`verilog-mode.el <https://www.veripool.org/verilog-mode>`__,
``dinotrace.el``, and ``sim-log.el`` installed. This will let you
back-annotate the values of signals onto your source code.

You may also want `Verilator <https://www.veripool.org/verilator>`__, a
public domain simulator, and other tools available at the `Veripool
<https://www.veripool.org>`__.


Will Dinotrace work with MS Windows?
------------------------------------

Yes, though it may not have the look and feel you expect. (Because your
expectations were unfortunately defined by Microsoft, not Unix.) Some
consider that a feature.

Read the installation instructions on the Website. You'll need cygwin, and
a bunch of subpackages that are a part of cygwin.


On-line Help, Printed Manuals, Other Sources of Help
====================================================


How do I get a printed copy of the Dinotrace manual?
----------------------------------------------------

See the `Dinotrace Manual PDF
<https://www.veripool.org/ftp/dinotrace.pdf>`__.


How do I install a piece of Texinfo documentation?
--------------------------------------------------

An info file for Dinotrace comes with the package, ``dinotrace.info``.
Copy this file to a directory in your info path, usually
``/usr/local/info``. Then edit the master directory, usually
``/usr/local/info/dir`` to include the line:

-  Dinotrace: (dinotrace). Signal waveform viewer


Common Things People Want To Do
===============================


How do I have Dinotrace always display a group of signals together?
-------------------------------------------------------------------

Dinotrace doesn't really have the concept of groups yet. Instead, use the
``signal_move`` or ``signal_copy`` to accomplish the desired effect.

For example, if out of a huge trace you want only ``foo`` and ``bar`` to be
displayed, put in your ``dinotrace.dino``:

::

   signal_delete *
   signal_add foo
   signal_add bar

Dinotrace will create similar code for you by selecting the ``Customize
Save As`` menu option then checking the ``Save signal ordering`` box.


How do I make a signal that is a concatenation of other signals?
----------------------------------------------------------------

Dinotrace will only collapse signals with the same basename. Make another
signal that is a concatenation of the two.

If the signal is never used, then it will be eliminated by Synopsys. If
you're using Verilint or a similar tool that finds unused nets, you may
want to make another signal, unused_ok, to make it obvious that the net is
fake. The below example does this. It uses the weird ORing on unused_ok so
that any number of any width signals can be added in, yet it will still
always hold a constant high.

::

   wire [2:0] first;
   wire [7:3] second;
   wire [7:0] concatenated_for_dino = @{first, second@};
   wire unused_ok = (|@{
                    concatenated_for_dino,
                    1'b1@});


How do I find the next time a signal changes?
---------------------------------------------

There is no direct way to move to the next edge. You can see the next time
that the value changes by using ``Value Examine`` or ``MouseButton2``, then
pan to that point manually or with ``Goto``.


How do I make a "register window" with the values at a current time?
--------------------------------------------------------------------

Just make a fake Verilog file with the registers laid out in ASCII as you
wish. Then use the Emacs Dinotrace keys to annotate and manipulate the
values. You can even have the signals commented out in your normal module;
annotation updates comments too. (As long as the needed signals are
uncommented elsewhere in the module.)


Can Dinotrace directly read VPD format files?
---------------------------------------------

Nope, sorry. VPD is a proprietary format, and the reading code must be
licensed for a fee before being used. This is obviously not possible with
the distribution philosophy of Dinotrace.

You can however convert VPD files on the fly using ``vpd2vcd``. See the
documentation for details.


Bugs/Problems
=============


How do I get around the "Over maximum of 256 signal states" error message?
--------------------------------------------------------------------------

You have two basic options. First, for one hot and similar state machines,
define another less wide signal that is derived from the wide signal, then
apply a ``signal_state`` command to that new signal.

Alternatively, do the decoding in Verilog. A configuration command
``signal_radix _ascii ascii`` with the code below should work:

::

   reg [8*4-1:0] machine_ascii;   // Decoded ascii state of machine
   always_comb begin
      casex (machine)
       4'b0000: machine_ascii = "----";
       4'b1000: machine_ascii = "s0  ";
       4'b0100: machine_ascii = "exit";
       4'b0010: machine_ascii = "stop";
       4'b0001: machine_ascii = "idle";
       default: machine_ascii = "%ERR";
      endcase
   end


Why do I see X's for a bus that isn't all X's?
----------------------------------------------

If a signal value has even a single bit that is X or Z, but isn't entirely
X or Z, then it will be displayed as X. You can see the real value by using
``Value Examine`` or ``MouseButton2``.


Why does(n't) Dinotrace compress bits into a single vector?
-----------------------------------------------------------

Dinotrace must form all vectors when the trace is being read in. The
``vector_separator`` command must be set to the character used to separate
bus bits from the rest of the signal name. If you don't want bits to be
collapsed to vectors, just set it to some character that doesn't occur in
signal names, like ``@``.


Why doesn't a signal appear in Dinotrace?
-----------------------------------------

Check first that it is really in the trace. The ``strings`` program will
find signal names in every format that Dinotrace supports.

If the signal exists in the trace, it may have been deleted by a
configuration command. Use the ``Signal Select`` popup to add all signals,
then use ``Signal Search`` to find it.

In Verilog VCD files, if an identical signal exists at many levels of
hierarchy, only the top level signal will exist. If your naming convention
is sane, and doesn't change names at hierarchy boundaries, you still should
find the signal, just with a different hierarchy in front of the base
signal name. Furthermore, signals with over 1024 bits are dropped, since
they would take too much space on the screen.

A bug is not impossible either, though usually Dinotrace manages to eek out
a warning message when it is about to lose a signal.


Why doesn't a signal I can see in Dinotrace appear in the annotation?
---------------------------------------------------------------------

Under ``Value Annotate`` is an option menu which chooses which signals are
included in the annotation. By default, deleted signals aren't
included. Often signals that are constant through the whole trace are
deleted, and thus don't get included in the annotation. Change the option
to include deleted signals, or add the needed signals back.


Why do I get the wrong value for an annotated signal?
-----------------------------------------------------

Because you know more than Emacs. (At least for the time being.) Emacs
simply does a search and replace for the signal name, totally ignoring the
bus bits, module name, and hierarchy.

First off, this means bus subscripts are ignored. The traced signal
``foo[5:0]=6'b101010`` when annotating ``foo[5]`` will show the value of
the whole vector, not just bit 5: :literal:`foo`101010'[5]`.

Furthermore, having multiple signals with the same name will confuse
things, as the hierarchy isn't known. If the trace has the signals
``a.foo[5:2]`` and ``b.foo[5:2]`` with different values, and the code
references foo, you could get either the a or b version. Your best bet is
to delete the signals that don't apply to the module you are
annotating. (If a signal like the clock is in many modules, but identical,
there's nothing to worry about.)
