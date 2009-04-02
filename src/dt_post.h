/******************************************************************************
 * DESCRIPTION: Dinotrace source: postscript header
 *
 * This file is part of Dinotrace.
 *
 * Author: Wilson Snyder <wsnyder@wsnyder.org>
 *
 * Code available from: http://www.veripool.org/dinotrace
 *
 ******************************************************************************
 *
 * Some of the code in this file was originally developed for Digital
 * Semiconductor, a division of Digital Equipment Corporation.  They
 * gratefuly have agreed to share it, and thus the base version has been
 * released to the public with the following provisions:
 *
 *
 * This software is provided 'AS IS'.
 *
 * DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THE INFORMATION
 * (INCLUDING ANY SOFTWARE) PROVIDED, INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR ANY PARTICULAR PURPOSE, AND
 * NON-INFRINGEMENT. DIGITAL NEITHER WARRANTS NOR REPRESENTS THAT THE USE
 * OF ANY SOURCE, OR ANY DERIVATIVE WORK THEREOF, WILL BE UNINTERRUPTED OR
 * ERROR FREE.  In no event shall DIGITAL be liable for any damages
 * whatsoever, and in particular DIGITAL shall not be liable for special,
 * indirect, consequential, or incidental damages, or damages for lost
 * profits, loss of revenue, or loss of use, arising out of or related to
 * any use of this software or the information contained in it, whether
 * such damages arise in contract, tort, negligence, under statute, in
 * equity, at law or otherwise. This Software is made available solely for
 * use by end users for information and non-commercial or personal use
 * only.  Any reproduction for sale of this Software is expressly
 * prohibited. Any rights not expressly granted herein are reserved.
 *
 ******************************************************************************
 *
 * Changes made over the basic version are covered by the GNU public licence.
 *
 * Dinotrace is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * Dinotrace is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Dinotrace; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 *****************************************************************************/

char dt_post[] = "% version: 9.4a \n\
% Contact wsnyder@wsnyder.org with problems with the header of this document\n\
/MT {moveto} def		% define MT\n\
/LT {lineto} def		% define LT\n\
/STROKE {currentpoint stroke MT} def	% define STROKE saving point\n\
\n\
/PAGESCALE      % (height width sigrf paper_height paper_width)\n\
{ newpath\n\
  /PG_WID exch def		% def PG_WID = 11 * 72\n\
  /PG_HGT exch def		% def PG_HGT = 8.5 * 72\n\
  /psigrf exch def		% signal rf time\n\
  /xstart exch def		% xstart of DINOTRACE window\n\
  /width exch def		% width of DINOTRACE window\n\
  /height exch def		% height of DINOTRACE window\n\
  /heightmul PG_HGT 50 sub height div def\n\
  /YADJ { height exch sub heightmul mul 40 add } def\n\
  /basewidth 0 def		% Prepare for sigwidth calc\n\
  /hierwidth 0 def		% Prepare for sigwidth calc\n\
  stroke /Times-Roman findfont 8 scalefont setfont\n\
} def \n\
\n\
/EPSPHDR      % (st_end_time res date note file pagenum dtversion)\n\
{ newpath			% clear current path\n\
  pop pop pop pop pop pop pop\n\
  1 setlinecap 1 setlinejoin 1 setlinewidth     % set line char\n\
  /Times-Roman findfont 8 scalefont setfont\n\
  } def\n\
\n\
/EPSLHDR      % (st_end_time res date note file pagenum dtversion)\n\
{ newpath			% clear current path\n\
  90 rotate			% rotates to landscape\n\
  0 PG_HGT neg translate	% translates so you can see the image\n\
  pop pop pop pop pop pop pop\n\
  1 setlinecap 1 setlinejoin 1 setlinewidth     % set line char\n\
  /Times-Roman findfont 8 scalefont setfont\n\
  } def\n\
\n\
/PAGEHDR      % (st_end_time res date note file pagenum dtversion)\n\
{ newpath			% clear current path\n\
  90 rotate			% rotates to landscape\n\
  0 PG_HGT neg translate	% translates so you can see the image\n\
\n\
  %3 setlinewidth		% set the line width of the border\n\
  %0 0 MT 0 PG_HGT LT PG_WID PG_HGT LT PG_WID 0 LT 0 0 LT stroke % draw bounding box\n\
\n\
  /Helvetica-BoldOblique findfont 30 scalefont setfont % choose large font\n\
  1 setlinecap 1 setlinejoin 1 setlinewidth	% set line char\n\
\n\
  20 20 MT 20 string cvs true charpath stroke	% draw logo\n\
  PG_WID 180 sub 20 MT\n\
    20 string cvs true charpath stroke		% draw page number\n\
\n\
  /Times-Roman findfont 10 scalefont setfont    % choose normal font\n\
  1 setlinecap 1 setlinejoin 1 setlinewidth     % set line char\n\
\n\
  /TPOS \n\
	250	\n\
	1000 PG_WID lt {	% B-Size page?\n\
		50 add\n\
		} if\n\
	def		% Define position for file info\n\
\n\
  TPOS 40 MT (File: ) show 150 string cvs show\n\
  TPOS 30 MT (Note: ) show 150 string cvs show\n\
  TPOS 20 MT (Date: ) show 25 string cvs show\n\
  TPOS 150 add 20 MT (Confidential) show\n\
  PG_WID 300 sub 30 MT (Resolution: ) show 25 string cvs show\n\
  PG_WID 300 sub 20 MT (Time: ) show 25 string cvs show\n\
  stroke /Times-Roman findfont 8 scalefont setfont\n\
  } def\n\
\n\
/RIGHTSHOW	% (string) right justify the signal names\n\
{ dup stringwidth pop	% get width of string\n\
  0 exch sub		% subtract stringwidth\n\
  0 rmoveto show	% adjust strating location of text and show\n\
} def\n\
\n\
/CENTERSHOW	% (string) center justify the signal names\n\
{ dup stringwidth pop	% get width of string\n\
  2 div 0 exch sub	% subtract stringwidth/2\n\
  0 rmoveto show	% adjust starting location of text and show\n\
} def\n\
\n\
/FITCENTERSHOW	% (x y string) if string will fit vs last printing, show it centered\n\
{ /v exch def\n\
  /y exch def\n\
  /x exch def\n\
  v stringwidth pop 2 div	% get width of string\n\
  x xc sub			% get width of bus\n\
  lt {				% is string wid lt bus wid?\n\
    x y MT v CENTERSHOW		% show value\n\
    /xc \n\
        v stringwidth pop 2 div	% get width of string\n\
	x add 3 add def		% define new ending point, leave some room\n\
    } if\n\
} def\n\
\n\
/SIGMARGIN	% (hiername basename) set margin to max of this width or prev widths\n\
{ stringwidth pop dup	% get width of base signal name\n\
  basewidth gt {		% compare to current width\n\
  /basewidth exch def	% set it\n\
  } { pop } ifelse\n\
 stringwidth pop dup	% get width of hier string\n\
  hierwidth gt {	% compare to current width\n\
  /hierwidth exch def	% set it\n\
  } { pop } ifelse\n\
} def\n\
\n\
/SIGNAME	% (ymdpt hiername basename) draw a signal name\n\
{ /basename exch def	% Base part of signal name\n\
  /hiername exch def	% Hierarchy part of signal name\n\
  /ymdpt exch YADJ def	% ymdpt - Midpoint of signame\n\
  hierwidth ymdpt 3 sub MT	% move to right edge of .\n\
  hiername RIGHTSHOW	% plot hiername to left\n\
  hierwidth ymdpt 3 sub MT	% move to right edge of .\n\
  basename show		% plot basename to right\n\
} def\n\
\n\
/XSCALESET	% set signal xscaling after all signals were margined\n\
{ /hierwidth hierwidth 20 add def	% left margin creation\n\
  /sigstart hierwidth basewidth add 10 add def	% space between signal and values\n\
  /sigxscale PG_WID sigstart sub width xstart sub div def	% Scaling for signal section\n\
  /XSCALE { sigxscale mul } def	% convert X screen to print coord\n\
  /XADJ { xstart sub sigxscale mul sigstart add } def	% convert X screen to print coord\n\
} def\n\
\n\
/START_GRID	% (ytop ybot ylabel) start a signal's information\n\
{ /ylabel exch YADJ def	% ylabel - Where to put label y coord\n\
  /ybot exch YADJ def	% ybot - Bottom of grid line y coord\n\
  /ytop exch YADJ def	% ytop - Top of grid line y coord\n\
  /xc 0 XADJ def	% xc- current x coord for determining if fits\n\
} def\n\
\n\
/GRID		% (x label) make grid\n\
{ /label exch def		% time label for grid\n\
  /x exch XADJ def		% x coord of grid\n\
  x ytop MT x ybot LT stroke	% draw grid line\n\
  x ylabel label FITCENTERSHOW	% draw label\n\
} def\n\
\n\
/CSRU		% (x label) make user cursor\n\
{ [] 0 setdash\n\
  GRID\n\
} def\n\
\n\
/CSRA		% (x label) make auto cursor\n\
{ [1 1] 0 setdash\n\
  GRID\n\
} def\n\
\n\
/CURSOR_DELTA	% (x1 x2 y label) draw a delta line between two cursors\n\
{ /label exch def		% time label for grid\n\
  /y exch YADJ def		% y coord of cursor\n\
  /xl exch XADJ def		% x coord of cursor 2 (earlier one)\n\
  /xh exch XADJ def		% x coord of cursor 1 (later one)\n\
  /xlh xh xl add 2 div def	% center point of lines\n\
  label stringwidth pop	6 add	% get width of string\n\
  xh xl sub			% get width of bus\n\
  lt {				% is string wid lt bus wid?\n\
    xlh 2 sub label stringwidth pop 2 div sub\n\
    y MT xl y LT		% first half\n\
    xlh y 3 sub MT label CENTERSHOW	% show value\n\
    xlh 2 add label stringwidth pop 2 div add\n\
    y MT xh y LT		% second half\n\
    }\n\
  {\n\
    xl y MT xh y LT		% draw line, no time can fit \n\
    } ifelse\n\
  stroke\n\
} def\n\
\n\
/START_SIG	% (ymdpt ytop ybot xstart) start a signal's information\n\
{ /xl exch XADJ def	% xl - last/left x coord\n\
  /ybot exch YADJ def	% ybot - Bottom of signal's y coord\n\
  /ytop exch YADJ def	% ytop - Top of signal's y coord\n\
  /ymdpt exch YADJ def	% ymdpt - Middle of signal's y coord\n\
  /sigrf psigrf XSCALE def % sigrf scaled from page rise-fall\n\
  stroke xl ymdpt MT\n\
  /x xl def\n\
} def\n\
\n\
% State drawing routines:\n\
%   Preserve last /x as /lx\n\
%   Leave point at (xl,ymdpt)\n\
\n\
/S0		% (x) low\n\
{ /xl x def\n\
  /x exch XADJ def\n\
  xl sigrf add ybot LT	% \\ \n\
  x sigrf sub ybot LT	% - \n\
  x ymdpt LT		% / \n\
  } def\n\
\n\
/S1		% (x) high\n\
{ /xl x def\n\
  /x exch XADJ def\n\
  xl ymdpt MT\n\
  xl sigrf add ytop LT	% / \n\
  STROKE\n\
  2 setlinewidth\n\
  x sigrf sub ytop LT	% = \n\
  STROKE\n\
  1 setlinewidth\n\
  x ymdpt LT		% \\ \n\
  } def\n\
\n\
/SZ		% (x) Tristate\n\
{ /xl x def\n\
  /x exch XADJ def\n\
  STROKE\n\
  [1 1] 0 setdash\n\
  x ymdpt LT	% .... \n\
  STROKE\n\
  [] 0 setdash\n\
  } def\n\
\n\
/SB		% (x) bus\n\
{ /xl x def\n\
  /x exch XADJ def\n\
  xl sigrf add ytop LT	% / \n\
  x sigrf sub ytop LT	% - \n\
  x ymdpt LT		% \\ \n\
  x sigrf sub ybot LT	% / \n\
  xl sigrf add ybot LT	% - \n\
  xl ymdpt LT		% \\ \n\
  x ymdpt MT\n\
  } def\n\
\n\
/SH		% (x) bus high\n\
{ /xl x def\n\
  /x exch XADJ def\n\
  xl sigrf add ytop LT	% / \n\
  STROKE\n\
  2 setlinewidth\n\
  x sigrf sub ytop LT	% = \n\
  STROKE\n\
  1 setlinewidth\n\
  x ymdpt LT		% \\ \n\
  x sigrf sub ybot LT	% / \n\
  xl sigrf add ybot LT	% \\ \n\
  x ymdpt MT\n\
  } def\n\
\n\
/SU		% (x) Unknown\n\
{ /xl x def\n\
  /x exch XADJ def\n\
  STROKE\n\
  xl ymdpt MT\n\
  xl sigrf add ytop LT	% / \n\
  x sigrf sub ytop LT	% - \n\
  x ymdpt LT		% \\ \n\
  x sigrf sub ybot LT	% / \n\
  xl sigrf add ybot LT	% - \n\
  closepath		% \\ \n\
  fill stroke\n\
  x ymdpt MT\n\
  } def\n\
\n\
/SA		% (x pct_high) analog\n\
{ /xl x def\n\
  /pct exch def\n\
  /x exch XADJ def\n\
  ytop ybot sub   pct mul   ybot add   /ypct exch def\n\
  xl sigrf add ypct LT	% / \n\
  x sigrf sub ypct LT	% - \n\
  } def\n\
\n\
/SV	% (valstrg) draw bus value\n\
{ /v exch def			% value as string\n\
  v stringwidth pop		% get width of string\n\
  x xl sub			% get width of bus\n\
  lt {				% is string wid lt bus wid?\n\
    xl x xl sub 2 div add	% calculate the mdpt\n\
    v stringwidth pop 2 div sub	% calculate start location of text\n\
    ymdpt 3 sub MT v show	% draw the bus value\n\
    } if\n\
\n\
  x ymdpt MT			% reset current point\n\
  } def\n\
\n\
/SN	% ( value fallback_value) draw bus waveform with decoded value, fallback if doesn't fit\n\
{ /fallback exch def		% fallback string\n\
  /v exch def			% convert to hex string\n\
  v stringwidth pop		% get width of string\n\
  x xl sub			% get width of bus\n\
  lt {				% is string wid lt bus wid?\n\
    xl x xl sub 2 div add	% calculate the mdpt\n\
    v stringwidth pop 2 div sub	% calculate start location of text\n\
    ymdpt 3 sub MT v show	% draw the bus value\n\
    } \n\
  { \n\
    fallback stringwidth pop	% get width of string\n\
    x xl sub			% get width of bus\n\
    lt {			% is string wid lt bus wid?\n\
      xl x xl sub 2 div add	% calculate the mdpt\n\
      fallback stringwidth pop 2 div sub	% calculate start location of text\n\
      ymdpt 3 sub MT fallback show	% draw the bus value\n\
      } if \n\
    } ifelse\n\
\n\
  x ymdpt MT			% reset current point\n\
  } def\n\
\n\
\n";
