/******************************************************************************
 * DESCRIPTION: Dinotrace source: grid drawing and requestors
 *
 * This file is part of Dinotrace.
 *
 * Author: Wilson Snyder <wsnyder@wsnyder.org>
 *
 * Code available from: https://www.veripool.org/dinotrace
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
 * the Free Software Foundation; either version 3, or (at your option)
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

#include <Xm/Label.h>
#include <Xm/LabelP.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/CascadeBP.h>
#include <Xm/ToggleB.h>
#include <Xm/SelectioB.h>
#include <Xm/Text.h>
#include <Xm/Form.h>
#include <Xm/Separator.h>
#include <Xm/BulletinB.h>

#include "functions.h"

/**********************************************************************/

extern void	grid_customize_ok_cb(Widget w, Trace_t* trace, XmAnyCallbackStruct* cb);
extern void	grid_customize_apply_cb(Widget w, Trace_t* trace, XmAnyCallbackStruct* cb);
extern void	grid_customize_reset_cb(Widget w, Trace_t* trace, XmAnyCallbackStruct* cb);
extern void	grid_customize_sensitives_cb(Widget w, Trace_t* trace, XmAnyCallbackStruct* cb);
extern void	grid_customize_option_cb(Widget w, Trace_t* trace, XmAnyCallbackStruct* cb);
extern void	grid_customize_align_cb (Widget w, Trace_t *trace, XmAnyCallbackStruct *cb);


/****************************** UTILITIES ******************************/

DTime_t	grid_primary_period (
    const Trace_t	*trace)
{
    int		grid_num;
    const Grid_t	*grid_ptr;
    DTime_t	step;
    step = 1;  /* Don't return 0, might get div by zero */
    for (grid_num=0; grid_num<MAXGRIDS; grid_num++) {
	grid_ptr = &(trace->grid[grid_num]);
	if (grid_ptr->visible) {
	    step = grid_ptr->period;
	    break;
	}
    }
    return (step);
}


/****************************** AUTO GRIDS ******************************/

static void	grid_calc_auto (
    Trace_t	*trace,
    Grid_t	*grid_ptr)
{
    Signal_t	*sig_ptr;
    Value_t	*cptr;
    int		rise1=0, rise2=0;
    int		fall1=0, fall2=0;

    if (DTPRINT_ENTRY) printf ("In grid_calc_auto\n");
    grid_ptr->signal_synced = NULL;

    /* Determine period, rise point and fall point of first signal */
    sig_ptr = sig_wildmat_signame (trace, grid_ptr->signal);

    /* Skip phase_count, as it is a CCLI artifact */
    if (sig_ptr && !strncmp(sig_ptr->signame, "phase_count", 11)) sig_ptr=sig_ptr->forward;

    /* Can't do anything if don't have a signal yet */
    if (!sig_ptr) {
	/* Effectively disable grid */
	switch (grid_ptr->period_auto) {
	case PA_AUTO:
	    grid_ptr->period = 0;
	    break;
	case PA_USER:
	case PA_EDGE:
	    break;
	}
	return;
    }

    /* Skip to end */
    cptr = value_at_time (sig_ptr, EOT);
    /* Ignore last - determined by EOT, not period */
    if ( cptr > sig_ptr->cptr) cptr = CPTR_PREV(cptr);

    /* Move back 3 grids, if we can */
    while ( cptr > sig_ptr->bptr) {
	cptr = CPTR_PREV(cptr);
	switch (cptr->siglw.stbits.state) {
	case STATE_1:
	    if (!rise2) rise2 = CPTR_TIME(cptr);
	    else if (!rise1) rise1 = CPTR_TIME(cptr);
	    break;
	case STATE_0:
	    if (!fall2) fall2 = CPTR_TIME(cptr);
	    else if (!fall1) fall1 = CPTR_TIME(cptr);
	    break;
	case STATE_B32:
	case STATE_B128:
	case STATE_F32:
	case STATE_F128:
	    if (!rise2) rise2 = CPTR_TIME(cptr);
	    else if (!rise1) rise1 = CPTR_TIME(cptr);
	    if (!fall2) fall2 = CPTR_TIME(cptr);
	    else if (!fall1) fall1 = CPTR_TIME(cptr);
	    break;
	}
    }

    /* Set defaults based on changes */
    switch (grid_ptr->period_auto) {
    case PA_AUTO:
    case PA_EDGE:
	grid_ptr->signal_synced = sig_ptr;
	if (rise1 < rise2)	grid_ptr->period = rise2 - rise1;
	else if (fall1 < fall2) grid_ptr->period = fall2 - fall1;
	if (grid_ptr->align_auto == AA_BOTH) {
	    if (rise2 && fall2)	grid_ptr->period = ABS(fall2 - rise2);
	    else if (rise1 && fall1)	grid_ptr->period = ABS(fall1 - rise1);
	}
	break;
    default: break;	/* User defined */
    }
    if (grid_ptr->period < 1) grid_ptr->period = 1;	/* Prevents round-down to 0 causing infinite loop */

    /* Alignment */
    switch (grid_ptr->align_auto) {
    case AA_ASS:
	grid_ptr->signal_synced = sig_ptr;
	if (rise1) grid_ptr->alignment = rise1 % grid_ptr->period;
	break;
    case AA_DEASS:
	grid_ptr->signal_synced = sig_ptr;
	if (fall1) grid_ptr->alignment = fall1 % grid_ptr->period;
	break;
    case AA_BOTH:
	grid_ptr->signal_synced = sig_ptr;
	if (fall1) grid_ptr->alignment = fall1 % grid_ptr->period;
	if (rise1) grid_ptr->alignment = rise1 % grid_ptr->period;
	break;
    default: break;	/* User defined */
    }

    if (DTPRINT_FILE) printf ("grid autoset signal %s align=%d %d\n", sig_ptr->signame,
			      grid_ptr->align_auto, grid_ptr->period_auto);
    if (DTPRINT_FILE) printf ("grid rises=%d,%d, falls=%d,%d, period=%d, align=%d\n",
			      rise1,rise2, fall1,fall2,
			      grid_ptr->period, grid_ptr->alignment);
}

void	grid_calc_autos (
    Trace_t		*trace)
{
    int		grid_num;

    for (grid_num=0; grid_num<MAXGRIDS; grid_num++) {
	grid_calc_auto (trace, &(trace->grid[grid_num]));
    }

    draw_needed (trace);
}

/****************************** EXAMINE ******************************/

char *grid_examine_string (
    /* Return string with examine information in it */
    Trace_t	*trace,
    Grid_t	*grid_ptr,
    DTime_t	time)
{
    static char	strg[2000];
    char	strg2[2000];

    if (DTPRINT_ENTRY) printf ("val_examine_popup_grid_string\n");

    sprintf (strg, "Grid #%d at Time ", grid_ptr->grid_num);
    time_to_string (trace, strg2, time, FALSE);
    strcat (strg, strg2);
    strcat (strg, "\nEvery ");
    time_to_string (trace, strg2, grid_ptr->period, FALSE);
    strcat (strg, strg2);
    if (grid_ptr->period) {
	strcat (strg, " starting at ");
	time_to_string (trace, strg2, (grid_ptr->alignment % grid_ptr->period), FALSE);
	strcat (strg, strg2);
    }
    strcat (strg, "\n");
    if (grid_ptr->signal_synced) {
	strcat (strg, "Synced to ");
	strcat (strg, grid_ptr->signal_synced->signame);
	strcat (strg, "\n");
    }

    return (strg);
}

/****************************** MENU OPTIONS ******************************/

static void    grid_align_choose (
    Trace_t	*trace,
    Grid_t	*grid_ptr)
{
    DTime_t	time;
    Position	x1,x2,y1,y2;
    int		last_x;
    XEvent	event;
    XMotionEvent *em;
    XButtonEvent *eb;

    if (DTPRINT_ENTRY) printf ("In grid_align_choose\n");

    /* not sure why this has to be done but it must be done */
    XUngrabPointer (XtDisplay (trace->work),CurrentTime);

    /* select the events the widget will respond to */
    XSelectInput (XtDisplay (trace->work),XtWindow (trace->work),
		 ButtonReleaseMask|PointerMotionMask|StructureNotifyMask|ExposureMask);

    /* change the GC function to drag the cursor */
    xgcv.function = GXinvert;
    XChangeGC (global->display,trace->gc,GCFunction,&xgcv);
    XSync (global->display,0);

    /* loop and service events until button is released */
    last_x = -1;	/* Skip first draw */
    y1 = 25;
    while ( 1 ) {
	/* wait for next event */
	XNextEvent (XtDisplay (trace->work),&event);

	/* if the pointer moved, erase previous line and draw new one */
	if (event.type == MotionNotify) {
	    XSetForeground (global->display, trace->gc, trace->xcolornums[grid_ptr->color]);
	    em = (XMotionEvent *)&event;
	    y2 = trace->height - trace->ystart + global->sighgt;
	    if (last_x >= 0) {
		x1 = x2 = last_x;
		XDrawLine (global->display,trace->wind,trace->gc,x1,y1,x2,y2);
	    }
	    x1 = x2 = last_x = em->x;
	    XDrawLine (global->display,trace->wind,trace->gc,x1,y1,x2,y2);
	    XSetForeground (global->display, trace->gc, trace->xcolornums[0]);
	}

	/* if window was exposed, must redraw it */
	if (event.type == Expose) win_expose_cb (0,trace);
	/* if window was resized, must redraw it */
	if (event.type == ConfigureNotify) win_resize_cb (0,trace);

	/* button released - calculate cursor position and leave the loop */
	if (event.type == ButtonRelease || event.type == ButtonPress) {
	    /* ButtonPress in case user is freaking out, some strange X behavior caused the ButtonRelease to be lost */
	    eb = (XButtonReleasedEvent *)&event;
	    time = posx_to_time_edge (trace, eb->x, eb->y);
	    break;
	}

	if (global->redraw_needed && !XtAppPending (global->appcontext)) {
	    /* change the GC function back to its default */
	    xgcv.function = GXcopy;
	    XChangeGC (global->display,trace->gc,GCFunction,&xgcv);
	    draw_perform();
	    /* change the GC function to drag the cursor */
	    xgcv.function = GXinvert;
	    XChangeGC (global->display,trace->gc,GCFunction,&xgcv);
	}
    }

    /* reset the events the widget will respond to */
    XSelectInput (XtDisplay (trace->work),XtWindow (trace->work),
		 ButtonPressMask|StructureNotifyMask|ExposureMask);

    /* change the GC function back to its default */
    xgcv.function = GXcopy;
    XChangeGC (global->display,trace->gc,GCFunction,&xgcv);

    /* make change */
    grid_ptr->alignment = time;
}


void grid_customize_align_cb (
    Widget		w,
    Trace_t		*trace,
    XmAnyCallbackStruct	*cb)
{
    int			grid_num;

    if (DTPRINT_ENTRY) printf ("In grid_customize_align_cb - trace=%p\n",trace);

    /*   Determine which grid was selected based on which array */
    for (grid_num = 0; grid_num < (MAXGRIDS-1); grid_num++) {	/* -1 as MAXGRID will be the "default if none match*/
	if (w == trace->gridscus.grid[grid_num].align) break;
    }
    global->click_grid = &(trace->grid[grid_num]);

    /* Apply changes */
    grid_customize_ok_cb (w,trace,cb);

    /* remove any previous events */
    remove_all_events (trace);

    /* process all subsequent button presses as grid aligns */
    grid_align_choose (trace, &(trace->grid[grid_num]));

    /* Redraw */
    draw_needed (trace);

    /* Show new menu */
    grid_customize_cb (trace->main);
}


void grid_reset_cb (
    Widget		w)
{
    Trace_t *trace = widget_to_trace(w);
    int		grid_num;
    Grid_t	*grid_ptr;

    if (DTPRINT_ENTRY) printf ("In grid_reset_cb - trace=%p\n",trace);

    /* reset the alignment back to zero and resolution to 100 */
    for (grid_num=0; grid_num<MAXGRIDS; grid_num++) {
	grid_ptr = &(trace->grid[grid_num]);

	grid_ptr->grid_num = grid_num;
	strcpy (grid_ptr->signal, "*");
	grid_ptr->color = grid_num+1;
	grid_ptr->period = 100;
	grid_ptr->alignment = 0;
	grid_ptr->period_auto = PA_AUTO;
	grid_ptr->align_auto = AA_ASS;
	if (grid_num == 0) {
	    /* Enable only one grid by default */
	    grid_ptr->visible = TRUE;
	    grid_ptr->wide_line = TRUE;
	}
	else {
	    grid_ptr->visible = FALSE;
	    grid_ptr->wide_line = FALSE;
	}
     }

    grid_calc_autos (trace);

    /* cancel the button actions */
    remove_all_events (trace);
}

/****************************** CUSTOMIZE ******************************/

void    grid_customize_sensitives_cb (
    Widget	w,
    Trace_t	*trace,
    XmAnyCallbackStruct *cb)
{
    int		grid_num;
    int		opt;

    if (DTPRINT_ENTRY) printf ("In grid_customize_sensitives_cb\n");

    for (grid_num=0; grid_num<MAXGRIDS; grid_num++) {
	/* Period value, desensitize if automatic */
	opt = option_to_number(trace->gridscus.grid[grid_num].autoperiod_options, trace->gridscus.grid[grid_num].autoperiod_pulldownbutton, 2);
	XtSetArg (arglist[0], XmNsensitive, (opt==0));
	XtSetValues (trace->gridscus.grid[grid_num].period, arglist, 1);

	/* Edge Mode */
	opt = option_to_number(trace->gridscus.grid[grid_num].autoalign_options, trace->gridscus.grid[grid_num].autoalign_pulldownbutton, 2);
	XtSetArg (arglist[0], XmNsensitive, (opt==0));
	XtSetValues (trace->gridscus.grid[grid_num].align, arglist, 1);
    }
}

void    grid_customize_widget_update_cb (
    Widget	w,
    Trace_t	*trace,
    XmAnyCallbackStruct *cb)
{
    int		grid_num;
    Grid_t	*grid_ptr;
    char	strg[MAXTIMELEN];
    int		opt;

    if (DTPRINT_ENTRY) printf ("In grid_customize_widget_update\n");

    for (grid_num=0; grid_num<MAXGRIDS; grid_num++) {
	grid_ptr = &(trace->grid[grid_num]);

	XmTextSetString (trace->gridscus.grid[grid_num].signal, grid_ptr->signal);

	XmToggleButtonSetState (trace->gridscus.grid[grid_num].visible, (grid_ptr->visible), TRUE);
	XmToggleButtonSetState (trace->gridscus.grid[grid_num].wide_line, (grid_ptr->wide_line), TRUE);

	/* Period value */
	time_to_string (trace, strg, grid_ptr->period, TRUE);
	XmTextSetString (trace->gridscus.grid[grid_num].period, strg);

	/* Color */
	XtSetArg (arglist[0], XmNmenuHistory, trace->gridscus.grid[grid_num].pulldownbutton[grid_ptr->color]);
	XtSetValues (trace->gridscus.grid[grid_num].options, arglist, 1);
	/* Must redraw color box on any exposures */
	XtAddEventHandler (trace->gridscus.dialog, ExposureMask, TRUE, (XtEventHandler)grid_customize_option_cb, trace);

	/* period Mode */
	opt=(int)grid_ptr->period_auto;	/* Weedy, enums are defined to be equiv to descriptions */
	XtSetArg (arglist[0], XmNmenuHistory, trace->gridscus.grid[grid_num].autoperiod_pulldownbutton[opt]);
	XtSetValues (trace->gridscus.grid[grid_num].autoperiod_options, arglist, 1);

	/* Edge Mode */
	opt=(int)grid_ptr->align_auto;	/* Weedy, enums are defined to be equiv to descriptions */
	XtSetArg (arglist[0], XmNmenuHistory, trace->gridscus.grid[grid_num].autoalign_pulldownbutton[opt]);
	XtSetValues (trace->gridscus.grid[grid_num].autoalign_options, arglist, 1);
    }

    /* Update sensitivity buttons too */
    grid_customize_sensitives_cb (NULL, trace, NULL);
}

void    grid_customize_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    int		grid_num;
    char	strg[30];
    int		i;
    Widget	form;

    if (DTPRINT_ENTRY) printf ("In grid_customize_cb - trace=%p\n",trace);

    if (!trace->gridscus.dialog) {
	XtSetArg (arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg (arglist[1], XmNdialogTitle, XmStringCreateSimple ("Grid Customization") );
	trace->gridscus.dialog = XmCreateFormDialog (trace->work,"search",arglist,2);

	XtSetArg (arglist[2], XmNverticalSpacing, 20);
	XtSetArg (arglist[3], XmNhorizontalSpacing, 20);
	trace->gridscus.masterForm = XmCreateForm (trace->gridscus.dialog,"masterForm",arglist,2);

	for (grid_num=0; grid_num<MAXGRIDS; grid_num++) {
	    XtSetArg (arglist[0], XmNhorizontalSpacing, 10);
	    XtSetArg (arglist[1], XmNverticalSpacing, 7);
	    XtSetArg (arglist[2], XmNtopAttachment, (grid_num==0 || grid_num==2) ? XmATTACH_FORM : XmATTACH_WIDGET );
	    XtSetArg (arglist[3], XmNtopWidget, trace->gridscus.grid[grid_num&~1].form);
	    XtSetArg (arglist[4], XmNleftAttachment, (grid_num==0 || grid_num==1) ? XmATTACH_FORM : XmATTACH_WIDGET );
	    XtSetArg (arglist[5], XmNleftWidget, trace->gridscus.grid[grid_num&~2].form);
	    form = trace->gridscus.grid[grid_num].form
		= XmCreateForm (trace->gridscus.masterForm, "cform", arglist, 6);

	    /* Create label for this grid */
	    sprintf (strg, "Grid #%d", grid_num);
	    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple (strg));
	    XtSetArg (arglist[1], XmNtopAttachment, XmATTACH_FORM );
	    XtSetArg (arglist[2], XmNleftAttachment, XmATTACH_FORM );
	    trace->gridscus.grid[grid_num].label1 = XmCreateLabel (form,"label",arglist,3);
	    DManageChild (trace->gridscus.grid[grid_num].label1, trace, MC_NOKEYS);

	    /* signal name */
	    XtSetArg (arglist[0], XmNrows, 1);
	    XtSetArg (arglist[1], XmNcolumns, 30);
	    XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[3], XmNtopWidget, trace->gridscus.grid[grid_num].label1);
	    XtSetArg (arglist[4], XmNleftAttachment, XmATTACH_FORM );
	    XtSetArg (arglist[5], XmNrightAttachment, XmATTACH_FORM );
	    XtSetArg (arglist[6], XmNresizeHeight, FALSE);
	    XtSetArg (arglist[7], XmNeditMode, XmSINGLE_LINE_EDIT);
	    trace->gridscus.grid[grid_num].signal = XmCreateText (form,"textn",arglist,8);
	    DManageChild (trace->gridscus.grid[grid_num].signal, trace, MC_NOKEYS);

	    /* visible button */
	    XtSetArg (arglist[0], XmNtopAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[1], XmNtopWidget, trace->gridscus.grid[grid_num].signal);
	    XtSetArg (arglist[2], XmNleftAttachment, XmATTACH_FORM );
	    XtSetArg (arglist[3], XmNlabelString, XmStringCreateSimple ("Visible"));
	    XtSetArg (arglist[4], XmNshadowThickness, 1);
	    trace->gridscus.grid[grid_num].visible = XmCreateToggleButton (form,"visn",arglist,5);
	    DManageChild (trace->gridscus.grid[grid_num].visible, trace, MC_NOKEYS);

	    /* double button */
	    XtSetArg (arglist[0], XmNtopAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[1], XmNtopWidget, trace->gridscus.grid[grid_num].signal);
	    XtSetArg (arglist[2], XmNleftAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[3], XmNleftWidget, trace->gridscus.grid[grid_num].visible);
	    XtSetArg (arglist[4], XmNlabelString, XmStringCreateSimple ("Wide Line"));
	    XtSetArg (arglist[5], XmNshadowThickness, 1);
	    trace->gridscus.grid[grid_num].wide_line = XmCreateToggleButton (form,"wideln",arglist,6);
	    DManageChild (trace->gridscus.grid[grid_num].wide_line, trace, MC_NOKEYS);

	    /*Color options */
	    trace->gridscus.grid[grid_num].pulldown = XmCreatePulldownMenu (form,"pulldown",arglist,0);

	    for (i=0; i<=MAX_SRCH; i++) {
		XtSetArg (arglist[0], XmNbackground, trace->xcolornums[i] );
		XtSetArg (arglist[1], XmNmarginRight, 20);
		XtSetArg (arglist[2], XmNmarginBottom, 2);
		XtSetArg (arglist[3], XmNlabelString, XmStringCreateSimple ("Color"));
		trace->gridscus.grid[grid_num].pulldownbutton[i] =
		    XmCreatePushButton (trace->gridscus.grid[grid_num].pulldown,"",arglist,4);
		DAddCallback (trace->gridscus.grid[grid_num].pulldownbutton[i], XmNactivateCallback, grid_customize_option_cb, trace);
		DManageChild (trace->gridscus.grid[grid_num].pulldownbutton[i], trace, MC_NOKEYS);
	    }
	    XtSetArg (arglist[0], XmNsubMenuId, trace->gridscus.grid[grid_num].pulldown);
	    XtSetArg (arglist[1], XmNtopAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[2], XmNtopWidget, trace->gridscus.grid[grid_num].signal);
	    XtSetArg (arglist[3], XmNleftAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[4], XmNleftWidget, trace->gridscus.grid[grid_num].wide_line);
	    trace->gridscus.grid[grid_num].options = XmCreateOptionMenu (form,"options",arglist,5);
	    DManageChild (trace->gridscus.grid[grid_num].options, trace, MC_NOKEYS);

	    /* auto button */
	    trace->gridscus.grid[grid_num].autoperiod_pulldown = XmCreatePulldownMenu (form,"autoperiod_pulldown",arglist,0);

	    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Manual Periodic") );
	    trace->gridscus.grid[grid_num].autoperiod_pulldownbutton[0] =
		XmCreatePushButtonGadget (trace->gridscus.grid[grid_num].autoperiod_pulldown,"pdbutton0",arglist,1);
	    DAddCallback (trace->gridscus.grid[grid_num].autoperiod_pulldownbutton[0], XmNactivateCallback, grid_customize_sensitives_cb, trace);
	    DManageChild (trace->gridscus.grid[grid_num].autoperiod_pulldownbutton[0], trace, MC_NOKEYS);

	    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Auto Periodic") );
	    trace->gridscus.grid[grid_num].autoperiod_pulldownbutton[1] =
		XmCreatePushButtonGadget (trace->gridscus.grid[grid_num].autoperiod_pulldown,"pdbutton0",arglist,1);
	    DAddCallback (trace->gridscus.grid[grid_num].autoperiod_pulldownbutton[1], XmNactivateCallback, grid_customize_sensitives_cb, trace);
	    DManageChild (trace->gridscus.grid[grid_num].autoperiod_pulldownbutton[1], trace, MC_NOKEYS);

	    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Auto Edges Only") );
	    trace->gridscus.grid[grid_num].autoperiod_pulldownbutton[2] =
	      XmCreatePushButtonGadget (trace->gridscus.grid[grid_num].autoperiod_pulldown,"pdbutton0",arglist,1);
	    DAddCallback (trace->gridscus.grid[grid_num].autoperiod_pulldownbutton[2], XmNactivateCallback, grid_customize_sensitives_cb, trace);
	    DManageChild (trace->gridscus.grid[grid_num].autoperiod_pulldownbutton[2], trace, MC_NOKEYS);

	    XtSetArg (arglist[0], XmNsubMenuId, trace->gridscus.grid[grid_num].autoperiod_pulldown);
	    XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	    XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[3], XmNtopWidget, trace->gridscus.grid[grid_num].options);
	    trace->gridscus.grid[grid_num].autoperiod_options = XmCreateOptionMenu (form,"options",arglist,4);
	    DManageChild (trace->gridscus.grid[grid_num].autoperiod_options, trace, MC_NOKEYS);

	    /* Period */
	    XtSetArg (arglist[0], XmNrows, 1);
	    XtSetArg (arglist[1], XmNcolumns, 10);
	    XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[3], XmNtopWidget, trace->gridscus.grid[grid_num].options);
	    XtSetArg (arglist[4], XmNleftAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[5], XmNleftWidget, trace->gridscus.grid[grid_num].autoperiod_options);
	    XtSetArg (arglist[6], XmNresizeHeight, FALSE);
	    XtSetArg (arglist[7], XmNeditMode, XmSINGLE_LINE_EDIT);
	    trace->gridscus.grid[grid_num].period = XmCreateText (form,"textn",arglist,8);
	    DManageChild (trace->gridscus.grid[grid_num].period, trace, MC_NOKEYS);

	    /* auto button */
	    trace->gridscus.grid[grid_num].autoalign_pulldown = XmCreatePulldownMenu (form,"autoalign_pulldown",arglist,0);

	    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Manual Edge") );
	    trace->gridscus.grid[grid_num].autoalign_pulldownbutton[0] =
		XmCreatePushButtonGadget (trace->gridscus.grid[grid_num].autoalign_pulldown,"pdbutton0",arglist,1);
	    DAddCallback (trace->gridscus.grid[grid_num].autoalign_pulldownbutton[0], XmNactivateCallback, (XtCallbackProc)grid_customize_sensitives_cb, trace);
	    DManageChild (trace->gridscus.grid[grid_num].autoalign_pulldownbutton[0], trace, MC_NOKEYS);

	    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Assertion Edge") );
	    trace->gridscus.grid[grid_num].autoalign_pulldownbutton[1] =
		XmCreatePushButtonGadget (trace->gridscus.grid[grid_num].autoalign_pulldown,"pdbutton0",arglist,1);
	    DAddCallback (trace->gridscus.grid[grid_num].autoalign_pulldownbutton[1], XmNactivateCallback, (XtCallbackProc)grid_customize_sensitives_cb, trace);
	    DManageChild (trace->gridscus.grid[grid_num].autoalign_pulldownbutton[1], trace, MC_NOKEYS);

	    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Deassertion Edge") );
	    trace->gridscus.grid[grid_num].autoalign_pulldownbutton[2] =
		XmCreatePushButtonGadget (trace->gridscus.grid[grid_num].autoalign_pulldown,"pdbutton0",arglist,1);
	    DAddCallback (trace->gridscus.grid[grid_num].autoalign_pulldownbutton[2], XmNactivateCallback, (XtCallbackProc)grid_customize_sensitives_cb, trace);
	    DManageChild (trace->gridscus.grid[grid_num].autoalign_pulldownbutton[2], trace, MC_NOKEYS);

	    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Both Edges") );
	    trace->gridscus.grid[grid_num].autoalign_pulldownbutton[3] =
		XmCreatePushButtonGadget (trace->gridscus.grid[grid_num].autoalign_pulldown,"pdbutton0",arglist,1);
	    DAddCallback (trace->gridscus.grid[grid_num].autoalign_pulldownbutton[3], XmNactivateCallback, (XtCallbackProc)grid_customize_sensitives_cb, trace);
	    DManageChild (trace->gridscus.grid[grid_num].autoalign_pulldownbutton[3], trace, MC_NOKEYS);

	    XtSetArg (arglist[0], XmNsubMenuId, trace->gridscus.grid[grid_num].autoalign_pulldown);
	    XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	    XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[3], XmNtopWidget, trace->gridscus.grid[grid_num].period);
	    trace->gridscus.grid[grid_num].autoalign_options = XmCreateOptionMenu (form,"options",arglist,4);
	    DManageChild (trace->gridscus.grid[grid_num].autoalign_options, trace, MC_NOKEYS);

	    /* Edge */
	    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Set Alignment") );
	    XtSetArg (arglist[1], XmNtopAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[2], XmNtopWidget, trace->gridscus.grid[grid_num].period);
	    XtSetArg (arglist[3], XmNleftAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[4], XmNleftWidget, trace->gridscus.grid[grid_num].autoalign_options);
	    trace->gridscus.grid[grid_num].align = XmCreatePushButton (form,"align",arglist,5);
	    DAddCallback (trace->gridscus.grid[grid_num].align, XmNactivateCallback, (XtCallbackProc)grid_customize_align_cb, trace);
	    DManageChild (trace->gridscus.grid[grid_num].align, trace, MC_NOKEYS);

	    DManageChild (form, trace, MC_NOKEYS);
	}

	DManageChild (trace->gridscus.masterForm, trace, MC_NOKEYS);

	/* Ok/apply/cancel */
	ok_apply_cancel (&trace->gridscus.okapply, trace->gridscus.dialog,
			 trace->gridscus.masterForm,
			 (XtCallbackProc)grid_customize_ok_cb, trace,
			 (XtCallbackProc)grid_customize_apply_cb, trace,
			 (XtCallbackProc)grid_customize_reset_cb, trace,
			 (XtCallbackProc)unmanage_cb, (Trace_t*)trace->gridscus.dialog);
    }

    /* Copy settings to local area to allow cancel to work */
    grid_customize_widget_update_cb (NULL, trace, NULL);

    /* manage the dialog on the screen */
    DManageChild (trace->gridscus.dialog, trace, MC_NOKEYS);
}

void    grid_customize_option_cb (
    Widget	w,
    Trace_t	*trace,
    XmAnyCallbackStruct *cb)	/* OR     XButtonPressedEvent	*ev; */
    /* Puts the color in the option menu, since Xm does not copy colors on selection */
    /* Also used as an event callback for exposures */
{
    int		i;
    Widget 	button;
    Position 	x,y,height,width;
    int		grid_num;

    if (DTPRINT_ENTRY) printf ("In grid_customize_option_cb - trace=%p\n",trace);

    for (grid_num=0; grid_num<MAXGRIDS; grid_num++) {
	i = option_to_number(trace->gridscus.grid[grid_num].options, trace->gridscus.grid[grid_num].pulldownbutton, MAX_SRCH);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Color"));
	XtSetValues ( XmOptionButtonGadget (trace->gridscus.grid[grid_num].options), arglist, 1);

	/* Find the coords of the button */
	button = XmOptionButtonGadget (trace->gridscus.grid[grid_num].options);
	x = (((XmCascadeButtonWidget) button)->core.x);
	y = (((XmCascadeButtonWidget) button)->core.y);
	width = (((XmCascadeButtonWidget) button)->core.width);
	height = (((XmCascadeButtonWidget) button)->core.height);

	/* Fill the button with the color */
	XSetForeground (global->display, trace->gc, trace->xcolornums[i]);
	XFillRectangle (global->display, XtWindow (button), trace->gc,
			x, y, width, height);
    }

    if (DTPRINT_ENTRY) printf ("Done grid_customize_option_cb - trace=%p\n",trace);
}

void    grid_customize_ok_cb (
    Widget	w,
    Trace_t	*trace,
    XmAnyCallbackStruct *cb)
{
    int			grid_num;
    Grid_t	*grid_ptr;

    if (DTPRINT_ENTRY) printf ("In grid_customize_ok_cb - trace=%p\n",trace);

    for (grid_num=0; grid_num<MAXGRIDS; grid_num++) {
	grid_ptr = &(trace->grid[grid_num]);

	strcpy (grid_ptr->signal, XmTextGetString (trace->gridscus.grid[grid_num].signal));
	grid_ptr->visible = XmToggleButtonGetState (trace->gridscus.grid[grid_num].visible);
	grid_ptr->wide_line = XmToggleButtonGetState (trace->gridscus.grid[grid_num].wide_line);
	grid_ptr->color = option_to_number(trace->gridscus.grid[grid_num].options, trace->gridscus.grid[grid_num].pulldownbutton, MAX_SRCH);
	grid_ptr->period_auto = (PeriodAuto_t)option_to_number(trace->gridscus.grid[grid_num].autoperiod_options, trace->gridscus.grid[grid_num].autoperiod_pulldownbutton, 2);
	grid_ptr->align_auto = (AlignAuto_t)option_to_number(trace->gridscus.grid[grid_num].autoalign_options, trace->gridscus.grid[grid_num].autoalign_pulldownbutton, 3);
	if (grid_ptr->period_auto == PA_USER) {
	    grid_ptr->period = string_to_time (trace, XmTextGetString (trace->gridscus.grid[grid_num].period));
	}
    }

    XtUnmanageChild (trace->gridscus.dialog);

    grid_calc_autos (trace);
    draw_needed (trace);
}

void    grid_customize_apply_cb (
    Widget	w,
    Trace_t	*trace,
    XmAnyCallbackStruct *cb)
{
    if (DTPRINT_ENTRY) printf ("In grid_customize_apply_cb - trace=%p\n",trace);

    grid_customize_ok_cb (w,trace,cb);
    grid_customize_cb (trace->main);
}

void    grid_customize_reset_cb (
    Widget	w,
    Trace_t	*trace,
    XmAnyCallbackStruct *cb)
{
    if (DTPRINT_ENTRY) printf ("In grid_customize_resize_cb - trace=%p\n",trace);

    XtUnmanageChild (trace->gridscus.dialog);
    grid_reset_cb (trace->main);
    grid_customize_cb (trace->main);
}

