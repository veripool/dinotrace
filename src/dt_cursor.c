/******************************************************************************
 *
 * Filename:
 *     dt_cursor.c
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
 *     AAG	14-Aug-90	Original Version
 *     AAG	22-Aug-90	Base Level V4.1
 *     AAG	29-Apr-91	Use X11, added button event variable for Ultrix
 *				 support
 */


#include <X11/Xlib.h>
#include <Xm/Xm.h>
#include "dinotrace.h"
#include "callbacks.h"


void    cur_add_cb(w, trace, cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    
    if (DTPRINT) printf("In cur_add_cb - trace=%d\n",trace);
    
    /* remove any previous events */
    remove_all_events(trace);
    
    /* process all subsequent button presses as cursor adds */
    set_cursor (trace, DC_CUR_ADD);
    add_event (ButtonPressMask, cur_add_ev);
    }

void    cur_mov_cb(w, trace, cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    
    if (DTPRINT) printf("In cur_mov_cb - trace=%d\n",trace);
    
    /* remove any previous events */
    remove_all_events(trace);
    
    /* process all subsequent button presses as cursor moves */
    set_cursor (trace, DC_CUR_MOVE);
    add_event (ButtonPressMask, cur_move_ev);
    }

void    cur_del_cb(w, trace, cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    
    if (DTPRINT) printf("In cur_del_cb - trace=%d\n",trace);
    
    /* remove any previous events */
    remove_all_events(trace);
    
    /* process all subsequent button presses as cursor deletes */
    set_cursor (trace, DC_CUR_DELETE);
    add_event (ButtonPressMask, cur_delete_ev);
    }

void    cur_clr_cb(w, trace, cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    int		i;
    
    if (DTPRINT) printf("In cb_clr_cursor.\n");
    
    /* clear the cursor array to zero */
    for (i=0; i < MAX_CURSORS; i++) {
	global->cursors[i] = 0;
	}
    
    /* reset the number of cursors */
    global->numcursors = 0;
    
    /* cancel the button actions */
    remove_all_events(trace);
    
    /* redraw the screen with no cursors */
    redraw_all (trace);
    }

void    cur_add_ev(w, trace, ev)
    Widget		w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    int		i,j,time;
    
    if (DTPRINT) printf("In add cursor - trace=%d x=%d y=%d\n",trace,ev->x,ev->y);
    
    /* check if there is room for another cursor */
    if (global->numcursors >= MAX_CURSORS)
	{
	sprintf(message,"Reached max of %d cursors in display",global->numcursors);
	dino_warning_ack(trace,message);
	return;
	}
    
    /* convert x value to a time value */
    time = posx_to_time (trace, ev->x);
    if (time<0) return;
    
    /* put into cursor array in chronological order */
    i = 0;
    while ( time > global->cursors[i] && global->cursors[i] != 0)
	{
	i++;
	}
    
    for (j=global->numcursors; j > i; j--)
	{
	global->cursors[j] = global->cursors[j-1];
	}
    
    global->cursors[i] = time;
    global->numcursors++;
    
    /* redraw the screen with new cursors */
    redraw_all (trace);
    }

void    cur_move_ev(w, trace, ev)
    Widget		w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    int		i,j,time;
    int		x1,x2,y1,y2;
    static	last_x = 0;
    XEvent	event;
    XMotionEvent *em;
    XButtonEvent *eb;
    
    if (DTPRINT) printf("In cursor_mov\n");
    
    /* check if there are any cursors to move */
    if (global->numcursors <= 0)
	{
	dino_warning_ack(trace,"No cursors to move");
	return;
	}
    
    /* convert x value to a time value */
    time = posx_to_time (trace, ev->x);
    if (time<0) return;
    
    /* find the closest cursor */
    i = 0;
    while ( time > global->cursors[i] && i < global->numcursors)
	{
	i++;
	}
    
    /* i is between cursors[i-1] and cursors[i] - determine which is closest */
    if ( i )
	{
	if (global->cursors[i]-time > time-global->cursors[i-1] || 
	    i == global->numcursors)
	    {
	    i--;
	    }
	}
    
    /* global->cursors[i] is the cursor to be moved - calculate starting x */
    last_x = (global->cursors[i] - global->time)* global->res + global->xstart;
    
    /* not sure why this has to be done but it must be done */
    XUngrabPointer(XtDisplay(trace->work),CurrentTime);
    
    /* select the events the widget will respond to */
    XSelectInput(XtDisplay(trace->work),XtWindow(trace->work),
		 ButtonReleaseMask|PointerMotionMask|StructureNotifyMask|ExposureMask);
    
    /* change the GC function to drag the cursor */
    xgcv.function = GXinvert;
    XChangeGC(global->display,trace->gc,GCFunction,&xgcv);
    XSync(global->display,0);
    
    /* draw the first line */
    y1 = 25;
    y2 = trace->height - trace->ystart + trace->sighgt;
    x1 = x2 = last_x = ev->x;
    XDrawLine(global->display,trace->wind,trace->gc,x1,y1,x2,y2);
    
    /* loop and service events until button is released */
    while ( 1 )
	{
	/* wait for next event */
	XNextEvent(XtDisplay(trace->work),&event);
	
	/* if the pointer moved, erase previous line and draw new one */
	if (event.type == MotionNotify)
	    {
	    em = (XMotionEvent *)&event;
	    x1 = x2 = last_x;
	    y1 = 25;
	    y2 = trace->height - trace->ystart + trace->sighgt;
	    XDrawLine(global->display,trace->wind,trace->gc,x1,y1,x2,y2);
	    x1 = x2 = em->x;
	    XDrawLine(global->display,trace->wind,trace->gc,x1,y1,x2,y2);
	    last_x = em->x;
	    }
	
	/* if window was exposed, must redraw it */
	if (event.type == Expose)
	    {
	    cb_window_expose(0,trace);
	    }
	
	/* if window was resized, must redraw it */
	if (event.type == ConfigureNotify)
	    {
	    cb_window_expose(0,trace);
	    }
	
	/* button released - calculate cursor position and leave the loop */
	if (event.type == ButtonRelease)
	    {
	    eb = (XButtonReleasedEvent *)&event;
	    time = posx_to_time (trace, eb->x);
	    break;
	    }
	}
    
    /* reset the events the widget will respond to */
    XSelectInput(XtDisplay(trace->work),XtWindow(trace->work),
		 ButtonPressMask|StructureNotifyMask|ExposureMask);
    
    /* change the GC function back to its default */
    xgcv.function = GXcopy;
    XChangeGC(global->display,trace->gc,GCFunction,&xgcv);
    
    /* squeeze cursor array, effectively removing cursor that moved */
    for (j=i;j<global->numcursors;j++)
	global->cursors[j] = global->cursors[j+1];
    
    /* put into cursor array in chronological order */
    i = 0;
    while ( time > global->cursors[i] && global->cursors[i] != 0)
	{
	i++;
	}
    
    for (j=global->numcursors; j > i; j--)
	{
	global->cursors[j] = global->cursors[j-1];
	}
    
    global->cursors[i] = time;
    
    /* redraw the screen with new cursor position */
    redraw_all (trace);
    }

void    cur_delete_ev(w, trace, ev)
    Widget		w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    int		i,j,time;
    
    if (DTPRINT) printf("In cur_delete_ev - trace=%d x=%d y=%d\n",trace,ev->x,ev->y);
    
    /* check if there are any cursors to delete */
    if (global->numcursors <= 0)
	{
	dino_warning_ack(trace,"No cursors to delete");
	return;
	}
    
    time = posx_to_time (trace, ev->x);
    if (time<0) return;
    
    /* find the closest cursor */
    i = 0;
    while ( time > global->cursors[i] ) {
	i++;
	}
    
    /* i is between cursors[i-1] and cursors[i] - determine which is closest */
    if ( i ) {
	if ( global->cursors[i] - time > time - global->cursors[i-1] ) {
	    i--;
	    }
	}
    
    /* delete the cursor */
    for (j=i; j < global->numcursors; j++) {
	global->cursors[j] = global->cursors[j+1];
	}
    
    global->numcursors--;
    
    /* redraw the screen with new cursors */
    redraw_all (trace);
    }

