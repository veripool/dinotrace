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
#include <X11/CursorFont.h>
#include <X11/Xm.h>

#include "dinotrace.h"
#include "callbacks.h"
#include "dino.bit"
#include "bigdino.bit"


void set_cursor (trace, cursor_num)
    TRACE		*trace;		/* Display information */
    int		cursor_num;		/* Entry in xcursors to display */
{
    XDefineCursor (trace->display, trace->wind, trace->xcursors[cursor_num]);
    }



void create_display(argc,argv,xs,ys,xp,yp,res,start_filename)
    int		argc;
    char		**argv;
    int		xs,ys,xp,yp,res;
    char *start_filename;
{
    char	title[24],data[10],string[20];
    int		fore,back,pd=0,pde=0,i;
    XImage	ximage;
    TRACE	*trace;
    
    /*** alloc space for trace to display state block ***/
    trace = (TRACE *)malloc( sizeof(TRACE) );
    curptr = trace;
    
    /*** create toplevel widget ***/
    toplevel = XtInitialize(DTVERSION, "dinotrace", NULL, 0, &argc, argv);
    change_title (trace);
    
    /*
     * Editor's Note: Thanks go to Sally C. Barry, former employee of DEC,
     * for her painstaking effort in the creation of the infamous 'Dino'
     * bitmap icons.
     */
    
    /*** create small dino pixmap from data ***/
    dpm = make_icon(XtDisplay(toplevel),DefaultRootWindow(XtDisplay(toplevel)),
		    dino_icon_bits,dino_icon_width,dino_icon_height);    
    
    /*** create big dino pixmap from data ***/
    bdpm = make_icon(XtDisplay(toplevel),DefaultRootWindow(XtDisplay(toplevel)),
		     bigdino_icon_bits,bigdino_icon_width,bigdino_icon_height);    
    
    /*** add pixmap and position toplevel widget ***/
    XtSetArg(arglist[0], XtNallowShellResize, TRUE);
    XtSetArg(arglist[1], XtNiconPixmap, bdpm);
    XtSetArg(arglist[2], "iconifyPixmap", dpm);
    XtSetArg(arglist[3], XmNx, xp);
    XtSetArg(arglist[4], XmNy, yp);
    XtSetValues(toplevel, arglist, 5);
    
    /****************************************
     * create the main window
     ****************************************/
    XtSetArg(arglist[0], XmNheight, ys);
    XtSetArg(arglist[1], XmNwidth, xs);
    XtSetArg(arglist[2], XmNx, 20);
    XtSetArg(arglist[3], XmNy, 350);
    /*CCCCCC XtSetArg(arglist[4], XmNacceptFocus, TRUE); */
    trace->main = XmCreateMainWindow(toplevel,"main", arglist, 4);
#ifdef NOTDONE
    XtAddCallback(trace->main, XmNfocusCallback, cb_window_focus, trace);
#endif
    
    /****************************************
     * create the menu bar
     ****************************************/
    trace->menu.menu = XmCreateMenuBar(trace->main,"menu", NULL, 0);
    XtManageChild(trace->menu.menu);
    
    /*** create a pulldownmenu widget ***/
#define	dt_menu_title(title) \
    pde++; \
	trace->menu.pulldownmenu[pde] = XmCreatePulldownMenu(trace->menu.menu,"",NULL,0); \
	    XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple(title) ); \
		XtSetArg(arglist[1], XmNsubMenuId, trace->menu.pulldownmenu[pde] ); \
		    trace->menu.pulldownentry[pde] = XmCreateCascadeButton \
			(trace->menu.menu, "", arglist, 2); \
			    XtManageChild(trace->menu.pulldownentry[pde])
    
    /*** create a pulldownmenu widget ***/
#define	dt_menu_entry(title,callback) \
    XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple(title) ); \
	trace->menu.pulldown[pd] = XmCreatePushButtonGadget \
	    (trace->menu.pulldownmenu[pde], "", arglist, 1); \
		XtAddCallback(trace->menu.pulldown[pd], XmNactivateCallback, callback, trace); \
		    XtManageChild(trace->menu.pulldown[pd]); \
			pd++

    /*** begin with -1 pde, since new menu will increment ***/
    pde = -1;

    dt_menu_title ("Display");
    dt_menu_entry	("Read", cb_read_trace);
    dt_menu_entry	("ReRead", cb_reread_trace);
    dt_menu_entry	("Clear", clear_display);
    dt_menu_entry	("Exit", delete_display);
    dt_menu_title ("Customize");
    dt_menu_entry	("Change", cus_dialog_cb);
    dt_menu_entry	("ReRead", cus_reread_cb);
    dt_menu_entry	("Restore", cus_restore_cb);
    dt_menu_title ("Cursor");
    dt_menu_entry	("Add", cur_add_cb);
    dt_menu_entry	("Move", cur_mov_cb);
    dt_menu_entry	("Delete", cur_del_cb);
    dt_menu_entry	("Clear", cur_clr_cb);
    dt_menu_entry	("Cancel", cancel_all_events);
    dt_menu_title ("Grid");
    dt_menu_entry	("Res", grid_res_cb);
    dt_menu_entry	("Align", grid_align_cb);
    dt_menu_entry	("Reset", grid_reset_cb);
    dt_menu_entry	("Cancel", cancel_all_events);
    dt_menu_title ("Signal");
    dt_menu_entry	("Add", sig_add_cb);
    dt_menu_entry	("Move", sig_mov_cb);
    dt_menu_entry	("Delete", sig_del_cb);
    dt_menu_entry	("Highlight", sig_highlight_cb);
    dt_menu_entry	("Reset", sig_reset_cb);
    dt_menu_entry	("Cancel", cancel_all_events);
    /*
    dt_menu_title ("Note");
    dt_menu_entry	("Add", );
    dt_menu_entry	("Move", );
    dt_menu_entry	("Delete", );
    dt_menu_entry	("Clear", );
    dt_menu_entry	("Cancel", );
    */
    dt_menu_title ("Printscreen");
    dt_menu_entry	("Print", ps_dialog);
    dt_menu_entry	("Reset", ps_reset);
    
    if (DTDEBUG) {
	dt_menu_title ("Debug");
	dt_menu_entry	("Print Signal Names", print_sig_names);
	dt_menu_entry	("Print Signal Info (Screen Only)", print_screen_traces);
	}
    
    /****************************************
     * create the command widget
     ****************************************/
    trace->command.command = XmCreateForm(trace->main, "form", arglist, 0);
    XtManageChild(trace->command.command);
    
    /*** create begin button in command region ***/
    XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("Begin") );
    XtSetArg(arglist[1], XmNleftAttachment, XmATTACH_FORM );
    XtSetArg(arglist[2], XmNleftOffset, 7);
    XtSetArg(arglist[3], XmNbottomAttachment, XmATTACH_FORM );
    trace->command.begin_but = XmCreatePushButton(trace->command.command, "begin", arglist, 4);
    XtAddCallback(trace->command.begin_but, XmNactivateCallback, cb_begin, trace);
    XtManageChild(trace->command.begin_but);
    
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
    XtSetArg(arglist[3], XmNheight, 33);
    XtSetArg(arglist[4], XmNbottomAttachment, XmATTACH_FORM );
    XtSetArg(arglist[5], XmNarrowDirection, XmARROW_LEFT );
    trace->command.resdec_but = XmCreateArrowButton(trace->command.command, "decres", arglist, 6);
    XtAddCallback(trace->command.resdec_but, XmNactivateCallback, cb_dec_res, trace);
    XtManageChild(trace->command.resdec_but);
    
    /*** create resolution increase button in command region ***/
    XtSetArg(arglist[0], XmNleftAttachment, XmATTACH_WIDGET );
    XtSetArg(arglist[1], XmNleftWidget, trace->command.reszoom_but);
    XtSetArg(arglist[2], XmNleftOffset, 2);
    XtSetArg(arglist[3], XmNheight, 33);
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

    XtAddCallback(trace->work, XmNexposeCallback, cb_window_expose, trace);
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
    XtRealizeWidget(toplevel);
    
    /* Initialize Various Parameters */
    trace->firstsig = NULL;
    trace->dispsig = NULL;
    trace->delsig = NULL;
    trace->display = XtDisplay(toplevel);
    trace->wind = XtWindow(trace->work);
    trace->custom.customize = NULL;
    trace->signal.customize = NULL;
    trace->prntscr.customize = NULL;
    trace->prompt_popup = NULL;
    trace->fileselect = NULL;
    trace->filename[0] = '\0';
    trace->loaded = 0;
    trace->numsig = 0;
    trace->numsigvis = 0;
    trace->numsigdel = 0;
    trace->numsigstart = 0;
    trace->time = 0;
    trace->busrep = HBUS;
    trace->xstart = 200;
    trace->ystart = 40;
    
    trace->signalstate_head = NULL;
    config_restore_defaults (trace);
    
    XGetGeometry( XtDisplay(toplevel), XtWindow(trace->work), &x1, &x1,
		 &x1, &x2, &x1, &x1, &x1);

    trace->res = ((float)(x2-trace->xstart))/(float)res;
    sprintf(string,"Res=%d ns",res);
    XtSetArg(arglist[0],XmNlabelString,XmStringCreateSimple(string));
    XtSetValues(trace->command.reschg_but,arglist,1);
    
#ifdef NOTDONE
    /* What does this do? */
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
#endif    

    trace->gc = XCreateGC(trace->display,trace->wind,
			  GCLineWidth|GCForeground|GCBackground,&xgcv);
    
    /* Define cursors */
    trace->xcursors[0] = XCreateFontCursor (trace->display, XC_top_left_arrow);
    trace->xcursors[1] = XCreateFontCursor (trace->display, XC_watch);
    trace->xcursors[2] = XCreateFontCursor (trace->display, XC_sb_left_arrow);
    trace->xcursors[3] = XCreateFontCursor (trace->display, XC_sb_right_arrow);
    trace->xcursors[4] = XCreateFontCursor (trace->display, XC_hand1);
    trace->xcursors[5] = XCreateFontCursor (trace->display, XC_center_ptr);
    trace->xcursors[6] = XCreateFontCursor (trace->display, XC_sb_h_double_arrow);
    trace->xcursors[7] = XCreateFontCursor (trace->display, XC_X_cursor);
    trace->xcursors[8] = XCreateFontCursor (trace->display, XC_left_side);
    trace->xcursors[9] = XCreateFontCursor (trace->display, XC_right_side);
    trace->xcursors[10] = XCreateFontCursor (trace->display, XC_spraycan);
    set_cursor (trace, DC_NORMAL);
    
    /* get font information */
    trace->text_font = XQueryFont(trace->display,XGContextFromGC(trace->gc));
    
    /* get color information */
    arglist[0].name = XmNforeground;
    arglist[0].value = (int)&(trace->xcolornums[0]);
    XtGetValues(trace->main,arglist,1);
    trace->xcolornums[1] = XWhitePixel (trace->display, 0);

    get_geometry(trace);
    
    /* Load up the file on the command line, if any */
    if (start_filename != NULL) {
	XSync(trace->display,0);
	strcpy (trace->filename, start_filename);
	cb_fil_read (trace);
	}
    }
