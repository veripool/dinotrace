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


#include <X11/DECwDwtApplProg.h>
#include <X11/DwtAppl.h>
#include <X11/Xlib.h>

#include "dinotrace.h"
#include "callbacks.h"
#include "dino.bit"
#include "bigdino.bit"
#include "arrow.bit"



void
create_display(argc,argv,xs,ys,xp,yp,res,start_filename)
    int		argc;
    char		**argv;
    int		xs,ys,xp,yp,res;
    char *start_filename;
{
    char	title[24],data[10],string[20];
    int		fore,back,pd=0,pde=0,i;
    XImage	ximage;

    /* initialize to NULL string */
    string[0] = '\0';

    /*** alloc space for ptr to display state block ***/
    ptr = (DISPLAY_SB *)malloc( sizeof(DISPLAY_SB) );
    curptr = ptr;

    /*** create toplevel widget ***/
    toplevel = XtInitialize(DTVERSION, "", NULL, 0, &argc, argv);
    change_title (ptr);

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
    XtSetArg(arglist[3], DwtNx, xp);
    XtSetArg(arglist[4], DwtNy, yp);
    XtSetValues(toplevel, arglist, 5);

    /****************************************
    * create the main window
    ****************************************/
    XtSetArg(arglist[0], DwtNheight, ys);
    XtSetArg(arglist[1], DwtNwidth, xs);
    XtSetArg(arglist[2], DwtNx, 20);
    XtSetArg(arglist[3], DwtNy, 350);
    XtSetArg(arglist[4], DwtNacceptFocus, TRUE);
    win_foc_cb[0].tag = (int)ptr;
    XtSetArg(arglist[5], DwtNfocusCallback, win_foc_cb);
    ptr->main = DwtMainWindowCreate(toplevel,"", arglist, 6);

    /****************************************
    * create the menu bar
    ****************************************/
    XtSetArg(arglist[0], DwtNorientation, DwtOrientationHorizontal );
    ptr->menu.menu = DwtMenuBarCreate(ptr->main,"", arglist, 1);

	/*** create 'File' pulldownmenu widget ***/
	ptr->menu.pulldownmenu[pde] = DwtMenuPulldownCreate(ptr->menu.menu,"",NULL,0);

	/*** create 'File' pulldownentry widget ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Display") );
	XtSetArg(arglist[1], DwtNsubMenuId, ptr->menu.pulldownmenu[pde] );
	ptr->menu.pulldownentry[pde] = DwtPullDownMenuEntryCreate(
                        ptr->menu.menu, "", arglist, 2);

	/*** create 'File' pulldown widget 'Read' ***/        
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Read") );
	rd_tr_cb[0].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, rd_tr_cb);
	ptr->menu.pulldown[pd++] = DwtPushButtonCreate( 
                        ptr->menu.pulldownmenu[pde], "", arglist, 2);

	/*** create 'File' pulldown widget 'ReRead' ***/ 
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("ReRead") );
	rerd_tr_cb[0].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, rerd_tr_cb);
	ptr->menu.pulldown[pd++] = DwtPushButtonCreate( 
                        ptr->menu.pulldownmenu[pde], "", arglist, 2);

	/*** create 'File' pulldown widget 'Clear' ***/        
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Clear") );
	clr_tr_cb[0].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, clr_tr_cb);
	ptr->menu.pulldown[pd++] = DwtPushButtonCreate( 
                        ptr->menu.pulldownmenu[pde], "", arglist, 2);

	/*** create 'File' pulldown widget 'Exit' ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Exit") );
	del_tr_cb[0].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, del_tr_cb);
	ptr->menu.pulldown[pd++] = DwtPushButtonCreate( 
                        ptr->menu.pulldownmenu[pde], "", arglist, 2);

	/*** increment pulldownentry count ***/
	pde++;

	/*** create 'Customize' pulldownmenu widget ***/
	ptr->menu.pulldownmenu[pde] = DwtMenuPulldownCreate(ptr->menu.menu,"",NULL,0);

	/*** create 'Customize' pulldownentry widget ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Customize") );
	XtSetArg(arglist[1], DwtNsubMenuId, ptr->menu.pulldownmenu[pde] );
	ptr->menu.pulldownentry[pde] = DwtPullDownMenuEntryCreate( 
                        ptr->menu.menu, "", arglist, 2);

	/*** create 'Customize' pulldown widget 'Change' ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Change") );
	custom_cb[0].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, custom_cb);
	ptr->menu.pulldown[pd++] = DwtPushButtonCreate( 
                        ptr->menu.pulldownmenu[pde], "", arglist, 2);

	/*** create 'Customize' pulldown widget 'Read' ***/
    /*
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Read") );
	custom_cb[2].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, custom_cb+2);
	XtSetArg(arglist[2], DwtNsensitive, FALSE);
        ptr->menu.pulldown[pd++] =
	    DwtPushButtonCreate (ptr->menu.pulldownmenu[pde], "", arglist, 3);
	    */

	/*** create 'Customize' pulldown widget 'ReRead' ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("ReRead") );
	custom_cb[26].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, custom_cb+26);
        ptr->menu.pulldown[pd++] =
	    DwtPushButtonCreate (ptr->menu.pulldownmenu[pde], "", arglist, 2);

        /*** create 'Customize' pulldown widget 'Save' ***/
    /*
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Save") );
	custom_cb[4].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, custom_cb+4);
	XtSetArg(arglist[2], DwtNsensitive, FALSE);
	ptr->menu.pulldown[pd++] = DwtPushButtonCreate(
                        ptr->menu.pulldownmenu[pde], "", arglist, 3);
			*/

	/*** create 'Customize' pulldown widget 'Restore' ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Restore") );
	custom_cb[24].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, custom_cb+24);
	ptr->menu.pulldown[pd++] = DwtPushButtonCreate(
                        ptr->menu.pulldownmenu[pde], "", arglist, 2);

	/*** increment pulldownentry count ***/
	pde++;

	/*** create 'Cursor' pulldownmenu widget ***/
	ptr->menu.pulldownmenu[pde] = DwtMenuPulldownCreate(ptr->menu.menu,
		"",NULL,0);
                                                                  
	/*** create 'Cursor' pulldownentry widget ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Cursor") );
	XtSetArg(arglist[1], DwtNsubMenuId, ptr->menu.pulldownmenu[pde] );
	ptr->menu.pulldownentry[pde] = DwtPullDownMenuEntryCreate(
                        ptr->menu.menu, "", arglist, 2);

	/*** create 'Cursor' pulldown widget 'Add' ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Add") );
	cursor_cb[0].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, cursor_cb);
	XtSetArg(arglist[2], DwtNsensitive, TRUE);
	ptr->menu.pulldown[pd++] = DwtPushButtonCreate( 
                        ptr->menu.pulldownmenu[pde], "", arglist, 3);

	/*** create 'Cursor' pulldown widget 'Add' ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Move") );
	cursor_cb[2].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, cursor_cb+2);
	XtSetArg(arglist[2], DwtNsensitive, TRUE);
	ptr->menu.pulldown[pd++] = DwtPushButtonCreate( 
                        ptr->menu.pulldownmenu[pde], "", arglist, 3);

	/*** create 'Cursor' pulldown widget 'Delete' ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Delete") );
	cursor_cb[4].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, cursor_cb+4);
	XtSetArg(arglist[2], DwtNsensitive, TRUE);
	ptr->menu.pulldown[pd++] = DwtPushButtonCreate( 
                        ptr->menu.pulldownmenu[pde], "", arglist, 3);

	/*** create 'Cursor' pulldown widget 'Clear' ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Clear") );
	cursor_cb[6].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, cursor_cb+6);
	XtSetArg(arglist[2], DwtNsensitive, TRUE);
	ptr->menu.pulldown[pd++] = DwtPushButtonCreate( 
                        ptr->menu.pulldownmenu[pde], "", arglist, 3);

	/*** create 'Cursor' pulldown widget 'Cancel' ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Cancel") );
	cursor_cb[8].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, cursor_cb+8);
	XtSetArg(arglist[2], DwtNsensitive, TRUE);
	ptr->menu.pulldown[pd++] = DwtPushButtonCreate( 
                        ptr->menu.pulldownmenu[pde], "", arglist, 3);

	/*** increment pulldownentry count ***/
	pde++;

	/*** create 'Grid' pulldownmenu widget ***/
	ptr->menu.pulldownmenu[pde] = DwtMenuPulldownCreate(ptr->menu.menu,"",NULL,0);

	/*** create 'Grid' pulldownentry widget ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Grid") );
	XtSetArg(arglist[1], DwtNsubMenuId, ptr->menu.pulldownmenu[pde] );
	ptr->menu.pulldownentry[pde] = DwtPullDownMenuEntryCreate(
                        ptr->menu.menu, "", arglist, 2);

	/*** create 'Grid' pulldown widget 'Res' ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Res") );
	grid_cb[0].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, grid_cb);
	XtSetArg(arglist[2], DwtNsensitive, TRUE);
	ptr->menu.pulldown[pd++] = DwtPushButtonCreate( 
                        ptr->menu.pulldownmenu[pde], "", arglist, 3);

	/*** create 'Grid' pulldown widget 'Align' ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Align") );
	grid_cb[2].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, grid_cb+2);
	XtSetArg(arglist[2], DwtNsensitive, TRUE);
	ptr->menu.pulldown[pd++] = DwtPushButtonCreate( 
                        ptr->menu.pulldownmenu[pde], "", arglist, 3);

	/*** create 'Grid' pulldown widget 'Reset' ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Reset") );
	grid_cb[4].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, grid_cb+4);
	XtSetArg(arglist[2], DwtNsensitive, TRUE);
	ptr->menu.pulldown[pd++] = DwtPushButtonCreate( 
                        ptr->menu.pulldownmenu[pde], "", arglist, 3);

	/*** create 'Grid' pulldown widget 'Cancel' ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Cancel") );
	grid_cb[6].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, grid_cb+6);
	XtSetArg(arglist[2], DwtNsensitive, TRUE);
	ptr->menu.pulldown[pd++] = DwtPushButtonCreate( 
                        ptr->menu.pulldownmenu[pde], "", arglist, 3);

	/*** increment pulldownentry count ***/
	pde++;

	/*** create 'Signal' pulldownmenu widget ***/
	ptr->menu.pulldownmenu[pde] = DwtMenuPulldownCreate(ptr->menu.menu,"",NULL,0);

	/*** create 'Signal' pulldownentry widget ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Signal") );
	XtSetArg(arglist[1], DwtNsubMenuId, ptr->menu.pulldownmenu[pde] );
	ptr->menu.pulldownentry[pde] = DwtPullDownMenuEntryCreate(
                        ptr->menu.menu, "", arglist, 2);

	/*** create 'Signal' pulldown widget 'Add' ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Add") );
	signal_cb[0].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, signal_cb);
	XtSetArg(arglist[2], DwtNsensitive, TRUE);
	ptr->menu.pulldown[pd++] = DwtPushButtonCreate( 
                        ptr->menu.pulldownmenu[pde], "", arglist, 3);

	/*** create 'Signal' pulldown widget 'Move' ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Move") );
	signal_cb[2].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, signal_cb+2);
	XtSetArg(arglist[2], DwtNsensitive, TRUE);
	ptr->menu.pulldown[pd++] = DwtPushButtonCreate( 
	                    ptr->menu.pulldownmenu[pde], "", arglist, 3);

	/*** create 'Signal' pulldown widget 'Delete' ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Delete") );
	signal_cb[4].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, signal_cb+4);
	XtSetArg(arglist[2], DwtNsensitive, TRUE);
	ptr->menu.pulldown[pd++] = DwtPushButtonCreate( 
                        ptr->menu.pulldownmenu[pde], "", arglist, 3);

	/*** create 'Signal' pulldown widget 'Reset' ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Reset") );
	signal_cb[6].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, signal_cb+6);
	XtSetArg(arglist[2], DwtNsensitive, TRUE);
	ptr->menu.pulldown[pd++] = DwtPushButtonCreate( 
                        ptr->menu.pulldownmenu[pde], "", arglist, 3);

	/*** create 'Signal' pulldown widget 'Cancel' ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Cancel") );
	signal_cb[8].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, signal_cb+8);
	XtSetArg(arglist[2], DwtNsensitive, TRUE);
	ptr->menu.pulldown[pd++] = DwtPushButtonCreate( 
                        ptr->menu.pulldownmenu[pde], "", arglist, 3);

	/*** increment pulldownentry count ***/
	pde++;

	/*** create 'Note' pulldownmenu widget ***/
	ptr->menu.pulldownmenu[pde] = DwtMenuPulldownCreate(ptr->menu.menu,"",NULL,0);

	/*** create 'Note' pulldownentry widget ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Note") );
	XtSetArg(arglist[1], DwtNsubMenuId, ptr->menu.pulldownmenu[pde] );
	ptr->menu.pulldownentry[pde] = DwtPullDownMenuEntryCreate(
                        ptr->menu.menu, "", arglist, 2);

	/*** create 'Note' pulldown widget 'Add' ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Add") );
	note_cb[0].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, note_cb);
	XtSetArg(arglist[2], DwtNsensitive, FALSE);
	ptr->menu.pulldown[pd++] = DwtPushButtonCreate( 
                        ptr->menu.pulldownmenu[pde], "", arglist, 3);

	/*** create 'Note' pulldown widget 'Move' ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Move") );
	note_cb[0].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, note_cb);
	XtSetArg(arglist[2], DwtNsensitive, FALSE);
	ptr->menu.pulldown[pd++] = DwtPushButtonCreate( 
                        ptr->menu.pulldownmenu[pde], "", arglist, 3);

	/*** create 'Note' pulldown widget 'Delete' ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Delete") );
	note_cb[0].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, note_cb);
	XtSetArg(arglist[2], DwtNsensitive, FALSE);
	ptr->menu.pulldown[pd++] = DwtPushButtonCreate( 
                        ptr->menu.pulldownmenu[pde], "", arglist, 3);

	/*** create 'Note' pulldown widget 'Clear' ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Clear") );
	note_cb[0].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, note_cb);
	XtSetArg(arglist[2], DwtNsensitive, FALSE);
	ptr->menu.pulldown[pd++] = DwtPushButtonCreate( 
                        ptr->menu.pulldownmenu[pde], "", arglist, 3);

	/*** create 'Note' pulldown widget 'Cancel' ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Cancel") );
	note_cb[0].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, note_cb);
	XtSetArg(arglist[2], DwtNsensitive, FALSE);
	ptr->menu.pulldown[pd++] = DwtPushButtonCreate( 
                        ptr->menu.pulldownmenu[pde], "", arglist, 3);

	/*** increment pulldownentry count ***/
	pde++;

	/*** create 'Printscreen' pulldownmenu widget ***/
	ptr->menu.pulldownmenu[pde] = DwtMenuPulldownCreate(ptr->menu.menu,"",NULL,0);

	/*** create 'Printscreen' pulldownentry widget ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Printscreen") );
	XtSetArg(arglist[1], DwtNsubMenuId, ptr->menu.pulldownmenu[pde] );
	ptr->menu.pulldownentry[pde] = DwtPullDownMenuEntryCreate(
                        ptr->menu.menu, "", arglist, 2);

	/*** create 'Printscreen' pulldown widget 'Print' ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Print") );
	dino_cb[0].proc = ps_dialog;
	dino_cb[0].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, dino_cb);
	XtSetArg(arglist[2], DwtNsensitive, TRUE);
	ptr->menu.pulldown[pd++] = DwtPushButtonCreate( 
                        ptr->menu.pulldownmenu[pde], "", arglist, 3);

	/*** create 'Printscreen' pulldown widget 'Reset' ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Reset") );
	dino_cb[0].proc = ps_reset;
	dino_cb[0].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, dino_cb);
	XtSetArg(arglist[2], DwtNsensitive, TRUE);
	ptr->menu.pulldown[pd++] = DwtPushButtonCreate( 
                        ptr->menu.pulldownmenu[pde], "", arglist, 3);

	/*** increment pulldownentry count ***/
	pde++;

    if (DTDEBUG) {
	/*** create 'Debug' pulldownmenu widget ***/
	ptr->menu.pulldownmenu[pde] = DwtMenuPulldownCreate(ptr->menu.menu,
		"",NULL,0);

	/*** create 'Debug' pulldownentry widget ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Debug") );
	XtSetArg(arglist[1], DwtNsubMenuId, ptr->menu.pulldownmenu[pde] );
	ptr->menu.pulldownentry[pde] = DwtPullDownMenuEntryCreate(
                        ptr->menu.menu, "", arglist, 2);

	/*** create 'Debug' pulldown widget 'Print Signal Names' ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Print Signal Names") );
	print_sig_names_cb[0].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, print_sig_names_cb);
	XtSetArg(arglist[2], DwtNsensitive, TRUE);
	ptr->menu.pulldown[pd++] = DwtPushButtonCreate( 
                        ptr->menu.pulldownmenu[pde], "", arglist, 3);

	/*** create 'Debug' pulldown widget 'Print Signal Info' ***/
	XtSetArg(arglist[0], DwtNlabel,
		DwtLatin1String("Print Signal Info (Entire Trace)") );
	print_all_traces_cb[0].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, print_all_traces_cb);
	XtSetArg(arglist[2], DwtNsensitive, FALSE);
	ptr->menu.pulldown[pd++] = DwtPushButtonCreate( 
                        ptr->menu.pulldownmenu[pde], "", arglist, 3);

	/*** create 'Debug' pulldown widget 'Print Signal Info' ***/
	XtSetArg(arglist[0], DwtNlabel,
		DwtLatin1String("Print Signal Info (Screen Only)") );
	print_screen_traces_cb[0].tag = (int)ptr;
	XtSetArg(arglist[1], DwtNactivateCallback, print_screen_traces_cb);
	XtSetArg(arglist[2], DwtNsensitive, TRUE);
	ptr->menu.pulldown[pd++] = DwtPushButtonCreate( 
                        ptr->menu.pulldownmenu[pde], "", arglist, 3);

	/*** increment pulldownentry count ***/
	pde++;
	}

    /*** manage all menubar pulldown widgets ***/
    for (i=0;i<pd;i++)
	XtManageChild(ptr->menu.pulldown[i]);

    /*** manage all menubar pulldownentry widgets ***/
    for (i=0;i<pde;i++)
	XtManageChild(ptr->menu.pulldownentry[i]);

    /*** manage  menubar ***/
    XtManageChild(ptr->menu.menu);

    /****************************************
    * create the horizontal scroll bar
    ****************************************/
    XtSetArg(arglist[0], DwtNorientation, DwtOrientationHorizontal );
    hsc_inc_cb[0].tag = (int)ptr;
    XtSetArg(arglist[1], DwtNunitIncCallback, hsc_inc_cb);
    hsc_dec_cb[0].tag = (int)ptr;
    XtSetArg(arglist[2], DwtNunitDecCallback, hsc_dec_cb);
    hsc_drg_cb[0].tag = (int)ptr;
    XtSetArg(arglist[3], DwtNdragCallback, hsc_drg_cb);
    hsc_bot_cb[0].tag = (int)ptr;
    XtSetArg(arglist[4], DwtNtoBottomCallback, hsc_bot_cb);
    hsc_top_cb[0].tag = (int)ptr;
    XtSetArg(arglist[5], DwtNtoTopCallback, hsc_top_cb);
    hsc_pginc_cb[0].tag = (int)ptr;
    XtSetArg(arglist[6], DwtNpageIncCallback, hsc_pginc_cb);
    hsc_pgdec_cb[0].tag = (int)ptr;
    XtSetArg(arglist[7], DwtNpageDecCallback, hsc_pgdec_cb);
    ptr->hscroll = DwtScrollBarCreate( ptr->main, "", arglist, 8);
    XtManageChild(ptr->hscroll);

    /****************************************
    * create the vertical scroll bar
    ****************************************/
    XtSetArg(arglist[0], DwtNorientation, DwtOrientationVertical );
    vsc_inc_cb[0].tag = (int)ptr;
    XtSetArg(arglist[1], DwtNunitIncCallback, vsc_inc_cb);
    vsc_dec_cb[0].tag = (int)ptr;
    XtSetArg(arglist[2], DwtNunitDecCallback, vsc_dec_cb);
    vsc_drg_cb[0].tag = (int)ptr;
    XtSetArg(arglist[3], DwtNdragCallback, vsc_drg_cb);
    vsc_bot_cb[0].tag = (int)ptr;
    XtSetArg(arglist[4], DwtNtoBottomCallback, vsc_bot_cb);
    vsc_top_cb[0].tag = (int)ptr;
    XtSetArg(arglist[5], DwtNtoTopCallback, vsc_top_cb);
    vsc_pginc_cb[0].tag = (int)ptr;
    XtSetArg(arglist[6], DwtNpageIncCallback, vsc_pginc_cb);
    vsc_pgdec_cb[0].tag = (int)ptr;
    XtSetArg(arglist[7], DwtNpageDecCallback, vsc_pgdec_cb);
    ptr->vscroll = DwtScrollBarCreate( ptr->main, "", arglist, 8);
    XtManageChild(ptr->vscroll);

    /****************************************
    * create the command widget
    ****************************************/
    XtSetArg(arglist[0], DwtNlines, 3 );
    ptr->command.command = DwtAttachedDBCreate(ptr->main, "", arglist, 1);
    XtManageChild(ptr->command.command);

	/*** create begin button in command region ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Begin") );
	XtSetArg(arglist[1], DwtNadbLeftAttachment, DwtAttachAdb);
	XtSetArg(arglist[2], DwtNadbLeftOffset, 7);
	begin_cb[0].tag = (int)ptr;
	XtSetArg(arglist[3], DwtNactivateCallback, begin_cb );
	ptr->command.begin_but = DwtPushButtonCreate(ptr->command.command, "",
	    arglist, 4);
	XtManageChild(ptr->command.begin_but);

	/*** create end button in command region ***/
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("End") );
	XtSetArg(arglist[1], DwtNadbRightAttachment, DwtAttachAdb);
	XtSetArg(arglist[2], DwtNadbRightOffset, 7);
	end_cb[0].tag = (int)ptr;
	XtSetArg(arglist[3], DwtNactivateCallback, end_cb);
	ptr->command.end_but = DwtPushButtonCreate(ptr->command.command, "",
	    arglist, 4);
	XtManageChild(ptr->command.end_but);

	/*** create resolution button in command region ***/
	XtSetArg(arglist[0], DwtNadbLeftAttachment, DwtAttachPosition);
	XtSetArg(arglist[1], DwtNadbLeftPosition, 45);
	chg_res_cb[0].tag = (int)ptr;
	XtSetArg(arglist[2], DwtNactivateCallback, chg_res_cb );
	ptr->command.reschg_but = DwtPushButtonCreate(ptr->command.command, "",
	    arglist, 3);
	XtManageChild(ptr->command.reschg_but);

	/*** create full button in command region ***/
 	XtSetArg(arglist[0], DwtNadbRightAttachment, DwtAttachWidget);
	XtSetArg(arglist[1], DwtNadbRightWidget, ptr->command.reschg_but);
	XtSetArg(arglist[2], DwtNadbRightOffset, 2);
	full_res_cb[0].tag = (int)ptr;
	XtSetArg(arglist[3], DwtNactivateCallback, full_res_cb );
	XtSetArg(arglist[4], DwtNlabel, DwtLatin1String("Full") );
	ptr->command.resfull_but = DwtPushButtonCreate(ptr->command.command, "",
	    arglist, 5);
	XtManageChild(ptr->command.resfull_but);

	/*** create zoom button in command region ***/
	XtSetArg(arglist[0], DwtNadbLeftAttachment, DwtAttachWidget);
	XtSetArg(arglist[1], DwtNadbLeftWidget, ptr->command.reschg_but);
	XtSetArg(arglist[2], DwtNadbLeftOffset, 2);
	zoom_res_cb[0].tag = (int)ptr;
	XtSetArg(arglist[3], DwtNactivateCallback, zoom_res_cb );
	XtSetArg(arglist[4], DwtNlabel, DwtLatin1String("Zoom") );
	ptr->command.reszoom_but = DwtPushButtonCreate(ptr->command.command, "",
	    arglist, 5);
	XtManageChild(ptr->command.reszoom_but);

	/*** create resolution decrease button in command region ***/
 	XtSetArg(arglist[0], DwtNadbRightAttachment, DwtAttachWidget);
	XtSetArg(arglist[1], DwtNadbRightWidget, ptr->command.resfull_but);
	XtSetArg(arglist[2], DwtNadbRightOffset, 2);
	dec_res_cb[0].tag = (int)ptr;
	XtSetArg(arglist[3], DwtNactivateCallback, dec_res_cb );
	XtSetArg(arglist[4], DwtNwidth, 35);
	ptr->command.resdec_but = DwtPushButtonCreate(ptr->command.command, "",
	    arglist, 5);
	XtManageChild(ptr->command.resdec_but);

	/*** create resolution increase button in command region ***/
	XtSetArg(arglist[0], DwtNadbLeftAttachment, DwtAttachWidget);
	XtSetArg(arglist[1], DwtNadbLeftWidget, ptr->command.reszoom_but);
	XtSetArg(arglist[2], DwtNadbLeftOffset, 2);
	inc_res_cb[0].tag = (int)ptr;
	XtSetArg(arglist[3], DwtNactivateCallback, inc_res_cb );
	XtSetArg(arglist[4], DwtNwidth, 35);
	ptr->command.resinc_but = DwtPushButtonCreate(ptr->command.command, "",
	    arglist, 5);
	XtManageChild(ptr->command.resinc_but);

    /****************************************
    * create the work area window
    ****************************************/
    XtSetArg(arglist[0], DwtNheight, 500);
    XtSetArg(arglist[1], DwtNwidth, 500);
    XtSetArg(arglist[2], DwtNx, 500);
    win_exp_cb[0].tag = (int)ptr;
    XtSetArg(arglist[3], DwtNexposeCallback, win_exp_cb);
    ptr->work = DwtWindowCreate(ptr->main,"", arglist, 4);
    XtManageChild(ptr->work);

    DwtMainSetAreas(ptr->main,
                    ptr->menu.menu,
                    ptr->work,
                    ptr->command.command,
                    ptr->hscroll,
                    ptr->vscroll);

    XtManageChild(ptr->main);
    XtRealizeWidget(toplevel);

    /* Initialize Various Parameters */
    ptr->sig.forward = NULL;
    ptr->sig.backward = NULL;
    ptr->del.forward = NULL;
    ptr->del.backward = NULL;
    ptr->disp = XtDisplay(toplevel);
    ptr->wind = XtWindow(ptr->work);
    ptr->custom.customize = NULL;
    ptr->signal.customize = NULL;
    ptr->prntscr.customize = NULL;
    ptr->fileselect = NULL;
    ptr->filename[0] = '\0';
    ptr->loaded = 0;
    ptr->numsig = 0;
    ptr->numsigvis = 0;
    ptr->numsigdel = 0;
    ptr->sigstart = 0;
    ptr->time = 0;
    ptr->pageinc = FPAGE;
    ptr->busrep = HBUS;
    ptr->xstart = 200;
    ptr->ystart = 40;
    ptr->separator = 0;

    ptr->signalstate_head = NULL;
    config_restore_defaults (ptr);

    XGetGeometry( XtDisplay(toplevel), XtWindow(ptr->work), &x1, &x1,
	&x1, &x2, &x1, &x1, &x1);
    ptr->res = ((float)(x2-ptr->xstart))/(float)res;
    sprintf(data,"%d",res);
    strcat(string,"Res=");
    strcat(string,data);
    strcat(string," ns");
    XtSetArg(arglist[0],DwtNlabel,DwtLatin1String(string));
    XtSetValues(ptr->command.reschg_but,arglist,1);

    xgcv.line_width = 0;
    arglist[0].name = DwtNforeground;
    arglist[0].value = (int)&fore;
    arglist[1].name = DwtNbackground;
    arglist[1].value = (int)&back;
    XtGetValues(ptr->work,arglist,2);

    xgcv.foreground = fore;
    xgcv.background = back;

    XtSetArg(arglist[0],DwtNbackground,back);
    XtSetValues(ptr->work,arglist,1);

    ptr->gc = XCreateGC(ptr->disp,ptr->wind,
	GCLineWidth|GCForeground|GCBackground,&xgcv);

    get_geometry(ptr);

    /* get font information */
    ptr->text_font = XQueryFont(ptr->disp,XGContextFromGC(ptr->gc));


    /* create the arrow fonts */
    left_arrow = XCreatePixmap(ptr->disp,ptr->wind,arrow_width,arrow_height,
                        (unsigned int) DefaultDepth(ptr->disp, 0));

    right_arrow = XCreatePixmap(ptr->disp,ptr->wind,arrow_width,arrow_height,
                        (unsigned int) DefaultDepth(ptr->disp, 0));

    ximage.height = arrow_height;
    ximage.width = arrow_width;
    ximage.xoffset = 0;
    ximage.format = XYBitmap;
    ximage.data = (char *)arrow_left_bits;
    ximage.byte_order = MSBFirst;
    ximage.bitmap_unit = 16; 
    ximage.bitmap_bit_order = MSBFirst;
    ximage.bitmap_pad = 16;
    ximage.bytes_per_line = (arrow_width+15)/16 * 2;
    ximage.depth = 1;
    XPutImage(ptr->disp,left_arrow,ptr->gc,&ximage,0,0,0,0,arrow_width,arrow_height);

    ximage.data = (char *)arrow_right_bits;
    XPutImage(ptr->disp,right_arrow,ptr->gc,&ximage,0,0,0,0,arrow_width,arrow_height);

    XtSetArg(arglist[0], DwtNbackgroundPixmap, left_arrow);
    XtSetValues(ptr->command.resdec_but,arglist,1);

    XtSetArg(arglist[0], DwtNbackgroundPixmap, right_arrow);
    XtSetValues(ptr->command.resinc_but,arglist,1);

    /* Load up the file on the command line, if any */
    if (start_filename != NULL) {
	XSync(ptr->disp,0);
	strcpy (ptr->filename, start_filename);
	cb_fil_read (ptr);
	}
    }
