#!/usr/bin/perl
# $Id$
#*****************************************************************************
# DESCRIPTION: Dinotrace make: convert documentation text file to header file
# 
# This file is part of Dinotrace.  
# 
# Author: Wilson Snyder <wsnyder@wsnyder.org> or <wsnyder@iname.com>
# 
# Code available from: http://www.veripool.com/dinotrace
# 
#*****************************************************************************
# 
# This file is covered by the GNU public licence.
# 
# Dinotrace is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
# 
# Dinotrace is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with Dinotrace; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.
# 
#****************************************************************************/

print "/* Created automatically by dinodoc.pl from dinotrace.txt */\n";

print "\nchar dinodoc[] = \"\\\n";

my $line = 0;
my $output = 1;
while(<STDIN>) {
    my $buf = $_;
    chop $buf;
    # Strip out the index, it's not useful in this form
    if ($buf =~ /^Index$/) {
	$output = 0;
    }

    if ($output) {
	# Quote backslashes
	$buf =~ s/\\/{backslash}/g;
	$buf =~ s/{backslash}/\\\\/g;
	$buf =~ s/\"/\\\"/g;
	print "$buf\\n\\\n";
	
	# Every so often, end the string, as some compilers barf on long lines 
	$line++;
	if ($line > 30) {
	    print "\"\n\"";
	    $line = 0;
	}
    }
}

print "\";\n\n";

