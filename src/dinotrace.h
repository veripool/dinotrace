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

#define DTVERSION	"Dinotrace V6.4"
#define EXPIRATION	(60*60*24*30)	/* In seconds - Comment out define for no expiration dates */
#undef	EXPIRATION

#pragma member_alignment

#define MAXSIGLEN	128	/* Maximum length of signal names */
#define MAXFNAMELEN	128	/* Maximum length of file names */
#define MAXSTATENAMES	96	/* Maximum number of state name translations */
#define MAXSTATELEN	32	/* Maximum length of state names */
#define MAX_SRCH	9	/* Maximum number of search values */
#define MAX_SIG		512	/* Maximum number of signals */
#define MAX_CURSORS	64	/* Maximum number of cursors */
#define	MIN_GRID_RES	512.0	/* Minimum grid resolution */
#define BLK_SIZE	512	/* Trace data block size */

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

#define SIG_SPACE	5
#define SIG_RF		2
#define DELU	 5
#define DELU2	10

#define PS_START_Y	600	/* Postscript y starting position */
#define XMARGIN	 5

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
#define TIMEREP_NS	0	/* Must be zero */
#define TIMEREP_CYC	1

#define IO_GRIDRES	1
#define IO_GRIDALIGN	2
#define IO_RES		3
#define IO_READCUSTOM	4
#define IO_SAVECUSTOM	5
#define IO_TIME		6

#define XSTART_MIN	50		/* Min Start X pos of signals on display (read_DECSIM) */
#define XSTART_MARGIN	10		/* Additional added fudge factor for xstart */

#define DIALOG_WIDTH	75
#define DIALOG_HEIGHT	50


/* Utilities */
#ifndef MAX
#define MAX(_a_,_b_) ( ( ( _a_ ) > ( _b_ ) ) ? ( _a_ ) : ( _b_ ) )
#endif
#ifndef MIN
#define MIN(_a_,_b_) ( ( ( _a_ ) < ( _b_ ) ) ? ( _a_ ) : ( _b_ ) )
#endif


extern int	DTDEBUG,		/* Debugging mode */
		DTPRINT;		/* Information printing mode */

#define	FF_DECSIM	1
#define	FF_TEMPEST	2
#define	FF_VERILOG	3
extern int		file_format;	/* Type of trace to support */

extern char		message[100];		/* generic string for messages */

extern XGCValues	xgcv;

extern Arg		arglist[20];

extern int		line_num;
extern char		*current_file;

/* Grid Automatic flags */
#define GRID_AUTO_ASS	-2
#define GRID_AUTO_DEASS	-1

typedef struct {
    Widget	menu;
    Widget	pdmenu[11];
    Widget	pdmenubutton[11];
    Widget	pdentry[20];
    Widget	pdentrybutton[63];
    Widget	pdsubbutton[2+MAX_SRCH*4];
    int		sig_highlight_pds;
    int		cur_highlight_pds;
    int		cur_add_pds;
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
    } COMMAND_WDGTS;

typedef struct {
    Widget customize;
    Widget page_label;
    Widget rpage;
    Widget tpage1;
    Widget tpage2;
    Widget tpage3;
    Widget bus_label;
    Widget rbus;
    Widget tbus1;
    Widget tbus2;
    Widget tbus3;
    Widget tbus4;
    Widget time_label;
    Widget rtime;
    Widget ttimens;
    Widget ttimecyc;
    Widget sighgt_label;
    Widget s1;
    Widget rfwid;
    Widget cursor_state;
    Widget grid_state;
    Widget grid_width;
    Widget grid_label;
    Widget b1;
    Widget b2;
    Widget b3;
    Widget format_label, format_radio, format_decsim, format_tempest, format_verilog;
    } CUSTOM_DATA;

typedef struct {
    Widget customize;
    Widget rsize;
    Widget rsizea;
    Widget rsizeb;
    Widget text;
    Widget label;
    Widget pagelabel;
    Widget s1;
    Widget b1;
    Widget b2;
    Widget b3;
    } PRINT_WDGTS;

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
    Widget popup;
    Widget label;
    } EXAMINE_WDGTS;

typedef struct {
    Widget popup;
    Widget text;
    Widget ok;
    Widget label1;
    Widget cancel;
    Widget pulldown;
    Widget pulldownbutton[MAX_SRCH+1];
    Widget options;
    } GOTOS_WDGTS;

/* 5.0: Structure for each signal-state assignment */
typedef struct st_signalstate {
    struct st_signalstate *next;	/* Next structure in a linked list */
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
    int			color;		/* Color number (index into trace->xcolornum) 0=OFF*/
    int			cursor;		/* Enable cursors, color or 0=OFF */
    int			value[3];	/* Value to search for, (96 bit LW format) */
    } VALSEARCH;

/* Signal searching structure */
typedef struct st_sigsearch {
    int			color;		/* Color number (index into trace->xcolornum) 0=OFF*/
    int			string[MAXSIGLEN];	/* Signal to search for */
    } SIGSEARCH;

/* Cursor information structure (one per cursor) */
typedef struct st_cursor {
    struct st_cursor	*next;		/* Forward link to next cursor */
    struct st_cursor	*prev;		/* Backward link to previous cursor */

    int			time;		/* Time cursor is placed at */
    int			color;		/* Color number (index into trace->xcolornum) */

    int			search;		/* Number of search cursor is for, 0 = manual */
    } CURSOR;

/* Signal information structure (one per each signal in a trace window) */
typedef struct st_signal {
    struct st_signal	*forward;	/* Forward link to next signal */
    struct st_signal	*backward;	/* Backward link to previous signal */

    struct st_signal	*copyof;	/* Link to signal this is copy of (or NULL) */
    struct st_trace	*trace;		/* Trace signal belongs to (originally) */
    /* struct st_signal	*verilog_copyof; / * Copy of another verilog signal (which has own data) */
    struct st_signal	*verilog_next;	/* Next verilog signal with same coding */

    char		*signame;	/* Signal name */
    XmString		xsigname;	/* Signal name as XmString */
    int			color;		/* Signal line's Color number (index into trace->xcolornum) */
    int			search;		/* Number of search color is for, 0 = manual */

    int			srch_ena;	/* Searching is enabled */

    int			type;		/* Type of signal, STATE_B32, _B64, etc */
    SIGNALSTATE		*decode;	/* Pointer to decode information, NULL if none */
    int			lws;		/* Number of LWs in a SIGNAL_LW record */
    int			blocks;		/* Number of time data blocks */
    int			msb_index;	/* Bit subscript of first index in a signal (<20:10> == 20), -1=none */
    int			lsb_index;	/* Bit subscript of last index in a signal (<20:10> == 10), -1=none */
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
	    } flag;
	int		flags;
	}		file_type;	/* File specific type of trace, two/fourstate, etc */

    SIGNAL_LW		*bptr;		/* begin of time data ptr */
    SIGNAL_LW		*cptr;		/* current time data ptr */

    unsigned int	value_mask[3];	/* Value Mask with 1s in bits that are to be set */
    unsigned int	pos_mask;	/* Mask to translate file positions */
    VALUE		file_value;	/* current state/time LW information for reading in */
    } SIGNAL;

/* Trace information structure (one per window) */
typedef struct st_trace {
    struct st_trace	*next_trace;	/* Pointer to the next trace display */

    SIGNAL		*firstsig;	/* Linked list of all nondeleted signals */
    SIGNAL		*dispsig;	/* Pointer within sigque to first signal on screen */

    int			numsig;		/* Total number of signals, excluding deleted */
    int			numsigvis;	/* Number of signals visible on the screen */
    int			numsigstart;	/* signal to start displaying */

    Window		wind;		/* X window */
    Pixel		xcolornums[11];	/* X color numbers (pixels) for normal/highlight */
    XFontStruct		*text_font;	/* Display's Text font */
    GC                  gc;
    GC                  hscroll_gc;

    Widget		shell;
    Widget		main;
    MENU_WDGTS		menu;
    Widget		menu_close;	/* Pointer to menu_close widget */
    Widget		work;
    Widget		hscroll;
    Widget		vscroll;
    COMMAND_WDGTS	command;
    CUSTOM_DATA		custom;
    PRINT_WDGTS		prntscr;
    SIGNAL_WDGTS	signal;
    EXAMINE_WDGTS	examine;
    GOTOS_WDGTS		gotos;
    VALUE_WDGTS		value;
    Widget		customize;	/* Customization widget */
    Widget		fileselect;	/* File selection widget */
    Widget		toplevel;	/* Top level shell */
    Widget		prompt_popup;	/* Data popup widget */
    int			prompt_type;	/* Type of data popup widget */

    char		filename[200];	/* Current file */
    int			loaded;		/* True if the filename is loaded in */
    char		vector_seperator;	/* Seperator character, usually "<" */

    int			width;		/* Screen width */
    int			height;		/* Screen height */
    int			ystart;		/* Start Y pos of signals on display (dispmgr) */
    int			sighgt;		/* Height of signals (customize) */
    int			sigrf;		/* Signal rise/fall time spec */
    int			pageinc;	/* Page increment = HPAGE/QPAGE/FPAGE */
    int			busrep;		/* Bus representation = IBUS/BBUS/OBUS/HBUS */
    int			timerep;	/* Time representation = TIMEREP_NS/TIMEREP_CYC */

    int			grid_res;	/* Grid resolution (time between ticks) */
    int			grid_align;	/* Grid alignment (time grid starts at) */
    int			grid_vis;	/* True if grid is visible */
    int			grid_res_auto;	/* Number or status of automatic grid resolution */
    int			grid_align_auto; /* Number or status of automatic grid alignment */
    int			cursor_vis;	/* True if cursors are visible */

    int			numpag;		/* Number of pages in dt_printscreen */

    int			start_time;	/* Time of beginning of trace */
    int			end_time;	/* Time of ending of trace */

    SIGNALSTATE		*signalstate_head;	/* Head of signal state information */
    } TRACE;

/* Global information */
typedef struct {
    TRACE		*trace_head;	/* Pointer to first trace */

    SIGNAL	 	*delsig;       	/* Linked list of deleted signals */
    SIGNAL		*selected_sig;	/* Selected signal to move or add */
    TRACE		*selected_trace; /* Selected signal's trace */

    CURSOR		*cursor_head;	/* Pointer to first cursor */

    VALSEARCH		val_srch[MAX_SRCH];	/* Value search information */

    SIGSEARCH		sig_srch[MAX_SRCH];	/* Signal search information */

    XtAppContext	appcontext;	/* X App context */
    Display		*display;	/* X display pointer */
    Cursor		xcursors[12];	/* X cursors */
    Pixmap		dpm,bdpm;	/* X pixmaps for the icons */

    int			argc;		/* Program argc for X stuff */
    char		**argv;		/* Program argv for X stuff */

    char		directory[200];	/* Current directory name */

    int			highlight_color; /* Color selected for sig/cursor highlight */
    int			goto_color;	/* Cursor color to place on a 'GOTO' -1=none */

    int			bsized;		/* True if b-sized printing in dt_printscreen */

    int			time;		/* Time of trace at left edge of screen */
    float		res;		/* Resolution of graph width (gadgets) */
    int			xstart;		/* Start X pos of signals on display (read_DECSIM) */

    int			click_time;	/* time clicked on for res_zoom_click */
    } GLOBAL;

extern GLOBAL *global;


