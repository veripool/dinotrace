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


#include <X11/Xlib.h>
#include <X11/cursorfont.h>
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
	XtSetArg(arglist[0], XmNsensitive, sensitive);
	XtSetValues (trace->menu_close, arglist, 1);
	}
    }

void trace_open_cb(w,trace)
    Widget		w;
    TRACE		*trace;
{
    int xp,yp,xs,ys;
    TRACE	*trace_new;

    if (DTPRINT) printf("In trace_open - trace=%d\n",trace);

    XtSetArg(arglist[0], XmNheight, (int)&ys);
    XtSetArg(arglist[1], XmNwidth, (int)&xs);
    XtSetArg(arglist[2], XmNx, (int)&xp);
    XtSetArg(arglist[3], XmNy, (int)&yp);
    XtGetValues(trace->toplevel, arglist, 4);

    XtSetArg(arglist[0], XmNheight, ys/2);
    /* printf ("&& && oldy = %d, newy = %d\n", ys, ys/2); */
    XtSetValues(trace->toplevel, arglist, 1);

    /* printf ("&& && new xs %d, ys = %d, xp %d, ys %d\n", xs, ys/2, xp, yp+ys/2); */
    trace_new = create_trace (xs, ys/2, xp, yp+ys/2);

    /* Ask for a file in the new window */
    XSync(global->display,0);
    trace_read_cb (NULL, trace_new);
    }

void trace_close_cb(w,trace)
    Widget		w;
    TRACE		*trace;
{
    TRACE	*trace_ptr;

    if (DTPRINT) printf("In trace_close - trace=%d\n",trace);

    /* clear the screen */
    XClearWindow(global->display, trace->wind);
    /* Nail the child */
    /* XtUnmanageChild(trace->toplevel); */
    /* free memory associated with the data */
    free_data(trace);
    /* destroy all the widgets created for the screen */
    XtDestroyWidget(trace->toplevel);
    /* free the display structure */
    XtFree(trace);

    /* relink pointers to ignore this trace */
    if (trace == global->trace_head)
	global->trace_head = trace->next_trace;
    for (trace_ptr = global->trace_head; trace_ptr; trace_ptr = trace_ptr->next_trace) {
	if (trace_ptr->next_trace == trace) trace_ptr->next_trace = trace->next_trace;
	}

    /* Update menus */
    set_menu_closes();
    }

void trace_clear_cb (w,trace)
    Widget		w;
    TRACE		*trace;
{
    TRACE	*trace_ptr;
    TRACE	*trace_next;

    if (DTPRINT) printf("In clear_trace - trace=%d\n",trace);

    /* nail all traces except for this window's */
    for (trace_ptr = global->trace_head; trace_ptr; ) {
	trace_next = trace_ptr->next_trace;
	if (trace_ptr != trace) {
	    trace_close_cb (w, trace_ptr);
	    }
	trace_ptr = trace_next;
	}

    /* clear the screen */
    XClearWindow(global->display, trace->wind);

    /* free memory associated with the data */
    free_data(trace);

    /* change the name on title bar back to the trace */
    change_title (trace);

    init_globals();
    }

void trace_exit_cb (w,trace)
    Widget		w;
    TRACE		*trace;
{
    TRACE		*trace_next;

    if (DTPRINT) printf("In trace_exit_cb - trace=%d\n",trace);

    for (trace = global->trace_head; trace; ) {
	trace_next = trace->next_trace;
	trace_close_cb (w, trace);
	trace = trace_next;
	}

    XtFree(global);

    /* all done */
    exit(1);
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

    global->goto_color = -1;

    for (i=0; i<MAX_SRCH; i++) {
	/* Value */
	global->val_srch[i].color = global->val_srch[i].cursor =
	    global->val_srch[i].value[0] = global->val_srch[i].value[1] =
		global->val_srch[i].value[2] = 0;

	/* Signal */
	global->sig_srch[i].color = 0;
	global->sig_srch[i].string[0] = '\0';
	}
    }

void create_globals(argc,argv,res)
    int		argc;
    char	**argv;
    int		res;
{
    int		argc_copy;
    char	**argv_copy;

    /* alloc space for globals block */
    global = (GLOBAL *)XtMalloc( sizeof(GLOBAL) );
    global->trace_head = NULL;
    global->argc = argc;
    global->argv = argv;
    global->res = RES_SCALE/(float)res;
    global->directory[0] = '\0';
    init_globals ();

    XtToolkitInitialize();

    global->appcontext = XtCreateApplicationContext();

    /* Save parameters and open display */
    argc_copy = global->argc;
    argv_copy = (char **)XtMalloc (global->argc * sizeof (char *));
    memcpy (argv_copy, global->argv, global->argc * sizeof (char *));

    global->display = XtOpenDisplay (global->appcontext, NULL, NULL, "Dinotrace",
				    NULL, 0, &argc_copy, &argv_copy);

    if (global->display==NULL) {
	printf ("Can't open display\n");
	exit(0);
	}

    /*
     * Editor's Note: Thanks go to Sally C. Barry, former employee of DEC,
     * for her painstaking effort in the creation of the infamous 'Dino'
     * bitmap icons.
     */
    
    /*** create small dino pixmap from data ***/
    global->dpm = make_icon(global->display, DefaultRootWindow(global->display),
			    dino_icon_bits,dino_icon_width,dino_icon_height);    
    
    /*** create big dino pixmap from data ***/
    global->bdpm = make_icon(global->display, DefaultRootWindow(global->display),
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


/* Create a trace display and link it into the global information */

TRACE *create_trace (xs,ys,xp,yp)
    int		xs,ys,xp,yp;
{
    char	string[20];
    int		x1,x2;
    int		i;
    int		pd=0,pde=0,pds=0;
    TRACE	*trace;
    int		argc_copy;
    char	**argv_copy;
    XColor	xcolor,xcolor2;
    Colormap	cmap;
    
    /*** alloc space for trace to display state block ***/
    trace = (TRACE *)XtMalloc( sizeof(TRACE) );
    trace->next_trace = global->trace_head;
    global->trace_head = trace;
    
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
    
    /*** add pixmap and position trace->toplevel widget ***/
    XtSetArg(arglist[0], XtNallowShellResize, TRUE);
    XtSetArg(arglist[1], XtNiconPixmap, global->bdpm);
    XtSetArg(arglist[2], "iconifyPixmap", global->dpm);
    XtSetArg(arglist[3], XmNx, xp);
    XtSetArg(arglist[4], XmNy, yp);
    XtSetArg(arglist[5], XmNheight, ys);
    XtSetArg(arglist[6], XmNwidth, xs);
    XtSetValues(trace->toplevel, arglist, 7);

    /****************************************
     * create the main window
     ****************************************/
    XtSetArg(arglist[0], XmNx, 20);
    XtSetArg(arglist[1], XmNy, 350);
    /*CCCCCC XtSetArg(arglist[4], XmNacceptFocus, TRUE); */
    trace->main = XmCreateMainWindow(trace->toplevel,"main", arglist, 2);
#ifdef NOTDONE
    XtAddCallback(trace->main, XmNfocusCallback, cb_window_focus, trace);
#endif
    
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

    XtSetArg (arglist[0], XmNcolormap, &cmap);
    XtSetArg (arglist[1], XmNforeground, &(trace->xcolornums[0]));
    XtGetValues (trace->main, arglist, 2);
    trace->xcolornums[1] = XWhitePixel (global->display, 0);
    if (XAllocNamedColor (global->display, cmap, "Red", &xcolor, &xcolor2))
	trace->xcolornums[2] = xcolor.pixel;	else trace->xcolornums[2] = trace->xcolornums[1];
    if (XAllocNamedColor (global->display, cmap, "ForestGreen", &xcolor, &xcolor2))
	trace->xcolornums[3] = xcolor.pixel;	else trace->xcolornums[3] = trace->xcolornums[1];
    if (XAllocNamedColor (global->display, cmap, "Blue", &xcolor, &xcolor2))
	trace->xcolornums[4] = xcolor.pixel;	else trace->xcolornums[4] = trace->xcolornums[1];
    if (XAllocNamedColor (global->display, cmap, "Magenta", &xcolor, &xcolor2))
	trace->xcolornums[5] = xcolor.pixel;	else trace->xcolornums[5] = trace->xcolornums[1];
    if (XAllocNamedColor (global->display, cmap, "Cyan", &xcolor, &xcolor2))
	trace->xcolornums[6] = xcolor.pixel;	else trace->xcolornums[6] = trace->xcolornums[1];
    if (XAllocNamedColor (global->display, cmap, "Yellow", &xcolor, &xcolor2))
	trace->xcolornums[7] = xcolor.pixel;	else trace->xcolornums[7] = trace->xcolornums[1];
    if (XAllocNamedColor (global->display, cmap, "Salmon", &xcolor, &xcolor2))
	trace->xcolornums[8] = xcolor.pixel;	else trace->xcolornums[8] = trace->xcolornums[1];
    if (XAllocNamedColor (global->display, cmap, "NavyBlue", &xcolor, &xcolor2))
	trace->xcolornums[9] = xcolor.pixel;	else trace->xcolornums[9] = trace->xcolornums[1];

    /*
    for (i=0; i<63; i++) {
	xcolor.pixel = i;
	XQueryColor (global->display, cmap, &xcolor);
	printf ("%d) = %x, %x, %x\n", i, xcolor.red, xcolor.green, xcolor.blue);
	}
	*/

    /****************************************
     * create the menu bar
     ****************************************/
    trace->menu.menu = XmCreateMenuBar(trace->main,"menu", NULL, 0);
    XtManageChild(trace->menu.menu);
    
    /*** create a pulldownmenu on the top bar ***/
#define	dt_menu_title(title,key) \
    pde++; \
	trace->menu.pdmenu[pde] = XmCreatePulldownMenu(trace->menu.menu,"",NULL,0); \
	    XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple(title) ); \
		XtSetArg(arglist[1], XmNsubMenuId, trace->menu.pdmenu[pde] ); \
		    XtSetArg(arglist[2], XmNmnemonic, key ); \
			trace->menu.pdmenubutton[pde] = XmCreateCascadeButton \
			    (trace->menu.menu, "", arglist, 3); \
				XtManageChild(trace->menu.pdmenubutton[pde])
				
    /*** create a pulldownmenu entry under the top bar ***/
#define	dt_menu_entry(title,key,callback) \
    pd++; \
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple(title) ); \
	    XtSetArg(arglist[1], XmNmnemonic, key ); \
		trace->menu.pdentrybutton[pd] = XmCreatePushButtonGadget \
		    (trace->menu.pdmenu[pde], "", arglist, 2); \
			XtAddCallback(trace->menu.pdentrybutton[pd], XmNactivateCallback, callback, trace); \
			    XtManageChild(trace->menu.pdentrybutton[pd])

    /*** create a pulldownmenu entry which is a sub menu ***/
#define	dt_menu_subtitle(title,key) \
    pd++; \
	trace->menu.pdentry[pd] = XmCreatePulldownMenu(trace->menu.pdmenu[pde],"",NULL,0); \
	    XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple(title) ); \
		XtSetArg(arglist[1], XmNsubMenuId, trace->menu.pdentry[pd] ); \
		    XtSetArg(arglist[2], XmNmnemonic, key ); \
			trace->menu.pdentrybutton[pd] = XmCreateCascadeButton \
			    (trace->menu.pdmenu[pde], "", arglist, 3); \
				XtManageChild(trace->menu.pdentrybutton[pd])
				
    /*** create a pulldownmenu entry under a subtitle ***/
#define	dt_menu_subentry(title,key,callback) \
    pds++; \
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple(title) ); \
	    XtSetArg(arglist[1], XmNmnemonic, key ); \
		trace->menu.pdsubbutton[pds] = XmCreatePushButtonGadget \
		    (trace->menu.pdentry[pd], "", arglist, 2); \
			XtAddCallback(trace->menu.pdsubbutton[pds], XmNactivateCallback, callback, trace); \
			    XtManageChild(trace->menu.pdsubbutton[pds])

    /*** create a pulldownmenu entry under a subtitle (uses special colors) ***/
#define	dt_menu_subentry_c(color, callback) \
    pds++; \
	XtSetArg(arglist[0], XmNbackground, color ); \
	    XtSetArg(arglist[1], XmNmarginRight, 50); \
		XtSetArg(arglist[2], XmNmarginBottom, 8); \
		    trace->menu.pdsubbutton[pds] = XmCreatePushButton \
			(trace->menu.pdentry[pd], "", arglist, 3); \
			    XtAddCallback(trace->menu.pdsubbutton[pds], XmNactivateCallback, callback, trace); \
				XtManageChild(trace->menu.pdsubbutton[pds])

    /*** begin with -1 pde, since new menu will increment ***/
    pd = pds = pde = -1;

    dt_menu_title ("Trace", 'T');
    dt_menu_entry	("Read",	'R',	trace_read_cb);
    dt_menu_entry	("ReRead",	'e',	trace_reread_cb);
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
    /* dt_menu_entry	("Reset", 	'R',	sig_reset_cb); */
    dt_menu_entry	("Cancel", 	'l',	cancel_all_events);

    dt_menu_title ("Value", 'V');
    dt_menu_entry	("Examine",	'E',	val_examine_cb);
    dt_menu_entry	("Search",	'S',	val_search_cb);
    dt_menu_entry	("Cancel", 	'l',	cancel_all_events);

    /*
    dt_menu_title ("Note", 'N');
    dt_menu_entry	("Add", );
    dt_menu_entry	("Move", );
    dt_menu_entry	("Delete", );
    dt_menu_entry	("Clear", );
    dt_menu_entry	("Cancel", 	'L',	cancel_all_events);
    */
    dt_menu_title ("Print", 'P');
    dt_menu_entry	("Print",	'P',	ps_dialog);
    dt_menu_entry	("Reset",	'e',	ps_reset);
    
    if (DTDEBUG) {
	dt_menu_title ("Debug", 'D');
	dt_menu_entry	("Print Signal Names", 'N', print_sig_names);
	dt_menu_entry	("Print Signal Info (Screen Only)", 'I', print_screen_traces);
	}

    dt_menu_title ("Help", 'H');
    dt_menu_entry	("On Version",	'V',	help_cb);
    XtSetArg (arglist[0], XmNmenuHelpWidget, trace->menu.pdmenubutton[pde]);
    XtSetValues (trace->menu.menu, arglist, 1);

    /****************************************
     * create the command widget
     ****************************************/
    XtSetArg(arglist[0], XmNresizePolicy, XmRESIZE_NONE);
    trace->command.command = XmCreateForm(trace->main, "form", arglist, 1);
    XtManageChild(trace->command.command);
    
    /*** create begin button in command region ***/
    XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("Begin") );
    XtSetArg(arglist[1], XmNleftAttachment, XmATTACH_FORM );
    XtSetArg(arglist[2], XmNleftOffset, 7);
    XtSetArg(arglist[3], XmNbottomAttachment, XmATTACH_FORM );
    trace->command.begin_but = XmCreatePushButton(trace->command.command, "begin", arglist, 4);
    XtAddCallback(trace->command.begin_but, XmNactivateCallback, cb_begin, trace);
    XtManageChild(trace->command.begin_but);
    
    /*** create goto button in command region ***/
    XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("Goto") );
    XtSetArg(arglist[1], XmNleftAttachment, XmATTACH_WIDGET );
    XtSetArg(arglist[2], XmNleftOffset, 7);
    XtSetArg(arglist[3], XmNbottomAttachment, XmATTACH_FORM );
    XtSetArg(arglist[4], XmNleftWidget, trace->command.begin_but);
    trace->command.goto_but = XmCreatePushButton(trace->command.command, "goto", arglist, 5);
    XtAddCallback(trace->command.goto_but, XmNactivateCallback, win_goto_cb, trace);
    XtManageChild(trace->command.goto_but);
    
    /*** create end button in command region ***/
    XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("End") );
    XtSetArg(arglist[1], XmNrightAttachment, XmATTACH_FORM );
    XtSetArg(arglist[2], XmNrightOffset, 15);
    XtSetArg(arglist[3], XmNbottomAttachment, XmATTACH_FORM );
    trace->command.end_but = XmCreatePushButton(trace->command.command, "end", arglist, 4);
    XtAddCallback(trace->command.end_but, XmNactivateCallback, cb_end, trace);
    XtManageChild(trace->command.end_but);
    
    /*** create resolution button in command region ***/
    XtSetArg(arglist[0], XmNleftAttachment, XmATTACH_POSITION );
    XtSetArg(arglist[1], XmNleftPosition, 45);
    XtSetArg(arglist[2], XmNbottomAttachment, XmATTACH_FORM );
    XtSetArg(arglist[3], XmNlabelString, XmStringCreateSimple("Res") );
    trace->command.reschg_but = XmCreatePushButton(trace->command.command, "res", arglist, 4);
    XtAddCallback(trace->command.reschg_but, XmNactivateCallback, cb_chg_res, trace);
    XtManageChild(trace->command.reschg_but);
    
    /* create the vertical scroll bar */
    XtSetArg(arglist[0], XmNorientation, XmVERTICAL );
    XtSetArg(arglist[1], XmNrightAttachment, XmATTACH_FORM );
    XtSetArg(arglist[2], XmNtopAttachment, XmATTACH_FORM );
    XtSetArg(arglist[3], XmNbottomAttachment, XmATTACH_FORM );
    XtSetArg(arglist[4], XmNbottomOffset, 40);
    XtSetArg(arglist[5], XmNwidth, 18);
    trace->vscroll = XmCreateScrollBar( trace->command.command, "vscroll", arglist, 6);
    XtAddCallback(trace->vscroll, XmNincrementCallback, vscroll_unitinc, trace);
    XtAddCallback(trace->vscroll, XmNdecrementCallback, vscroll_unitdec, trace);
    XtAddCallback(trace->vscroll, XmNdragCallback, vscroll_drag, trace);
    XtAddCallback(trace->vscroll, XmNtoBottomCallback, vscroll_bot, trace);
    XtAddCallback(trace->vscroll, XmNtoTopCallback, vscroll_top, trace);
    XtAddCallback(trace->vscroll, XmNpageIncrementCallback, vscroll_pageinc, trace);
    XtAddCallback(trace->vscroll, XmNpageDecrementCallback, vscroll_pagedec, trace);
    XtManageChild(trace->vscroll);

    /* create the horizontal scroll bar */
    XtSetArg(arglist[0], XmNorientation, XmHORIZONTAL );
    XtSetArg(arglist[1], XmNbottomAttachment, XmATTACH_WIDGET );
    XtSetArg(arglist[2], XmNbottomWidget, trace->command.end_but);
    XtSetArg(arglist[3], XmNleftAttachment, XmATTACH_FORM );
    XtSetArg(arglist[4], XmNrightAttachment, XmATTACH_WIDGET );
    XtSetArg(arglist[5], XmNrightWidget, trace->vscroll);
    XtSetArg(arglist[6], XmNheight, 18);

    trace->hscroll = XmCreateScrollBar( trace->command.command, "hscroll", arglist, 7);
    XtAddCallback(trace->hscroll, XmNincrementCallback, hscroll_unitinc, trace);
    XtAddCallback(trace->hscroll, XmNdecrementCallback, hscroll_unitdec, trace);
    XtAddCallback(trace->hscroll, XmNdragCallback, hscroll_drag, trace);
    XtAddCallback(trace->hscroll, XmNtoBottomCallback, hscroll_bot, trace);
    XtAddCallback(trace->hscroll, XmNtoTopCallback, hscroll_top, trace);
    XtAddCallback(trace->hscroll, XmNpageIncrementCallback, hscroll_pageinc, trace);
    XtAddCallback(trace->hscroll, XmNpageDecrementCallback, hscroll_pagedec, trace);
    XtManageChild(trace->hscroll);
    
    /*** create full button in command region ***/
    XtSetArg(arglist[0], XmNrightAttachment, XmATTACH_WIDGET );
    XtSetArg(arglist[1], XmNrightWidget, trace->command.reschg_but);
    XtSetArg(arglist[2], XmNrightOffset, 2);
    XtSetArg(arglist[3], XmNlabelString, XmStringCreateSimple("Full") );
    XtSetArg(arglist[4], XmNbottomAttachment, XmATTACH_FORM );
    trace->command.resfull_but = XmCreatePushButton(trace->command.command, "full", arglist, 5);
    XtAddCallback(trace->command.resfull_but, XmNactivateCallback, cb_full_res, trace);
    XtManageChild(trace->command.resfull_but);
    
    /*** create zoom button in command region ***/
    XtSetArg(arglist[0], XmNleftAttachment, XmATTACH_WIDGET );
    XtSetArg(arglist[1], XmNleftWidget, trace->command.reschg_but);
    XtSetArg(arglist[2], XmNleftOffset, 2);
    XtSetArg(arglist[3], XmNlabelString, XmStringCreateSimple("Zoom") );
    XtSetArg(arglist[4], XmNbottomAttachment, XmATTACH_FORM );
    trace->command.reszoom_but = XmCreatePushButton(trace->command.command, "zoom", arglist, 5);
    XtAddCallback(trace->command.reszoom_but, XmNactivateCallback, cb_zoom_res, trace);
    XtManageChild(trace->command.reszoom_but);

    /*** create resolution decrease button in command region ***/
    XtSetArg(arglist[0], XmNrightAttachment, XmATTACH_WIDGET );
    XtSetArg(arglist[1], XmNrightWidget, trace->command.resfull_but);
    XtSetArg(arglist[2], XmNrightOffset, 2);
    XtSetArg(arglist[3], XmNheight, 32);
    XtSetArg(arglist[4], XmNbottomAttachment, XmATTACH_FORM );
    XtSetArg(arglist[5], XmNarrowDirection, XmARROW_LEFT );
    trace->command.resdec_but = XmCreateArrowButton(trace->command.command, "decres", arglist, 6);
    XtAddCallback(trace->command.resdec_but, XmNactivateCallback, cb_dec_res, trace);
    XtManageChild(trace->command.resdec_but);
    
    /*** create resolution increase button in command region ***/
    XtSetArg(arglist[0], XmNleftAttachment, XmATTACH_WIDGET );
    XtSetArg(arglist[1], XmNleftWidget, trace->command.reszoom_but);
    XtSetArg(arglist[2], XmNleftOffset, 2);
    XtSetArg(arglist[3], XmNheight, 32);
    XtSetArg(arglist[4], XmNbottomAttachment, XmATTACH_FORM );
    XtSetArg(arglist[5], XmNarrowDirection, XmARROW_RIGHT );
    trace->command.resinc_but = XmCreateArrowButton(trace->command.command, "incres", arglist, 6);
    XtAddCallback(trace->command.resinc_but, XmNactivateCallback, cb_inc_res, trace);
    XtManageChild(trace->command.resinc_but);
    
    /****************************************
     * create the work area window
     ****************************************/

    XtSetArg(arglist[0], XmNtopAttachment, XmATTACH_FORM );
    XtSetArg(arglist[1], XmNleftAttachment, XmATTACH_FORM );
    XtSetArg(arglist[2], XmNrightAttachment, XmATTACH_WIDGET );
    XtSetArg(arglist[3], XmNrightWidget, trace->vscroll); 
    XtSetArg(arglist[4], XmNbottomAttachment, XmATTACH_WIDGET );
    XtSetArg(arglist[5], XmNbottomWidget, trace->hscroll);
    trace->work = XmCreateDrawingArea(trace->command.command,"work", arglist, 6);

    XtAddCallback(trace->work, XmNexposeCallback, win_expose_cb, trace);
/*
    XtAddCallback(trace->work, XmNresizeCallback, win_expose_cb, trace);
*/
    XtManageChild(trace->work);
    
#ifdef NOTDONE    
    DwtMainSetAreas(trace->main,
                    trace->menu.menu,
                    trace->work,
                    trace->command.command,
                    trace->hscroll,
                    trace->vscroll);
#endif
    
    XtManageChild(trace->main);
    XtRealizeWidget(trace->toplevel);
    
    /* Initialize Various Parameters */
    trace->firstsig = NULL;
    trace->dispsig = NULL;
    trace->wind = XtWindow(trace->work);
    trace->custom.customize = NULL;
    trace->signal.add = NULL;
    trace->signal.search = NULL;
    trace->value.search = NULL;
    trace->prntscr.customize = NULL;
    trace->prompt_popup = NULL;
    trace->fileselect = NULL;
    trace->filename[0] = '\0';
    trace->loaded = 0;
    trace->numsig = 0;
    trace->numsigvis = 0;
    trace->numsigstart = 0;
    trace->busrep = HBUS;
    trace->ystart = 40;
    
    trace->signalstate_head = NULL;
    config_restore_defaults (trace);
    
    XGetGeometry( global->display, XtWindow(trace->work), &x1, &x1,
		 &x1, &x2, &x1, &x1, &x1);

    sprintf(string,"Res=%d ns",(int)(RES_SCALE/global->res) );
    XtSetArg(arglist[0],XmNlabelString,XmStringCreateSimple(string));
    XtSetValues(trace->command.reschg_but,arglist,1);
    
#ifdef NOTDONE
    /* Why is this needed? */
    xgcv.line_width = 0;
    arglist[0].name = XmNforeground;
    arglist[0].value = (int)&fore;
    arglist[1].name = XmNbackground;
    arglist[1].value = (int)&back;
    XtGetValues(trace->work,arglist,2);
    
    xgcv.foreground = fore;
    xgcv.background = back;
    
    XtSetArg(arglist[0],XmNbackground,back);
    XtSetValues(trace->work,arglist,1);
    trace->gc = XCreateGC(global->display,trace->wind,
			  GCLineWidth|GCForeground|GCBackground,&xgcv);
#endif    

    trace->gc = XCreateGC(global->display,trace->wind, NULL, NULL);

    /* get font information */
    trace->text_font = XQueryFont(global->display,XGContextFromGC(trace->gc));
    
    set_cursor (trace, DC_NORMAL);

    get_geometry(trace);

    set_menu_closes();

    return (trace);
    }


