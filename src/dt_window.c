/******************************************************************************
 *
 * Filename:
 *     dt_window.c
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
 *
 * Modification History:
 *     AAG	 5-Jul-89	Original Version
 *     AAG	22-Aug-90	Base Level V4.1
 *     AAG	29-Apr-91	Use X11, removed aggregate initializer, fixed
 *				 casts for Ultrix support
 *     AAG	 9-Jul-91	Fixed sigstart on vscroll inc/dec/drag and
 *				 added get_geom call
 *     WPS	 8-Jan-93	Added pageinc and unitdec routines
 *     WPS	15-Feb-93	Added zoom, full, new_res routines
 */


#include <X11/Xlib.h>
#include <Xm/Xm.h>

#include "dinotrace.h"
#include "callbacks.h"



void cb_window_expose(w,trace)
    Widget		w;
    TRACE		*trace;
{
    if (DTPRINT) printf("In Window Expose - trace=%d\n",trace);
    if (!trace->loaded) return;

    /* redraw the entire screen */
    get_geometry (trace);
    draw (trace);
    }

void cb_window_focus(w,trace)
    Widget		w;
    TRACE		*trace;
{
    if (DTPRINT) printf("In Window Focus - trace=%d\n",trace);

    /* redraw the entire screen */
    get_geometry (trace);
    draw (trace);
    }

void hscroll_unitinc(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmScrollBarCallbackStruct *cb;
{
    if (DTPRINT) printf("In hscroll_unitinc - trace=%d  old_time=%d",trace,global->time);
    global->time += trace->grid_res;
    if (DTPRINT) printf(" new time=%d\n",global->time);

    new_time(trace);
    }

void hscroll_unitdec(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmScrollBarCallbackStruct *cb;
{
    if (DTPRINT) printf("In hscroll_unitdec - trace=%d  old_time=%d",trace,global->time);
    global->time -= trace->grid_res;
    new_time(trace);
    }

void hscroll_pageinc(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmScrollBarCallbackStruct *cb;
{
    if (DTPRINT) printf("In hscroll_pageinc - trace=%d  old_time=%d",trace,global->time);

    if ( trace->pageinc == QPAGE )
	global->time += (int)(((trace->width-global->xstart)/global->res)/4);
    else if ( trace->pageinc == HPAGE )
	global->time += (int)(((trace->width-global->xstart)/global->res)/2);
    else if ( trace->pageinc == FPAGE )
	global->time += (int)((trace->width-global->xstart)/global->res);

    if (DTPRINT) printf(" new time=%d\n",global->time);

    new_time(trace);
    }

void hscroll_pagedec(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmScrollBarCallbackStruct *cb;
{
    if (DTPRINT) printf("In hscroll_pagedec - trace=%d  old_time=%d",trace,global->time);

    if ( trace->pageinc == QPAGE )
	global->time -= (int)(((trace->width-global->xstart)/global->res)/4);
    else if ( trace->pageinc == HPAGE )
	global->time -= (int)(((trace->width-global->xstart)/global->res)/2);
    else if ( trace->pageinc == FPAGE )
	global->time -= (int)((trace->width-global->xstart)/global->res);

    if (DTPRINT) printf(" new time=%d\n",global->time);

    new_time(trace);
    }

void hscroll_drag(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmScrollBarCallbackStruct *cb;
{
    int inc;

    XtSetArg (arglist[0], XmNvalue, &inc);
    XtGetValues(trace->hscroll,arglist,1);
    if (DTPRINT) printf("inc=%d\n",inc);

    global->time = inc;

    new_time(trace);
    }

void cb_begin(w, trace )
    Widget		w;
    TRACE		*trace;
{
    if (DTPRINT) printf("In cb_start trace=%d\n",trace);
    global->time = trace->start_time;
    new_time(trace);
    }

void cb_end(w, trace )
    Widget		w;
    TRACE		*trace;
{
    if (DTPRINT) printf("In cb_end trace=%d\n",trace);
    global->time = trace->end_time - (int)((trace->width-XMARGIN-global->xstart)/global->res);
    new_time(trace);
    }

void hscroll_bot(w,trace,cb)
    Widget		w;
    TRACE	*trace;
    XmScrollBarCallbackStruct *cb;
{
    if (DTPRINT) printf("In hscroll_bot trace=%d\n",trace);
    }

void hscroll_top(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmScrollBarCallbackStruct *cb;
{
    if (DTPRINT) printf("In hscroll_top trace=%d\n",trace);
    }

/* Vertically scroll + or - inc lines */

void vscroll_new(trace,inc)
    TRACE		*trace;
    int inc;
{
    if (DTPRINT) printf("in vscroll_new inc=%d start=%d\n",inc,trace->numsigstart);

    while ((inc > 0) && trace->dispsig && trace->dispsig->forward ) {
	trace->numsigstart++;
	trace->dispsig = trace->dispsig->forward;
	inc--;
	}
    
    while ((inc < 0) && trace->dispsig && trace->dispsig->backward ) {
	trace->numsigstart--;
	trace->dispsig = trace->dispsig->backward;
	inc++;
	}
    
    get_geometry (trace);
    XClearWindow (global->display, trace->wind);
    draw (trace);
    }

void vscroll_unitinc(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmScrollBarCallbackStruct *cb;
{
    vscroll_new (trace, 1);
    }

void vscroll_unitdec(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmScrollBarCallbackStruct *cb;
{
    vscroll_new (trace, -1);
    }

void vscroll_pageinc(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmScrollBarCallbackStruct *cb;
{
    vscroll_new (trace, trace->numsigvis);
    }

void vscroll_pagedec(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmScrollBarCallbackStruct *cb;
{
    int sigs;

    /* Not numsigvis because may not be limited by screen size */
    sigs = (int)((trace->height-trace->ystart)/trace->sighgt);

    if ( global->numcursors > 0 &&
	 trace->cursor_vis &&
	 trace->numsigvis > 1 &&
	 trace->numsigvis >= sigs ) {
	sigs--;
	}

    vscroll_new (trace, -sigs);
    }

void vscroll_drag(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmScrollBarCallbackStruct *cb;
{
    int		inc;

    if (DTPRINT) printf("In vscroll_drag trace=%d\n",trace);

    arglist[0].name = XmNvalue;
    arglist[0].value = (int)&inc;
    XtGetValues(trace->vscroll,arglist,1);

    /*
    ** The sig pointer is reset to the start and the loop will set
    ** it to the signal that inc represents
    */
    trace->numsigstart = 0;
    trace->dispsig = trace->firstsig;
    vscroll_new (trace, inc);
    }

void vscroll_bot(w,trace,cb)
    Widget		w;
    TRACE		*trace;
    XmScrollBarCallbackStruct *cb;
{
    if (DTPRINT) printf("In vscroll_bot trace=%d\n",trace);
    }

void vscroll_top(w,trace,cb)
    Widget		w;
    TRACE		*trace;
    XmScrollBarCallbackStruct *cb;
{
    if (DTPRINT) printf("In vscroll_top trace=%d\n",trace);
    }

void cb_chg_res(w,trace,cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In cb_chg_res - trace=%d\n",trace);
    get_data_popup(trace,"Resolution",IO_RES);
    }


void new_res(trace, redisplay)
    TRACE		*trace;
    int		redisplay;	/* TRUE to refresh the screen after change */
{
    char	string[20];

    if (DTPRINT) printf ("In new_res - res = %f\n",global->res);

    if (global->res==0.0) global->res=0.1;	/* prevent div zero error */

    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	/* change res button's value */
	sprintf(string,"Res=%d ns",(int)(RES_SCALE/global->res) );
	XtSetArg(arglist[0],XmNlabelString,XmStringCreateSimple(string));
	XtSetValues(trace->command.reschg_but,arglist,1);
	}

    if (redisplay) {
	redraw_all (trace);
	}
    }

void cb_inc_res(w,trace,cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In cb_inc_res - trace=%d\n",trace);

    /* increase the resolution by 10% */
    global->res = global->res*1.1;
    new_res (trace, TRUE);
    }

void cb_dec_res(w,trace,cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In cb_dec_res - trace=%d\n",trace);

    /* decrease the resolution by 10% */
    global->res = global->res*0.9;
    new_res (trace, TRUE);
    }

void cb_full_res(w,trace,cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In cb_full_res - trace=%d\n",trace);

    /*    printf("%d %d %d %d %d\n",
	   global->xstart, trace->width,XMARGIN, trace->end_time, trace->start_time);	   */

    /* set resolution  */
    if (trace->end_time != trace->start_time) {
	global->time = trace->start_time;
	global->res = ((float)(trace->width - global->xstart)) /
	    ((float)(trace->end_time - trace->start_time));
	new_res (trace, FALSE);
	new_time (trace);
	}
    }

void cb_zoom_res(w,trace,cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In cb_zoom_res - trace=%d\n",trace);

    /* process all subsequent button presses as res_zoom clicks */
    trace->click_time = -1;
    remove_all_events (trace);
    set_cursor (trace, DC_ZOOM_1);
    add_event (ButtonPressMask, res_zoom_click_ev);
    }

void res_zoom_click_ev(w, trace, ev)
    Widget		w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    int		time,tmp;

    if (DTPRINT) printf("In res_zoom_click1_ev - trace=%d x=%d y=%d\n",trace,ev->x,ev->y);

    /* convert x value to a time value */
    time = posx_to_time (trace, ev->x);
    if (time<0) return;

    /* If no click time defined, define one and wait for second click */
    if ( trace->click_time < 0) {
	trace->click_time = time;
	set_cursor (trace, DC_ZOOM_2);
	return;
	}

    if (DTPRINT) printf ("click1 = %d, click2 = %d\n",trace->click_time, time);

    /* Got 2 clicks, set res */
    if (time != trace->click_time) {
	/* Swap so time is the max */
	if (time < trace->click_time) {
	    tmp = time;
	    time = trace->click_time;
	    trace->click_time = tmp;
	    }
       
	/* Set new res & time */
	global->res = ((float)(trace->width - global->xstart)) /
	    ((float)(time - trace->click_time));
	
	global->time = trace->click_time;

	new_res (trace, FALSE);
	new_time(trace);
	}

    /* remove handlers */
    remove_all_events (trace);
    }

