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
 *
 */


#include <X11/DECwDwtApplProg.h>
#include <X11/Xlib.h>
#include "dinotrace.h"
#include "callbacks.h"


void
cur_add_cb(w, ptr, cb)
Widget			w;
DISPLAY_SB		*ptr;
DwtAnyCallbackStruct	*cb;
{

    if (DTPRINT) printf("In cur_add_cb - ptr=%d\n",ptr);

    /* remove any previous events */
    remove_all_events(ptr);

    /* process all subsequent button presses as cursor adds */
    XtAddEventHandler(ptr->work,ButtonPressMask,TRUE,add_cursor,ptr);
}

void
cur_mov_cb(w, ptr, cb)
Widget			w;
DISPLAY_SB		*ptr;
DwtAnyCallbackStruct	*cb;
{

    if (DTPRINT) printf("In cur_mov_cb - ptr=%d\n",ptr);

    /* remove any previous events */
    remove_all_events(ptr);

    /* process all subsequent button presses as cursor moves */
    XtAddEventHandler(ptr->work,ButtonPressMask,TRUE,move_cursor,ptr);
}

void
cur_del_cb(w, ptr, cb)
Widget			w;
DISPLAY_SB		*ptr;
DwtAnyCallbackStruct	*cb;
{

    if (DTPRINT) printf("In cur_del_cb - ptr=%d\n",ptr);

    /* remove any previous events */
    remove_all_events(ptr);

    /* process all subsequent button presses as cursor deletes */
    XtAddEventHandler(ptr->work,ButtonPressMask,TRUE,delete_cursor,ptr);
}

void
cur_clr_cb(w, ptr, cb)
Widget			w;
DISPLAY_SB		*ptr;
DwtAnyCallbackStruct	*cb;
{
    int		i;

    if (DTPRINT) printf("In cb_clr_cursor.\n");

    /* clear the cursor array to zero */
    for (i=0; i < MAX_CURSORS; i++)
    {
	ptr->cursors[i] = 0;
    }

    /* reset the number of cursors */
    ptr->numcursors = 0;

    /* cancel the button actions */
    remove_all_events(ptr);

    /* redraw the screen with no cursors */
    get_geometry(ptr);
    XClearWindow(ptr->disp,ptr->wind);
    drawsig(ptr);
    draw(ptr);

    return;
}

void
add_cursor(w, ptr, ev)
Widget			w;
DISPLAY_SB		*ptr;
XButtonPressedEvent	*ev;
{
    int		i,j,time;

    if (DTPRINT) printf("In add cursor - ptr=%d x=%d y=%d\n",ptr,ev->x,ev->y);

    /* check if button has been clicked on trace portion of screen */
    if ( ev->x < ptr->xstart || ev->x > ptr->width - XMARGIN )
	return;

    /* check if there is room for another cursor */
    if (ptr->numcursors >= MAX_CURSORS)
    {
	sprintf(message,"Reached max of %d cursors in display",ptr->numcursors);
	dino_message_ack(ptr,message);
	return;
    }

    /* convert x value to a time value */
    time = (ev->x + ptr->time * ptr->res - ptr->xstart) / ptr->res;

    /* put into cursor array in chronological order */
    i = 0;
    while ( time > ptr->cursors[i] && ptr->cursors[i] != 0)
    {
	i++;
    }

    for (j=ptr->numcursors; j > i; j--)
    {
	ptr->cursors[j] = ptr->cursors[j-1];
    }

    ptr->cursors[i] = time;
    ptr->numcursors++;

    /* redraw the screen with new cursors */
    get_geometry(ptr);
    XClearWindow(ptr->disp,ptr->wind);
    drawsig(ptr);
    draw(ptr);

    return;
}

void
move_cursor(w, ptr, ev)
Widget			w;
DISPLAY_SB		*ptr;
XButtonPressedEvent	*ev;
{
    int		i,j,time;
    static	last_x = 0;
    XEvent	event;
    XMotionEvent *em;
    XButtonEvent *eb;

    if (DTPRINT) printf("In cursor_mov\n");

    /* check if button has been clicked on trace portion of screen */
    if ( ev->x < ptr->xstart || ev->x > ptr->width - XMARGIN )
	return;

    /* check if there are any cursors to move */
    if (ptr->numcursors <= 0)
    {
	dino_message_ack(ptr,"No cursors to move");
	return;
    }

    /* convert x value to a time value */
    time = (ev->x + ptr->time * ptr->res - ptr->xstart) / ptr->res;

    /* find the closest cursor */
    i = 0;
    while ( time > ptr->cursors[i] && i < ptr->numcursors)
    {
	i++;
    }

    /* i is between cursors[i-1] and cursors[i] - determine which is closest */
    if ( i )
    {
	if (ptr->cursors[i]-time > time-ptr->cursors[i-1] || 
							i == ptr->numcursors)
	{
	    i--;
	}
    }

    /* ptr->cursors[i] is the cursor to be moved - calculate starting x */
    last_x = (ptr->cursors[i] - ptr->time)* ptr->res + ptr->xstart;

    /* not sure why this has to be done but it must be done */
    XUngrabPointer(XtDisplay(ptr->work),CurrentTime);

    /* select the events the widget will respond to */
    XSelectInput(XtDisplay(ptr->work),XtWindow(ptr->work),
	ButtonReleaseMask|PointerMotionMask|StructureNotifyMask|ExposureMask);

    /* change the GC function to drag the cursor */
    xgcv.function = GXinvert;
    XchangeGC(ptr->disp,ptr->gc,GCFunction,&xgcv);
    XSync(ptr->disp,0);

    /* draw the first line */
    y1 = 25;
    y2 = ptr->height - ptr->ystart + ptr->sighgt;
    x1 = x2 = last_x = ev->x;
    XDrawLine(ptr->disp,ptr->wind,ptr->gc,x1,y1,x2,y2);

    /* loop and service events until button is released */
    while ( 1 )
    {
	/* wait for next event */
	XNextEvent(XtDisplay(ptr->work),&event);

	/* if the pointer moved, erase previous line and draw new one */
	if (event.type == MotionNotify)
	{
	    em = (XMotionEvent *)&event;
	    x1 = x2 = last_x;
	    y1 = 25;
	    y2 = ptr->height - ptr->ystart + ptr->sighgt;
	    XDrawLine(ptr->disp,ptr->wind,ptr->gc,x1,y1,x2,y2);
	    x1 = x2 = em->x;
	    XDrawLine(ptr->disp,ptr->wind,ptr->gc,x1,y1,x2,y2);
	    last_x = em->x;
	}

	/* if window was exposed, must redraw it */
	if (event.type == Expose)
	{
	    cb_window_expose(0,ptr);
	}

	/* if window was resized, must redraw it */
	if (event.type == ConfigureNotify)
	{
	    cb_window_expose(0,ptr);
	}

	/* button released - calculate cursor position and leave the loop */
	if (event.type == ButtonRelease)
	{
	    eb = (XButtonReleasedEvent *)&event;
	    time = (eb->x + ptr->time * ptr->res - ptr->xstart) / ptr->res;
	    break;
	}
    }

    /* reset the events the widget will respond to */
    XSelectInput(XtDisplay(ptr->work),XtWindow(ptr->work),
	ButtonPressMask|StructureNotifyMask|ExposureMask);

    /* change the GC function back to its default */
    xgcv.function = GXcopy;
    XchangeGC(ptr->disp,ptr->gc,GCFunction,&xgcv);

    /* squeeze cursor array, effectively removing cursor that moved */
    for (j=i;j<ptr->numcursors;j++)
	ptr->cursors[j] = ptr->cursors[j+1];

    /* put into cursor array in chronological order */
    i = 0;
    while ( time > ptr->cursors[i] && ptr->cursors[i] != 0)
    {
	i++;
    }

    for (j=ptr->numcursors; j > i; j--)
    {
	ptr->cursors[j] = ptr->cursors[j-1];
    }

    ptr->cursors[i] = time;

    /* redraw the screen with new cursor position */
    get_geometry(ptr);
    XClearWindow(ptr->disp,ptr->wind);
    drawsig(ptr);
    draw(ptr);

    return;
}

void
delete_cursor(w, ptr, ev)
Widget			w;
DISPLAY_SB		*ptr;
XButtonPressedEvent	*ev;
{
    int		i,j,time;

    if (DTPRINT) printf("In delete_cursor - ptr=%d x=%d y=%d\n",ptr,ev->x,ev->y);

    /* check if button has been clicked on trace portion of screen */
    if ( ev->x < ptr->xstart || ev->x > ptr->width - XMARGIN )
	return;

    /* check if there are any cursors to delete */
    if (ptr->numcursors <= 0)
    {
	dino_message_ack(ptr,"No cursors to delete");
	return;
    }

    /* convert x value to a time value */
    time = (ev->x + ptr->time * ptr->res - ptr->xstart) / ptr->res;

    /* find the closest cursor */
    i = 0;
    while ( time > ptr->cursors[i] )
    {
	i++;
    }

    /* i is between cursors[i-1] and cursors[i] - determine which is closest */
    if ( i )
    {
	if ( ptr->cursors[i] - time > time - ptr->cursors[i-1] )
	{
	    i--;
	}
    }

    /* delete the cursor */
    for (j=i; j < ptr->numcursors; j++)
    {
	ptr->cursors[j] = ptr->cursors[j+1];
    }

    ptr->numcursors--;

    /* redraw the screen with new cursors */
    get_geometry(ptr);
    XClearWindow(ptr->disp,ptr->wind);
    drawsig(ptr);
    draw(ptr);

    return;
}

