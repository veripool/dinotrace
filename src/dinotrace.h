/******************************************************************************
 *
 * Filename:
 *	Dinotrace.h
 *
 * Subsystem:
 *     Dinotrace
 *
 * Version:
 *     Dinotrace V4.0
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

#define DTVERSION	"Dinotrace V5.3"	/* also change in dinopost.h */

#define MAXSIGLEN	128	/* Maximum length of signal names */
#define MAXFNAMELEN	128	/* Maximum length of file names */
#define MAXSTATENAMES	96	/* Maximum number of state name translations */
#define MAXSTATELEN	32	/* Maximum length of state names */
#define MAX_SIG		512	/* Maximum number of signals */
#define MAX_CURSORS	64	/* Maximum number of cursors */
#define	MIN_GRID_RES	512.0	/* Minimum grid resolution */
#define BLK_SIZE	512	/* Trace data block size */

/* Cursors for various actions - index into xcursor array */
#define DC_NORMAL	0	/* XC_top_left_arrow	Normal cursor */
#define DC_BUSY		1	/* XC_watch		Busy cursor */
#define DC_SIG_ADD	2	/* XC_sb_left_arrow	Signal Add */
#define DC_SIG_MOVE_2	2	/* XC_sb_left_arrow	Signal Move (place) */
#define DC_SIG_MOVE_1	3	/* XC_sb_right_arrow	Signal Move (pick) */
#define DC_SIG_DELETE	4	/* XC_hand1		Signal Delete */
#define DC_CUR_ADD	5	/* XC_center_ptr	Cursor Add */
#define DC_CUR_MOVE	6	/* XC_sb_h_double_arrow	Cursor Move (drag) */
#define DC_CUR_DELETE	7	/* XC_X_cursor		Cursor Delete */
#define DC_ZOOM_1	8	/* XC_left_side		Zoom point 1 */
#define DC_ZOOM_2	9	/* XC_right_side	Zoom point 2 */
#define DC_SIG_HIGHLIGHT 10	/* XC_spraycan		Signal Highlight */

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

/* Encodings for dt_file cvd2d */
#define CHECK      0
#define NOCHECK    1
#define NOCHECKEND 2


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

#define IO_GRIDRES	1
#define IO_GRIDALIGN	2
#define IO_RES		3
#define IO_READCUSTOM	4
#define IO_SAVECUSTOM	5

#define XSTART_MIN	50		/* Min Start X pos of signals on display (read_DECSIM) */
#define XSTART_MARGIN	10		/* Additional added fudge factor for xstart */

#define DIALOG_WIDTH	75
#define DIALOG_HEIGHT	50

#define MAX(_a_,_b_) ( ( ( _a_ ) > ( _b_ ) ) ? ( _a_ ) : ( _b_ ) )
#define MIN(_a_,_b_) ( ( ( _a_ ) < ( _b_ ) ) ? ( _a_ ) : ( _b_ ) )

int		DTDEBUG=FALSE,		/* Debugging mode */
		DTPRINT=FALSE;		/* Information printing mode */

#define	DECSIM		1
#define	HLO_TEMPEST	2
int		trace_format=DECSIM;	/* Type of trace to support */

int		x1,y1,x2,y2;

char		message[100];		/* generic string for messages */

GC		gc;
XGCValues	xgcv;

Arg		arglist[10];

Widget	toplevel,main_wid,custom_wid,main_menu_wid,
	main_menu_pd1_wid,main_menu_pd2_wid,
	main_menu_pde1_wid,main_menu_pde2_wid,
	main_pd1_wid,main_pd2_wid;

Pixmap  dpm,bdpm;

/* Grid Automatic flags */
#define GRID_AUTO_ASS	-2
#define GRID_AUTO_DEASS	-1

/* 5.0: Structure for each signal-state assignment */
typedef struct struct_signalstate {
    struct struct_signalstate *next;	/* Next structure in a linked list */
    char signame[MAXSIGLEN];		/* Signal name to translate, Nil = wildcard */
    char statename[MAXSTATENAMES][MAXSTATELEN];	/* Name for each state, nil=keep */
    };
typedef struct struct_signalstate SIGNALSTATE;

/* Signal LW: A structure for each transition of each signal, */
/* 32/64/96 state signals have an additional 1, 2, or 3 LWs after this */
typedef struct {
    unsigned int state:3;
    unsigned int time:29;
    } SIGNAL_LW;
#define EOT	0x1FFFFFFF	/* SIGNAL_LW End of time indicator */

typedef struct {
    char array[][MAXSIGLEN];
    } SIGNALNAMES;

typedef struct {
    Widget menu;
    Widget pulldownmenu[10];
    Widget pulldownentry[10];
    Widget pulldown[53];
    } MENU_WDGTS;

typedef struct {
    Widget command;
    Widget begin_but;
    Widget resdec_but;
    Widget reschg_but;
    Widget resinc_but;
    Widget resfull_but;
    Widget reszoom_but;
    Widget end_but;
    } COMMAND_WDGTS;

typedef struct {
    Widget customize;
    Widget rpage;
    Widget tpage1;
    Widget tpage2;
    Widget tpage3;
    Widget rbus;
    Widget tbus1;
    Widget tbus2;
    Widget tbus3;
    Widget tbus4;
    Widget s1;
    Widget rfwid;
    Widget cursor_state;
    Widget grid_state;
    Widget grid_width;
    Widget grid_label;
    Widget page_label;
    Widget bus_label;
    Widget sighgt_label;
    Widget b1;
    Widget b2;
    Widget b3;
    int			pageinc;
    int			busrep;
    int			sigrf;
    int			cursor_vis;
    int			grid_vis;
    int			sighgt;
    } CUSTOM_DATA;

typedef struct {
    Widget customize;
    Widget rpage;
    Widget tpage1;
    Widget tpage2;
    Widget tpage3;
    Widget rbus;
    Widget tbus1;
    Widget tbus2;
    Widget tbus3;
    Widget tbus4;
    Widget s1;
    Widget rfwid;
    Widget cursor_state;
    Widget text;
    Widget label;
    Widget pagelabel;
    Widget b1;
    Widget b2;
    Widget b3;
    int			pageinc;
    int			busrep;
    int			sigrf;
    int			cursor_vis;
    int			grid_vis;
    int			grid_res;
    int			grid_align;
    int			sighgt;
    } PS_DATA;

/* Signal information structure */
typedef struct {
    int			*forward;	/* Forward link to next signal */
    int	 		*backward;	/* Backward link to previous signal */
    char		*signame;	/* Signal name */
    XmString		xsigname;	/* Signal name as XmString */
    int			color;		/* Color number (index into trace->xcolornum) */
    int			type;
    SIGNALSTATE		*decode;	/* Pointer to decode information, NULL if none */
    int			inc;
    int			ind_e;		/* something in dt_file */
    int			blocks;		/* Number of time data blocks */
    int			bits;		/* Number of bits in a bus, 0=single */
    int			binary_type;	/* type of trace if binary, two/fourstate */
    int			binary_pos;	/* position of bits in binary trace */
    int			*bptr;		/* begin of time data ptr */
    int			*cptr;		/* current time data ptr */
    } SIGNAL_SB;

typedef struct {
    SIGNAL_SB		*firstsig;	/* Linked list of all nondeleted signals */
    SIGNAL_SB	 	*delsig;       	/* Linked list of deleted signals */
    SIGNAL_SB		*dispsig;	/* Pointer within sigque to first signal on screen */

    int			numsig;		/* Total number of signals, including deleted */
    int			numsigvis;	/* Number of signals visible on the screen */
    int			numsigdel;	/* Number of signals deleted */
    int			numsigstart;	/* signal to start displaying */

    Display		*display;	/* X display pointer */
    Window		wind;		/* X window */
    Cursor		xcursors[11];	/* X cursors */
    Pixel		xcolornums[2];	/* X color numbers (pixels) for normal/highlight */

    Widget		shell;
    Widget		main;
    MENU_WDGTS		menu;
    Widget		work;
    Widget		hscroll;
    Widget		vscroll;
    COMMAND_WDGTS	command;
    CUSTOM_DATA		custom;
    PS_DATA		prntscr;
    PS_DATA		signal;
    Widget		customize;	/* Customization widget */
    Widget		fileselect;	/* File selection widget */
    Widget		prompt_popup;	/* Data popup widget */
    int			prompt_type;	/* Type of data popup widget */

    char		filename[200];	/* Current file */
    int			loaded;		/* True if the filename is loaded in */

    float		res;		/* Resolution of graph width */
    int			width;		/* Screen width */
    int			height;		/* Screen height */
    int			xstart;		/* Start X pos of signals on display (read_DECSIM) */
    int			ystart;		/* Start Y pos of signals on display (dispmgr) */
    int			sighgt;		/* Height of signals (customize) */
    int			sigrf;		/* Signal rise/fall time spec */
    int			pageinc;	/* Page increment = HPAGE/QPAGE/FPAGE */
    int			busrep;		/* Bus representation = IBUS/BBUS/OBUS/HBUS */

    int			grid_vis;	/* True if grid is visible */
    int			grid_res_auto;	/* Number or status of automatic grid resolution */
    int			grid_res;	/* Grid resolution (time between ticks) */
    int			grid_align_auto; /* Number or status of automatic grid alignment */
    int			grid_align;	/* Grid alignment (time grid starts at) */

    SIGNALNAMES		*signame;
    SIGNAL_LW		*(*bptr)[];	/* Signal information for beginning of trace */
    SIGNAL_LW		*(*cptr)[];	/* Signal information for current time */
    int			time;		/* Time of trace at left edge of screen */
    int			start_time;	/* Time of beginning of trace */
    int			end_time;	/* Time of ending of trace */
    int			click_time;	/* time clicked on for res_zoom_click */
    short int		*bus;
    GC                  gc;

    int			select_moved;	/* Selection has been made */
    SIGNAL_SB		*selected_sig_ptr;	/* Selected signal to move or add */

    int			cursor_vis;	/* True if cursors are visible */
    int			numcursors;	/* Number of cursors */
    int			cursors[MAX_CURSORS];	/* Time of each cursor */

    XFontStruct		*text_font;	/* Display's Text font */
    int			numpag;		/* Number of pages in dt_printscreen */

    SIGNALSTATE		*signalstate_head;	/* Head of signal state information */
    } TRACE;

TRACE *curptr;


