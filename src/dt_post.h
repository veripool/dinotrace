
char dinopost[] = "% $Id$\n\
% Contact RICKS::SNYDER with any problems with this document\n\
/MT {moveto} def		% define MT\n\
/LT {lineto} def		% define LT\n\
\n\
/PAGESCALE      % stack: paper_wid paper_height sigrf width height \n\
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
/EPSPHDR      % stack: dtversion pagenum file note date res st_end_time\n\
{ newpath			% clear current path\n\
  pop pop pop pop pop pop pop\n\
  1 setlinecap 1 setlinejoin 1 setlinewidth     % set line char\n\
  /Times-Roman findfont 8 scalefont setfont\n\
  } def\n\
\n\
/EPSLHDR      % stack: dtversion pagenum file note date res st_end_time\n\
{ newpath			% clear current path\n\
  90 rotate			% rotates to landscape\n\
  0 PG_HGT neg translate	% translates so you can see the image\n\
  pop pop pop pop pop pop pop\n\
  1 setlinecap 1 setlinejoin 1 setlinewidth     % set line char\n\
  /Times-Roman findfont 8 scalefont setfont\n\
  } def\n\
\n\
/PAGEHDR      % stack: dtversion pagenum file note date res st_end_time\n\
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
/RIGHTSHOW	% right justify the signal names adj=(x2-x1-stringwidth)\n\
{ dup stringwidth pop	% get width of string\n\
  0 exch sub		% subtract stringwidth\n\
  0 rmoveto show	% adjust strating location of text and show\n\
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
/START { stroke YADJ exch XADJ exch MT } def\n\
\n\
/STATE_1\n\
{ /x exch def\n\
  currentpoint pop\n\
  sigrf add y2 LT\n\
  x XADJ y2 LT\n\
  } def\n\
\n\
/STATE_0\n\
{ /x exch def\n\
  currentpoint pop\n\
  sigrf add y1 LT\n\
  x XADJ y1 LT\n\
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
    xc DELU add y2 LT		% draw 1st segment\n\
    xc DELU2 add ym LT		% draw 2nd segment\n\
    xc DELU add y1 LT		% draw 3rd segment\n\
    closepath			% draw 4th segment\n\
    xc DELU2 add /xc exch def	% add one 'diamond' width to current xc\n\
    }\n\
    {\n\
    /xh x xc sub 2 div xc add def % calculate midpoint of remaining segment\n\
    xc ym MT			% move to start of last 'diamond'\n\
    xh y2 LT			% draw 1st segment\n\
    x ym LT			% draw 2nd segment\n\
    xh y1 LT			% draw 3rd segment\n\
    closepath			% draw 4th segment\n\
    exit\n\
    }\n\
  ifelse }\n\
  loop\n\
  stroke\n\
  x ym MT			% reset current point\n\
  } def\n\
\n\
/STATE_B32			% draw bus waveform with value (max 32 bits)\n\
{ /v exch def			% convert to hex string\n\
  /x exch XADJ def		% store end location of bus\n\
  currentpoint pop		% get current x location\n\
  /xc exch def			% store current x location\n\
  xc sigrf add y2 LT		% draw 1st segment\n\
  x sigrf sub y2 LT		% draw 2nd segment\n\
  x ym LT			% draw 3rd segment\n\
  x sigrf sub y1 LT		% draw 4th segment\n\
  xc sigrf add y1 LT		% draw 5th segment\n\
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
/STATE_Z\n\
{ /x exch def\n\
  currentpoint pop\n\
  sigrf add dup ym LT stroke\n\
  [3 3] 0 setdash\n\
  ym MT\n\
  x XADJ ym LT stroke\n\
  [] 0 setdash\n\
  x XADJ ym MT\n\
  } def\n";
