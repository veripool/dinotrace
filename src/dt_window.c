#ident "$Id$"
/******************************************************************************
 * DESCRIPTION: Dinotrace source: window scrolling, etc
 *
 * This file is part of Dinotrace.
 *
 * Author: Wilson Snyder <wsnyder@wsnyder.org>
 *
 * Code available from: http://www.veripool.org/dinotrace
 *
 ******************************************************************************
 *
 * Some of the code in this file was originally developed for Digital
 * Semiconductor, a division of Digital Equipment Corporation.  They
 * gratefuly have agreed to share it, and thus the base version has been
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

#include <Xm/RowColumnP.h>
#include <Xm/LabelP.h>
#include <Xm/CascadeBP.h>
#include <Xm/BulletinB.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <Xm/Text.h>
#include <Xm/Form.h>
#include <Xm/Separator.h>

#include "functions.h"


static void win_note_ok_cb (Widget w, Trace_t* trace, XmSelectionBoxCallbackStruct* cb);
static void win_note_cancel_cb (Widget w, Trace_t* trace, XmAnyCallbackStruct* cb);

/****************************** CALLBACKS ******************************/

void win_expose_cb (
    Widget	w,
    Trace_t	*trace)
{
    if (DTPRINT_ENTRY) printf ("In win_expose_cb - trace=%p\n",trace);
    /* special, call draw directly so screen doesn't flicker */
    draw_expose_needed (trace);
}

void win_resize_cb (
    Widget	w,
    Trace_t	*trace)
{
    if (DTPRINT_ENTRY) printf ("In win_resize_cb - trace=%p\n",trace);
    draw_needupd_sig_start ();
    draw_all_needed ();	/* All, as xstart might have changed */
}

void win_refresh_cb (
    Widget	w,
    Trace_t	*trace)
{
    if (DTPRINT_ENTRY) printf ("In win_refresh_cb - trace=%p\n",trace);
    draw_all_needed ();
    /* Main loop won't refresh if in manual refresh mode */
    if (global->redraw_manually) draw_perform();
}

void hscroll_unitinc_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In hscroll_unitinc - trace=%p  old_time=%d",trace,global->time);
    global->time += grid_primary_period (trace);
    if (DTPRINT_ENTRY) printf (" new time=%d\n",global->time);

    new_time (trace);
}

void hscroll_unitdec_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In hscroll_unitdec - trace=%p  old_time=%d",trace,global->time);
    global->time -= grid_primary_period (trace);
    new_time (trace);
}

void hscroll_pageinc_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In hscroll_pageinc - trace=%p  old_time=%d",trace,global->time);

    switch ( global->pageinc ) {
    case PAGEINC_QUARTER:
	global->time += (int) ( TIME_WIDTH (trace) /4);
	break;
    case PAGEINC_HALF:
	global->time += (int) ( TIME_WIDTH (trace) /2);
	break;
    case PAGEINC_FULL:
	global->time += (int) ( TIME_WIDTH (trace)   );
	break;
    }

    if (DTPRINT_ENTRY) printf (" new time=%d\n",global->time);

    new_time (trace);
}

void hscroll_pagedec_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In hscroll_pagedec - trace=%p  old_time=%d pageinc=%d",trace,global->time,global->pageinc);

    switch ( global->pageinc ) {
    case PAGEINC_QUARTER:
	global->time -= (int) ( TIME_WIDTH (trace) /4);
	break;
    case PAGEINC_HALF:
	global->time -= (int) ( TIME_WIDTH (trace) /2);
	break;
    case PAGEINC_FULL:
	global->time -= (int) ( TIME_WIDTH (trace)   );
	break;
    }

    if (DTPRINT_ENTRY) printf (" new time=%d\n",global->time);

    new_time (trace);
}

void hscroll_drag_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    int inc;

    XtSetArg (arglist[0], XmNvalue, &inc);
    XtGetValues (trace->hscroll,arglist,1);

    global->time = inc;

    new_time (trace);
}

void win_begin_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In win_begin_cb trace=%p\n",trace);
    global->time = trace->start_time;
    new_time (trace);
}

void win_end_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In win_end_cb trace=%p\n",trace);
    global->time = trace->end_time - TIME_WIDTH (trace);
    new_time (trace);
}

void win_namescroll_change_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    int inc;

    if (DTPRINT_ENTRY) printf ("In win_namescroll_change trace=%p\n",trace);

    XtSetArg (arglist[0], XmNvalue, &inc);
    XtGetValues (trace->command.namescroll,arglist,1);

    global->namepos = inc;
    draw_all_needed();
}

/* Vertically scroll + or - inc lines */

void vscroll_new (
    Trace_t	*trace,
    int 	inc)	/* Lines to move, signed, +1, -1, or +- n */
{
    uint_t	signum;
    Signal_t	*sig_ptr;

    if (DTPRINT_ENTRY) printf ("in vscroll_new inc=%d start=%d\n",inc,trace->numsigstart);

    /* Move to requested position */
    while ( (inc > 0) && trace->dispsig && trace->dispsig->forward ) {
	trace->dispsig = trace->dispsig->forward;
	inc--;
    }
    while ( (inc < 0) && trace->dispsig && trace->dispsig->backward ) {
	trace->dispsig = trace->dispsig->backward;
	inc++;
    }

    /* Calculate numsigstart */
    trace->numsigstart = 0;
    for (sig_ptr = trace->firstsig; sig_ptr && (sig_ptr != trace->dispsig); sig_ptr = sig_ptr->forward) {
	trace->numsigstart++;
    }

    /* If blank space on bottom of screen, scroll to fill it */
    for (signum=0; (signum < trace->numsigvis) && sig_ptr; sig_ptr = sig_ptr->forward)  signum++;
    while ( (signum < trace->numsigvis) && trace->dispsig && trace->dispsig->backward ) {
	trace->dispsig = trace->dispsig->backward;
	signum++;
	trace->numsigstart--;
    }

    draw_needed (trace);
}

void vscroll_unitinc_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    vscroll_new (trace, 1);
}

void vscroll_unitdec_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    vscroll_new (trace, -1);
}

void vscroll_pageinc_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    vscroll_new (trace, trace->numsigvis);
}

void vscroll_pagedec_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    vscroll_new (trace, -(trace->numsigvis));
}

void vscroll_drag_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    int		inc;

    if (DTPRINT_ENTRY) printf ("In vscroll_drag trace=%p\n",trace);

    XtSetArg (arglist[0], XmNvalue, &inc);
    XtGetValues (trace->vscroll, arglist, 1);

    /*
    ** The sig pointer is reset to the start and the loop will set
    ** it to the signal that inc represents
    */
    trace->dispsig = trace->firstsig;
    vscroll_new (trace, inc);
}

void win_chg_res_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In win_chg_res_cb - trace=%p\n",trace);
    get_data_popup (trace,"Resolution",IO_RES);
}


void new_res (
    Trace_t	*trace,
    float	res_new)	/* Desired res, pass global->res to not change */
{
    char	string[MAXTIMELEN+30], timestrg[MAXTIMELEN];

    if (DTPRINT_ENTRY) printf ("In new_res - res = %f\n",res_new);

    if (res_new != global->res) {
	global->res_default = FALSE;	/* Has changed */
	global->res = res_new;
    }

    if (global->res==0.0) global->res=0.1;	/* prevent div zero error */

    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	if (trace->toplevel) {
	    /* change res button's value */
	    time_to_string (trace, timestrg, (int)(RES_SCALE/global->res), TRUE);
	    sprintf (string,"Res=%s %s", timestrg,
		     time_units_to_string (global->timerep, FALSE));
	    XtSetArg (arglist[0],XmNlabelString,XmStringCreateSimple (string));
	    XtSetValues (trace->command.reschg_but,arglist,1);
	}
    }

    draw_all_needed ();
}

void win_inc_res_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In win_inc_res_cb - trace=%p\n",trace);

    /* increase the resolution by 10% */
    new_res (trace, global->res * 1.1);
}

void win_dec_res_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In win_dec_res_cb - trace=%p\n",trace);

    /* decrease the resolution by 10% */
    new_res (trace, global->res * 0.9);
}

void win_full_res_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    win_full_res (trace);
}

void win_full_res (
    Trace_t *trace)
{
    if (DTPRINT_ENTRY) printf ("In win_full_res_cb - trace=%p\n",trace);

    /* printf ("%d %d %d %d %d\n",
       global->xstart, trace->width,XMARGIN, trace->end_time, trace->start_time);   */

    /* xstart matters, so recalc if needed */
    draw_update();

    /* set resolution  */
    if (trace->end_time != trace->start_time) {
	global->time = trace->start_time;
	new_res (trace,
		 ( ((float)(trace->width - global->xstart - XMARGIN)) /
		   ((float)(trace->end_time - trace->start_time)) )
		 );
	new_time (trace);
    }
}

void win_zoom_res_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In win_zoom_res_cb - trace=%p\n",trace);

    /* process all subsequent button presses as res_zoom clicks */
    global->click_time = -1;	/* time must be signed */
    remove_all_events (trace);
    set_cursor (DC_ZOOM_1);
    add_event (ButtonPressMask, res_zoom_click_ev);
}

void res_zoom_click_ev (
    Widget	w,
    Trace_t	*trace,
    XButtonPressedEvent	*ev)
{
    DTime_t		time,tmp;

    if (DTPRINT_ENTRY) printf ("In res_zoom_click1_ev - trace=%p x=%d y=%d b=%d\n",trace,ev->x,ev->y,ev->button);
    if (ev->type != ButtonPress || (ev->button!=1 && ev->button!=3)) return;

    /* convert x value to a time value */
    time = posx_to_time (trace, ev->x);
    if (time<0) return;

    /* If no click time defined, define one and wait for second click */
    if ( global->click_time < 0) {
	global->click_time = time;
	set_cursor (DC_ZOOM_2);
	return;
    }

    if (DTPRINT_ENTRY) printf ("click1 = %d, click2 = %d\n",global->click_time, time);

    /* Got 2 clicks, set res */
    if (time != global->click_time) {
	/* Swap so time is the max */
	if (time < global->click_time) {
	    tmp = time;
	    time = global->click_time;
	    global->click_time = tmp;
	}

	/* Set new res & time */
	global->time = global->click_time;
	new_res (trace,
		 ( ((float)(trace->width - global->xstart)) /
		   ((float)(time - global->click_time)) )
		 );
	new_time (trace);
    }

    /* remove handlers */
    remove_all_events (trace);
}

/****************************** NOTES ******************************/

void    win_note (
    Trace_t*	trace,
    const char* text1,
    const char* text2,
    const char* notetext,
    Boolean_t	forcursor)
{
    char* notestr;

    if (DTPRINT_ENTRY) printf ("in win_note\n");

    trace->note.forcursor = forcursor;

    if (!trace->note.dialog) {
	XtSetArg (arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg (arglist[1], XmNdialogTitle, XmStringCreateSimple ("Note Change") );
	XtSetArg (arglist[2], XmNverticalSpacing, 10);
	XtSetArg (arglist[3], XmNhorizontalSpacing, 10);
	trace->note.dialog = XmCreateFormDialog (trace->work,"note",arglist,4);

	/* Create label widget for notetext widget */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Note") );
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNleftOffset, 5);
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_FORM );
	XtSetArg (arglist[4], XmNtopOffset, 5);
	trace->note.notelabel = XmCreateLabel (trace->note.dialog,"",arglist,5);
	DManageChild (trace->note.notelabel, trace, MC_NOKEYS);

	/* Create the print note text widget */
	XtSetArg (arglist[0], XmNrows, 1);
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNleftOffset, 5);
	XtSetArg (arglist[3], XmNcolumns, 30);
	XtSetArg (arglist[4], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[5], XmNtopOffset, 0);
	XtSetArg (arglist[6], XmNtopWidget, trace->note.notelabel);
	XtSetArg (arglist[7], XmNresizeHeight, FALSE);
	XtSetArg (arglist[8], XmNeditMode, XmSINGLE_LINE_EDIT);
	trace->note.notetext = XmCreateText (trace->note.dialog,"notetext",arglist,9);
	DAddCallback (trace->note.notetext, XmNactivateCallback, win_note_ok_cb, trace);
	DManageChild (trace->note.notetext, trace, MC_NOKEYS);

	/* Ok/apply/cancel */
	ok_apply_cancel (&trace->note.okapply, trace->note.dialog,
			 dmanage_last,
			 (XtCallbackProc)win_note_ok_cb, trace,
			 NULL, NULL,
			 NULL, NULL,
			 (XtCallbackProc)win_note_cancel_cb, trace);
    }


    /* right units */
    if (!text1) text1="";
    if (!text2) text2="";
    if (!notetext) notetext="";
    notestr = (char*)XtMalloc(100+strlen(text1)+strlen(text2));
    strcpy(notestr,text1);
    strcat(notestr,text2);
    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ((char*)notestr));
    XtSetValues (trace->note.notelabel, arglist, 1);

    /* Update with current search values */
    XmTextSetString (trace->note.notetext, (char*)notetext);
    XtSetValues (trace->note.notetext, arglist, 1);

    /* manage the dialog on the screen */
    DManageChild (trace->note.dialog, trace, MC_NOKEYS);
    XSync (global->display,0);
}

void    win_note_ok_cb (
    Widget	w,
    Trace_t	*trace,
    XmSelectionBoxCallbackStruct *cb)
{
    char	*note = NULL;

    if (DTPRINT_ENTRY) printf ("In win_note_ok_cb - trace=%p\n",trace);

    /* Get value */
    note = XmTextGetString (trace->note.notetext);
    DFree (global->select_note);
    if (!note[0]) note = NULL;

    if (trace->note.forcursor) {
	cur_note (global->selected_cursor, note);
    } else {
	sig_note (global->selected_sig, note);
    }

    /* unmanage the popup window */
    XtUnmanageChild (trace->note.dialog);
}

void    win_note_cancel_cb (
    Widget	w,
    Trace_t	*trace,
    XmAnyCallbackStruct	*cb)
{
    if (DTPRINT_ENTRY) printf ("In win_note_cancel_cb - trace=%p\n",trace);

    /* unmanage the popup on the screen */
    XtUnmanageChild (trace->note.dialog);
}

/****************************** GOTO ******************************/

void    win_goto_cb (
    Widget	w)
{
    int		i;
    Trace_t *trace = widget_to_trace(w);

    if (DTPRINT_ENTRY) printf ("In win_goto_cb - trace=%p\n",trace);

    if (!trace->gotos.dialog) {
	XtSetArg (arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg (arglist[1], XmNdialogTitle, XmStringCreateSimple ("Goto Time") );
	XtSetArg (arglist[2], XmNverticalSpacing, 10);
	XtSetArg (arglist[3], XmNhorizontalSpacing, 10);
	trace->gotos.dialog = XmCreateFormDialog (trace->work,"goto",arglist,4);
	DAddCallback (trace->gotos.dialog, XmNmapCallback, win_goto_option_cb, trace);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Time"));
	XtSetArg (arglist[1], XmNtopAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNtopOffset, 5);
	XtSetArg (arglist[3], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[4], XmNleftOffset, 5);
	trace->gotos.label1 = XmCreateLabel (trace->gotos.dialog,"label1",arglist,5);
	DManageChild (trace->gotos.label1, trace, MC_NOKEYS);

	/* create the goto text widget */
	XtSetArg (arglist[0], XmNrows, 1);
	XtSetArg (arglist[1], XmNcolumns, 12);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_FORM );
	XtSetArg (arglist[3], XmNtopOffset, 5);
	XtSetArg (arglist[4], XmNleftAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[5], XmNleftWidget, trace->gotos.label1 );
	XtSetArg (arglist[6], XmNresizeHeight, FALSE);
	XtSetArg (arglist[7], XmNeditMode, XmSINGLE_LINE_EDIT);
	trace->gotos.text = XmCreateText (trace->gotos.dialog,"textn",arglist,8);
	DAddCallback (trace->gotos.text, XmNactivateCallback, win_goto_ok_cb, trace);
	DManageChild (trace->gotos.text, trace, MC_NOKEYS);

 	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("ns"));
	XtSetArg (arglist[1], XmNtopAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNtopOffset, 5);
	XtSetArg (arglist[3], XmNleftAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNleftWidget, trace->gotos.text );
 	trace->gotos.label2 = XmCreateLabel (trace->gotos.dialog,"label2",arglist,5);
	DManageChild (trace->gotos.label2, trace, MC_NOKEYS);

	/* Make option menu */
	trace->gotos.pulldown = XmCreatePulldownMenu (trace->gotos.dialog,"pulldown",arglist,0);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("None") );
	trace->gotos.pulldownbutton[0] =
	    XmCreatePushButtonGadget (trace->gotos.pulldown,"pdbutton0",arglist,1);
	DAddCallback (trace->gotos.pulldownbutton[0], XmNactivateCallback, win_goto_option_cb, trace);
	DManageChild (trace->gotos.pulldownbutton[0], trace, MC_NOKEYS);

	for (i=0; i<MAX_SRCH; i++) {
	    XtSetArg (arglist[0], XmNbackground, trace->xcolornums[i] );
	    XtSetArg (arglist[1], XmNmarginRight, 20);
	    XtSetArg (arglist[2], XmNmarginBottom, 4);
	    trace->gotos.pulldownbutton[i+1] =
		XmCreatePushButton (trace->gotos.pulldown,"",arglist,3);
	    DAddCallback (trace->gotos.pulldownbutton[i+1], XmNactivateCallback, win_goto_option_cb, trace);
	    DManageChild (trace->gotos.pulldownbutton[i+1], trace, MC_NOKEYS);
	}

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Place Cursor:"));
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNleftOffset, 5);
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNtopWidget, trace->gotos.text );
	XtSetArg (arglist[5], XmNsubMenuId, trace->gotos.pulldown);
	trace->gotos.options = XmCreateOptionMenu (trace->gotos.dialog,"options",arglist,6);
	DManageChild (trace->gotos.options, trace, MC_NOKEYS);


	/* Create label widget for notetext widget */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Cursor Note") );
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNleftOffset, 5);
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNtopOffset, 5);
	XtSetArg (arglist[5], XmNtopWidget, trace->gotos.options);
	trace->gotos.notelabel = XmCreateLabel (trace->gotos.dialog,"",arglist,6);
	DManageChild (trace->gotos.notelabel, trace, MC_NOKEYS);

	/* Create the print note text widget */
	XtSetArg (arglist[0], XmNrows, 1);
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNleftOffset, 5);
	XtSetArg (arglist[3], XmNcolumns, 30);
	XtSetArg (arglist[4], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[5], XmNtopOffset, 0);
	XtSetArg (arglist[6], XmNtopWidget, trace->gotos.notelabel);
	XtSetArg (arglist[7], XmNresizeHeight, FALSE);
	XtSetArg (arglist[8], XmNeditMode, XmSINGLE_LINE_EDIT);
	trace->gotos.notetext = XmCreateText (trace->gotos.dialog,"notetext",arglist,9);
	DAddCallback (trace->gotos.notetext, XmNactivateCallback, win_goto_ok_cb, trace);
	DManageChild (trace->gotos.notetext, trace, MC_NOKEYS);

	/* Ok/apply/cancel */
	ok_apply_cancel (&trace->gotos.okapply, trace->gotos.dialog,
			 dmanage_last,
			 (XtCallbackProc)win_goto_ok_cb, trace,
			 NULL, NULL,
			 NULL, NULL,
			 (XtCallbackProc)win_goto_cancel_cb, trace);
    }

    /* right units */
    XtSetArg (arglist[0], XmNlabelString,
	      XmStringCreateSimple (time_units_to_string (global->timerep, FALSE)));
    XtSetValues (trace->gotos.label2, arglist, 1);

    /* make right one active */
    XtSetArg (arglist[0], XmNmenuHistory, trace->gotos.pulldownbutton[global->goto_color + 1]);
    XtSetValues (trace->gotos.options, arglist, 1);
    XmTextSetString (trace->gotos.text, "");

    /* Must redraw color box on any exposures */
    XtAddEventHandler (trace->gotos.dialog, ExposureMask, TRUE,
		       (XtEventHandler)win_goto_option_cb, trace);

    /* manage the dialog on the screen */
    DManageChild (trace->gotos.dialog, trace, MC_NOKEYS);
    XSync (global->display,0);

    /* Update button - must be after manage*/
    win_goto_option_cb (w, trace, NULL);
}

void    win_goto_option_cb (
    Widget	w,
    Trace_t	*trace,
    XmSelectionBoxCallbackStruct *cb)	/* OR     XButtonPressedEvent	*ev; */
    /* Puts the color in the option menu, since Xm does not copy colors on selection */
    /* Also used as an event callback for exposures */
{
    int		i;
    Widget 	button;
    Position 	x,y,height,width;

    if (DTPRINT_ENTRY) printf ("In win_goto_option_cb - trace=%p\n",trace);

    i = option_to_number(trace->gotos.options, trace->gotos.pulldownbutton, MAX_SRCH);

    if (i <= 0) {
	/* Put "None" in the button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("None"));
	XtSetValues ( XmOptionButtonGadget (trace->gotos.options), arglist, 1);
    }
    else {
	/* Put "Place" in the button */
	XSetForeground (global->display, trace->gc, trace->xcolornums[i-1]);
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Place"));
	XtSetValues ( XmOptionButtonGadget (trace->gotos.options), arglist, 1);

	/* Find the coords of the button */
	button = XmOptionButtonGadget (trace->gotos.options);
	x = (((XmCascadeButtonWidget) button)->core.x);
	y = (((XmCascadeButtonWidget) button)->core.y);
	width = (((XmCascadeButtonWidget) button)->core.width);
	height = (((XmCascadeButtonWidget) button)->core.height);

	/* Fill the button with the color */
	XSetForeground (global->display, trace->gc, trace->xcolornums[i-1]);
	XFillRectangle (global->display, XtWindow (button), trace->gc,
		       x, y, width, height);
    }
}


void    win_goto_ok_cb (
    Widget	w,
    Trace_t	*trace,
    XmSelectionBoxCallbackStruct *cb)
{
    char	*strg;
    DTime_t	time;
    char	*note = NULL;

    if (DTPRINT_ENTRY) printf ("In win_goto_ok_cb - trace=%p\n",trace);

    /* Get menu */
    global->goto_color = -1 + option_to_number(trace->gotos.options, trace->gotos.pulldownbutton, MAX_SRCH);

    if (DTPRINT_ENTRY) printf ("\tnew goto_color=%d\n",global->goto_color);

    /* Get value */
    strg = XmTextGetString (trace->gotos.text);
    time = string_to_time (trace, strg);
    note = XmTextGetString (trace->gotos.notetext);
    if (!note[0]) note=NULL;

    /* unmanage the popup window */
    XtRemoveEventHandler (trace->gotos.dialog, ExposureMask, TRUE,
			  (XtEventHandler)win_goto_option_cb, trace);
    XtUnmanageChild (trace->gotos.dialog);

    if (time > 0) {
	/* Center it on the screen */
	global->time = time - ( TIME_WIDTH (trace) / 2);

	/* Limit time extent */
	/* V6.3 bug - Don't subtract the window length */
	if ( time > trace->end_time ) {
	    time = trace->end_time;
	}
	if ( time < trace->start_time ) {
	    time = trace->start_time;
	}

	/* Add cursor if wanted */
	if (global->goto_color > 0) {
	    /* make the cursor */
	    cur_new (time, global->goto_color, USER, note);
	}

	new_time (trace);
    }
}

void    win_goto_cancel_cb (
    Widget	w,
    Trace_t	*trace,
    XmAnyCallbackStruct	*cb)
{
    if (DTPRINT_ENTRY) printf ("In win_goto_cancel_cb - trace=%p\n",trace);

    /* unmanage the popup on the screen */
    XtRemoveEventHandler (trace->gotos.dialog, ExposureMask, TRUE,
			  (XtEventHandler)win_goto_option_cb, trace);
    XtUnmanageChild (trace->gotos.dialog);
}

/**********************************************************************/

void    debug_toggle_print_cb (
    Widget	w)
{
    if (DTPRINT) DTPRINT = 0;
    else DTPRINT = ~0;
    printf ("Printing now %d\n",DTPRINT);
}

