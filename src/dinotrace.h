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
 *
 */

#define DTVERSION	"Dinotrace V5.0"

#define MAXSIGLEN 128		/* Maximum length of signal names */
#define MAXFNAMELEN 128		/* Maximum length of file names */
#define MAXSTATENAMES 64	/* Maximum number of state name translations */
#define MAXSTATELEN  32		/* Maximum number of state name translations */
#define MAX_SIG  512
#define MAX_CURSORS  64
#define	MIN_GRID_RES 512.0
#define BLK_SIZE 512
#define CHECK      0
#define NOCHECK    1
#define NOCHECKEND 2

/* All of the states a signal can be in (have only 3 bits so 0-7) */
#define STATE_0   0
#define STATE_1   1
#define STATE_U   2
#define STATE_Z   3
#define STATE_B32 4
#define STATE_B64 5
#define STATE_B96 6

/* Grid Automatic flags */
#define GRID_AUTO_ASS	-2
#define GRID_AUTO_DEASS	-1

#define PS_START_Y	600

#define EOT		0x1FFFFFFF

#define SIG_SPACE	5
#define SIG_RF		2
#define DELU	 5
#define DELU2	10

#define XMARGIN	 5

#define QPAGE 4
#define HPAGE 2
#define FPAGE 1

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

#define MAX(_a_,_b_) ( ( _a_ > _b_ ) ? _a_ : _b_ )
#define MIN(_a_,_b_) ( ( _a_ < _b_ ) ? _a_ : _b_ )

int		DTDEBUG=FALSE,
		DTPRINT=FALSE;

#define	DECSIM		1
#define	HLO_TEMPEST	2
int		trace_format=DECSIM;

int		x1,y1,x2,y2;

int		screen_num=0;	/* to specify screen other than default */

char		message[100];	/* generic string for messages */

GC		gc;
XGCValues	xgcv;

Arg		arglist[10];
DwtCallback	cb_arglist[10];

Widget	toplevel,main_wid,custom_wid,main_menu_wid,
	main_menu_pd1_wid,main_menu_pd2_wid,
	main_menu_pde1_wid,main_menu_pde2_wid,
	main_pd1_wid,main_pd2_wid;

Pixmap  dpm,bdpm,left_arrow,right_arrow;

/* 5.0: Structure for each signal-state assignment */
typedef struct struct_signalstate {
    struct struct_signalstate *next;	/* Next structure in a linked list */
    char signame[MAXSIGLEN];		/* Signal name to translate, Nil = wildcard */
    char statename[MAXSTATENAMES][MAXSTATELEN];	/* Name for each state, nil=keep */
    };
typedef struct struct_signalstate SIGNALSTATE;

typedef struct {
    unsigned int state:3;
    unsigned int time:29;
    } SIGNAL_LW;

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

typedef struct {
    int			*forward;
    int	 		*backward;
    char		*signame;
    int			type;
    SIGNALSTATE		*decode;
    int			inc;
    int			ind_s,ind_e;
    int			blocks;
    GC			gc;
    int			*bptr;    /* begin ptr */
    int			*cptr;    /* current ptr */
} SIGNAL_SB;

typedef struct {
    int			*forward;
    int			*backward;
} QUELNK;

typedef struct {
    QUELNK		sig;
    QUELNK	 	del;
    SIGNAL_SB		*deleted;
    Display		*disp;
    Window		wind;
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
    Widget		customize;
    Widget		fileselect;		/* File selection widget */
    char		filename[200];
    int			*startsig;		/* ptr to SIGNAL_SB */
    int			*delsig;		/* ptr to SIGNAL_SB */
    float		res;		/* Resolution of graph width */
    int			width;
    int			height;
    int			inc;
    int			numsig;
    int			numsigvis;
    int			numsigdel;
    int			xstart;		/* Start X pos of signals on display (read_DECSIM) */
    int			ystart;		/* Start Y pos of signals on display (dispmgr) */
    int			sighgt;		/* Height of signals (customize) */
    int			sigrf;
    int			sigstart;	/* signal to start displaying */
    int			pageinc;
    int			busrep;
    int			cursor_vis;
    int			grid_vis;
    int			grid_res_auto;
    int			grid_res;
    int			grid_align_auto;
    int			grid_align;
    SIGNALNAMES		*signame;
    SIGNAL_LW		*(*bptr)[];    /* begin ptr */
    SIGNAL_LW		*(*cptr)[];    /* current ptr */
    int			time;                 
    int			start_time;
    int			end_time;
    short int		*bus;
    int			separator;
    GC                  gc;
    GC                  highlite_gc;
    int			numcursors;
    int			cursors[MAX_CURSORS];
    XFontStruct		*text_font;
    int			numpag;

    SIGNALSTATE		*signalstate_head;
    } DISPLAY_SB;

DISPLAY_SB	*ptr;

typedef struct {
    DISPLAY_SB *ptr;
    unsigned int value;
} DINOCB;

DISPLAY_SB *curptr;

XPoint left[7]={5,7,
		17,7,
		17,2,
		25,12,
		17,22,
		17,17,
		5,17};
XPoint right[7]={5,7,
		17,7,
		17,2,
		25,12,
		17,22,
		17,17,
		5,17};


