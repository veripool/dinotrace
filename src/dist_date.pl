#!/usr/local/bin/perl
# $Id$
#*****************************************************************************
# dist_date.pl --- create header file with compilation date in it
# 
# This file is part of Dinotrace.  
# 
# Author: Wilson Snyder <wsnyder@ultranet.com> or <wsnyder@iname.com>
# 
# Code available from: http://www.ultranet.com/~wsnyder/veripool/dinotrace
# 
#*****************************************************************************
# 
# This file is covered by the GNU public licence.
# 
# Dinotrace is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public Licens as published by
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

my $date_number = time;
my $date_string = localtime($date_number);

my $filename="dist_date.h";

open (TFILE, ">$filename") or die "Can't write $filename.";
print TFILE "/* Date of compiliation.  Created with perl dist_date.pl */\n";
print TFILE "#define DIST_DATE $date_number\n";
print TFILE "#define DIST_DATE_STRG \"$date_string\"\n";
close (TFILE);