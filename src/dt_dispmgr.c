/******************************************************************************
 *
 * Filename:
 *     Dispmgr.c
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
 *     This module contains the routines that manage all display functions
 *     (creating, destroying, circulating) for Dinotrace including the
 *     display callbacks.
 *
 * Modification History:
 *     AAG	 5-Jul-89	Original Version
 *     AAG	22-Aug-90	Base Level V4.1
 *     AAG	20-Nov-90	Added code to support siz, pos, and res options
 *     AAG	29-Apr-91	Use X11 for Ultrix support
 *     WPS	03-Jan-93	Default grid_res of 40, sighgt 20 for our project
 *				Additional gadget support, title in icon
 */
static char rcsid[] = "$Id$";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>

#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/ArrowB.h>
#include <Xm/DrawingA.h>
#include <Xm/ScrollBar.h>
#include <Xm/RowColumn.h>
#include <Xm/CascadeB.h>
#include <Xm/MainW.h>

#include "dinotrace.h"
#include "callbacks.h"
#include "dino.bit"
#include "bigdino.bit"

/* Application Resources */
/* Colors:
   Black, White, Aquamarine, Blue, BlueViolet, Brown, CadetBlue, Coral,
   CornflowerBlue, Cyan, DarkGreen, DarkOliveGreen, DarkOrchid, DarkSlateBlue,
   DarkSlateGrey, DarkTurquoise, Firebrick, ForestGreen, Gold, Goldenrod, Green,
   GreenYellow, IndianRed, Khaki, LightBlue, LightGrey, LightSteelBlue, LimeGreen,
   Magenta, Maroon, MediumAquamarine, MediumForestGreen, MediumGoldenrod,
   MediumOrchid, MediumSeaGreen, MediumSlateBlue, MediumSpringGreen,
   MediumTurquoise, MidnightBlue, NavyBlue, Orange, OrangeRed, Orchid, PaleGreen,
   Pink, Plum, Red, Salmon, SeaGreen, Sienna, SkyBlue, SlateBlue, SpringGreen,
   SteelBlue, Tan, Thistle, Turquoise, Violet, VioletRed, Wheat, Yellow,
   YellowGreen */

#define Offset(field) XtOffsetOf(GLOBAL, field)
XtResource resources[] = {
    {"barcolor", "Barcolor", XtRString, sizeof(String), Offset(barcolor_name), XtRImmediate, (XtPointer) NULL},
    {"color1", "Color1", XtRString, sizeof(String), Offset(color_names[1]), XtRImmediate, (XtPointer) "White"},
    {"color2", "Color2", XtRString, sizeof(String), Offset(color_names[2]), XtRImmediate, (XtPointer) "Red"},
    {"color3", "Color3", XtRString, sizeof(String), Offset(color_names[3]), XtRImmediate, (XtPointer) "ForestGreen"},
    {"color4", "Color4", XtRString, sizeof(String), Offset(color_names[4]), XtRImmediate, (XtPointer) "Blue"},
    {"color5", "Color5", XtRString, sizeof(String), Offset(color_names[5]), XtRImmediate, (XtPointer) "Magenta"},
    {"color6", "Color6", XtRString, sizeof(String), Offset(color_names[6]), XtRImmediate, (XtPointer) "Cyan"},
    {"color7", "Color7", XtRString, sizeof(String), Offset(color_names[7]), XtRImmediate, (XtPointer) "Yellow"},
    {"color8", "Color8", XtRString, sizeof(String), Offset(color_names[8]), XtRImmediate, (XtPointer) "Salmon"},
    {"color9", "Color9", XtRString, sizeof(String), Offset(color_names[9]), XtRImmediate, (XtPointer) "NavyBlue"},
    {"signalfont", "SignalFont", XtRString, sizeof(String), Offset(signal_font_name), XtRImmediate, (XtPointer) "-*-Fixed-Medium-R-Normal--*-120-*-*-*-*-*-1"},
    {"timefont",   "TimeFont",   XtRString, sizeof(String), Offset(time_font_name),   XtRImmediate, (XtPointer) "-*-Courier-Medium-R-Normal--*-120-*-*-*-*-*-1"},
    {"valuefont",  "ValueFont",  XtRString, sizeof(String), Offset(value_font_name),  XtRImmediate, (XtPointer) "-*-Fixed-Medium-R-Normal--*-100-*-*-*-*-*-1"}
    };
#undef Offset

void debug_event_cb (w,trace,cb)
    Widget			w;
    TRACE			*trace;
    XmDrawingAreaCallbackStruct	*cb;
{
    printf ("DEBUG_EVENT_CB %d %s\n",w, ""/*(events[cb->event->type]*/);
    }

extern void    val_examine_popup_act ();
extern void    val_examine_unpopup_act ();
static XtActionsRec actions[] = {
    {"value_examine_popup", val_examine_popup_act},
    {"value_examine_unpopup", val_examine_unpopup_act}
    };

char *translations = "<Btn2Down> : value_examine_popup()\n";
/*<Key>F10: hscroll.PageDownOrRight(1)*/


static int last_set_cursor_num = DC_NORMAL;
int  last_set_cursor ()
{return (last_set_cursor_num);}

void set_cursor (trace, cursor_num)
    TRACE	*trace;			/* Display information */
    int		cursor_num;		/* Entry in xcursors to display */
{
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	XDefineCursor (global->display, trace->wind, global->xcursors[cursor_num]);
	}
    XmSetMenuCursor (global->display, global->xcursors[cursor_num]);
    last_set_cursor_num = cursor_num;
    }

/* Make the close menu options on all of the menus be active */
void set_menu_closes ()
{
    TRACE	*trace;
    int		sensitive;

    sensitive = (global->trace_head && global->trace_head->next_trace)?TRUE:FALSE;

    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	XtSetArg (arglist[0], XmNsensitive, sensitive);
	XtSetValues (trace->menu_close, arglist, 1);
	}
    }

/* Split the current trace, return new trace */
TRACE *trace_create_split_window (trace)
    TRACE		*trace;
{
    Position x,y,width,height;
    Position new_x,new_y,new_width,new_height;
    TRACE	*trace_new;

    if (DTPRINT_ENTRY) printf ("In trace_open_split_window - trace=%d\n",trace);

    /* Get orignal sizes */
    XtSetArg (arglist[0], XmNheight,&height);
    XtSetArg (arglist[1], XmNwidth, &width);
    XtSetArg (arglist[2], XmNx, &x);
    XtSetArg (arglist[3], XmNy, &y);
    XtGetValues (trace->toplevel, arglist, 4);
    if (DTPRINT_DISPLAY) printf ("Old size: %dx%d+%d+%d\n", width, height, x, y);

    /* Shrink this window */
    new_x = x; new_y = y; new_width=width; new_height=height;
    if (global->shrink_geometry.xp)	new_x = x + (( width * global->shrink_geometry.x ) / 100);
    if (global->shrink_geometry.yp) 	new_y = y + (( height * global->shrink_geometry.y ) / 100);
    if (global->shrink_geometry.widthp) new_width = ( width * global->shrink_geometry.width ) / 100;
    if (global->shrink_geometry.heightp) new_height = ( height * global->shrink_geometry.height ) / 100;
    if (DTPRINT_DISPLAY) printf ("Shrink size: %dx%d+%d+%d\n", new_width, new_height, new_x, new_y);
    XtSetArg (arglist[0], XmNheight, new_height);
    XtSetArg (arglist[1], XmNwidth, new_width);
    XtSetArg (arglist[2], XmNx, new_x);
    XtSetArg (arglist[3], XmNy, new_y);
    XtSetValues (trace->toplevel, arglist, 4);

    /* Create new window */
    new_x = x; new_y = y; new_width=width; new_height=height;
    if (global->open_geometry.xp)	new_x = x + (( width * global->open_geometry.x ) / 100);
    else	new_x = global->open_geometry.x;
    if (global->open_geometry.yp) 	new_y = y + (( height * global->open_geometry.y ) / 100);
    else	new_y = global->open_geometry.y;
    if (global->open_geometry.widthp)	new_width = ( width * global->open_geometry.width ) / 100;
    else	new_width = global->open_geometry.width;
    if (global->open_geometry.heightp)	new_height = ( height * global->open_geometry.height ) / 100;
    else	new_height = global->open_geometry.height;
    if (DTPRINT_DISPLAY) printf ("New size: %dx%d+%d+%d\n", new_width, new_height, new_x, new_y);
    trace_new = create_trace (new_width, new_height, new_x, new_y);

    return (trace_new);
    }

void trace_open_cb (w,trace,cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    TRACE	*trace_new;

    trace_new = trace_create_split_window (trace);

    /* Ask for a file in the new window */
    XSync (global->display,0);
    trace_read_cb (NULL, trace_new);
    }

void trace_close_cb (w,trace,cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    TRACE	*trace_ptr;

    if (DTPRINT_ENTRY) printf ("In trace_close - trace=%d\n",trace);

    assert (trace!=global->deleted_trace_head);

    /* clear the screen */
    XClearWindow (global->display, trace->wind);
    /* Nail the child */
    /* XtUnmanageChild (trace->toplevel); */
    /* free memory associated with the data */
    free_data (trace);
    /* destroy all the widgets created for the screen */
    XtDestroyWidget (trace->toplevel);

    /* relink pointers to ignore this trace */
    if (trace == global->trace_head)
	global->trace_head = trace->next_trace;
    for (trace_ptr = global->deleted_trace_head; trace_ptr; trace_ptr = trace_ptr->next_trace) {
	if (trace_ptr->next_trace == trace) trace_ptr->next_trace = trace->next_trace;
	}

    /* free the display structure */
    DFree (trace);

    /* Update menus */
    set_menu_closes ();
    }

void trace_clear_cb (w,trace,cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    TRACE	*trace_ptr;
    TRACE	*trace_next;

    if (DTPRINT_ENTRY) printf ("In clear_trace - trace=%d\n",trace);

    /* nail all traces except for this window's */
    for (trace_ptr = global->trace_head; trace_ptr; ) {
	trace_next = trace_ptr->next_trace;
	if (trace_ptr != trace) {
	    trace_close_cb (w, trace_ptr, cb);
	    }
	trace_ptr = trace_next;
	}

    /* clear the screen */
    XClearWindow (global->display, trace->wind);

    /* free memory associated with the data */
    free_data (trace);

    /* change the name on title bar back to the trace */
    change_title (trace);
    }

void trace_exit_cb (w,trace,cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    TRACE		*trace_next;

    if (DTPRINT_ENTRY) printf ("In trace_exit_cb - trace=%d\n",trace);

    for (trace = global->trace_head; trace; ) {
	trace_next = trace->next_trace;
	trace_close_cb (w, trace, cb);
	trace = trace_next;
	}

    DFree (global);

    /* all done */
    exit (1);
    }

void init_globals ()
{
    int i;
    char *pchar;
    int		cfg_num;

    if (DTPRINT_ENTRY) printf ("in init_globals\n");

    global = DNewCalloc (GLOBAL);
    global->trace_head = NULL;
    global->directory[0] = '\0';
    global->res = RES_SCALE/ (float)(250);
    global->res_default = TRUE;

    global->preserved_trace = NULL;
    global->selected_sig = NULL;
    global->cursor_head = NULL;
    global->signalstate_head = NULL;
    global->xstart = 200;
    global->time = 0;
    global->time_precision = TIMEREP_NS;
    global->tempest_time_mult = 2;
    global->click_to_edge = 1;
    global->start_geometry.width = 800;
    global->start_geometry.height = 600; 
    global->start_geometry.x = 100;
    global->start_geometry.y = 100;
    global->open_geometry.width = 100;
    global->open_geometry.height = 50; 
    global->open_geometry.x = 0;
    global->open_geometry.y = 50;
    global->open_geometry.xp = global->open_geometry.yp = TRUE;
    global->open_geometry.heightp = global->open_geometry.widthp = TRUE;
    global->shrink_geometry.width = 100;
    global->shrink_geometry.height = 50; 
    global->shrink_geometry.x = 0;
    global->shrink_geometry.y = 0;
    global->shrink_geometry.xp = global->shrink_geometry.yp = TRUE;
    global->shrink_geometry.heightp = global->shrink_geometry.widthp = TRUE;

    global->goto_color = -1;

    /* Annotate stuff */
    global->anno_poppedup = FALSE;
    for (i=0; i <= MAX_SRCH; i++) {
	global->anno_ena_signal[i] = (i!=0);
	global->anno_ena_cursor[i] = (i!=0);
	global->anno_ena_cursor_dotted[i] = 0;
	}
#ifdef VMS
    strcpy (global->anno_filename, "sys$login:dinotrace.danno");
#else
    global->anno_filename[0] = '\0';
    if (NULL != (pchar = getenv ("HOME"))) strcpy (global->anno_filename, pchar);
    if (global->anno_filename[0]) strcat (global->anno_filename, "/");
    strcat (global->anno_filename, "dinotrace.danno");
#endif

    /* Search stuff */
    for (i=0; i<MAX_SRCH; i++) {
	/* Colors */
	global->color_names[i] = NULL;

	/* Value */
	memset ((char *)&global->val_srch[i], 0, sizeof (VALSEARCH));
	strcpy (global->val_srch[i].signal, "*");

	/* Signal */
	memset ((char *)&global->sig_srch[i], 0, sizeof (SIGSEARCH));
	}

    /* Config stuff */
    for (cfg_num=0; cfg_num<MAXCFGFILES; cfg_num++) {
	global->config_enable[cfg_num] = TRUE;
	global->config_filename[cfg_num][0] = '\0';
    }

#ifdef VMS
    strcpy (global->config_filename[0], "DINODISK:DINOTRACE.DINO");
    strcpy (global->config_filename[1], "DINOCONFIG:");
    strcpy (global->config_filename[2], "SYS$LOGIN:DINOTRACE.DINO");
#else
    global->config_filename[0][0] = '\0';
    if (NULL != (pchar = getenv ("DINODISK"))) strcpy (global->config_filename[0], pchar);
    if (global->config_filename[0][0]) strcat (global->config_filename[0], "/");
    strcat (global->config_filename[0], "dinotrace.dino");
	
    global->config_filename[1][0] = '\0';
    if (NULL != (pchar = getenv ("DINOCONFIG"))) strcpy (global->config_filename[1], pchar);
	
    global->config_filename[2][0] = '\0';
    if (NULL != (pchar = getenv ("HOME"))) strcpy (global->config_filename[2], pchar);
    if (global->config_filename[2][0]) strcat (global->config_filename[2], "/");
    strcat (global->config_filename[2], "dinotrace.dino");
#endif
    }

void create_globals (argc, argv, sync)
    int		argc;
    char	**argv;
    Boolean	sync;
{
    int		argc_copy;
    char	**argv_copy;
    char	display_name[512];

    /* alloc variables */
    global->argc = argc;
    global->argv = argv;

    XtToolkitInitialize ();

    global->appcontext = XtCreateApplicationContext ();

    /* Save parameters and open display */
    argc_copy = global->argc;
    argv_copy = (char **)XtMalloc (global->argc * sizeof (char *));
    memcpy (argv_copy, global->argv, global->argc * sizeof (char *));

    global->display = XtOpenDisplay (global->appcontext, NULL, NULL, "Dinotrace",
				    NULL, 0, &argc_copy, argv_copy);

    if (global->display==NULL) {
	display_name[0] = '\0';
	printf ("Can't open display '%s'\n", XDisplayName (display_name));
	exit (0);
	}

    XSynchronize (global->display, sync);
    if (DTPRINT_ENTRY) printf ("in create_globals, syncronization is %d\n", sync);

    /*
     * Editor's Note: Thanks go to Sally C. Barry, former employee of DEC,
     * for her painstaking effort in the creation of the infamous 'Dino'
     * bitmap icons.
     */
    
    /*** create small dino pixmap from data ***/
    global->dpm = make_icon (global->display, DefaultRootWindow (global->display),
			    dino_icon_bits,dino_icon_width,dino_icon_height);    
    
    /*** create big dino pixmap from data ***/
    global->bdpm = make_icon (global->display, DefaultRootWindow (global->display),
			     bigdino_icon_bits,bigdino_icon_width,bigdino_icon_height);    

    /* Define cursors */
    global->xcursors[0] = XCreateFontCursor (global->display, XC_top_left_arrow);
    global->xcursors[1] = XCreateFontCursor (global->display, XC_watch);
    global->xcursors[2] = XCreateFontCursor (global->display, XC_sb_left_arrow);
    global->xcursors[3] = XCreateFontCursor (global->display, XC_sb_right_arrow);
    global->xcursors[4] = XCreateFontCursor (global->display, XC_hand1);
    global->xcursors[5] = XCreateFontCursor (global->display, XC_center_ptr);
    global->xcursors[6] = XCreateFontCursor (global->display, XC_sb_h_double_arrow);
    global->xcursors[7] = XCreateFontCursor (global->display, XC_X_cursor);
    global->xcursors[8] = XCreateFontCursor (global->display, XC_left_side);
    global->xcursors[9] = XCreateFontCursor (global->display, XC_right_side);
    global->xcursors[10] = XCreateFontCursor (global->display, XC_spraycan);
    global->xcursors[11] = XCreateFontCursor (global->display, XC_question_arrow);
    global->xcursors[12] = XCreateFontCursor (global->display, XC_cross);

    config_global_defaults ();
    }

TRACE *malloc_trace ()
    /* Allocate a trace structure and return it */
    /* This should NOT do any windowing initialization */
{
    TRACE	*trace;
    
    /*** alloc space for trace to display state block ***/
    trace = DNewCalloc (TRACE);
    trace->next_trace = global->trace_head;
    global->trace_head = trace;
    if (global->deleted_trace_head) global->deleted_trace_head->next_trace = global->trace_head;

    /* Initialize Various Parameters */
    trace->firstsig = NULL;
    trace->dispsig = NULL;
    trace->custom.customize = NULL;
    trace->signal.add = NULL;
    trace->signal.search = NULL;
    trace->value.search = NULL;
    trace->prntscr.customize = NULL;
    trace->prompt_popup = NULL;
    trace->annotate.dialog = NULL;
    trace->fileselect.dialog = NULL;
    trace->filename[0] = '\0';
    trace->loaded = FALSE;
    trace->numsig = 0;
    trace->numsigvis = 0;
    trace->numsigstart = 0;
    trace->busrep = HBUS;
    trace->ystart = 40;

    return (trace);
    }
    

void dm_menu_title (TRACE *trace,
		    char *title,	
		    char key		/* Or '\0' for none */
		    )
    /*** create a pulldownmenu on the top bar ***/
{
    int arg=0;

    trace->menu.pde++;
    trace->menu.pdmenu[trace->menu.pde] = XmCreatePulldownMenu (trace->menu.menu,"",NULL,0);
    XtSetArg (arglist[arg], XmNlabelString, XmStringCreateSimple (title) );		arg++;
    XtSetArg (arglist[arg], XmNsubMenuId, trace->menu.pdmenu[trace->menu.pde] );	arg++;
    if (key !='\0') { XtSetArg (arglist[arg], XmNmnemonic, key );	arg++; }
    trace->menu.pdmenubutton[trace->menu.pde] = XmCreateCascadeButton (trace->menu.menu, "mt", arglist, arg);
    XtManageChild (trace->menu.pdmenubutton[trace->menu.pde]);
    }
	
void dm_menu_entry (TRACE *trace,
		    char *title,	
		    char key,		/* Or '\0' for none */
		    char *accel,	/* Accelerator, or NULL */
		    char *accel_string,	/* Accelerator string, or NULL */
		    void (*callback)()
		    )
    /*** create a pulldownmenu entry under the top bar ***/
{
    int arg=0;

    trace->menu.pdm++;
    XtSetArg (arglist[arg], XmNlabelString, XmStringCreateSimple (title) );	arg++;
    if (key != '\0') { XtSetArg (arglist[arg], XmNmnemonic, key );	arg++; }
    if (accel != NULL) { XtSetArg (arglist[arg], XmNacceleratorText, XmStringCreateSimple (accel_string) );	arg++; }
    if (accel_string != NULL) { XtSetArg (arglist[arg], XmNaccelerator, accel );	arg++; }
    trace->menu.pdentrybutton[trace->menu.pdm] = XmCreatePushButtonGadget (trace->menu.pdmenu[trace->menu.pde], "me", arglist, arg);
    XtAddCallback (trace->menu.pdentrybutton[trace->menu.pdm], XmNactivateCallback, callback, trace);
    XtManageChild (trace->menu.pdentrybutton[trace->menu.pdm]);
    }

void dm_menu_subtitle (TRACE *trace,
		       char *title,	
		       char key			/* Or '\0' for none */
		       )
    /*** create a pulldownmenu entry under the top bar ***/
{
    int arg=0;

    trace->menu.pdm++;
    trace->menu.pdentry[trace->menu.pdm] = XmCreatePulldownMenu (trace->menu.pdmenu[trace->menu.pde],"",NULL,0);
    XtSetArg (arglist[arg], XmNlabelString, XmStringCreateSimple (title) );	arg++;
    XtSetArg (arglist[arg], XmNsubMenuId, trace->menu.pdentry[trace->menu.pdm] );	arg++;
    if (key != '\0') { XtSetArg (arglist[arg], XmNmnemonic, key );	arg++; }
    trace->menu.pdentrybutton[trace->menu.pdm] = XmCreateCascadeButton (trace->menu.pdmenu[trace->menu.pde], "mst", arglist, arg);
    XtManageChild (trace->menu.pdentrybutton[trace->menu.pdm]);
    }
				
void dm_menu_subentry (TRACE *trace,
		       char *title,	
		       char key,		/* Or '\0' for none */
		       char *accel,	/* Accelerator, or NULL */
		       char *accel_string,	/* Accelerator string, or NULL */
		       void (*callback)()
		       )
    /*** create a pulldownmenu entry under a subtitle ***/
{
    int arg=0;

    trace->menu.pds++;
    XtSetArg (arglist[arg], XmNlabelString, XmStringCreateSimple (title) );	arg++;
    if (key != '\0') { XtSetArg (arglist[arg], XmNmnemonic, key );	arg++; }
    if (accel != NULL) { XtSetArg (arglist[arg], XmNacceleratorText, XmStringCreateSimple (accel_string) );	arg++; }
    if (accel_string != NULL) { XtSetArg (arglist[arg], XmNaccelerator, accel );	arg++; }
    trace->menu.pdsubbutton[trace->menu.pds] = XmCreatePushButtonGadget (trace->menu.pdentry[trace->menu.pdm], "mse", arglist, arg);
    XtAddCallback (trace->menu.pdsubbutton[trace->menu.pds], XmNactivateCallback, callback, trace);
    XtManageChild (trace->menu.pdsubbutton[trace->menu.pds]);
    }

void dm_menu_subentry_colors (TRACE *trace,
			      char *cur_accel,	/* Accelerator, or NULL */
			      char *cur_accel_string,	/* Accelerator string, or NULL */
			      char *next_accel,	/* Accelerator, or NULL */
			      char *next_accel_string,	/* Accelerator string, or NULL */
			      void (*callback)()
			      )
    /*** create a pulldownmenu entry under a subtitle (uses special colors) ***/
{
    int color;

    for (color=0; color <= MAX_SRCH; color++) {
	trace->menu.pds++;
	XtSetArg (arglist[0], XmNbackground, trace->xcolornums[color]);
	XtSetArg (arglist[1], XmNmarginBottom, 8);
	/*XtSetArg (arglist[2], XmNmarginRight, 50);*/
	trace->menu.pdsubbutton[trace->menu.pds] = XmCreatePushButton (trace->menu.pdentry[trace->menu.pdm], "", arglist, 2);
	XtAddCallback (trace->menu.pdsubbutton[trace->menu.pds], XmNactivateCallback, callback, trace);
	XtManageChild (trace->menu.pdsubbutton[trace->menu.pds]);
	}
    dm_menu_subentry (trace, (cur_accel ? "Curr":"Current"), 'C', cur_accel, cur_accel_string, callback);
    dm_menu_subentry (trace, "Next", 'N', next_accel, next_accel_string, callback);
    }


/* Try to allocate the given font.  If it doesn't exist, use a default font, rather
*  then printing a error.  Roughly equivelent to:
*  return (XLoadQueryFont (global->display, global->signal_font_name))
*  but that crashes if a font isn't found.
*/
XFontStruct *grab_font (trace, font_name)
    TRACE	*trace;
    char	*font_name;		/* Name of the font */
{
    int	num;
    char **list;

    list = XListFonts (global->display, font_name, 1, &num);
    XFreeFontNames(list);

    if (num > 0) {
	return (XLoadQueryFont (global->display, font_name));
    }
    else {
	return (XLoadQueryFont (global->display, "-*-*-medium-r-*-*-*-*-*-*-*-*-*-*"));
    }
}

/* Create a trace display and link it into the global information */
TRACE *create_trace (xs,ys,xp,yp)
    int		xs,ys,xp,yp;
{
    /*    int		x1,x2;
	  unsigned int junk; */
    int		i;
    TRACE	*trace;
    int		argc_copy;
    char	**argv_copy;
    XColor	xcolor,xcolor2;
    Colormap	cmap;
    
    /*** alloc space for trace to display state block ***/
    trace = malloc_trace ();
    
    /*** create trace->toplevel widget ***/
    argc_copy = global->argc;
    argv_copy = (char **)XtMalloc (global->argc * sizeof (char *));
    memcpy (argv_copy, global->argv, global->argc * sizeof (char *));
    XtSetArg (arglist[0], XtNargc, argc_copy);
    XtSetArg (arglist[1], XtNargv, argv_copy);
    trace->toplevel = XtAppCreateShell (NULL, "Dinotrace",
				 applicationShellWidgetClass, global->display, arglist, 2);
    /* printf ("&& && trace %d, top=%d\n", trace, trace->toplevel); */

    change_title (trace);

    XtGetApplicationResources (trace->toplevel, (XtPointer) global, resources,
			      XtNumber (resources), (Arg *) NULL, 0);

    XtAppAddActions (global->appcontext, actions, XtNumber(actions));

    /*** add pixmap and position trace->toplevel widget ***/
    XtSetArg (arglist[0], XtNallowShellResize, TRUE);
    XtSetArg (arglist[1], XtNiconPixmap, global->bdpm);
    XtSetArg (arglist[2], "iconifyPixmap", global->dpm);
    XtSetArg (arglist[3], XmNx, xp);
    XtSetArg (arglist[4], XmNy, yp);
    XtSetArg (arglist[5], XmNheight, ys);
    XtSetArg (arglist[6], XmNwidth, xs);
    XtSetValues (trace->toplevel, arglist, 7);

    /****************************************
     * create the main window
     ****************************************/
    XtSetArg (arglist[0], XmNx, 20);
    XtSetArg (arglist[1], XmNy, 350);
    /*CCCCCC XtSetArg (arglist[4], XmNacceptFocus, TRUE); */
    trace->main = XmCreateMainWindow (trace->toplevel,"main", arglist, 2);
    /*XtAddCallback (trace->main, XmNfocusCallback, win_focus_cb, trace);*/
    
    /* Find the colors to use */
    XtSetArg (arglist[0], XmNcolormap, &cmap);
    XtSetArg (arglist[1], XmNforeground, &(trace->xcolornums[0]));
    XtSetArg (arglist[2], XmNbackground, &(trace->barcolornum));
    XtGetValues (trace->main, arglist, 3);

    for (i=1; i<=9; i++) {
	if (DTPRINT_DISPLAY) printf ("%d = '%s'\n", i, global->color_names[i] ? global->color_names[i]:"NULL");
	if ( (global->color_names[i] != NULL)	
	    && (XAllocNamedColor (global->display, cmap, global->color_names[i], &xcolor, &xcolor2)))
	    trace->xcolornums[i] = xcolor.pixel;
	else trace->xcolornums[i] = XWhitePixel (global->display, 0);
	}

    if (global->barcolor_name == NULL || global->barcolor_name[0]=='\0') {
	/* Default is 7% green above background */
	xcolor.pixel = trace->barcolornum;
	XQueryColor (global->display, cmap, &xcolor);
	if (xcolor.green < 58590) 
	    xcolor.green += xcolor.green * 0.07;
	else xcolor.green -= xcolor.green * 0.07;
	if (DTPRINT_DISPLAY) printf (" = %x, %x, %x \n", xcolor.red, xcolor.green, xcolor.blue);
	if (XAllocColor (global->display, cmap, &xcolor))
	    trace->barcolornum = xcolor.pixel;
	}
    else {
	if (XAllocNamedColor (global->display, cmap, global->barcolor_name, &xcolor, &xcolor2))
	    trace->barcolornum = xcolor.pixel;
	}
	    
    /****************************************
     * create the menu bar
     ****************************************/

    /*** begin with -1 pde, since new menu will increment ***/
    trace->menu.pdm = trace->menu.pde = trace->menu.pds = -1;
    trace->menu.menu = XmCreateMenuBar (trace->main,"menu", NULL, 0);
    XtManageChild (trace->menu.menu);

    dm_menu_title (trace, "Trace", 'T');
    dm_menu_entry (trace, 	"Read...",	'R',	NULL, NULL,	trace_read_cb);
    dm_menu_entry (trace, 	"ReRead",	'e',	NULL, NULL,	trace_reread_cb);
    dm_menu_entry (trace, 	"ReRead All",	'A',	NULL, NULL,	trace_reread_all_cb);
    dm_menu_entry (trace, 	"Open...",	'O',	NULL, NULL,	trace_open_cb);
    dm_menu_entry (trace, 	"Close",	'C',	NULL, NULL,	trace_close_cb);
    trace->menu_close = trace->menu.pdentrybutton[trace->menu.pdm];
    dm_menu_entry (trace, 	"Clear",	'l',	NULL, NULL,	trace_clear_cb);
    dm_menu_entry (trace, 	"Exit", 	'x',	NULL, NULL,	trace_exit_cb);

    dm_menu_title (trace, "Customize", 'u');
    dm_menu_entry (trace, 	"Change...",	'C',	NULL, NULL,	cus_dialog_cb);
    dm_menu_entry (trace, 	"Grid...",	'G',	NULL, NULL,	grid_customize_cb);
    dm_menu_entry (trace, 	"Read...",	'R',	NULL, NULL,	cus_read_cb);
    dm_menu_entry (trace, 	"ReRead",	'e',	NULL, NULL,	cus_reread_cb);
    if (DTDEBUG) {
	dm_menu_entry (trace, 	"Write",	'W',	NULL, NULL,	config_write_cb);
	}
    dm_menu_entry (trace, 	"Restore",	'R',	NULL, NULL,	cus_restore_cb);

    dm_menu_title (trace, "Cursor", 'C');
    dm_menu_subtitle (trace, 	"Add",		'A');
    trace->menu.cur_add_pds = trace->menu.pds+1;
    dm_menu_subentry_colors (trace, "<Key>F3:", "F3", "Shift<Key>F3:", "S-F3",cur_add_cb);
    dm_menu_entry (trace, 	"Move",		'M',	NULL, NULL,	cur_mov_cb);
    dm_menu_entry (trace, 	"Delete",	'D',	NULL, NULL,	cur_del_cb);
    dm_menu_entry (trace, 	"Clear", 	'C',	NULL, NULL,	cur_clr_cb);
    dm_menu_subtitle (trace,	 "Highlight",	'H');
    trace->menu.cur_highlight_pds = trace->menu.pds+1;
    dm_menu_subentry_colors (trace, 		NULL, NULL, NULL, NULL,	cur_highlight_cb);
    dm_menu_entry (trace, 	"Cancel", 	'l',	NULL, NULL,	cancel_all_events);

    dm_menu_title (trace, "Signal", 'S');
    dm_menu_entry (trace, 	"Add...",	'A',	NULL, NULL,	sig_add_cb);
    dm_menu_entry (trace, 	"Move",		'M',	NULL, NULL,	sig_mov_cb);
    dm_menu_entry (trace, 	"Copy",		'C',	NULL, NULL,	sig_copy_cb);
    dm_menu_entry (trace, 	"Delete",	'D',	NULL, NULL,	sig_del_cb);
    dm_menu_entry (trace, 	"Search...",	'S',	NULL, NULL,	sig_search_cb);
    dm_menu_subtitle (trace, 	"Highlight",	'H');
    trace->menu.sig_highlight_pds = trace->menu.pds+1;
    dm_menu_subentry_colors (trace, "<Key>F4:", "F4", "Shift<Key>F4:", "S-F4",sig_highlight_cb);
    dm_menu_entry (trace, 	"Select...",	'e',	NULL, NULL,	sig_select_cb);
    dm_menu_entry (trace, 	"Cancel", 	'l',	NULL, NULL,	cancel_all_events);

    dm_menu_title (trace, "Value", 'V');
    dm_menu_entry (trace,	"Annotate", 	'\0',	"<Key>F2:", "F2", val_annotate_do_cb);
    dm_menu_entry (trace, 	"Annotate...",	'n', 	NULL, NULL,	val_annotate_cb);
    dm_menu_entry (trace, 	"Examine",	'E',	NULL, NULL,	val_examine_cb);
    XtSetArg (arglist[0], XmNacceleratorText, XmStringCreateSimple ("MB2") );
    XtSetValues (trace->menu.pdentrybutton[trace->menu.pdm], arglist, 1);
    dm_menu_entry (trace, 	"Search...",	'S',	NULL, NULL,	val_search_cb);
    dm_menu_subtitle (trace, 	"Highlight",	'H');
    trace->menu.val_highlight_pds = trace->menu.pds+1;
    dm_menu_subentry_colors (trace, "<Key>F5:", "F5", "Shift<Key>F5:", "S-F5",val_highlight_cb);
    dm_menu_entry (trace, 	"Cancel", 	'l',	NULL, NULL,	cancel_all_events);

    dm_menu_title (trace, "Print", 'P');
    dm_menu_entry (trace, 	"Print...",	'i',	NULL, NULL,	ps_dialog);
    dm_menu_entry (trace, 	"Print",	'P',	NULL, NULL,	ps_print_direct_cb);
    dm_menu_entry (trace, 	"Reset",	'e',	NULL, NULL,	ps_reset);
    
    if (DTDEBUG) {
	dm_menu_title (trace, "Debug", 'D');
	dm_menu_entry	(trace, "Print Signal Names",	'N', NULL, NULL,	print_sig_names);
	dm_menu_entry	(trace, "Print Signal Info (Screen Only)", 'I', NULL, NULL,	print_screen_traces);
	dm_menu_entry	(trace, "Print Signal States",	't', NULL, NULL,	print_signal_states);
	dm_menu_entry	(trace, "Integrity Check",	'I', NULL, NULL,	debug_integrity_check_cb);
	dm_menu_entry	(trace, "Toggle Print",		'P', NULL, NULL,	debug_toggle_print_cb);
	dm_menu_entry	(trace, "Increase DebugTemp",	'+', NULL, NULL,	debug_increase_debugtemp_cb);
	dm_menu_entry	(trace, "Decrease DebugTemp",	'-', NULL, NULL,	debug_decrease_debugtemp_cb);
	}

    dm_menu_title (trace, "Help", 'H');
    dm_menu_entry (trace, 	"On Version",	'V',	NULL, NULL,	help_cb);
    dm_menu_entry (trace, 	"On Trace",	'T',	NULL, NULL,	help_trace_cb);
    XtSetArg (arglist[0], XmNmenuHelpWidget, trace->menu.pdmenubutton[trace->menu.pde]);
    XtSetValues (trace->menu.menu, arglist, 1);

    /****************************************
     * create the command widget
     ****************************************/
    XtSetArg (arglist[0], XmNresizePolicy, XmRESIZE_NONE);
    trace->command.command = XmCreateForm (trace->main, "form", arglist, 1);
    XtManageChild (trace->command.command);
    
    /*** create begin button in command region ***/
    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Begin") );
    XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
    XtSetArg (arglist[2], XmNleftOffset, 7);
    XtSetArg (arglist[3], XmNbottomAttachment, XmATTACH_FORM );
    trace->command.begin_but = XmCreatePushButton (trace->command.command, "begin", arglist, 4);
    XtAddCallback (trace->command.begin_but, XmNactivateCallback, win_begin_cb, trace);
    XtManageChild (trace->command.begin_but);
    
    /*** create goto button in command region ***/
    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Goto") );
    XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_WIDGET );
    XtSetArg (arglist[2], XmNleftOffset, 7);
    XtSetArg (arglist[3], XmNbottomAttachment, XmATTACH_FORM );
    XtSetArg (arglist[4], XmNleftWidget, trace->command.begin_but);
    trace->command.goto_but = XmCreatePushButton (trace->command.command, "goto", arglist, 5);
    XtAddCallback (trace->command.goto_but, XmNactivateCallback, win_goto_cb, trace);
    XtManageChild (trace->command.goto_but);
    
    /*** create end button in command region ***/
    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("End") );
    XtSetArg (arglist[1], XmNrightAttachment, XmATTACH_FORM );
    XtSetArg (arglist[2], XmNrightOffset, 15);
    XtSetArg (arglist[3], XmNbottomAttachment, XmATTACH_FORM );
    trace->command.end_but = XmCreatePushButton (trace->command.command, "end", arglist, 4);
    XtAddCallback (trace->command.end_but, XmNactivateCallback, win_end_cb, trace);
    XtManageChild (trace->command.end_but);
    
    /*** create refresh button in command region ***/
    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Refresh") );
    XtSetArg (arglist[1], XmNrightAttachment, XmATTACH_WIDGET );
    XtSetArg (arglist[2], XmNrightOffset, 7);
    XtSetArg (arglist[3], XmNbottomAttachment, XmATTACH_FORM );
    XtSetArg (arglist[4], XmNrightWidget, trace->command.end_but);
    trace->command.refresh_but = XmCreatePushButton (trace->command.command, "refresh", arglist, 5);
    XtAddCallback (trace->command.refresh_but, XmNactivateCallback, win_refresh_cb, trace);
    XtManageChild (trace->command.refresh_but);
    
    /*** create resolution button in command region ***/
    XtSetArg (arglist[0], XmNleftAttachment, XmATTACH_POSITION );
    XtSetArg (arglist[1], XmNleftPosition, 45);
    XtSetArg (arglist[2], XmNbottomAttachment, XmATTACH_FORM );
    XtSetArg (arglist[3], XmNlabelString, XmStringCreateSimple ("Res=123456.7 units") );
    trace->command.reschg_but = XmCreatePushButton (trace->command.command, "res", arglist, 4);
    XtAddCallback (trace->command.reschg_but, XmNactivateCallback, win_chg_res_cb, trace);
    XtManageChild (trace->command.reschg_but);
    /* No more size changes */
    XtSetArg (arglist[0], XmNrecomputeSize, FALSE );
    XtSetValues (trace->command.reschg_but,arglist,1);
    
    /* create the vertical scroll bar */
    XtSetArg (arglist[0], XmNorientation, XmVERTICAL );
    XtSetArg (arglist[1], XmNrightAttachment, XmATTACH_FORM );
    XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_FORM );
    XtSetArg (arglist[3], XmNbottomAttachment, XmATTACH_FORM );
    XtSetArg (arglist[4], XmNbottomOffset, 40);
    XtSetArg (arglist[5], XmNwidth, 18);
    trace->vscroll = XmCreateScrollBar ( trace->command.command, "vscroll", arglist, 6);
    XtAddCallback (trace->vscroll, XmNincrementCallback, vscroll_unitinc, trace);
    XtAddCallback (trace->vscroll, XmNdecrementCallback, vscroll_unitdec, trace);
    XtAddCallback (trace->vscroll, XmNdragCallback, vscroll_drag, trace);
    XtAddCallback (trace->vscroll, XmNtoBottomCallback, vscroll_bot, trace);
    XtAddCallback (trace->vscroll, XmNtoTopCallback, vscroll_top, trace);
    XtAddCallback (trace->vscroll, XmNpageIncrementCallback, vscroll_pageinc, trace);
    XtAddCallback (trace->vscroll, XmNpageDecrementCallback, vscroll_pagedec, trace);
    XtManageChild (trace->vscroll);

    /* create the horizontal scroll bar */
    XtSetArg (arglist[0], XmNorientation, XmHORIZONTAL );
    XtSetArg (arglist[1], XmNbottomAttachment, XmATTACH_WIDGET );
    XtSetArg (arglist[2], XmNbottomWidget, trace->command.end_but);
    XtSetArg (arglist[3], XmNleftAttachment, XmATTACH_FORM );
    XtSetArg (arglist[4], XmNrightAttachment, XmATTACH_WIDGET );
    XtSetArg (arglist[5], XmNrightWidget, trace->vscroll);
    XtSetArg (arglist[6], XmNheight, 18);
    trace->hscroll = XmCreateScrollBar ( trace->command.command, "hscroll", arglist, 7);
    XtAddCallback (trace->hscroll, XmNincrementCallback, hscroll_unitinc, trace);
    XtAddCallback (trace->hscroll, XmNdecrementCallback, hscroll_unitdec, trace);
    XtAddCallback (trace->hscroll, XmNdragCallback, hscroll_drag, trace);
    XtAddCallback (trace->hscroll, XmNtoBottomCallback, hscroll_bot, trace);
    XtAddCallback (trace->hscroll, XmNtoTopCallback, hscroll_top, trace);
    XtAddCallback (trace->hscroll, XmNpageIncrementCallback, hscroll_pageinc, trace);
    XtAddCallback (trace->hscroll, XmNpageDecrementCallback, hscroll_pagedec, trace);
    XtManageChild (trace->hscroll);
    
    /*** create full button in command region ***/
    XtSetArg (arglist[0], XmNrightAttachment, XmATTACH_WIDGET );
    XtSetArg (arglist[1], XmNrightWidget, trace->command.reschg_but);
    XtSetArg (arglist[2], XmNrightOffset, 2);
    XtSetArg (arglist[3], XmNlabelString, XmStringCreateSimple ("Full") );
    XtSetArg (arglist[4], XmNbottomAttachment, XmATTACH_FORM );
    trace->command.resfull_but = XmCreatePushButton (trace->command.command, "full", arglist, 5);
    XtAddCallback (trace->command.resfull_but, XmNactivateCallback, win_full_res_cb, trace);
    XtManageChild (trace->command.resfull_but);
    
    /*** create zoom button in command region ***/
    XtSetArg (arglist[0], XmNleftAttachment, XmATTACH_WIDGET );
    XtSetArg (arglist[1], XmNleftWidget, trace->command.reschg_but);
    XtSetArg (arglist[2], XmNleftOffset, 2);
    XtSetArg (arglist[3], XmNlabelString, XmStringCreateSimple ("Zoom") );
    XtSetArg (arglist[4], XmNbottomAttachment, XmATTACH_FORM );
    trace->command.reszoom_but = XmCreatePushButton (trace->command.command, "zoom", arglist, 5);
    XtAddCallback (trace->command.reszoom_but, XmNactivateCallback, win_zoom_res_cb, trace);
    XtManageChild (trace->command.reszoom_but);

    /*** create resolution decrease button in command region ***/
    XtSetArg (arglist[0], XmNrightAttachment, XmATTACH_WIDGET );
    XtSetArg (arglist[1], XmNrightWidget, trace->command.resfull_but);
    XtSetArg (arglist[2], XmNrightOffset, 2);
    XtSetArg (arglist[3], XmNheight, 32);
    XtSetArg (arglist[4], XmNbottomAttachment, XmATTACH_FORM );
    XtSetArg (arglist[5], XmNarrowDirection, XmARROW_LEFT );
    trace->command.resdec_but = XmCreateArrowButton (trace->command.command, "decres", arglist, 6);
    XtAddCallback (trace->command.resdec_but, XmNactivateCallback, win_dec_res_cb, trace);
    XtManageChild (trace->command.resdec_but);
    
    /*** create resolution increase button in command region ***/
    XtSetArg (arglist[0], XmNleftAttachment, XmATTACH_WIDGET );
    XtSetArg (arglist[1], XmNleftWidget, trace->command.reszoom_but);
    XtSetArg (arglist[2], XmNleftOffset, 2);
    XtSetArg (arglist[3], XmNheight, 32);
    XtSetArg (arglist[4], XmNbottomAttachment, XmATTACH_FORM );
    XtSetArg (arglist[5], XmNarrowDirection, XmARROW_RIGHT );
    trace->command.resinc_but = XmCreateArrowButton (trace->command.command, "incres", arglist, 6);
    XtAddCallback (trace->command.resinc_but, XmNactivateCallback, win_inc_res_cb, trace);
    XtManageChild (trace->command.resinc_but);
    
    /****************************************
     * create the work area window
     ****************************************/

    XtSetArg (arglist[0], XmNtopAttachment, XmATTACH_FORM );
    XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
    XtSetArg (arglist[2], XmNrightAttachment, XmATTACH_WIDGET );
    XtSetArg (arglist[3], XmNrightWidget, trace->vscroll); 
    XtSetArg (arglist[4], XmNbottomAttachment, XmATTACH_WIDGET );
    XtSetArg (arglist[5], XmNbottomWidget, trace->hscroll);
    XtSetArg (arglist[6], XtNtranslations, XtParseTranslationTable (translations));
    trace->work = XmCreateDrawingArea (trace->command.command,"work", arglist, 7);

    XtAddCallback (trace->work, XmNexposeCallback, win_expose_cb, trace);
    XtAddCallback (trace->work, XmNresizeCallback, win_resize_cb, trace);
    XtManageChild (trace->work);
    
    XtManageChild (trace->main);
    XtRealizeWidget (trace->toplevel);
    
    /* Display parameters */
    trace->wind = XtWindow (trace->work);
    trace->gc = XCreateGC (global->display, trace->wind, 0, NULL);

    /* Choose fonts */
    global->signal_font = grab_font (trace, global->signal_font_name);
    global->time_font   = grab_font (trace, global->time_font_name);
    global->value_font  = grab_font (trace, global->value_font_name);

    XSetFont (global->display, trace->gc, global->signal_font->fid);

    ps_reset (NULL, trace, NULL);

    config_trace_defaults (trace);
    
    new_res (trace, global->res);

    set_cursor (trace, DC_NORMAL);

    get_geometry (trace);

    set_menu_closes ();

    return (trace);
    }


