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
#include <X11/Xm.h>
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
    XtAddEventHandler(trace->work,ButtonPressMask,TRUE,add_cursor,trace);
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
    XtAddEventHandler(trace->work,ButtonPressMask,TRUE,move_cursor,trace);
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
    XtAddEventHandler(trace->work,ButtonPressMask,TRUE,delete_cursor,trace);
    }

void    cur_clr_cb(w, trace, cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    int		i;
    
    if (DTPRINT) printf("In cb_clr_cursor.\n");
    
    /* clear the cursor array to zero */
    for (i=0; i < MAX_CURSORS; i++)
	{
	trace->cursors[i] = 0;
	}
    
    /* reset the number of cursors */
    trace->numcursors = 0;
    
    /* cancel the button actions */
    remove_all_events(trace);
    
    /* redraw the screen with no cursors */
    get_geometry(trace);
    XClearWindow(trace->display,trace->wind);
    draw(trace);
    }

void    add_cursor(w, trace, ev)
    Widget		w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    int		i,j,time;
    
    if (DTPRINT) printf("In add cursor - trace=%d x=%d y=%d\n",trace,ev->x,ev->y);
    
    /* check if button has been clicked on trace portion of screen */
    if ( ev->x < trace->xstart || ev->x > trace->width - XMARGIN )
	return;
    
    /* check if there is room for another cursor */
    if (trace->numcursors >= MAX_CURSORS)
	{
	sprintf(message,"Reached max of %d cursors in display",trace->numcursors);
	dino_warning_ack(trace,message);
	return;
	}
    
    /* convert x value to a time value */
    time = (ev->x + trace->time * trace->res - trace->xstart) / trace->res;
    
    /* put into cursor array in chronological order */
    i = 0;
    while ( time > trace->cursors[i] && trace->cursors[i] != 0)
	{
	i++;
	}
    
    for (j=trace->numcursors; j > i; j--)
	{
	trace->cursors[j] = trace->cursors[j-1];
	}
    
    trace->cursors[i] = time;
    trace->numcursors++;
    
    /* redraw the screen with new cursors */
    get_geometry(trace);
    XClearWindow(trace->display,trace->wind);
    draw(trace);
    }

void    move_cursor(w, trace, ev)
    Widget		w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    int		i,j,time;
    static	last_x = 0;
    XEvent	event;
    XMotionEvent *em;
    XButtonEvent *eb;
    
    if (DTPRINT) printf("In cursor_mov\n");
    
    /* check if button has been clicked on trace portion of screen */
    if ( ev->x < trace->xstart || ev->x > trace->width - XMARGIN )
	return;
    
    /* check if there are any cursors to move */
    if (trace->numcursors <= 0)
	{
	dino_warning_ack(trace,"No cursors to move");
	return;
	}
    
    /* convert x value to a time value */
    time = (ev->x + trace->time * trace->res - trace->xstart) / trace->res;
    
    /* find the closest cursor */
    i = 0;
    while ( time > trace->cursors[i] && i < trace->numcursors)
	{
	i++;
	}
    
    /* i is between cursors[i-1] and cursors[i] - determine which is closest */
    if ( i )
	{
	if (trace->cursors[i]-time > time-trace->cursors[i-1] || 
	    i == trace->numcursors)
	    {
	    i--;
	    }
	}
    
    /* trace->cursors[i] is the cursor to be moved - calculate starting x */
    last_x = (trace->cursors[i] - trace->time)* trace->res + trace->xstart;
    
    /* not sure why this has to be done but it must be done */
    XUngrabPointer(XtDisplay(trace->work),CurrentTime);
    
    /* select the events the widget will respond to */
    XSelectInput(XtDisplay(trace->work),XtWindow(trace->work),
		 ButtonReleaseMask|PointerMotionMask|StructureNotifyMask|ExposureMask);
    
    /* change the GC function to drag the cursor */
    xgcv.function = GXinvert;
    XChangeGC(trace->display,trace->gc,GCFunction,&xgcv);
    XSync(trace->display,0);
    
    /* draw the first line */
    y1 = 25;
    y2 = trace->height - trace->ystart + trace->sighgt;
    x1 = x2 = last_x = ev->x;
    XDrawLine(trace->display,trace->wind,trace->gc,x1,y1,x2,y2);
    
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
	    XDrawLine(trace->display,trace->wind,trace->gc,x1,y1,x2,y2);
	    x1 = x2 = em->x;
	    XDrawLine(trace->display,trace->wind,trace->gc,x1,y1,x2,y2);
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
	    time = (eb->x + trace->time * trace->res - trace->xstart) / trace->res;
	    break;
	    }
	}
    
    /* reset the events the widget will respond to */
    XSelectInput(XtDisplay(trace->work),XtWindow(trace->work),
		 ButtonPressMask|StructureNotifyMask|ExposureMask);
    
    /* change the GC function back to its default */
    xgcv.function = GXcopy;
    XChangeGC(trace->display,trace->gc,GCFunction,&xgcv);
    
    /* squeeze cursor array, effectively removing cursor that moved */
    for (j=i;j<trace->numcursors;j++)
	trace->cursors[j] = trace->cursors[j+1];
    
    /* put into cursor array in chronological order */
    i = 0;
    while ( time > trace->cursors[i] && trace->cursors[i] != 0)
	{
	i++;
	}
    
    for (j=trace->numcursors; j > i; j--)
	{
	trace->cursors[j] = trace->cursors[j-1];
	}
    
    trace->cursors[i] = time;
    
    /* redraw the screen with new cursor position */
    get_geometry(trace);
    XClearWindow(trace->display,trace->wind);
    draw(trace);
    }

void    delete_cursor(w, trace, ev)
    Widget		w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    int		i,j,time;
    
    if (DTPRINT) printf("In delete_cursor - trace=%d x=%d y=%d\n",trace,ev->x,ev->y);
    
    /* check if button has been clicked on trace portion of screen */
    if ( ev->x < trace->xstart || ev->x > trace->width - XMARGIN )
	return;
    
    /* check if there are any cursors to delete */
    if (trace->numcursors <= 0)
	{
	dino_warning_ack(trace,"No cursors to delete");
	return;
	}
    
    /* convert x value to a time value */
    time = (ev->x + trace->time * trace->res - trace->xstart) / trace->res;
    
    /* find the closest cursor */
    i = 0;
    while ( time > trace->cursors[i] )
	{
	i++;
	}
    
    /* i is between cursors[i-1] and cursors[i] - determine which is closest */
    if ( i )
	{
	if ( trace->cursors[i] - time > time - trace->cursors[i-1] )
	    {
	    i--;
	    }
	}
    
    /* delete the cursor */
    for (j=i; j < trace->numcursors; j++)
	{
	trace->cursors[j] = trace->cursors[j+1];
	}
    
    trace->numcursors--;
    
    /* redraw the screen with new cursors */
    get_geometry(trace);
    XClearWindow(trace->display,trace->wind);
    draw(trace);
    }

