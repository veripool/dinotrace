/******************************************************************************
 *
 * Filename:
 *	Dinotrace.h
 *
 * Subsystem:
 *     Dinotrace
 *
 * Version:
 *     Dinotrace V6.0
 *
 * Author:
 *     Allen Gallotta
 *
 * Abstract:
 *     This module contains 
 *
 * Modification History:
 *     AAG	 5-Jul-89	Original Version
 *     AAG	 6-Nov-90	Added screen global and changed version to 4.2
 *     AAG	 9-Jul-91	Changed to version 4.3, added trace format
 *				 support
 *     WPS	 4-Jan-93	Version 5.0
 *     WPS	 4-Jan-93	Version 5.1 - Upped max state names to 96
 *     WPS	 11-Mar-93	Version 5.3 - Massive commenting
 *
 */

#define DTVERSION	"Dinotrace V7.5a"
/*#define EXPIRATION	((60*60*24)*6*30) / * 6months - In seconds - Comment out define for no expiration dates */
#undef	EXPIRATION

/* Turn off alignment for any structures that will be read/written onto Disk! */
/*
#ifndef __osf__
#pragma member_alignment
#endif
*/

#ifdef __osf__
#define	INLINE inline
#else
#define	INLINE
#endif

#define MAXSIGLEN	256	/* Maximum length of signal names */
#define MAXTIMELEN	20	/* Maximum length of largest time as a string */
#define MAXFNAMELEN	200	/* Maximum length of file names */
#define MAXSTATENAMES	130	/* Maximum number of state name translations */
#define MAXSTATELEN	32	/* Maximum length of state names */
#define MAXGRIDS	4	/* Maximum number of grids */
#define MAXSCREENWIDTH	5000	/* Maximum width of screen */
#define	MIN_GRID_RES	5	/* Minimum grid resolution, in pixels between grid lines */
#define	GRID_TIME_Y	20	/* Y Coordinate of where to print grid times */
#define BLK_SIZE	512	/* Trace data block size */
#define	CLICKCLOSE	20	/* Number of pixels that are "close" for click-to-edges */

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
#define STATE_B32 4
#define STATE_B64 5
#define STATE_B96 6

/* dino_message_ack types */
#define	dino_error_ack(tr,msg)		dino_message_ack (tr, 0, msg)
#define	dino_warning_ack(tr,msg)	dino_message_ack (tr, 1, msg)
#define	dino_information_ack(tr,msg)	dino_message_ack (tr, 2, msg)

#define SIG_SPACE	5	/* Space for drawing signal names */
#define PDASH_HEIGHT	2	/* Heigth of primary dash, <= SIG_SPACE, in pixels */
#define SDASH_HEIGHT	2	/* Heigth of primary dash, <= SIG_SPACE, in pixels */
#define SIG_RF		2	/* Rise fall number of pixels */
#define DELU	 	5	/* Delta distance for drawing U's */
#define DELU2		10	/* 2x DELU */

#define PS_START_Y	600	/* Postscript y starting position */
#define XMARGIN	 	5	/* Space on left margin before signals */

/* Half/Quarter/Full page enums */
#define QPAGE 4
#define HPAGE 2
#define FPAGE 1

/* Bus representation enums */
#define IBUS 1
#define BBUS 2
#define OBUS 3
#define HBUS 4

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

#define IO_GRIDRES	1
#define IO_RES		3

#define XSTART_MIN	50		/* Min Start X pos of signals on display (read_DECSIM) */
#define XSTART_MARGIN	10		/* Additional added fudge factor for xstart */

#define DIALOG_WIDTH	75
#define DIALOG_HEIGHT	50


/* Utilities */
#ifndef MAX
# if __GNUC__
#   define MAX(a,b) \
       ({typedef _ta = (a), _tb = (b);  \
         _ta _a = (a); _tb _b = (b);     \
         _a > _b ? _a : _b; })
# else
#  define MAX(_a_,_b_) ( ( ( _a_ ) > ( _b_ ) ) ? ( _a_ ) : ( _b_ ) )
# endif
#endif

#ifndef MIN
# if __GNUC__
#   define MIN(a,b) \
       ({typedef _ta = (a), _tb = (b);  \
         _ta _a = (a); _tb _b = (b);     \
         _a < _b ? _a : _b; })
# else
#   define MIN(_a_,_b_) ( ( ( _a_ ) < ( _b_ ) ) ? ( _a_ ) : ( _b_ ) )
# endif
#endif

#ifndef ABS
# if __GNUC__
#   define ABS(a) \
       ({typedef _ta = (a);  \
         _ta _a = (a);     \
         _a < 0 ? - _a : _a; })
# else
#   define ABS(_a_) ( ( ( _a_ ) < 0 ) ? ( - (_a_) ) : ( _a_ ) )
# endif
#endif

#define max_sigs_on_screen(_trace_) \
        ((int)(((_trace_)->height - (_trace_)->ystart) / (_trace_)->sighgt))

#define TIME_TO_XPOS(_xtime_) \
        ( ((_xtime_) - global->time) * global->res + global->xstart )

/* Avoid binding error messages on XtFree, NOTE ALSO clears the pointer! */
#define DFree(ptr) { XtFree((char *)ptr); ptr = NULL; }

/* Useful for debugging messages */
#define DeNull(_str_) ( ((_str_)==NULL) ? "NULL" : (_str_) )
#define DeNullSignal(_sig_) ( ((_sig_)==NULL) ? "NULLPTR" : (DeNull((_sig_)->signame) ) )

typedef	long 	DTime;			/* Note "Time" is defined by X.h - some uses of -1 */

/* Colors */
#define MAX_SRCH	9		/* Maximum number of search values, or cursor/signal colors */
#define MAXCOLORS	10		/* Maximum number of colors, all types, including bars */
#define COLOR_CURRENT	(MAX_SRCH+1)
#define COLOR_NEXT	(MAX_SRCH+2)
typedef	int	ColorNum;
typedef	int	VSearchNum;

extern Boolean	DTDEBUG;		/* Debugging mode */
extern int	DTPRINT;		/* Information printing mode */
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
extern int		file_format;	/* Type of trace to support */
extern struct st_filetypes {
    Boolean		selection;	/* True if user can select this format */
    char	       	*name;		/* Name of this file type */
    char		*extension;	/* File extension */
    char		*mask;		/* File Open mask */
    /*char		*pipe;		/ * Pipe to use (if NULL, no pipe) */
    /* void		(*routine);	/ * Routine to read it */
    } filetypes[FF_NUMFORMATS];

extern char		message[1000];	/* generic string for messages */

extern XGCValues	xgcv;

extern Arg		arglist[20];

typedef struct {
    Widget	menu;
    Widget	pdmenu[11];
    Widget	pdmenubutton[11];
    Widget	pdentry[22];
    Widget	pdentrybutton[72];
    Widget	pdsubbutton[4+(MAX_SRCH+2)*5];
    int		sig_highlight_pds;
    int		cur_highlight_pds;
    int		cur_add_pds;
    int		val_highlight_pds;
    int		pdm, pde, pds;		/* Temp for loading structure */
    } MENU_WDGTS;

typedef struct {
    Widget command;
    Widget begin_but;
    Widget goto_but;
    Widget resdec_but;
    Widget resfull_but;
    Widget reschg_but;
    Widget reszoom_but;
    Widget resinc_but;
    Widget end_but;
    Widget refresh_but;
    } COMMAND_WDGTS;

typedef struct {
    Widget customize;
    Widget page_label;
    Widget rpage, tpage1, tpage2, tpage3;
    Widget bus_label;
    Widget rbus, tbus1, tbus2, tbus3, tbus4;
    Widget time_label;
    Widget rtime, ttimens, ttimecyc, ttimeus, ttimeps;
    Widget sighgt_label;
    Widget s1;
    Widget rfwid;
    Widget cursor_state;
    Widget click_to_edge;
    Widget refreshing;
    Widget b1;
    Widget b2;
    Widget b3;
    } CUSTOM_WDGTS;

typedef struct {
    struct st_trace	*trace;		/* Link back, as callbacks won't know without it */
    enum { BEGIN=0, END=1} type;
    DTime  dastime;
    /* Begin stuff */
    Widget time_label;
    Widget time_pulldown;
    Widget time_pulldownbutton[4];
    Widget time_option;
    Widget time_text;
    } RANGE_WDGTS;

typedef struct {
    Widget customize;
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
    RANGE_WDGTS begin_range;
    RANGE_WDGTS end_range;
    Widget print;
    Widget cancel;
    } PRINT_WDGTS;

typedef struct {
    Widget dialog;
    Widget label1, label2, label3, label4;
    Widget cursors[MAX_SRCH+1];
    Widget cursors_dotted[MAX_SRCH+1];
    Widget signals[MAX_SRCH+1];
    Widget text;
    Widget ok;
    Widget apply;
    Widget cancel;
    } ANNOTATE_WDGTS;

typedef struct {
    Widget add;
    Widget search;
    Widget label1, label2, label3;
    Widget label4, label5;
    Widget enable[MAX_SRCH];
    Widget text[MAX_SRCH];
    Widget ok;
    Widget apply;
    Widget cancel;
    } SIGNAL_WDGTS;

typedef struct {
    Widget search;
    Widget label1, label2, label3;
    Widget label4, label5;
    Widget enable[MAX_SRCH];
    Widget cursor[MAX_SRCH];
    Widget text[MAX_SRCH];
    Widget ok;
    Widget apply;
    Widget cancel;
    } VALUE_WDGTS;

typedef struct {
    Widget dialog;
    Widget work_area;
    Widget format_menu, format_option, format_item[FF_NUMFORMATS];
    Widget save_ordering;
    } FILE_WDGTS;

typedef struct {
    Widget popup;
    Widget label;
    } EXAMINE_WDGTS;

typedef struct {
    Widget popup;
    Widget text;
    Widget ok;
    Widget label1,label2;
    Widget cancel;
    Widget pulldown;
    Widget pulldownbutton[MAX_SRCH+1];
    Widget options;
    } GOTOS_WDGTS;

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
    } GRID_WDGTS;

typedef struct {
    Widget popup;
    Widget ok;
    Widget apply;
    Widget cancel;
    GRID_WDGTS  grid[MAXGRIDS];
    } GRIDS_WDGTS;

typedef struct st_geometry {
    Position		x, y, height, width;	
    Boolean		xp, yp, heightp, widthp;	/* Above element is a percentage */
    } GEOMETRY;

/* 5.0: Structure for each signal-state assignment */
typedef struct st_signalstate {
    struct st_signalstate *next;	/* Next structure in a linked list */
    int	 numstates;			/* Number of states in the structure */
    char signame[MAXSIGLEN];		/* Signal name to translate, Nil = wildcard */
    char statename[MAXSTATENAMES][MAXSTATELEN];	/* Name for each state, nil=keep */
    } SIGNALSTATE;

/* Signal LW: A structure for each transition of each signal, */
/* 32/64/96 state signals have an additional 1, 2, or 3 LWs after this */
typedef union un_signal_lw {
    struct {
	unsigned int state:3;
	unsigned int time:29;
	} sttime;
    unsigned int number;
    } SIGNAL_LW;
#define EOT	0x1FFFFFFF	/* SIGNAL_LW End of time indicator if in .time */

/* Value: A signal_lw and 3 data elements */
/* A cptr points to at least a SIGNAL_LW and at most a VALUE */
/* since from 0 to 2 of the unsigned ints are dropped in the cptr array */
typedef struct st_value {
    SIGNAL_LW		siglw;
    unsigned int	number[3];	/* [0]=bits 31-0, [1]=bits 63-32, [2]=bits 95-64 */
    } VALUE;

/* Value searching structure */
typedef struct st_valsearch {
    ColorNum		color;		/* Color number (index into trace->xcolornum) 0=OFF*/
    ColorNum		cursor;		/* Enable cursors, color or 0=OFF */
    int			value[3];	/* Value to search for, (96 bit LW format) */
    } VALSEARCH;

/* Signal searching structure */
typedef struct st_sigsearch {
    ColorNum		color;		/* Color number (index into trace->xcolornum) 0=OFF*/
    char 		string[MAXSIGLEN];	/* Signal to search for */
    } SIGSEARCH;

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
    } CURSOR;

typedef struct st_grid {
    int			period;		/* Grid period (time between ticks) */
    int			alignment;	/* Grid alignment (time grid starts at) */
    enum { PA_USER=0, PA_AUTO=1, PA_EDGE=2 } period_auto;	/* Status of automatic grid resolution */
    enum { AA_USER=0, AA_ASS=1, AA_DEASS=2 } align_auto; /* Status of automatic grid alignment */
    Boolean		visible;	/* True if grid is visible */
    Boolean		wide_line;	/* True to draw a double-width line */
    char 		signal[MAXSIGLEN];	/* Signal name of the clock */
    ColorNum		color;		/* Color to print in, 0 = black */
    } GRID;

/* Signal information structure (one per each signal in a trace window) */
typedef struct st_signal {
    struct st_signal	*forward;	/* Forward link to next signal */
    struct st_signal	*backward;	/* Backward link to previous signal */

    SIGNAL_LW		*bptr;		/* begin of time data ptr */
    SIGNAL_LW		*cptr;		/* current time data ptr */

    struct st_signal	*copyof;	/* Link to signal this is copy of (or NULL) */
    struct st_trace	*trace;		/* Trace signal belongs to (originally) */
    struct st_signal	*verilog_next;	/* Next verilog signal with same coding */

    char		*signame;	/* Signal name */
    XmString		xsigname;	/* Signal name as XmString */
    char		*signame_buspos;/* Signal name portion where bus bits begin (INSIDE signame) */

    ColorNum		color;		/* Signal line's Color number (index into trace->xcolornum) */
    ColorNum		search;		/* Number of search color is for, 0 = manual */

    Boolean		srch_ena;	/* Searching is enabled */
    Boolean		deleted;	/* Signal is deleted */
    Boolean		deleted_preserve; /* Preserve the deletion of this signal (not deleted because constant) */
    Boolean		preserve_done;	/* Preservation process has moved this signal to new link structure */

    int			type;		/* Type of signal, STATE_B32, _B64, etc */
    SIGNALSTATE		*decode;	/* Pointer to decode information, NULL if none */
    int			lws;		/* Number of LWs in a SIGNAL_LW record */
    int			blocks;		/* Number of time data blocks */
    int			msb_index;	/* Bit subscript of first index in a signal (<20:10> == 20), -1=none */
    int			lsb_index;	/* Bit subscript of last index in a signal (<20:10> == 10), -1=none */
    int			bit_index;	/* Bit subscript of this bit, ignoring <>'s, tempest only */
    int			bits;		/* Number of bits in a bus, 0=single */
    int			file_pos;	/* Position of the bits in the file line */
    int			file_end_pos;	/* Ending position of the bits in the file line */

    union {
	struct {
	    int		pin_input:1;	/* (Tempest) Pin is an input */
	    int		pin_output:1;	/* (Tempest) Pin is an output */
	    int		pin_psudo:1;	/* (Tempest) Pin is an psudo-pin */
	    int		pin_timestamp:1; /* (Tempest) Pin is a time-stamp */
	    int		four_state:1;	/* (Tempest, Binary) Signal is four state (U, Z) */
	    int		perm_vector:1;	/* (Verilog) Signal is a permanent vector, don't vectorize */
	    int		vector_msb:1;	/* (Tempest) Signal starts a defined vector, may only be new MSB */
	    } flag;
	int		flags;
	}		file_type;	/* File specific type of trace, two/fourstate, etc */

    unsigned int	value_mask[3];	/* Value Mask with 1s in bits that are to be set */
    unsigned int	pos_mask;	/* Mask to translate file positions */
    VALUE		file_value;	/* current state/time LW information for reading in */
    } SIGNAL;

/* Signal list structure */
typedef struct st_signal_list {
    struct st_signal_list	*forward;	/* Forward link to next signal */
    struct st_trace		*trace;		/* Trace signal belongs to (originally) */
    struct st_signal		*signal;	/* Selected signal */
    } SIGNAL_LIST;

typedef struct {
    Widget select;
    Widget label1, label2, label3;
    Widget label4, label5;
    Widget enable[MAX_SRCH];
    Widget cursor[MAX_SRCH];
    Widget add_sigs, add_pat, add_all;
    Widget delete_sigs, delete_pat, delete_all, delete_const;
    Widget ok;
    Widget apply;	/* ?? */
    Widget cancel;

    XmString *del_strings;
    XmString *add_strings;
    SIGNAL   **del_signals;
    SIGNAL   **add_signals;
    int	    del_size;
    int	    add_size;
    } SELECT_WDGTS;

/* Trace information structure (one per window) */
typedef struct st_trace {
    struct st_trace	*next_trace;	/* Pointer to the next trace display */

    SIGNAL		*firstsig;	/* Linked list of all nondeleted signals */
    SIGNAL		*dispsig;	/* Pointer within sigque to first signal on screen */

    int			numsig;		/* Total number of signals, excluding deleted */
    int			numsigvis;	/* Maximum number of signals visible on the screen */
    int			numsigstart;	/* signal to start displaying */

    Window		wind;		/* X window */
    Pixel		xcolornums[MAXCOLORS];	/* X color numbers (pixels) for normal/highlight */
    Pixel		barcolornum;	/* X color number for the signal bar background */
    GC                  gc;
    GC                  hscroll_gc;

    MENU_WDGTS		menu;
    Widget		menu_close;	/* Pointer to menu_close widget */
    COMMAND_WDGTS	command;
    CUSTOM_WDGTS	custom;
    PRINT_WDGTS		prntscr;
    ANNOTATE_WDGTS	annotate;
    SIGNAL_WDGTS	signal;
    EXAMINE_WDGTS	examine;
    GOTOS_WDGTS		gotos;
    VALUE_WDGTS		value;
    GRIDS_WDGTS		gridscus;
    SELECT_WDGTS	select;
    FILE_WDGTS		fileselect;
    Widget		shell;
    Widget		main;
    Widget		work;
    Widget		hscroll;
    Widget		vscroll;
    Widget		customize;	/* Customization widget */
    Widget		toplevel;	/* Top level shell */
    Widget		prompt_popup;	/* Data popup widget */
    int			prompt_type;	/* Type of data popup widget */
    Widget		message;	/* Message (error/warn/etc) widget */

    char		filename[MAXFNAMELEN];	/* Current file */
    char		printname[MAXFNAMELEN];	/* Print filename */
    struct stat		filestat;	/* Information on the current file */
    int			fileformat;	/* Type of trace file (see FF_*) */
    Boolean		loaded;		/* True if the filename is loaded in */
    char		vector_seperator;	/* Bus seperator character, usually "<" */
    char		vector_endseperator;	/* Bus ending seperator character, usually ">" */

    int			redraw_needed;	/* Need to refresh the screen when get a chance, 0=NO, 1=YES, 2=Expose Only */
    Position		width;		/* Screen width */
    Position		height;		/* Screen height */
    Position		ystart;		/* Start Y pos of signals on display (dispmgr) */
    Position		sighgt;		/* Height of signals (customize) */
    int			sigrf;		/* Signal rise/fall time spec */
    int			busrep;		/* Bus representation = IBUS/BBUS/OBUS/HBUS */
    TimeRep		timerep;	/* Time representation = TIMEREP_NS/TIMEREP_CYC */

    GRID		grid[MAXGRIDS];	/* Grid information */
    Boolean		cursor_vis;	/* True if cursors are visible */

    DTime		start_time;	/* Time of beginning of trace */
    DTime		end_time;	/* Time of ending of trace */

    SIGNALSTATE		*signalstate_head;	/* Head of signal state information */
    } TRACE;

/* Global information */
typedef struct {
    TRACE		*trace_head;	/* Pointer to first trace */
    TRACE		*preserved_trace;	/* Pointer to old trace when reading in new one */

    SIGNAL	 	*delsig;       	/* Linked list of deleted signals */
    SIGNAL		*selected_sig;	/* Selected signal to move or add */
    TRACE		*selected_trace; /* Selected signal's trace */
    SIGNAL_LIST		*select_head;	/* Pointer to selected signal list head */

    CURSOR		*cursor_head;	/* Pointer to first cursor */

    VALSEARCH		val_srch[MAX_SRCH];	/* Value search information */

    SIGSEARCH		sig_srch[MAX_SRCH];	/* Signal search information */

    XtAppContext	appcontext;	/* X App context */
    Display		*display;	/* X display pointer */
    Cursor		xcursors[13];	/* X cursors */
    Pixmap		dpm,bdpm;	/* X pixmaps for the icons */

    int			argc;		/* Program argc for X stuff */
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

    GEOMETRY		start_geometry;	/* Geometry to open first trace with */
    GEOMETRY		open_geometry;	/* Geometry to open later traces with */
    GEOMETRY		shrink_geometry; /* Geometry to shrink trace->open traces with */
    Boolean		suppress_config;/* Don't read in any group, user, or directory config files */

    Boolean		anno_poppedup;	/* Annotation has been poped up on some window */
    Boolean		anno_ena_signal[MAX_SRCH+1];   /* Annotation signal enables */
    Boolean		anno_ena_cursor[MAX_SRCH+1];    /* Annotation cursor enables */
    Boolean		anno_ena_cursor_dotted[MAX_SRCH+1];    /* Annotation cursor enables */
    char		anno_filename[MAXFNAMELEN]; /* Annotation file name */
    char		anno_socket[MAXFNAMELEN];	/* Annotation socket number */

    int			pageinc;	/* Page increment = HPAGE/QPAGE/FPAGE */
    Boolean		click_to_edge;	/* True if clicking to edges is enabled */
    TimeRep		time_precision;	/* Time precision = TIMEREP_NS/TIMEREP_CYC */
    char		time_format[12]; /* Time format = printf format or *NULL */
    int			tempest_time_mult;	/* Time multiplier for tempest */
    Boolean		save_enables;	/* True to save enable wires */
    Boolean		save_ordering;	/* True to save signal ordering */

    char		printnote[MAXFNAMELEN];	/* Note to print */
    PrintSize		print_size;	/* Size of paper for dt_printscreen */
    Boolean		print_all_signals; /* Print all signals in the trace */
    DTime		print_begin_time;  /* Starting time for printing */
    DTime		print_end_time;	/* Ending time for printing */

    int			redraw_needed;	/* Some trace needs to refresh the screen when get a chance, 0=NO, 1=YES, 2=Do All */
    Boolean		redraw_manually;/* True if in manual refreshing mode */

    DTime		time;		/* Time of trace at left edge of screen */
    float		res;		/* Resolution of graph width (gadgets) */
    Boolean		res_default;	/* True if resolution has never changed from initial value */
    Position		xstart;		/* Start X pos of signals on display (read_DECSIM) */
    DTime		click_time;	/* time clicked on for res_zoom_click */
    GRID		*click_grid;	/* grid being set by grid_align */
    } GLOBAL;

extern GLOBAL *global;

