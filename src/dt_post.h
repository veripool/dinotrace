
char dinopost[] = "% $Id$\n\
% Contact RICKS::SNYDER with any problems with this document\n\
/MT {moveto} def		% define MT\n\
/LT {lineto} def		% define LT\n\
\n\
/PAGESCALE      % stack: height width sigrf paper_heignt paper_width\n\
{ newpath\n\
  /PG_WID exch def		% def PG_WID = 11 * 72\n\
  /PG_HGT exch def		% def PG_HGT = 8.5 * 72\n\
  /sigrf exch def		% signal rf time\n\
  /xstart exch def		% xstart of DINOTRACE window\n\
  /width exch def		% width of DINOTRACE window\n\
  /height exch def		% height of DINOTRACE window\n\
  /YADJ { PG_HGT 50 sub height div mul 50 add } def\n\
  /YTRN { PG_HGT 50 sub height div mul } def\n\
  /DELU 5 def			% width of U diamond\n\
  /DELU2 10 def			% twice DELU\n\
} def \n\
\n\
/EPSPHDR      % stack: st_end_time res date note file pagenum dtversion\n\
{ newpath			% clear current path\n\
  pop pop pop pop pop pop pop\n\
  1 setlinecap 1 setlinejoin 1 setlinewidth     % set line char\n\
  /Times-Roman findfont 8 scalefont setfont\n\
  } def\n\
\n\
/EPSLHDR      % stack: st_end_time res date note file pagenum dtversion\n\
{ newpath			% clear current path\n\
  90 rotate			% rotates to landscape\n\
  0 PG_HGT neg translate	% translates so you can see the image\n\
  pop pop pop pop pop pop pop\n\
  1 setlinecap 1 setlinejoin 1 setlinewidth     % set line char\n\
  /Times-Roman findfont 8 scalefont setfont\n\
  } def\n\
\n\
/PAGEHDR      % stack: st_end_time res date note file pagenum dtversion\n\
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
  %TPOS 10 MT (Digital Equipment Corporation Confidential) show\n\
  TPOS 150 add 20 MT (DEC Confidential) show\n\
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
/SIGMARGIN	% set margin to max of this width or prev widths\n\
{ stringwidth pop dup	% get width of string\n\
  sigwidth gt {		% compare to current width\n\
  /sigwidth exch def	% set it\n\
  } { pop } ifelse\n\
} def\n\
\n\
/XSCALESET	% set signal xscaling after all signals were margined\n\
{ /sigwidth sigwidth 20 add def		% left margin creation\n\
  /sigstart sigwidth 10 add def		% space between signal and values\n\
  /sigxscale PG_WID sigstart sub width xstart sub div def	% Scaling for signal section\n\
  /XADJ { xstart sub sigxscale mul sigstart add } def	% convert X screen to print coord\n\
} def\n\
\n\
/START_GRID	% (yh yl ylabel) start a signal's information\n\
{ /ylabel exch YADJ def	% ylabel - Where to put label y coord\n\
  /yl exch YADJ def	% yl - Bottom of grid line y coord\n\
  /yh exch YADJ def	% yh - Top of grid line y coord\n\
  /xc 0 XADJ def	% xc- current x coord for determining if fits\n\
} def\n\
\n\
/GRID		% (x label) start a signal's information\n\
{ /label exch def		% time label for grid\n\
  /x exch XADJ def		% x coord of grid\n\
  x yh MT x yl LT stroke	% draw grid line\n\
  x ylabel label FITCENTERSHOW	% draw label\n\
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
/START_SIG	% (ym yh yl xstart ystart) start a signal's information\n\
{ stroke YADJ exch XADJ exch MT		% start at given xstart & ystart\n\
  /yl exch YADJ def	% yl - Bottom of signal's y coord\n\
  /yh exch YADJ def	% yh - Top of signal's y coord\n\
  /ym exch YADJ def	% ym - Middle of signal's y coord\n\
} def\n\
\n\
/STATE_1\n\
{ /x exch def\n\
  currentpoint pop\n\
  sigrf add yh LT\n\
  x XADJ yh LT\n\
  } def\n\
\n\
/STATE_0\n\
{ /x exch def\n\
  currentpoint pop\n\
  sigrf add yl LT\n\
  x XADJ yl LT\n\
  } def\n\
\n\
/STATE_U\n\
{ /x exch XADJ def		% store end location of U\n\
  currentpoint pop		% get current x location\n\
  /xc exch def			% store current x location\n\
  xc ym LT			% draw line to beginning of U\n\
  { xc x DELU2 sub lt		% is current x past end location - 1 width\n\
    {				% draw diamond\n\
    xc ym MT			% move to start of 'diamond'\n\
    xc DELU add yh LT		% draw 1st segment\n\
    xc DELU2 add ym LT		% draw 2nd segment\n\
    xc DELU add yl LT		% draw 3rd segment\n\
    closepath			% draw 4th segment\n\
    xc DELU2 add /xc exch def	% add one 'diamond' width to current xc\n\
    }\n\
    {\n\
    /xh x xc sub 2 div xc add def % calculate midpoint of remaining segment\n\
    xc ym MT			% move to start of last 'diamond'\n\
    xh yh LT			% draw 1st segment\n\
    x ym LT			% draw 2nd segment\n\
    xh yl LT			% draw 3rd segment\n\
    closepath			% draw 4th segment\n\
    exit\n\
    }\n\
  ifelse }\n\
  loop\n\
  stroke\n\
  x ym MT			% reset current point\n\
  } def\n\
\n\
/STATE_B			% draw bus waveform with value\n\
{ /v exch def			% convert to hex string\n\
  /x exch XADJ def		% store end location of bus\n\
  currentpoint pop		% get current x location\n\
  /xc exch def			% store current x location\n\
  xc sigrf add yh LT		% draw 1st segment\n\
  x sigrf sub yh LT		% draw 2nd segment\n\
  x ym LT			% draw 3rd segment\n\
  x sigrf sub yl LT		% draw 4th segment\n\
  xc sigrf add yl LT		% draw 5th segment\n\
  xc ym LT stroke		% draw 6th segment and stroke\n\
\n\
  v stringwidth pop		% get width of string\n\
  x xc sub			% get width of bus\n\
  lt {				% is string wid lt bus wid?\n\
    xc x xc sub 2 div add	% calculate the mdpt\n\
    v stringwidth pop 2 div sub	% calculate start location of text\n\
    ym 3 sub MT v show		% draw the bus value\n\
    } if\n\
\n\
  x ym MT			% reset current point\n\
  } def\n\
\n\
/STATE_B_FB			% (x fallback_value value) draw bus waveform with decoded value, fallback if doesn't fit\n\
{ /v exch def			% convert to hex string\n\
  /fallback exch def		% fallback string\n\
  /x exch XADJ def		% store end location of bus\n\
  currentpoint pop		% get current x location\n\
  /xc exch def			% store current x location\n\
  xc sigrf add yh LT		% draw 1st segment\n\
  x sigrf sub yh LT		% draw 2nd segment\n\
  x ym LT			% draw 3rd segment\n\
  x sigrf sub yl LT		% draw 4th segment\n\
  xc sigrf add yl LT		% draw 5th segment\n\
  xc ym LT stroke		% draw 6th segment and stroke\n\
\n\
  v stringwidth pop		% get width of string\n\
  x xc sub			% get width of bus\n\
  lt {				% is string wid lt bus wid?\n\
    xc x xc sub 2 div add	% calculate the mdpt\n\
    v stringwidth pop 2 div sub	% calculate start location of text\n\
    ym 3 sub MT v show		% draw the bus value\n\
    } \n\
  { \n\
    fallback stringwidth pop	% get width of string\n\
    x xc sub			% get width of bus\n\
    lt {			% is string wid lt bus wid?\n\
      xc x xc sub 2 div add	% calculate the mdpt\n\
      fallback stringwidth pop 2 div sub	% calculate start location of text\n\
      ym 3 sub MT fallback show	% draw the bus value\n\
      } if \n\
    } ifelse\n\
\n\
  x ym MT			% reset current point\n\
  } def\n\
\n\
/STATE_Z			% (x) Tristate\n\
{ /x exch def\n\
  currentpoint pop\n\
  sigrf add dup ym LT stroke\n\
  [2 2] 0 setdash\n\
  ym MT\n\
  x XADJ ym LT stroke\n\
  [] 0 setdash\n\
  x XADJ ym MT\n\
  } def\n";
