/* $Id$ */
/******************************************************************************
 * dinotrace.h --- structure definitions
 *
 * This file is part of Dinotrace.  
 *
 * Author: Wilson Snyder <wsnyder@world.std.com> or <wsnyder@ultranet.com>
 *
 * Code available from: http://www.ultranet.com/~wsnyder/dinotrace
 *
 ******************************************************************************
 *
 * Some of the code in this file was originally developed for Digital
 * Semiconductor, a division of Digital Equipment Corporation.  They
 * gratefuly have agreed to share it, and thus the bas version has been
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
 * the Free Software Foundation; either version 2, or (at your option)
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

/**********************************************************************/
/* Standard headers required for everybody */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/stat.h>

#if HAVE_MATH_H
# include <math.h>
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif
#if HAVE_FCNTL_H
# include <fcntl.h>
#endif
#if TM_IN_SYS_TIME
#include <sys/time.h>
#else
#include <time.h>
#endif

#ifdef VMS
# include <file.h>
# include <strdef.h>
# include <unixio.h>
# include <descrip.h>
#endif

#include <X11/Xlib.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>

/***********************************************************************/
/* Turn off alignment for any structures that will be read/written onto Disk! */
/*
#ifndef __osf__
#pragma member_alignment
#endif
*/

#define MAXSIGLEN	256	/* Maximum length of signal names */
#define MAXTIMELEN	20	/* Maximum length of largest time as a string */
#define MAXFNAMELEN	200	/* Maximum length of file names */
#define MAXSTATENAMES	512	/* Maximum number of state name translations */
#define MAXVALUELEN	40	/* Maximum length of values or state names, 32hex digits + 4 sep + NULL */
#define MAXGRIDS	4	/* Maximum number of grids */
#define MAXSCREENWIDTH	5000	/* Maximum width of screen */
#define MAXCFGFILES	5	/* Maximum number of config files */
#define	MIN_GRID_RES	5	/* Minimum grid resolution, in pixels between grid lines */
#define BLK_SIZE	512	/* Trace data block size (512 transitions/signal == 2K min/sig) */
#define	CLICKCLOSE	20	/* Number of pixels that are "close" for click-to-edges */

/* Top to bottom:
 * Y_TOP_BOTTOM grid_time_height Y_TEXT Y_GRID_TOP
 * signals....
 * Y_GRID_BOTTOM [Y_TEXT cursor_rel_height Y_TEXT cursor_abs_height ] Y_TOP_BOTTOM */
#define Y_TOP_BOTTOM 	4	/* Y Pixels to leave empty on top and bottom of display */
#define Y_CURSOR_TOP 	7	/* Y Pixels cursor drawn before first signal */
#define Y_GRID_TOP 	15	/* Y Pixels grid drawn before first signal */
#define Y_GRID_BOTTOM	2	/* Y Pixels grid drawn after last potential signal */
#define Y_TEXT_SPACE	1	/* Y Pixels between text fields */
#define Y_SIG_SPACE	5	/* Space between high and low adjacent signals */

#define	RES_SCALE	((float)500.0)	/* Scaling factor for entering resolution */

/* Cursors for various actions - index into xcursor array */
#define DC_NORMAL	0	/* XC_top_left_arrow	Normal cursor */
#define DC_BUSY		1	/* XC_watch		Busy cursor */
#define DC_SIG_ADD	2	/* XC_sb_left_arrow	Signal Add */
#define DC_SIG_MOVE_2	2	/* XC_sb_left_arrow	Signal Move (place) */
#define DC_SIG_MOVE_1	3	/* XC_sb_right_arrow	Signal Move (pick) */
#define DC_SIG_COPY_2	2	/* XC_sb_left_arrow	Signal Copy (place) */
#define DC_SIG_COPY_1	3	/* XC_sb_right_arrow	Signal Copy (pick) */
#define DC_SIG_DELETE	4	/* XC_hand1		Signal Delete */
#define DC_SIG_HIGHLIGHT 10	/* XC_spraycan		Signal Highlight */
#define DC_CUR_ADD	5	/* XC_center_ptr	Cursor Add */
#define DC_CUR_MOVE	6	/* XC_sb_h_double_arrow	Cursor Move (drag) */
#define DC_CUR_DELETE	7	/* XC_X_cursor		Cursor Delete */
#define DC_CUR_HIGHLIGHT 10	/* XC_spraycan		Cursor Highlight */
#define DC_ZOOM_1	8	/* XC_left_side		Zoom point 1 */
#define DC_ZOOM_2	9	/* XC_right_side	Zoom point 2 */
#define DC_VAL_EXAM	11	/* XC_question_arrow	Value Examine */
#define DC_VAL_HIGHLIGHT 12	/* XC_cross		Value Highlight */

/* All of the states a signal can be in (have only 3 bits so 0-7) */
#define STATE_0   0
#define STATE_1   1
#define STATE_U   2
#define STATE_Z   3
#define STATE_B32 4		/* 2-32 bit vector */
#define STATE_B128 5		/* 33-128 bit vector */
#define STATE_UN6 6		/* not used */
#define STATE_UN7 7		/* not used */

#define PDASH_HEIGHT	2	/* Heigth of primary dash, <= SIG_SPACE, in pixels */
#define SDASH_HEIGHT	2	/* Heigth of primary dash, <= SIG_SPACE, in pixels */
#define SIG_RF		2	/* Rise fall number of pixels */
#define DELU	 	5	/* Delta distance for drawing U's */
#define DELU2		10	/* 2x DELU */

#define PS_START_Y	600	/* Postscript y starting position */

#define IO_GRIDRES	1
#define IO_RES		3

#define XMARGIN	 	5	/* Space on left margin before signals */
#define XSTART_MIN	50		/* Min Start X pos of signals on display (read_DECSIM) */
#define XSTART_MARGIN	10		/* Additional added fudge factor for xstart */

#define DIALOG_WIDTH	75
#define DIALOG_HEIGHT	50

/**********************************************************************/
/* Basic Types */

typedef	int 	DTime;			/* Note "Time" is defined by X.h - some uses of -1 */

/**********************************************************************/
/* Enums */

/* DManageChild keysim or not */
typedef enum {
    MC_NOKEYS=FALSE,
    MC_GLOBALKEYS=TRUE
} MCKeys_t;

/* Half/Quarter/Full page enums */
typedef enum {
    PAGEINC_FULL=1,
    PAGEINC_HALF=2,
    PAGEINC_QUARTER=4
} PageInc_t;

/* Bus representation enums */
typedef enum {
    BUSREP_OCT_UN=1,
    BUSREP_HEX_UN=2,
    BUSREP_DEC_UN=3
} BusRep_t;

/* Time representation enums */
#define TIMEREP_PS 1.0
#define TIMEREP_NS 1000.0
#define TIMEREP_US 1000000.0
#define TIMEREP_CYC -1.0
typedef double TimeRep;

/* Print page sizes */
typedef enum {
    PRINTSIZE_A,		/* Must be zero */
    PRINTSIZE_B,
    PRINTSIZE_EPSPORT,
    PRINTSIZE_EPSLAND
} PrintSize;

/* Colors */
#define MAX_SRCH	9		/* Maximum number of search values, or cursor/signal colors */
#define MAXCOLORS	10		/* Maximum number of colors, all types, including bars */
#define COLOR_CURRENT	(MAX_SRCH+1)
#define COLOR_NEXT	(MAX_SRCH+2)
typedef	int	ColorNum;
typedef	int	VSearchNum;

extern Boolean	DTDEBUG;		/* Debugging mode */
extern uint_t	DTPRINT;		/* Information printing mode */
extern int	DebugTemp;		/* Temp value for trying things */

#define DTPRINT_ENTRY	 (DTPRINT & 0x00000001)	/* Print routine entries */
#define DTPRINT_CONFIG	 (DTPRINT & 0x00000010)	/* Print config reading information */
#define DTPRINT_FILE	 (DTPRINT & 0x00000100)	/* Print file reading information */
#define DTPRINT_DISPLAY	 (DTPRINT & 0x00000200)	/* Print dispmgr information */
#define DTPRINT_DRAW	 (DTPRINT & 0x00000400)	/* Print dispmgr information */
#define DTPRINT_PRINT	 (DTPRINT & 0x00000800)	/* Print postscript printing information */
#define DTPRINT_SEARCH	 (DTPRINT & 0x00001000)	/* Print searching value/signal information */
#define DTPRINT_BUSSES	 (DTPRINT & 0x00002000)	/* Print make busses information */
#define DTPRINT_SOCKET	 (DTPRINT & 0x00010000)	/* Print socket connection information */
#define DTPRINT_PRESERVE (DTPRINT & 0x00100000)	/* Print signal preservation information */

/* File formats.  See also hardcoded case statement in dinotrace.c */
#define	FF_AUTO		0		/* Automatic selection */
#define	FF_DECSIM	1		/* May be ascii or binary */
#define	FF_TEMPEST	2
#define	FF_VERILOG	3
#define	FF_DECSIM_BIN	4
#define	FF_DECSIM_ASCII	5
#define	FF_NUMFORMATS	6		/* Number of formats */
extern uint_t		file_format;	/* Type of trace to support */
extern struct st_filetypes {
    Boolean		selection;	/* True if user can select this format */
    char	       	*name;		/* Name of this file type */
    char		*extension;	/* File extension */
    char		*mask;		/* File Open mask */
    /* void		(*routine);	/ * Routine to read it */
} filetypes[FF_NUMFORMATS];

extern char		message[1000];	/* generic string for messages */

extern XGCValues	xgcv;

extern Arg		arglist[20];

/* Define some types that are needed before defined */
typedef struct st_trace Trace;
typedef struct st_signal Signal;

/**********************************************************************/
/* Widget structures */

typedef struct {
    Widget	menu;
    Widget	pdmenu[11];
    Widget	pdmenubutton[11];
    Widget	pdsep[10];
    Widget	pdentry[22];
    Widget	pdentrybutton[72];
    Widget	pdsubbutton[4+(MAX_SRCH+2)*5];
    uint_t	sig_highlight_pds;
    uint_t	cur_highlight_pds;
    uint_t	cur_add_pds;
    uint_t	val_highlight_pds;
    uint_t	pdm, pdmsep, pde, pds;		/* Temp for loading structure */
} MenuWidgets;

typedef struct {
    Widget form;
    Widget begin_but;
    Widget goto_but;
    Widget resdec_but;
    Widget resfull_but;
    Widget reschg_but;
    Widget reszoom_but;
    Widget resinc_but;
    Widget end_but;
    Widget refresh_but;
    /* Slider */
    Widget namescroll;
} CommandWidgets;

typedef struct {
    Widget dialog;
    Widget form;
    Widget page_label;
    Widget rpage, tpage1, tpage2, tpage3;
    Widget bus_label;
    Widget rbus, tbus1, tbus2, tbus3, tbus4, tbus5;
    Widget time_label;
    Widget rtime, ttimens, ttimecyc, ttimeus, ttimeps;
    Widget sighgt_label;
    Widget s1;
    Widget rfwid;
    Widget cursor_state;
    Widget click_to_edge;
    Widget refreshing;
    Widget sep;
    Widget b1;
    Widget b2;
    Widget b3;
} CustomWidgets;

typedef struct {
    Trace *trace;		/* Link back, as callbacks won't know without it */
    enum { BEGIN=0, END=1} type;
    DTime  dastime;
    /* Begin stuff */
    Widget time_label;
    Widget time_pulldown;
    Widget time_pulldownbutton[4];
    Widget time_option;
    Widget time_text;
} RangeWidgets;

typedef struct {
    Widget dialog;
    Widget form;
    Widget size_menu;
    Widget size_option;
    Widget sizea;
    Widget sizeb;
    Widget sizeep;
    Widget sizeel;
    Widget text;
    Widget label;
    Widget notetext;
    Widget notelabel;
    Widget pagelabel;
    Widget all_signals;
    RangeWidgets begin_range;
    RangeWidgets end_range;
    Widget sep;
    Widget print;
    Widget defaults;
    Widget cancel;
} PrintWidgets;

typedef struct {
    Widget dialog;
    Widget label1, label2, label3, label4;
    Widget cursors[MAX_SRCH+1];
    Widget cursors_dotted[MAX_SRCH+1];
    Widget signals[MAX_SRCH+1];
    Widget text;
    Widget sep;
    Widget ok;
    Widget apply;
    Widget cancel;
} AnnotateWidgets;

typedef struct {
    Widget search;
    Widget form;
    Widget add;
    Widget label1, label2, label3;
    Widget label4, label5;
    Widget enable[MAX_SRCH];
    Widget text[MAX_SRCH];
    Widget sep;
    Widget ok;
    Widget apply;
    Widget cancel;
} SignalWidgets;

typedef struct {
    Widget search;
    Widget form;
    Widget label1, label2, label3;
    Widget label4, label5, label6;
    Widget enable[MAX_SRCH];
    Widget cursor[MAX_SRCH];
    Widget text[MAX_SRCH];
    Widget signal[MAX_SRCH];
    Widget sep;
    Widget ok;
    Widget apply;
    Widget cancel;
} ValueWidgets;

typedef struct {
    Widget dialog;
    Widget work_area;
    Widget format_menu, format_option, format_item[FF_NUMFORMATS];
    Widget save_ordering;
} FileWidgets;

typedef struct {
    Widget dialog;
    Widget form;
    Widget work_area;
    Widget save_ordering;
    Widget config_label;
    Widget config_enable[MAXCFGFILES];
    Widget config_filename[MAXCFGFILES];
} CusReadWidgets;

typedef struct {
    Widget popup;
    Widget label;
} ExamineWidgets;

typedef struct {
    Widget dialog;
    Widget form;
    Widget text;
    Widget label1,label2;
    Widget pulldown;
    Widget pulldownbutton[MAX_SRCH+1];
    Widget options;
    Widget sep;
    Widget ok;
    Widget cancel;
} GotoWidgets;

typedef struct {
    Widget label1;
    Widget visible;
    Widget wide_line;
    Widget siglabel, signal;
    Widget periodlabel, period;
    Widget align;
    /* Auto Period stuff */
    Widget autoperiod_pulldown;
    Widget autoperiod_pulldownbutton[3];
    Widget autoperiod_options;
    /* Auto Align stuff */
    Widget autoalign_pulldown;
    Widget autoalign_pulldownbutton[4];
    Widget autoalign_options;
    /* Color stuff */
    Widget pulldown;
    Widget pulldownbutton[MAX_SRCH+1];
    Widget options;
} GridWidgets;

typedef struct {
    Widget dialog;
    Widget form;
    GridWidgets  grid[MAXGRIDS];
    Widget sep;
    Widget ok;
    Widget apply;
    Widget cancel;
} GridsWidgets;

typedef struct {
    Widget select;
    Widget label1, label2, label3;
    Widget label4, label5;
    Widget enable[MAX_SRCH];
    Widget cursor[MAX_SRCH];
    Widget add_sigs, add_sigs_form, add_pat, add_all;
    Widget delete_sigs, delete_pat, delete_all, delete_const;
    Widget sep;
    Widget ok;
    Widget apply;	/* ?? */
    Widget cancel;

    XmString *del_strings;
    XmString *add_strings;
    Signal   **del_signals;
    Signal   **add_signals;
    uint_t    del_size;
    uint_t    add_size;
} SelectWidgets;

/**********************************************************************/
/* Structures */

typedef struct {
    Position		x, y, height, width;	
    Boolean		xp, yp, heightp, widthp;	/* Above element is a percentage */
} Geometry;

/* Structure for each signal-state assignment */
typedef struct st_signalstate {
    struct st_signalstate *next;	/* Next structure in a linked list */
    uint_t	 numstates;		/* Number of states in the structure */
    char signame[MAXSIGLEN];		/* Signal name to translate, Nil = wildcard */
    char statename[MAXSTATENAMES][MAXVALUELEN];	/* Name for each state, nil=keep */
} SignalState;

/* Signal LW: A structure for each transition of each signal, */
/* 32/64/96 state signals have an additional 1, 2, or 3 LWs after this */
typedef union un_signal_lw {
    struct {
	uint_t state:3;
	uint_t time:29;
    } stbits;
    uint_t number;
} SignalLW_t;
#define EOT	0x1FFFFFFF	/* SignalLW End of time indicator if in .time */

/* Value: A signal_lw and 3 data elements */
/* A cptr points to at least a SignalLW and at most a Value */
/* since from 0 to 2 of the uint_ts are dropped in the cptr array */
typedef struct {
    SignalLW_t	siglw;
    uint_t	number[4];	/* [0]=bits 31-0, [1]=bits 63-32, [2]=bits 95-64, [3]=bits 127-96 */
} Value_t;

/* Value searching structure */
typedef struct {
    ColorNum	color;		/* Color number (index into trace->xcolornum) 0=OFF*/
    ColorNum	cursor;		/* Enable cursors, color or 0=OFF */
    uint_t	value[4];	/* Value to search for, (128 bit LW format) */
    char	signal[MAXSIGLEN];	/* Signal to search for */
} ValSearch;

/* Signal searching structure */
typedef struct {
    ColorNum	color;		/* Color number (index into trace->xcolornum) 0=OFF*/
    char 	string[MAXSIGLEN];	/* Signal to search for */
} SigSearch;

/* Cursor information structure (one per cursor) */
typedef enum {
    USER=0,	/* User placed it, preserve across rereading traces */
    SEARCH,	/* Value search, replace as needed */
    SEARCHOLD,	/* Old search, used by val_update_search only */
    CONFIG	/* Config file read in, replace when reread */
} CursorType;

typedef struct st_cursor {
    struct st_cursor	*next;		/* Forward link to next cursor */
    struct st_cursor	*prev;		/* Backward link to previous cursor */

    DTime		time;		/* Time cursor is placed at */
    ColorNum		color;		/* Color number (index into trace->xcolornum) */
    CursorType		type;		/* Type of cursor */
} DCursor; /* Not 'Cursor', as that's defined in X11.h */

typedef struct {
    DTime		period;		/* Grid period (time between ticks) */
    DTime		alignment;	/* Grid alignment (time grid starts at) */
    enum { PA_USER=0, PA_AUTO=1, PA_EDGE=2 } period_auto;	/* Status of automatic grid resolution */
    enum { AA_USER=0, AA_ASS=1, AA_DEASS=2 } align_auto; /* Status of automatic grid alignment */
    Boolean		visible;	/* True if grid is visible */
    Boolean		wide_line;	/* True to draw a double-width line */
    char 		signal[MAXSIGLEN];	/* Signal name of the clock */
    ColorNum		color;		/* Color to print in, 0 = black */
} Grid;

/* Signal information structure (one per each signal in a trace window) */
struct st_signal {
    struct st_signal	*forward;	/* Forward link to next signal */
    struct st_signal	*backward;	/* Backward link to previous signal */

    SignalLW_t		*bptr;		/* begin of time data ptr */
    SignalLW_t		*cptr;		/* current time data ptr */

    struct st_signal	*copyof;	/* Link to signal this is copy of (or NULL) */
    Trace		*trace;		/* Trace signal belongs to (originally) */
    struct st_signal	*verilog_next;	/* Next verilog signal with same coding */

    char		*signame;	/* Signal name */
    XmString		xsigname;	/* Signal name as XmString */
    char		*signame_buspos;/* Signal name portion where bus bits begin (INSIDE signame) */

    ColorNum		color;		/* Signal line's Color number (index into trace->xcolornum) */
    ColorNum		search;		/* Number of search color is for, 0 = manual */

    Boolean		srch_ena[MAX_SRCH];	/* Searching is enabled */
    Boolean		deleted;	/* Signal is deleted */
    Boolean		deleted_preserve; /* Preserve the deletion of this signal (not deleted because constant) */
    Boolean		preserve_done;	/* Preservation process has moved this signal to new link structure */

    uint_t		type;		/* Type of signal, STATE_B32, _B64, etc */
    SignalState		*decode;	/* Pointer to decode information, NULL if none */
    char *		(*decode_fptr)(); /* Pointer to function that decodes statenames, NULL if none */
    uint_t		lws;		/* Number of LWs in a SignalLW record */
    ulong_t		blocks;		/* Number of time data blocks allocated, in # of ints */
    int			msb_index;	/* Bit subscript of first index in a signal (<20:10> == 20), -1=none */
    int			lsb_index;	/* Bit subscript of last index in a signal (<20:10> == 10), -1=none */
    int			bit_index;	/* Bit subscript of this bit, ignoring <>'s, tempest only */
    int			bits;		/* Number of bits in a bus, 0=single */
    uint_t		file_pos;	/* Position of the bits in the file line */
    uint_t		file_end_pos;	/* Ending position of the bits in the file line */

    union {
	struct {
	    uint_t	pin_input:1;	/* (Tempest) Pin is an input */
	    uint_t	pin_output:1;	/* (Tempest) Pin is an output */
	    uint_t	pin_psudo:1;	/* (Tempest) Pin is an psudo-pin */
	    uint_t	pin_timestamp:1; /* (Tempest) Pin is a time-stamp */
	    uint_t	four_state:1;	/* (Tempest, Binary) Signal is four state (U, Z) */
	    uint_t	perm_vector:1;	/* (Verilog) Signal is a permanent vector, don't vectorize */
	    uint_t	vector_msb:1;	/* (Tempest) Signal starts a defined vector, may only be new MSB */
	} flag;
	uint_t		flags;
    }		file_type;	/* File specific type of trace, two/fourstate, etc */

    uint_t		value_mask[4];	/* Value Mask with 1s in bits that are to be set */
    uint_t		pos_mask;	/* Mask to translate file positions */
    Value_t		file_value;	/* current state/time LW information for reading in */
}; /*Signal;  typedef'd above */

/* Signal list structure */
typedef struct st_signal_list {
    struct st_signal_list	*forward;	/* Forward link to next signal */
    Trace		*trace;		/* Trace signal belongs to (originally) */
    Signal		*signal;	/* Selected signal */
} SignalList;

/* Trace information structure (one per window) */
struct st_trace {
    struct st_trace	*next_trace;	/* Pointer to the next trace display */

    Signal		*firstsig;	/* Linked list of all nondeleted signals */
    Signal		*dispsig;	/* Pointer within sigque to first signal on screen */

    uint_t		numsig;		/* Total number of signals, excluding deleted */
    uint_t		numsigvis;	/* Maximum number of signals visible on the screen */
    uint_t		numsigstart;	/* signal to start displaying */

    Window		wind;		/* X window */
    Pixel		xcolornums[MAXCOLORS];	/* X color numbers (pixels) for normal/highlight */
    Pixel		barcolornum;	/* X color number for the signal bar background */
    GC                  gc;
    GC                  hscroll_gc;

    MenuWidgets		menu;
    Widget		menu_close;	/* Pointer to menu_close widget */
    CommandWidgets	command;
    CustomWidgets	custom;
    PrintWidgets	prntscr;
    AnnotateWidgets	annotate;
    SignalWidgets	signal;
    ExamineWidgets	examine;
    GotoWidgets		gotos;
    ValueWidgets	value;
    GridsWidgets	gridscus;
    SelectWidgets	select;
    FileWidgets		fileselect;
    CusReadWidgets	cusread;
    Widget		shell;
    Widget		main;
    Widget		work;
    Widget		hscroll;
    Widget		vscroll;
    Widget		customize;	/* Customization widget */
    Widget		toplevel;	/* Top level shell */
    Widget		prompt_popup;	/* Data popup widget */
    uint_t		prompt_type;	/* Type of data popup widget */
    Widget		message;	/* Message (error/warn/etc) widget */
    Widget		help_doc;	/* Help documentation */
    Widget		help_doc_text;	/* Help documentation */

    char		filename[MAXFNAMELEN];	/* Current file */
    char		printname[MAXFNAMELEN];	/* Print filename */
    struct stat		filestat;	/* Information on the current file */
    uint_t		fileformat;	/* Type of trace file (see FF_*) */
    Boolean		loaded;		/* True if the filename is loaded in */
    char		vector_seperator;	/* Bus seperator character, usually "<" */
    char		vector_endseperator;	/* Bus ending seperator character, usually ">" */

    uint_t		redraw_needed;	/* Need to refresh the screen when get a chance, TRD_* bit fielded */
#define				TRD_REDRAW	0x1
#define				TRD_EXPOSE	0x2

    Dimension		width;		/* Screen width */
    Dimension		sighgt;		/* Height of signals (customize) */

    Position		ygridtime;	/* Start Y pos of grid times (get_geom) */
    Position		ystart;		/* Start Y pos of signals (get_geom) */
    Position		yend;		/* End Y pos of signals (get_geom) */
    Position		ycursortimerel;	/* Start Y pos of cursor relative times (get_geom) */
    Position		ycursortimeabs;	/* Start Y pos of cursor absolute times (get_geom) */
    Dimension		height;		/* Screen height */

    uint_t		sigrf;		/* Signal rise/fall time spec */
    BusRep_t		busrep;		/* Bus representation = IBUS/BBUS/OBUS/HBUS/DBUS */
    TimeRep		timerep;	/* Time representation = TIMEREP_NS/TIMEREP_CYC */

    Grid		grid[MAXGRIDS];	/* Grid information */
    Boolean		cursor_vis;	/* True if cursors are visible */

    DTime		start_time;	/* Time of beginning of trace */
    DTime		end_time;	/* Time of ending of trace */
}; /*Trace;  typedef'd above */

/* Global information */
typedef struct {
    Trace		*trace_head;	/* Pointer to first trace, set deleted_trace->next too  */
    Trace		*preserved_trace;	/* Pointer to old trace when reading in new one */
    Trace		*deleted_trace_head;	/* Pointer to trace with deleted signals, which then links to teace_head */

    Signal		*selected_sig;	/* Selected signal to move or add */
    Trace		*selected_trace; /* Selected signal's trace */
    SignalList		*select_head;	/* Pointer to selected signal list head */

    DCursor		*cursor_head;	/* Pointer to first cursor */

    ValSearch		val_srch[MAX_SRCH];	/* Value search information */

    SigSearch		sig_srch[MAX_SRCH];	/* Signal search information */

    SignalState		*signalstate_head;	/* Head of signal state information */

    XtAppContext	appcontext;	/* X App context */
    Display		*display;	/* X display pointer */
    Cursor		xcursors[13];	/* X cursors */
    Pixmap		dpm,bdpm;	/* X pixmaps for the icons */

    uint_t		argc;		/* Program argc for X stuff */
    char		**argv;		/* Program argv for X stuff */

    char		directory[MAXFNAMELEN];	/* Current directory name */

    ColorNum		highlight_color; /* Color selected for sig/cursor highlight */
    ColorNum		goto_color;	/* Cursor color to place on a 'GOTO' -1=none */
    char *		color_names[MAXCOLORS];	/* Names of the colors from the user */
    char *		barcolor_name;		/* name of the signal bar color */

    char *		signal_font_name;	/* name of font for signal names */
    char *		time_font_name;		/* name of font for times */
    char *		value_font_name;	/* name of font for values */
    XFontStruct		*signal_font;		/* Signal Text font */
    XFontStruct		*time_font;		/* Time Text font */
    XFontStruct		*value_font;		/* Value Text font */

    Geometry		start_geometry;	/* Geometry to open first trace with */
    Geometry		open_geometry;	/* Geometry to open later traces with */
    Geometry		shrink_geometry; /* Geometry to shrink trace->open traces with */

    Boolean		anno_poppedup;	/* Annotation has been poped up on some window */
    Boolean		anno_ena_signal[MAX_SRCH+1];   /* Annotation signal enables */
    Boolean		anno_ena_cursor[MAX_SRCH+1];    /* Annotation cursor enables */
    Boolean		anno_ena_cursor_dotted[MAX_SRCH+1];    /* Annotation cursor enables */
    char		anno_filename[MAXFNAMELEN]; /* Annotation file name */
    char		anno_socket[MAXFNAMELEN];	/* Annotation socket number */

    PageInc_t		pageinc;	/* Page increment = HPAGE/QPAGE/FPAGE */
    Boolean		click_to_edge;	/* True if clicking to edges is enabled */
    TimeRep		time_precision;	/* Time precision = TIMEREP_NS/TIMEREP_CYC */
    char		time_format[12]; /* Time format = printf format or *NULL */
    uint_t		tempest_time_mult;	/* Time multiplier for tempest */
    Boolean		save_enables;	/* True to save enable wires */
    Boolean		save_ordering;	/* True to save signal ordering */

    char		printnote[MAXFNAMELEN];	/* Note to print */
    PrintSize		print_size;	/* Size of paper for dt_printscreen */
    Boolean		print_all_signals; /* Print all signals in the trace */
    DTime		print_begin_time;  /* Starting time for printing */
    DTime		print_end_time;	/* Ending time for printing */

    Boolean		redraw_manually;/* True if in manual refreshing mode */
    uint_t		redraw_needed;	/* Some trace needs to refresh the screen when get a chance, GRD_* bit fielded */
#define				GRD_TRACE	0x1U
#define				GRD_ALL		0x2U
#define				GRD_MANUAL	0x4U
    uint_t		updates_needed;	/* Things that are out of date and need to be called before redraw */
#define				GUD_SIG_START	0x100U	/* Call sig_update_start */
#define				GUD_SIG_SEARCH	0x200U	/* Call sig_update_search */
#define				GUD_VAL_SEARCH	0x400U	/* Call val_update_search */
#define				GUD_VAL_STATES	0x800U	/* Call val_update_states */

    DTime		time;		/* Time of trace at left edge of screen */
    float		res;		/* Resolution of graph width (gadgets) */
    Boolean		res_default;	/* True if resolution has never changed from initial value */
    Position		xstart;		/* Start X pos of signals on display */
    uint_t		namepos;	/* Position of first visible character based on xstart */
    uint_t		namepos_hier;	/* Maximum hiearchy width in chars */
    uint_t		namepos_base;	/* Maximum basename width in chars */
    uint_t		namepos_visible;/* Visible size of names in chars */
    DTime		click_time;	/* time clicked on for res_zoom_click */
    Grid		*click_grid;	/* grid being set by grid_align */

    Boolean		config_enable[MAXCFGFILES];/* Read in this config file */
    char		config_filename[MAXCFGFILES][MAXFNAMELEN];	/* Config files */
} Global;

extern Global *global;

