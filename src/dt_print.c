#ident "$Id$"
/******************************************************************************
 * dt_printscreen.c --- postscript output routines
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

#include "dinotrace.h"

#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include <Xm/Text.h>
#include <Xm/BulletinB.h>
#include <Xm/Scale.h>
#include <Xm/Label.h>
#include <Xm/Separator.h>

#include "functions.h"

#include "dt_post.h"

/**********************************************************************/

extern void	ps_drawsig(), ps_draw(), ps_draw_grid(), ps_draw_cursors();

/* If XTextWidth is called in this file, then something's wrong, as X widths != print widths */


void    ps_range_sensitives_cb (
    Widget		w,
    RangeWidgets		*range_ptr,	/* <<<< NOTE not Trace!!! */
    XmSelectionBoxCallbackStruct *cb)
{
    int		opt;
    int		active;
    char	strg[MAXTIMELEN];
    Trace	*trace;

    if (DTPRINT_ENTRY) printf ("In ps_range_sensitives_cb\n");

    trace = range_ptr->trace;	/* Snarf from range, as the callback doesn't have it */

    opt = option_to_number(range_ptr->time_option, range_ptr->time_pulldownbutton, 3);
    
    switch (opt) {
      case 3:	/* Window */
	active = FALSE;
	range_ptr->dastime = (range_ptr->type == BEGIN) ? global->time 
	    :   (global->time + TIME_WIDTH (trace));
	break;
      case 2:	/* Trace */ 
	active = FALSE;
	range_ptr->dastime = (range_ptr->type == BEGIN) ? trace->start_time : trace->end_time;
	break;
      case 1:	/* Cursor */
	active = FALSE;
	range_ptr->dastime = (range_ptr->type == BEGIN) ? cur_time_first(trace) : cur_time_last(trace);
	break;
      default:	/* Manual */
	active = TRUE;
	break;
    }

    XtSetArg (arglist[0], XmNsensitive, active);
    XtSetValues (range_ptr->time_text, arglist, 1);
    if (!active) {
	time_to_string (trace, strg, range_ptr->dastime, TRUE);
	XmTextSetString (range_ptr->time_text, strg);
    }
    else {
	range_ptr->dastime = string_to_time (range_ptr->trace, XmTextGetString (range_ptr->time_text));
    }
}


void    ps_range_create (
    Trace		*trace,
    RangeWidgets	*range_ptr,
    Widget		above,		/* Upper widget for form attachment */
    char		*descrip,	/* Description of selection */
    Boolean		type)		/* True if END, else beginning */
{
    if (DTPRINT_ENTRY) printf ("In ps_create_range - trace=%p\n",trace);

    if (!range_ptr->trace) {
	range_ptr->trace = trace;
	range_ptr->type = type;

	/* Label */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple (descrip) );
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopOffset, 0);
	XtSetArg (arglist[4], XmNtopWidget, above);
	range_ptr->time_label = XmCreateLabel (trace->prntscr.form,"",arglist,5);
	DManageChild (range_ptr->time_label, trace, MC_NOKEYS);
	
	/* Begin pulldown */
	range_ptr->time_pulldown = XmCreatePulldownMenu (trace->prntscr.form,"time_pulldown",arglist,0);

	if (range_ptr->type == BEGIN)
	    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Window Left Edge") );
	else XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Window Right Edge") );
	range_ptr->time_pulldownbutton[3] =
	    XmCreatePushButtonGadget (range_ptr->time_pulldown,"pdbutton0",arglist,1);
	DAddCallback (range_ptr->time_pulldownbutton[3], XmNactivateCallback, (XtCallbackProc)ps_range_sensitives_cb, range_ptr);
	DManageChild (range_ptr->time_pulldownbutton[3], trace, MC_NOKEYS);

	if (range_ptr->type == BEGIN)
	    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Trace Beginning") );
	else XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Trace End") );
	range_ptr->time_pulldownbutton[2] =
	    XmCreatePushButtonGadget (range_ptr->time_pulldown,"pdbutton0",arglist,1);
	DAddCallback (range_ptr->time_pulldownbutton[2], XmNactivateCallback, (XtCallbackProc)ps_range_sensitives_cb, range_ptr);
	DManageChild (range_ptr->time_pulldownbutton[2], trace, MC_NOKEYS);

	if (range_ptr->type == BEGIN)
	    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("First Cursor") );
	else XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Last Cursor") );
	range_ptr->time_pulldownbutton[1] =
	    XmCreatePushButtonGadget (range_ptr->time_pulldown,"pdbutton0",arglist,1);
	DAddCallback (range_ptr->time_pulldownbutton[1], XmNactivateCallback, (XtCallbackProc)ps_range_sensitives_cb, range_ptr);
	DManageChild (range_ptr->time_pulldownbutton[1], trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Entered Time") );
	range_ptr->time_pulldownbutton[0] =
	    XmCreatePushButtonGadget (range_ptr->time_pulldown,"pdbutton0",arglist,1);
	DAddCallback (range_ptr->time_pulldownbutton[0], XmNactivateCallback, (XtCallbackProc)ps_range_sensitives_cb, range_ptr);
	DManageChild (range_ptr->time_pulldownbutton[0], trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNsubMenuId, range_ptr->time_pulldown);
	XtSetArg (arglist[1], XmNx, 20);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopOffset, 0);
	XtSetArg (arglist[4], XmNtopWidget, range_ptr->time_label);
	range_ptr->time_option = XmCreateOptionMenu (trace->prntscr.form,"options",arglist,5);
	DManageChild (range_ptr->time_option, trace, MC_NOKEYS);

	/* Default */
	XtSetArg (arglist[0], XmNmenuHistory, range_ptr->time_pulldownbutton[3]);
	XtSetValues (range_ptr->time_option, arglist, 1);

	/* Begin Text */
	XtSetArg (arglist[0], XmNrows, 1);
	XtSetArg (arglist[1], XmNcolumns, 10);
	XtSetArg (arglist[2], XmNx, 200);
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNtopOffset, 0);
	XtSetArg (arglist[5], XmNtopWidget, range_ptr->time_label);
	XtSetArg (arglist[6], XmNresizeHeight, FALSE);
	XtSetArg (arglist[7], XmNeditMode, XmSINGLE_LINE_EDIT);
	range_ptr->time_text = XmCreateText (trace->prntscr.form,"textn",arglist,8);
	DManageChild (range_ptr->time_text, trace, MC_NOKEYS);
    }

    /* Get initial values correct */
    ps_range_sensitives_cb (NULL, range_ptr, NULL);
}


DTime	ps_range_value (
    RangeWidgets		*range_ptr)
    /* Read the range value */
{
    /* Make sure have latest cursor, etc */
    ps_range_sensitives_cb (NULL, range_ptr, NULL);

    return (range_ptr->dastime);
}


void    ps_dialog_cb (
    Widget		w)
{
    Trace *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In print_screen - trace=%p\n",trace);
    
    if (!trace->prntscr.dialog) {
	XtSetArg (arglist[0],XmNdefaultPosition, TRUE);
	XtSetArg (arglist[1],XmNdialogTitle, XmStringCreateSimple ("Print Screen Menu"));
	/* XtSetArg (arglist[2],XmNwidth, 300);
	   XtSetArg (arglist[3],XmNheight, 225); */
	trace->prntscr.dialog = XmCreateBulletinBoardDialog (trace->work, "print",arglist,2);
	
	XtSetArg (arglist[0], XmNverticalSpacing, 10);
	trace->prntscr.form = XmCreateForm (trace->prntscr.dialog, "form", arglist, 1);
	DManageChild (trace->prntscr.form, trace, MC_NOKEYS);

	/* create label widget for text widget */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("File Name") );
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_FORM );
	XtSetArg (arglist[3], XmNtopOffset, 5);
	trace->prntscr.label = XmCreateLabel (trace->prntscr.form,"",arglist,4);
	DManageChild (trace->prntscr.label, trace, MC_NOKEYS);
	
	/* create the file name text widget */
	XtSetArg (arglist[0], XmNrows, 1);
	XtSetArg (arglist[1], XmNcolumns, 30);
	XtSetArg (arglist[2], XmNx, 10);
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNtopOffset, 5);
	XtSetArg (arglist[5], XmNtopWidget, trace->prntscr.label);
	XtSetArg (arglist[6], XmNresizeHeight, FALSE);
	XtSetArg (arglist[7], XmNeditMode, XmSINGLE_LINE_EDIT);
	trace->prntscr.text = XmCreateText (trace->prntscr.form,"",arglist,8);
	DManageChild (trace->prntscr.text, trace, MC_NOKEYS);
	DAddCallback (trace->prntscr.text, XmNactivateCallback, ps_print_req_cb, trace);
	
	/* create label widget for notetext widget */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Note") );
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopOffset, 5);
	XtSetArg (arglist[4], XmNtopWidget, trace->prntscr.text);
	trace->prntscr.label = XmCreateLabel (trace->prntscr.form,"",arglist,5);
	DManageChild (trace->prntscr.label, trace, MC_NOKEYS);
	
	/* create the print note text widget */
	XtSetArg (arglist[0], XmNrows, 1);
	XtSetArg (arglist[1], XmNcolumns, 30);
	XtSetArg (arglist[2], XmNx, 10);
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNtopOffset, 0);
	XtSetArg (arglist[5], XmNtopWidget, trace->prntscr.label);
	XtSetArg (arglist[6], XmNresizeHeight, FALSE);
	XtSetArg (arglist[7], XmNeditMode, XmSINGLE_LINE_EDIT);
	trace->prntscr.notetext = XmCreateText (trace->prntscr.form,"notetext",arglist,8);
	DAddCallback (trace->prntscr.notetext, XmNactivateCallback, ps_print_req_cb, trace);
	DManageChild (trace->prntscr.notetext, trace, MC_NOKEYS);
	
	/* Create radio box for page size */
	trace->prntscr.size_menu = XmCreatePulldownMenu (trace->prntscr.form,"size",arglist,0);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("A-Sized"));
	trace->prntscr.sizea = XmCreatePushButtonGadget (trace->prntscr.size_menu,"sizea",arglist,1);
	DManageChild (trace->prntscr.sizea, trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("B-Sized"));
	trace->prntscr.sizeb = XmCreatePushButtonGadget (trace->prntscr.size_menu,"sizeb",arglist,1);
	DManageChild (trace->prntscr.sizeb, trace, MC_NOKEYS);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("EPS Portrait"));
	trace->prntscr.sizeep = XmCreatePushButtonGadget (trace->prntscr.size_menu,"sizeep",arglist,1);
	DManageChild (trace->prntscr.sizeep, trace, MC_NOKEYS);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("EPS Landscape"));
	trace->prntscr.sizeel = XmCreatePushButtonGadget (trace->prntscr.size_menu,"sizeel",arglist,1);
	DManageChild (trace->prntscr.sizeel, trace, MC_NOKEYS);
	
	XtSetArg (arglist[0], XmNx, 10);
	XtSetArg (arglist[1], XmNlabelString, XmStringCreateSimple ("Layout"));
	XtSetArg (arglist[2], XmNsubMenuId, trace->prntscr.size_menu);
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNtopOffset, 0);
	XtSetArg (arglist[5], XmNtopWidget, trace->prntscr.notetext);
	trace->prntscr.size_option = XmCreateOptionMenu (trace->prntscr.form,"sizeo",arglist,6);
	DManageChild (trace->prntscr.size_option, trace, MC_NOKEYS);

	/* Create all_signals button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Include off-screen signals"));
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNshadowThickness, 1);
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNtopOffset, 0);
	XtSetArg (arglist[5], XmNtopWidget, trace->prntscr.size_option);
	trace->prntscr.all_signals = XmCreateToggleButton (trace->prntscr.form,
							   "all_signals",arglist,6);
	DManageChild (trace->prntscr.all_signals, trace, MC_NOKEYS);

	ps_range_create (trace, &(trace->prntscr.begin_range),
			 trace->prntscr.all_signals, "Begin Printing at:", 0);

	ps_range_create (trace, &(trace->prntscr.end_range),
			 trace->prntscr.begin_range.time_text, "End Printing at:", 1);

	/* Create Separator */
	XtSetArg (arglist[0], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[1], XmNtopWidget, trace->prntscr.end_range.time_text);
	XtSetArg (arglist[2], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[3], XmNrightAttachment, XmATTACH_FORM );
	trace->prntscr.sep = XmCreateSeparator (trace->prntscr.form, "sep",arglist,4);
	DManageChild (trace->prntscr.sep, trace, MC_NOKEYS);
	
	/* Create Print button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Print") );
	XtSetArg (arglist[1], XmNx, 10 );
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopWidget, trace->prntscr.sep);
	trace->prntscr.print = XmCreatePushButton (trace->prntscr.form, "print",arglist,4);
	DAddCallback (trace->prntscr.print, XmNactivateCallback, ps_print_req_cb, trace);
	DManageChild (trace->prntscr.print, trace, MC_NOKEYS);
	
	/* create defaults button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Defaults") );
	XtSetArg (arglist[1], XmNx, 140 );
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopWidget, trace->prntscr.sep);
	trace->prntscr.defaults = XmCreatePushButton (trace->prntscr.form,"defaults",arglist,4);
	DAddCallback (trace->prntscr.defaults, XmNactivateCallback, ps_reset_cb, trace->prntscr.dialog);
	DManageChild (trace->prntscr.defaults, trace, MC_NOKEYS);

	/* create cancel button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Cancel") );
	XtSetArg (arglist[1], XmNx, 230 );
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopWidget, trace->prntscr.sep);
	trace->prntscr.cancel = XmCreatePushButton (trace->prntscr.form,"cancel",arglist,4);
	DAddCallback (trace->prntscr.cancel, XmNactivateCallback, unmanage_cb, trace->prntscr.dialog);
	DManageChild (trace->prntscr.cancel, trace, MC_NOKEYS);
	}
    
    /* reset page size */
    switch (global->print_size) {
      default:
      case PRINTSIZE_A:
	XtSetArg (arglist[0], XmNmenuHistory, trace->prntscr.sizea);
	break;
      case PRINTSIZE_B:
	XtSetArg (arglist[0], XmNmenuHistory, trace->prntscr.sizeb);
	break;
      case PRINTSIZE_EPSPORT:
	XtSetArg (arglist[0], XmNmenuHistory, trace->prntscr.sizeep);
	break;
      case PRINTSIZE_EPSLAND:
	XtSetArg (arglist[0], XmNmenuHistory, trace->prntscr.sizeel);
	break;
    }
    XtSetValues (trace->prntscr.size_option, arglist, 1);

    /* reset flags */
    XtSetArg (arglist[0], XmNset, global->print_all_signals ? 1:0);
    XtSetValues (trace->prntscr.all_signals,arglist,1);

    /* reset file name */
    XtSetArg (arglist[0], XmNvalue, trace->printname);
    XtSetValues (trace->prntscr.text,arglist,1);
    
    /* reset file note */
    XtSetArg (arglist[0], XmNvalue, global->printnote);
    XtSetValues (trace->prntscr.notetext,arglist,1);
    
    /* if a file has been read in, make printscreen buttons active */
    XtSetArg (arglist[0],XmNsensitive, (trace->loaded)?TRUE:FALSE);
    XtSetValues (trace->prntscr.print,arglist,1);
    
    /* manage the popup on the screen */
    DManageChild (trace->prntscr.dialog, trace, MC_NOKEYS);
}

void    ps_print_direct_cb (
    Widget		w)
{
    Trace *trace = widget_to_trace(w);
    ps_print_internal (trace);
}

void    ps_print_req_cb (
    Widget		w)
{
    Trace *trace = widget_to_trace(w);
    Widget	clicked;
    
    if (DTPRINT_ENTRY) printf ("In ps_print_req_cb - trace=%p\n",trace);
    
    XtSetArg (arglist[0], XmNmenuHistory, &clicked);
    XtGetValues (trace->prntscr.size_option, arglist, 1);
    if (clicked == trace->prntscr.sizeep)
	global->print_size = PRINTSIZE_EPSPORT;
    else if (clicked == trace->prntscr.sizeel)
	global->print_size = PRINTSIZE_EPSLAND;
    else if (clicked == trace->prntscr.sizeb)
	global->print_size = PRINTSIZE_B;
    else global->print_size = PRINTSIZE_A;

    global->print_all_signals = XmToggleButtonGetState (trace->prntscr.all_signals);

    /* ranges */
    global->print_begin_time = ps_range_value ( &(trace->prntscr.begin_range) );
    global->print_end_time = ps_range_value ( &(trace->prntscr.end_range) );

    /* get note */
    strcpy (global->printnote, XmTextGetString (trace->prntscr.notetext));

    /* open output file */
    strcpy (trace->printname, XmTextGetString (trace->prntscr.text));

    /* hide the print screen window */
    XtUnmanageChild (trace->prntscr.dialog);
    
    ps_print_internal (trace);
}
    
void    ps_print_internal (Trace *trace)
{
    FILE	*psfile=NULL;
    int		sigs_per_page;
    DTime	time_per_page;
    int		horiz_pages;		/* Number of pages to print the time on (horizontal) */
    int		vert_pages;		/* Number of pages to print signal names on (vertical) */
    int		horiz_page, vert_page;
    char	*timeunits;
    int		encapsulated;
    Signal	*sig_ptr, *sig_end_ptr=NULL;
    uint_t	numprt;
    DTime	printtime;	/* Time current page starts on */
    char	pagenum[20];
    char	sstrg[MAXTIMELEN];
    char	estrg[MAXTIMELEN];
    
    if (DTPRINT_ENTRY) printf ("In ps_print_internal - trace=%p\n",trace);
    if (!trace->loaded) return;
    
    if (trace->printname) {
	/* Open the file */
	if (DTPRINT_ENTRY) printf ("Filename=%s\n", trace->printname);
	psfile = fopen (trace->printname,"w");
    }
    else {
	if (DTPRINT_ENTRY) printf ("Null filename\n");
	psfile = NULL;
    }

    if (psfile == NULL) {
	sprintf (message,"Bad Filename: %s\n", trace->printname);
	dino_error_ack (trace,message);
	return;
    }
    
    draw_update ();

    set_cursor (trace, DC_BUSY);
    XSync (global->display,0);
    
    /* calculate time per page */
    time_per_page = TIME_WIDTH (trace);
    sigs_per_page = trace->numsigvis;
    
    /* Reset stuff if doing all times */
    /* calculate number of pages needed to draw the entire trace */
    horiz_pages = (global->print_end_time - global->print_begin_time)/time_per_page;
    if ( (global->print_end_time - global->print_begin_time) % time_per_page )
	horiz_pages++;

    /* Reset stuff if doing all signals */
    vert_pages = 1;
    if (global->print_all_signals) {
	/* calculate number of pages needed to draw the entire trace */
	vert_pages = (int)((trace->numsig) / sigs_per_page);
	if ( (trace->numsig) % sigs_per_page )
	    vert_pages++;
    }
    
    /* encapsulated? */
    encapsulated = (global->print_size==PRINTSIZE_EPSPORT)
	|| (global->print_size==PRINTSIZE_EPSLAND);
    
    /* File header information */
    fprintf (psfile, "%%!PS-Adobe-1.0\n");
    fprintf (psfile, "%%%%Title: %s\n", trace->printname);
    fprintf (psfile, "%%%%Creator: %s %sPostscript\n", DTVERSION,
	     encapsulated ? "Encapsulated ":"");
    fprintf (psfile, "%%%%CreationDate: %s\n", date_string(0));
    fprintf (psfile, "%%%%Pages: %d\n", encapsulated ? 0 : horiz_pages * vert_pages );
    /* Took page size, subtracted 50 to loose title information */
    if (encapsulated) {
	if (global->print_size==PRINTSIZE_EPSLAND)
	    fprintf (psfile, "%%%%BoundingBox: 0 0 569 792\n");
	else fprintf (psfile, "%%%%BoundingBox: 0 0 792 569\n");
    }
    fprintf (psfile, "%%%%EndComments\n");
    if (encapsulated) fprintf (psfile,"save\n");
    
    /* include the postscript macro information */
    fputs (dt_post, psfile);
    
    /* Grab units */
    timeunits = time_units_to_string (trace->timerep, FALSE);

    /* output the page scaling and rf time */
    fprintf (psfile,"\n%d %d %d %d %d %d PAGESCALE\n",
	     trace->height, trace->width, global->xstart, trace->sigrf,
	     (int) ( ( (global->print_size==PRINTSIZE_B) ? 11.0 :  8.5) * 72.0),
	     (int) ( ( (global->print_size==PRINTSIZE_B) ? 16.8 : 10.8) * 72.0)
	     );
	
    /* output signal name width scalling */
    fprintf (psfile,"/sigwidth 0 def\nstroke /Times-Roman findfont 8 scalefont setfont\n");
    /* Signal to start on */
    if (vert_pages > 1) {
	sig_ptr = trace->firstsig;
    }
    else {
	sig_ptr = trace->dispsig;
    }
    for (numprt = 0; sig_ptr && ( numprt<trace->numsigvis || (vert_pages>1));
	 sig_ptr = sig_ptr->forward, numprt++) {
	fprintf (psfile,"(%s) SIGMARGIN\n", sig_ptr->signame);
    }
    fprintf (psfile,"XSCALESET\n\n");

    /* print out each page */
    for (horiz_page=0; horiz_page<horiz_pages; horiz_page++) {
	
	/* Time at left edge of printout */
	printtime = time_per_page * horiz_page + global->print_begin_time;

	/* Signal to start on */
	if (vert_pages > 1) {
	    sig_ptr = trace->firstsig;
	}
	else {
	    sig_ptr = trace->dispsig;
	}

	for (vert_page=0; vert_page<vert_pages; vert_page++) {
	    fprintf (psfile,"%% Beginning of page %d - %d\n", horiz_page, vert_page);

	    if (vert_page > 0) {
		/* move pointer forward to next signal set */
		sig_ptr = sig_end_ptr;
	    }

	    /* Find last signal to print on this page */
	    if (! sig_ptr) break;
	    for (sig_end_ptr = sig_ptr, numprt = 0; sig_end_ptr && numprt<trace->numsigvis;
		 sig_end_ptr = sig_end_ptr->forward, numprt++) ;

	    /******* NEW PAGE */
	    if (vert_pages == 1) {
		if (horiz_pages == 1) 	{}	/* Just 1 page */
		else			sprintf (pagenum, "Page %d", horiz_page+1);
	    }
	    else {
		if (horiz_pages == 1) 	sprintf (pagenum, "Page %d", vert_page+1);
		else			sprintf (pagenum, "Page %d-%d", horiz_page+1, vert_page+1);
	    }
	    
	    /* decode times */
	    time_to_string (trace, sstrg, printtime, FALSE);
	    time_to_string (trace, estrg, printtime + time_per_page, FALSE);

	    /* output the page header macro */
	    fprintf (psfile, "(%s-%s %s) (%d %s/page) (%s) (%s) (%s) (%s) (%s) %s\n",
		     sstrg,			/* start time */
		     estrg,			/* end time */
		     timeunits,
		     time_per_page,			/* resolution */
		     timeunits,
		     date_string(0),			/* time & date */
		     global->printnote,		/* filenote */
		     trace->filename,		/* filename */
		     pagenum,				/* page number */
		     DTVERSION,			/* version (for title) */
		     ( (global->print_size==PRINTSIZE_EPSPORT)
		      ? "EPSPHDR" :
		      ( (global->print_size==PRINTSIZE_EPSLAND)
		       ? "EPSLHDR" : "PAGEHDR"))	/* which macro to use */
		     );
	    
	    /* draw the signal names and the traces */
	    ps_drawsig (trace, psfile, sig_ptr, sig_end_ptr);
	    ps_draw (trace, psfile, sig_ptr, sig_end_ptr, printtime);
	    
	    /* print the page */
	    if (encapsulated)
		fprintf (psfile,"stroke\nrestore\n");
	    else fprintf (psfile,"stroke\nshowpage\n");

	} /* vert page */
    } /* horiz page */
    
    /* close output file */
    fclose (psfile);
    new_time (trace);
    set_cursor (trace, DC_NORMAL);
}

void    ps_reset (
    Trace *trace)
{
    char 		*pchar;
    if (DTPRINT_ENTRY) printf ("In ps_reset - trace=%p",trace);
    
    /* Init print name */
#ifdef VMS
    strcpy (trace->printname, "sys$login:dinotrace.ps");
#else
    trace->printname[0] = '\0';
    if (NULL != (pchar = getenv ("HOME"))) strcpy (trace->printname, pchar);
    if (trace->printname[0]) strcat (trace->printname, "/");
    strcat (trace->printname, "dinotrace.ps");
#endif
}

void    ps_reset_cb (
    Widget		w)
{
    Trace *trace = widget_to_trace(w);
    ps_reset (trace);
    XtUnmanageChild (trace->prntscr.dialog);
    ps_dialog_cb (w);
}

void ps_draw_grid (trace, psfile, printtime, grid_ptr, draw_numbers)
    Trace	*trace;
    FILE	*psfile;
    DTime	printtime;	/* Time to start on */
    Grid	*grid_ptr;		/* Grid information */
    Boolean	draw_numbers;		/* Whether to print the times or not */
{ 
    char 	strg[MAXTIMELEN];	/* String value to print out */
    int		end_time;
    DTime	xtime;
    Position	x2;			/* Coordinate of current time */
    Position	yl,yh,yt;		/* Starting and ending points */

    if (grid_ptr->period < 1) return;

    /* Is the grid too small or invisible?  If so, skip it */
    if ((! grid_ptr->visible) || ((grid_ptr->period * global->res) < MIN_GRID_RES)) {
	return;
    }

    /* set the line attributes as the specified dash pattern */
    fprintf (psfile,"stroke\n[%d YTRN %d YTRN] 0 setdash\n",Y_SIG_SPACE/2, trace->sighgt - Y_SIG_SPACE/2);
    
    /* Start to left of right edge */
    xtime = printtime;
    end_time = printtime + TIME_WIDTH (trace);

    /* Move starting point to the right to hit where a grid line is aligned */
    xtime = ((xtime / grid_ptr->period) * grid_ptr->period) + (grid_ptr->alignment % grid_ptr->period);

    /* If possible, put one grid line inside the signal names */
    if (((grid_ptr->period * global->res) < 0)
	&& (xtime >= grid_ptr->period)) {
	xtime -= grid_ptr->period;
    }

    /* Other coordinates */
    yt = trace->height - 20;
    yl = trace->sighgt;
    yh = trace->height - trace->ystart + Y_SIG_SPACE/4;

    /* Start grid */
    fprintf (psfile,"%d %d %d START_GRID\n", yh, yl, yt-10);

    /* Loop through grid times */
    for ( ; xtime <= end_time; xtime += grid_ptr->period) {
	x2 = ( ((xtime) - printtime) * global->res + global->xstart );	/* Similar to TIME_TO_XPOS (xtime) */

	/* compute the time value and draw it if it fits */
	if (draw_numbers) {
	    time_to_string (trace, strg, xtime, FALSE);
	}
	else {
	    strg[0] = '\0';
	}

	/* Draw if space, centered on grid line */
	fprintf (psfile,"%d (%s) GRID\n", x2, strg);
    }
}

void ps_draw_grids (trace, psfile, printtime)
    Trace	*trace;
    FILE	*psfile;
    DTime	printtime;
{           
    int		grid_num;
    Grid	*grid_ptr;
    Grid	*grid_smallest_ptr;

    /* WARNING, major weedyness follows: */
    /* Determine which grid has the smallest period and only plot it's lines. */
    /* If we don't do this, labels may overlap */

    grid_smallest_ptr = &(trace->grid[0]);
    for (grid_num=0; grid_num<MAXGRIDS; grid_num++) {
	grid_ptr = &(trace->grid[grid_num]);
	if ( grid_ptr->visible
	    && ((grid_ptr->period * global->res) >= MIN_GRID_RES)	/* not too small */
	    && (grid_ptr->period > 0)
	    && (grid_ptr->period < grid_smallest_ptr->period) ) {
	    grid_smallest_ptr = grid_ptr;
	}
    }


    /* Draw each grid */
    for (grid_num=0; grid_num<MAXGRIDS; grid_num++) {
	grid_ptr = &(trace->grid[grid_num]);
	ps_draw_grid (trace, psfile, printtime, grid_ptr, (grid_ptr == grid_smallest_ptr ) );
    }
}


void ps_draw (trace, psfile, sig_ptr, sig_end_ptr, printtime)
    Trace	*trace;
    FILE	*psfile;
    Signal	*sig_ptr;	/* Vertical signal to start on */
    Signal	*sig_end_ptr;	/* Last signal to print */
    DTime	printtime;	/* Time to start on */
{
    int c=0,adj,ymdpt,xloc,xend,xstart,ystart;
    int y1,y2;
    Value_t *cptr,*nptr;
    char dvstrg[MAXVALUELEN];
    char vstrg[MAXVALUELEN];
    unsigned int value;
    int unstroked=0;		/* Number commands not stroked */
    
    if (DTPRINT_ENTRY) printf ("In ps_draw - filename=%s, printtime=%d sig=%s\n",trace->filename, printtime, sig_ptr->signame);
    
    xend = trace->width - XMARGIN;
    adj = printtime * global->res - global->xstart;
    
    if (DTPRINT_PRINT) printf ("global->res=%f adj=%d\n",global->res,adj);
    
    /* Loop and draw each signal individually */
    for (; sig_ptr && sig_ptr!=sig_end_ptr; sig_ptr = sig_ptr->forward) {

	y1 = trace->height - trace->ystart - c * trace->sighgt - Y_SIG_SPACE;
	ymdpt = y1 - (int)(trace->sighgt/2) + Y_SIG_SPACE;
	y2 = y1 - trace->sighgt + 2*Y_SIG_SPACE;
	c++;
	xloc = 0;
	
	/* find data start */
	cptr = sig_ptr->bptr;
	if (printtime >= CPTR_TIME(cptr) ) {
	    while ((CPTR_TIME(cptr) != EOT) &&
		   (printtime > CPTR_TIME(CPTR_NEXT(cptr)))) {
		cptr = CPTR_NEXT(cptr);
	    }
	}

	/* Compute starting points for signal */
	xstart = global->xstart;
	switch ( cptr->siglw.stbits.state )
	    {
	  case STATE_0: ystart = y2; break;
	  case STATE_1: ystart = y1; break;
	  case STATE_U: ystart = ymdpt; break;
	  case STATE_Z: ystart = ymdpt; break;
	  case STATE_B32: ystart = ymdpt; break;
	  case STATE_B128: ystart = ymdpt; break;
	  default: printf ("Error: State=%d\n",cptr->siglw.stbits.state); ystart = ymdpt; break;
	    }
	
	/* output y information - note reverse from draw() due to y-axis */
	/* output starting positional information */
	fprintf (psfile,"%d %d %d %d %d START_SIG\n",
		 ymdpt, y1, y2, xstart, ystart);
	
	/* Loop as long as the time and end of trace are in current screen */
	while ( CPTR_TIME(cptr) != EOT && xloc < xend )
	    {
	    /* find the next transition */
	    nptr = CPTR_NEXT(cptr);
	    
	    /* if next transition is the end, don't draw */
	    if (CPTR_TIME(nptr) == EOT) break;
	    
	    /* find the x location for the end of this segment */
	    xloc = CPTR_TIME(nptr) * global->res - adj;
	    
	    /* Determine what the state of the signal is and build transition */
	    switch ( cptr->siglw.stbits.state ) {
	      case STATE_0: if ( xloc > xend ) xloc = xend;
		fprintf (psfile,"%d STATE_0\n",xloc);
		break;
		
	      case STATE_1: if ( xloc > xend ) xloc = xend;
		fprintf (psfile,"%d STATE_1\n",xloc);
		break;
		
	      case STATE_U: if ( xloc > xend ) xloc = xend;
		fprintf (psfile,"%d STATE_U\n",xloc);
		break;
		
	      case STATE_Z: if ( xloc > xend ) xloc = xend;
		fprintf (psfile,"%d STATE_Z\n",xloc);
		break;
		
	      case STATE_B32: if ( xloc > xend ) xloc = xend;
		value = cptr->number[0];

		if (trace->busrep == BUSREP_HEX_UN)
		    sprintf (vstrg,"%x", value);
		else if (trace->busrep == BUSREP_OCT_UN)
		    sprintf (vstrg,"%o", value);
		else
		    sprintf (vstrg,"%d", value);

		/* Below evaluation left to right important to prevent error */
		if ( (sig_ptr->decode != NULL) &&
		    (value < sig_ptr->decode->numstates) &&
		    (sig_ptr->decode->statename[value][0] != '\0')) {
		    strcpy (dvstrg, sig_ptr->decode->statename[value]);
		    fprintf (psfile,"%d (%s) (%s) STATE_B_FB\n",xloc,vstrg,dvstrg);
		}
		else {
		    fprintf (psfile,"%d (%s) STATE_B\n",xloc,vstrg);
		}
		
		break;
		
	      case STATE_B128:
		if ( xloc > xend ) xloc = xend;

		value_to_string (trace, vstrg, cptr, ' ');

		fprintf (psfile,"%d (%s) STATE_B\n",xloc,vstrg);
		break;
		
	      default: printf ("Error: State=%d\n",cptr->siglw.stbits.state); break;
	    } /* end switch */
	    
	    if (unstroked++ > 400) {
		/* Must stroke every so often to avoid overflowing printer stack */
		fprintf (psfile,"currentpoint stroke MT\n");
		unstroked=0;
	    }

	    cptr = CPTR_NEXT(cptr);
	    }
	} /* end of FOR */
    
    /* Draw grids */
    ps_draw_grids (trace, psfile, printtime);

    /* draw the cursors if they are visible */
    ps_draw_cursors (trace, psfile, printtime);
} /* End of DRAW */


void ps_drawsig (
    Trace	*trace,
    FILE	*psfile,
    Signal	*sig_ptr,	/* Vertical signal to start on */
    Signal	*sig_end_ptr)	/* Last signal to print */
{
    int		c=0,ymdpt;
    int		y1;
    
    if (DTPRINT_ENTRY) printf ("In ps_drawsig - filename=%s\n",trace->filename);
    
    /* don't draw anything if there is no file is loaded */
    if (!trace->loaded) return;
    
    /* loop thru all the visible signals */
    for (; sig_ptr && sig_ptr!=sig_end_ptr; sig_ptr = sig_ptr->forward) {

	/* calculate the location to start drawing the signal name */
	/* printf ("xstart=%d, %s\n",global->xstart, sig_ptr->signame); */
	
	/* calculate the y location to draw the signal name and draw it */
	y1 = trace->height - trace->ystart - c * trace->sighgt - Y_SIG_SPACE;
	ymdpt = y1 - (int)(trace->sighgt/2) + Y_SIG_SPACE;
	
	fprintf (psfile,"sigwidth %d YADJ 3 sub MT (%s) RIGHTSHOW\n",
		 ymdpt, sig_ptr->signame);
	c++;
    }
}

void ps_draw_cursors (
    Trace	*trace,
    FILE	*psfile,
    DTime	printtime)	/* Time to start on */
{
    Position	x1,x2,y1,y2;
    char 	strg[MAXTIMELEN];
    int		adj,xend;
    DCursor	*csr_ptr;
    
    /* reset the line attributes */
    fprintf (psfile,"stroke [] 0 setdash\n");
    
    /* draw the cursors if they are visible */
    if ( trace->cursor_vis ) {
	/* initial the y values for drawing */
	y1 = trace->height - 25 - 10;
	y2 = trace->height - ( (int)((trace->height-trace->ystart)/trace->sighgt)-1) *
	    trace->sighgt - trace->sighgt/2 - trace->ystart - 2;
	xend = trace->width - XMARGIN;
	adj = printtime * global->res - global->xstart;

	/* Start cursor, for now == start grid */
	fprintf (psfile,"%d %d %d START_GRID\n", y2, y1, y2-8);

	for (csr_ptr = global->cursor_head; csr_ptr; csr_ptr = csr_ptr->next) {

	    /* check if cursor is on the screen */
	    if (csr_ptr->time > printtime) {
		
		/* draw the vertical cursor line */
		x1 = csr_ptr->time * global->res - adj;
		if (x1 > xend) break;	/* past end of screen, since sorted list no more to do */
		
		/* draw the cursor value */
		time_to_string (trace, strg, csr_ptr->time, FALSE);
		fprintf (psfile,"%d (%s) GRID\n", x1, strg);
		
		/* if there is a previous visible cursor, draw delta line */
		if ( csr_ptr->prev && csr_ptr->prev->time > printtime ) {

		    x2 = csr_ptr->prev->time * global->res - adj;
		    time_to_string (trace, strg, csr_ptr->time - csr_ptr->prev->time, TRUE);
		    fprintf (psfile,"%d %d %d (%s) CURSOR_DELTA\n", x1, x2, y2+7, strg);
		}
	    }
	}
    }
}

