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
static char rcsid[] = "$Id$";

#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <X11/Xlib.h>
#include <Xm/Xm.h>
#include "dinotrace.h"
#include "callbacks.h"

/****************************** UTILITIES ******************************/

void    cur_remove (csr_ptr)
    /* Cursor is removed from list, but not freed! */
    CURSOR	*csr_ptr;	/* Cursor to remove */
{
    CURSOR	*next_csr_ptr, *prev_csr_ptr;
    
    if (DTPRINT_ENTRY) printf ("In cur_remove\n");
    
    /* redirect the next pointer */
    prev_csr_ptr = csr_ptr->prev;
    if (prev_csr_ptr) {
	prev_csr_ptr->next = csr_ptr->next;
	}
    
    /* if not the last cursors redirect the prev pointer */
    next_csr_ptr = csr_ptr->next;
    if ( next_csr_ptr != NULL ) {
	next_csr_ptr->prev = csr_ptr->prev;
	}

    /* if this is the first cursor, change the head */
    if ( csr_ptr == global->cursor_head ) {
        global->cursor_head = csr_ptr->next;
	}
    }

void cur_add (ctime, color, type)
    DTime	ctime;
    ColorNum	color;
    CursorType	type;
{
    CURSOR *new_csr_ptr;
    CURSOR *prev_csr_ptr, *csr_ptr;
    
    if (DTPRINT_ENTRY) printf ("In cur_add - time=%d\n",ctime);
    
    new_csr_ptr = (CURSOR *)XtMalloc (sizeof (CURSOR));
    new_csr_ptr->time = ctime;
    new_csr_ptr->color = color;
    new_csr_ptr->type = type;

    prev_csr_ptr = NULL;
    for (csr_ptr = global->cursor_head;
	 csr_ptr && ( csr_ptr->time < new_csr_ptr->time );
	 csr_ptr = csr_ptr->next) {
	prev_csr_ptr = csr_ptr;
	}
    
    if (csr_ptr && csr_ptr->time == new_csr_ptr->time) {
	if ((new_csr_ptr->type == USER) || (csr_ptr->type != USER)) {
	    /* Auto over auto or user over user is OK */
	    csr_ptr->time = ctime;
	    csr_ptr->color = color;
	    csr_ptr->type = type;
	    }
	/* else Don't go over existing user one */
	/* Don't need new structure */
	DFree (new_csr_ptr);
	}
    else {
	/* Insert into first position? */
	if (!prev_csr_ptr) {
	    global->cursor_head = new_csr_ptr;
	    }
	else {
	    prev_csr_ptr->next = new_csr_ptr;
	    }
	
	new_csr_ptr->next = csr_ptr;
	new_csr_ptr->prev = prev_csr_ptr;
	if (csr_ptr) csr_ptr->prev = new_csr_ptr;
	}
    }


void cur_delete_of_type (type)
    CursorType	type;
{
    CURSOR	*csr_ptr,*new_csr_ptr;

    for (csr_ptr = global->cursor_head; csr_ptr; ) {
	new_csr_ptr = csr_ptr;
	csr_ptr = csr_ptr->next;
	if (new_csr_ptr->type==type) {
	    cur_remove (new_csr_ptr);
	    DFree (new_csr_ptr);
	    }
	}
    }

void cur_print (FILE *writefp)
{
    CURSOR	*csr_ptr;
    char strg[MAXSIGLEN];

    for (csr_ptr = global->cursor_head; csr_ptr; csr_ptr = csr_ptr->next) {
	time_to_string (global->trace_head, strg, csr_ptr->time, FALSE);
	switch (csr_ptr->type) {
	  case USER:
	    fprintf (writefp, "cursor_add %s %d -user\n", strg, csr_ptr->color);
	    break;
	  case CONFIG:
	    fprintf (writefp, "cursor_add %s %d\n", strg, csr_ptr->color);
	    break;
	  default:
	    break;
	    }
	}
    }


/****************************** MENU OPTIONS ******************************/

void    cur_add_cb (w, trace, cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT_ENTRY) printf ("In cur_add_cb - trace=%d\n",trace);
    
    /* remove any previous events */
    remove_all_events (trace);
    
    /* Grab color number from the menu button pointer */
    global->highlight_color = submenu_to_color (trace, w, trace->menu.cur_add_pds);

    /* process all subsequent button presses as cursor adds */
    set_cursor (trace, DC_CUR_ADD);
    add_event (ButtonPressMask, cur_add_ev);
    }

void    cur_mov_cb (w, trace, cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    
    if (DTPRINT_ENTRY) printf ("In cur_mov_cb - trace=%d\n",trace);
    
    /* remove any previous events */
    remove_all_events (trace);
    
    /* process all subsequent button presses as cursor moves */
    set_cursor (trace, DC_CUR_MOVE);
    add_event (ButtonPressMask, cur_move_ev);
    }

void    cur_del_cb (w, trace, cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    
    if (DTPRINT_ENTRY) printf ("In cur_del_cb - trace=%d\n",trace);
    
    /* remove any previous events */
    remove_all_events (trace);
    
    /* process all subsequent button presses as cursor deletes */
    set_cursor (trace, DC_CUR_DELETE);
    add_event (ButtonPressMask, cur_delete_ev);
    }

void    cur_clr_cb (w, trace, cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    CURSOR	*csr_ptr;
    
    if (DTPRINT_ENTRY) printf ("In cur_clr_cb.\n");
    
    /* clear the cursor array to zero */
    while ( global->cursor_head ) {
	csr_ptr = global->cursor_head;
	global->cursor_head = csr_ptr->next;
	DFree (csr_ptr);
	}
    
    /* cancel the button actions */
    remove_all_events (trace);
    
    draw_all_needed (trace);
    }

void    cur_highlight_cb (w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT_ENTRY) printf ("In cur_highlight_cb - trace=%d\n",trace);
    
    /* remove any previous events */
    remove_all_events (trace);
     
    /* Grab color number from the menu button pointer */
    global->highlight_color = submenu_to_color (trace, w, trace->menu.cur_highlight_pds);

    /* process all subsequent button presses as signal deletions */ 
    set_cursor (trace, DC_CUR_HIGHLIGHT);
    add_event (ButtonPressMask, cur_highlight_ev);
    }

/****************************** EVENTS ******************************/

void    cur_add_ev (w, trace, ev)
    Widget		w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    DTime	time;
    
    if (DTPRINT_ENTRY) printf ("In add cursor - trace=%d x=%d y=%d\n",trace,ev->x,ev->y);
    if (ev->type != ButtonPress || ev->button!=1) return;
    
    /* convert x value to a time value */
    time = posx_to_time_edge (trace, ev->x, ev->y);
    if (time<0) return;
    
    /* make the cursor */
    cur_add (time, global->highlight_color, USER);
    
    draw_all_needed (trace);
    }

void    cur_move_ev (w, trace, ev)
    Widget		w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    DTime	time;
    Position	x1,x2,y1,y2;
    static	last_x = 0;
    XEvent	event;
    CURSOR	*csr_ptr;
    XMotionEvent *em;
    XButtonEvent *eb;
    
    if (DTPRINT_ENTRY) printf ("In cursor_mov\n");
    if (ev->type != ButtonPress || ev->button!=1) return;
    
    csr_ptr = posx_to_cursor (trace, ev->x);
    if (!csr_ptr) return;

    /* csr_ptr is the cursor to be moved - calculate starting x */
    last_x = TIME_TO_XPOS (csr_ptr->time);
    
    /* not sure why this has to be done but it must be done */
    XUngrabPointer (XtDisplay (trace->work),CurrentTime);
    
    /* select the events the widget will respond to */
    XSelectInput (XtDisplay (trace->work),XtWindow (trace->work),
		 ButtonReleaseMask|PointerMotionMask|StructureNotifyMask|ExposureMask);
    
    /* change the GC function to drag the cursor */
    xgcv.function = GXinvert;
    XChangeGC (global->display,trace->gc,GCFunction,&xgcv);
    XSync (global->display,0);
    
    /* draw the first line */
    y1 = 25;
    y2 = trace->height - trace->ystart + trace->sighgt;
    x1 = x2 = last_x = ev->x;
    XDrawLine (global->display,trace->wind,trace->gc,x1,y1,x2,y2);
    
    /* loop and service events until button is released */
    while ( 1 )
	{
	/* wait for next event */
	XNextEvent (XtDisplay (trace->work),&event);

	/* if the pointer moved, erase previous line and draw new one */
	if (event.type == MotionNotify)
	    {
	    XSetForeground (global->display, trace->gc, trace->xcolornums[csr_ptr->color]);
	    em = (XMotionEvent *)&event;
	    x1 = x2 = last_x;
	    y1 = 25;
	    y2 = trace->height - trace->ystart + trace->sighgt;
	    XDrawLine (global->display,trace->wind,trace->gc,x1,y1,x2,y2);
	    x1 = x2 = em->x;
	    XDrawLine (global->display,trace->wind,trace->gc,x1,y1,x2,y2);
	    last_x = em->x;
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
    
    /* remove, change time, and add back the cursor */
    cur_remove (csr_ptr);
    cur_add (time, csr_ptr->color, USER);
    XtFree ((char *)csr_ptr);
    
    draw_all_needed (trace);
    }

void    cur_delete_ev (w, trace, ev)
    Widget		w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    CURSOR	*csr_ptr;
    
    if (DTPRINT_ENTRY) printf ("In cur_delete_ev - trace=%d x=%d y=%d\n",trace,ev->x,ev->y);
    if (ev->type != ButtonPress || ev->button!=1) return;
    
    csr_ptr = posx_to_cursor (trace, ev->x);
    if (!csr_ptr) return;
    
    /* delete the cursor */
    cur_remove (csr_ptr);
    DFree (csr_ptr);
    
    draw_all_needed (trace);
    }

void    cur_highlight_ev (w, trace, ev)
    Widget		w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    CURSOR	*csr_ptr;
    
    if (DTPRINT_ENTRY) printf ("In cur_highlight_ev - trace=%d x=%d y=%d\n",trace,ev->x,ev->y);
    if (ev->type != ButtonPress || ev->button!=1) return;
    
    csr_ptr = posx_to_cursor (trace, ev->x);
    if (!csr_ptr) return;
    
    /* change color */
    csr_ptr->color = global->highlight_color;
    
    draw_all_needed (trace);
    }

