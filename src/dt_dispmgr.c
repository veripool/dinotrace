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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

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
    {"color9", "Color9", XtRString, sizeof(String), Offset(color_names[9]), XtRImmediate, (XtPointer) "NavyBlue"}
    };
#undef Offset


void set_cursor (trace, cursor_num)
    TRACE	*trace;			/* Display information */
    int		cursor_num;		/* Entry in xcursors to display */
{
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	XDefineCursor (global->display, trace->wind, global->xcursors[cursor_num]);
	}
    XmSetMenuCursor (global->display, global->xcursors[cursor_num]);
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

void trace_open_cb (w,trace,cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    Position x,y,width,height;
    Position new_x,new_y,new_width,new_height;
    TRACE	*trace_new;

    if (DTPRINT) printf ("In trace_open - trace=%d\n",trace);

    /* Get orignal sizes */
    XtSetArg (arglist[0], XmNheight,&height);
    XtSetArg (arglist[1], XmNwidth, &width);
    XtSetArg (arglist[2], XmNx, &x);
    XtSetArg (arglist[3], XmNy, &y);
    XtGetValues (trace->toplevel, arglist, 4);
    if (DTPRINT) printf ("Old size: %dx%d+%d+%d\n", width, height, x, y);

    /* Shrink this window */
    new_x = x; new_y = y; new_width=width; new_height=height;
    if (global->shrink_geometry.xp)	new_x = x + (( width * global->shrink_geometry.x ) / 100);
    if (global->shrink_geometry.yp) 	new_y = y + (( height * global->shrink_geometry.y ) / 100);
    if (global->shrink_geometry.widthp) new_width = ( width * global->shrink_geometry.width ) / 100;
    if (global->shrink_geometry.heightp) new_height = ( height * global->shrink_geometry.height ) / 100;
    if (DTPRINT) printf ("Shrink size: %dx%d+%d+%d\n", new_width, new_height, new_x, new_y);
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
    if (DTPRINT) printf ("New size: %dx%d+%d+%d\n", new_width, new_height, new_x, new_y);
    trace_new = create_trace (new_width, new_height, new_x, new_y);

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

    if (DTPRINT) printf ("In trace_close - trace=%d\n",trace);

    /* clear the screen */
    XClearWindow (global->display, trace->wind);
    /* Nail the child */
    /* XtUnmanageChild (trace->toplevel); */
    /* free memory associated with the data */
    free_data (trace);
    /* destroy all the widgets created for the screen */
    XtDestroyWidget (trace->toplevel);
    /* free the display structure */
    DFree (trace);

    /* relink pointers to ignore this trace */
    if (trace == global->trace_head)
	global->trace_head = trace->next_trace;
    for (trace_ptr = global->trace_head; trace_ptr; trace_ptr = trace_ptr->next_trace) {
	if (trace_ptr->next_trace == trace) trace_ptr->next_trace = trace->next_trace;
	}

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

    if (DTPRINT) printf ("In clear_trace - trace=%d\n",trace);

    /* nail all traces except for this window's */
    for (trace_ptr = global->trace_head; trace_ptr; ) {
	trace_next = trace_ptr->next_trace;
	if (trace_ptr != trace) {
	    trace_close_cb (w, trace_ptr);
	    }
	trace_ptr = trace_next;
	}

    /* clear the screen */
    XClearWindow (global->display, trace->wind);

    /* free memory associated with the data */
    free_data (trace);

    /* change the name on title bar back to the trace */
    change_title (trace);

    init_globals();
    }

void trace_exit_cb (w,trace,cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    TRACE		*trace_next;

    if (DTPRINT) printf ("In trace_exit_cb - trace=%d\n",trace);

    for (trace = global->trace_head; trace; ) {
	trace_next = trace->next_trace;
	trace_close_cb (w, trace);
	trace = trace_next;
	}

    DFree (global);

    /* all done */
    exit (1);
    }

void init_globals ()
{
    int i;

    if (DTPRINT) printf ("in init_globals\n");

    global->delsig = NULL;
    global->selected_sig = NULL;
    global->cursor_head = NULL;
    global->xstart = 200;
    global->time = 0;
    global->time_precision = TIMEREP_NS;
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
    global->res_default = TRUE;

    for (i=0; i<MAX_SRCH; i++) {
	/* Colors */
	global->color_names[i] = NULL;

	/* Value */
	global->val_srch[i].color = global->val_srch[i].cursor =
	    global->val_srch[i].value[0] = global->val_srch[i].value[1] =
		global->val_srch[i].value[2] = 0;

	/* Signal */
	global->sig_srch[i].color = 0;
	global->sig_srch[i].string[0] = '\0';
	}
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
	printf ("Can't open display '%s'\n", XDisplayName (display_name));
	exit (0);
	}

    XSynchronize (global->display, sync);
    if (DTPRINT) printf ("in create_globals, syncronization is %d\n", sync);

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
    }

TRACE *malloc_trace ()
    /* Allocate a trace structure and return it */
{
    TRACE	*trace;
    
    /*** alloc space for trace to display state block ***/
    trace = (TRACE *)XtMalloc ( sizeof(TRACE) );
    memset (trace, 0, sizeof (TRACE));
    trace->next_trace = global->trace_head;
    global->trace_head = trace;

    return (trace);
    }
    
/* Create a trace display and link it into the global information */

TRACE *create_trace (xs,ys,xp,yp)
    int		xs,ys,xp,yp;
{
    char	string[20];
    /*    int		x1,x2;
	  unsigned int junk; */
    int		i;
    int		pd=0,pde=0,pds=0;
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
	if (DTPRINT) printf ("%d = '%s'\n", i, global->color_names[i] ? global->color_names[i]:"NULL");
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
	if (DTPRINT) printf (" = %x, %x, %x \n", xcolor.red, xcolor.green, xcolor.blue);
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
    trace->menu.menu = XmCreateMenuBar (trace->main,"menu", NULL, 0);
    XtManageChild (trace->menu.menu);
    
    /*** create a pulldownmenu on the top bar ***/
#define	dt_menu_title(title,key) \
    pde++; \
	trace->menu.pdmenu[pde] = XmCreatePulldownMenu (trace->menu.menu,"",NULL,0); \
	    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple (title) ); \
		XtSetArg (arglist[1], XmNsubMenuId, trace->menu.pdmenu[pde] ); \
		    XtSetArg (arglist[2], XmNmnemonic, key ); \
			trace->menu.pdmenubutton[pde] = XmCreateCascadeButton \
			    (trace->menu.menu, "", arglist, 3); \
				XtManageChild (trace->menu.pdmenubutton[pde])
				
    /*** create a pulldownmenu entry under the top bar ***/
#define	dt_menu_entry(title,key,callback) \
    pd++; \
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple (title) ); \
	    XtSetArg (arglist[1], XmNmnemonic, key ); \
		trace->menu.pdentrybutton[pd] = XmCreatePushButtonGadget \
		    (trace->menu.pdmenu[pde], "", arglist, 2); \
			XtAddCallback (trace->menu.pdentrybutton[pd], XmNactivateCallback, callback, trace); \
			    XtManageChild (trace->menu.pdentrybutton[pd])

    /*** create a pulldownmenu entry which is a sub menu ***/
#define	dt_menu_subtitle(title,key) \
    pd++; \
	trace->menu.pdentry[pd] = XmCreatePulldownMenu (trace->menu.pdmenu[pde],"",NULL,0); \
	    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple (title) ); \
		XtSetArg (arglist[1], XmNsubMenuId, trace->menu.pdentry[pd] ); \
		    XtSetArg (arglist[2], XmNmnemonic, key ); \
			trace->menu.pdentrybutton[pd] = XmCreateCascadeButton \
			    (trace->menu.pdmenu[pde], "", arglist, 3); \
				XtManageChild (trace->menu.pdentrybutton[pd])
				
    /*** create a pulldownmenu entry under a subtitle ***/
#define	dt_menu_subentry(title,key,callback) \
    pds++; \
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple (title) ); \
	    XtSetArg (arglist[1], XmNmnemonic, key ); \
		trace->menu.pdsubbutton[pds] = XmCreatePushButtonGadget \
		    (trace->menu.pdentry[pd], "", arglist, 2); \
			XtAddCallback (trace->menu.pdsubbutton[pds], XmNactivateCallback, callback, trace); \
			    XtManageChild (trace->menu.pdsubbutton[pds])

    /*** create a pulldownmenu entry under a subtitle (uses special colors) ***/
#define	dt_menu_subentry_c(color, callback) \
    pds++; \
	XtSetArg (arglist[0], XmNbackground, color ); \
	    XtSetArg (arglist[1], XmNmarginRight, 50); \
		XtSetArg (arglist[2], XmNmarginBottom, 8); \
		    trace->menu.pdsubbutton[pds] = XmCreatePushButton \
			(trace->menu.pdentry[pd], "", arglist, 3); \
			    XtAddCallback (trace->menu.pdsubbutton[pds], XmNactivateCallback, callback, trace); \
				XtManageChild (trace->menu.pdsubbutton[pds])

    /*** begin with -1 pde, since new menu will increment ***/
    pd = pds = pde = -1;

    dt_menu_title ("Trace", 'T');
    dt_menu_entry	("Read",	'R',	trace_read_cb);
    dt_menu_entry	("ReRead",	'e',	trace_reread_cb);
    dt_menu_entry	("ReRead All",	'A',	trace_reread_all_cb);
    dt_menu_entry	("Open",	'O',	trace_open_cb);
    dt_menu_entry	("Close",	'C',	trace_close_cb);
    trace->menu_close = trace->menu.pdentrybutton[pd];
    dt_menu_entry	("Clear",	'l',	trace_clear_cb);
    dt_menu_entry	("Exit", 	'x',	trace_exit_cb);

    dt_menu_title ("Customize", 'u');
    dt_menu_entry	("Change",	'C',	cus_dialog_cb);
    dt_menu_entry	("ReRead",	'e',	cus_reread_cb);
    dt_menu_entry	("Restore",	'R',	cus_restore_cb);

    dt_menu_title ("Cursor", 'C');
    dt_menu_subtitle	("Add",		'A');
    trace->menu.cur_add_pds = pds+1;
    for (i=0; i<=MAX_SRCH; i++) {
	dt_menu_subentry_c	(trace->xcolornums[i], cur_add_cb);
	}
    dt_menu_entry	("Move",	'M',	cur_mov_cb);
    dt_menu_entry	("Delete",	'D',	cur_del_cb);
    dt_menu_entry	("Clear", 	'C',	cur_clr_cb);
    dt_menu_subtitle	("Highlight",	'H');
    trace->menu.cur_highlight_pds = pds+1;
    for (i=0; i<=MAX_SRCH; i++) {
	dt_menu_subentry_c	(trace->xcolornums[i], cur_highlight_cb);
	}
    dt_menu_entry	("Cancel", 	'l',	cancel_all_events);

    dt_menu_title ("Grid", 'G');
    dt_menu_entry	("Res",	 	'R',	grid_res_cb);
    dt_menu_entry	("Align",	'A',	grid_align_cb);
    dt_menu_entry	("Reset",	's',	grid_reset_cb);
    dt_menu_entry	("Cancel", 	'l',	cancel_all_events);

    dt_menu_title ("Signal", 'S');
    dt_menu_entry	("Add",		'A',	sig_add_cb);
    dt_menu_entry	("Move",	'M',	sig_mov_cb);
    dt_menu_entry	("Copy",	'C',	sig_copy_cb);
    dt_menu_entry	("Delete",	'D',	sig_del_cb);
    dt_menu_entry	("Search",	'S',	sig_search_cb);
    dt_menu_subtitle	("Highlight",	'H');
    trace->menu.sig_highlight_pds = pds+1;
    for (i=0; i<=MAX_SRCH; i++) {
	dt_menu_subentry_c	(trace->xcolornums[i], sig_highlight_cb);
	}
    dt_menu_entry	("Select",	'e',	sig_select_cb);
    dt_menu_entry	("Cancel", 	'l',	cancel_all_events);

    dt_menu_title ("Value", 'V');
    dt_menu_entry	("Examine",	'E',	val_examine_cb);
    dt_menu_entry	("Search",	'S',	val_search_cb);
    dt_menu_entry	("Cancel", 	'l',	cancel_all_events);

    dt_menu_title ("Print", 'P');
    dt_menu_entry	("Print",	'P',	ps_dialog);
    dt_menu_entry	("Reset",	'e',	ps_reset);
    
    if (DTDEBUG) {
	dt_menu_title ("Debug", 'D');
	dt_menu_entry	("Print Signal Names", 'N', print_sig_names);
	dt_menu_entry	("Print Signal Info (Screen Only)", 'I', print_screen_traces);
	dt_menu_entry	("Integrity Check", 'I', debug_integrity_check_cb);
	dt_menu_entry	("Toggle Print", 'P', debug_toggle_print_cb);
	dt_menu_entry	("Increase DebugTemp", '+', debug_increase_debugtemp_cb);
	dt_menu_entry	("Decrease DebugTemp", '-', debug_decrease_debugtemp_cb);
	}

    dt_menu_title ("Help", 'H');
    dt_menu_entry	("On Version",	'V',	help_cb);
    dt_menu_entry	("On Trace",	'T',	help_trace_cb);
    XtSetArg (arglist[0], XmNmenuHelpWidget, trace->menu.pdmenubutton[pde]);
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
    
    /*** create resolution button in command region ***/
    XtSetArg (arglist[0], XmNleftAttachment, XmATTACH_POSITION );
    XtSetArg (arglist[1], XmNleftPosition, 45);
    XtSetArg (arglist[2], XmNbottomAttachment, XmATTACH_FORM );
    XtSetArg (arglist[3], XmNlabelString, XmStringCreateSimple ("Res=xxxxxx ns") );
    trace->command.reschg_but = XmCreatePushButton (trace->command.command, "res", arglist, 4);
    XtAddCallback (trace->command.reschg_but, XmNactivateCallback, win_chg_res_cb, trace);
    XtManageChild (trace->command.reschg_but);
    
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
    trace->work = XmCreateDrawingArea (trace->command.command,"work", arglist, 6);

    XtAddCallback (trace->work, XmNexposeCallback, win_expose_cb, trace);
/*
    XtAddCallback (trace->work, XmNresizeCallback, win_expose_cb, trace);
*/
    XtManageChild (trace->work);
    
    XtManageChild (trace->main);
    XtRealizeWidget (trace->toplevel);
    
    /* Initialize Various Parameters */
    trace->firstsig = NULL;
    trace->dispsig = NULL;
    trace->wind = XtWindow (trace->work);
    trace->custom.customize = NULL;
    trace->signal.add = NULL;
    trace->signal.search = NULL;
    trace->value.search = NULL;
    trace->prntscr.customize = NULL;
    trace->prompt_popup = NULL;
    trace->fileselect.dialog = NULL;
    trace->filename[0] = '\0';
    trace->loaded = 0;
    trace->numsig = 0;
    trace->numsigvis = 0;
    trace->numsigstart = 0;
    trace->busrep = HBUS;
    trace->ystart = 40;
    
    trace->signalstate_head = NULL;
    config_restore_defaults (trace);
    
    /*
    XGetGeometry (global->display, XtWindow (trace->work), (Window*) &x1, &x1,
		 &x1, &x2, &x1, &x1, &x1);
		 */

    trace->gc = XCreateGC (global->display, trace->wind, 0, NULL);

    /* get font information */
    trace->text_font = XQueryFont (global->display,XGContextFromGC (trace->gc));
    
    new_res (trace, FALSE);

    set_cursor (trace, DC_NORMAL);

    get_geometry (trace);

    set_menu_closes ();

    return (trace);
    }


