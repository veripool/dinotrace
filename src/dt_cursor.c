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
    
    if (DTPRINT) printf ("In cur_remove\n");
    
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

void cur_add (ctime, color, search)
    DTime	ctime;
    ColorNum	color;
    int		search;
{
    CURSOR *new_csr_ptr;
    CURSOR *prev_csr_ptr, *csr_ptr;
    
    if (DTPRINT) printf ("In cur_add - time=%d\n",ctime);
    
    new_csr_ptr = (CURSOR *)XtMalloc (sizeof (CURSOR));
    new_csr_ptr->time = ctime;
    new_csr_ptr->color = color;
    new_csr_ptr->search = search;

    prev_csr_ptr = NULL;
    for (csr_ptr = global->cursor_head;
	 csr_ptr && ( csr_ptr->time < new_csr_ptr->time );
	 csr_ptr = csr_ptr->next) {
	prev_csr_ptr = csr_ptr;
	}

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

/****************************** MENU OPTIONS ******************************/

void    cur_add_cb (w, trace, cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    int i;

    if (DTPRINT) printf ("In cur_add_cb - trace=%d\n",trace);
    
    /* remove any previous events */
    remove_all_events (trace);
    
    /* Grab color number from the menu button pointer */
    global->highlight_color = 0;
    for (i=1; i<=MAX_SRCH; i++) {
	if (w == trace->menu.pdsubbutton[i + trace->menu.cur_add_pds]) {
	    global->highlight_color = i;
	    }
	}

    /* process all subsequent button presses as cursor adds */
    set_cursor (trace, DC_CUR_ADD);
    add_event (ButtonPressMask, cur_add_ev);
    }

void    cur_mov_cb (w, trace, cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    
    if (DTPRINT) printf ("In cur_mov_cb - trace=%d\n",trace);
    
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
    
    if (DTPRINT) printf ("In cur_del_cb - trace=%d\n",trace);
    
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
    
    if (DTPRINT) printf ("In cur_clr_cb.\n");
    
    /* clear the cursor array to zero */
    while ( global->cursor_head ) {
	csr_ptr = global->cursor_head;
	global->cursor_head = csr_ptr->next;
	DFree (csr_ptr);
	}
    
    /* cancel the button actions */
    remove_all_events (trace);
    
    /* redraw the screen with no cursors */
    redraw_all (trace);
    }

void    cur_highlight_cb (w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    int i;

    if (DTPRINT) printf ("In cur_highlight_cb - trace=%d\n",trace);
    
    /* remove any previous events */
    remove_all_events (trace);
     
    /* Grab color number from the menu button pointer */
    global->highlight_color = 0;
    for (i=1; i<=MAX_SRCH; i++) {
	if (w == trace->menu.pdsubbutton[i + trace->menu.cur_highlight_pds]) {
	    global->highlight_color = i;
	    }
	}

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
    
    if (DTPRINT) printf ("In add cursor - trace=%d x=%d y=%d\n",trace,ev->x,ev->y);
    
    /* convert x value to a time value */
    time = posx_to_time_edge (trace, ev->x, ev->y);
    if (time<0) return;
    
    /* make the cursor */
    cur_add (time, global->highlight_color, 0);
    
    /* redraw the screen with new cursors */
    redraw_all (trace);
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
    
    if (DTPRINT) printf ("In cursor_mov\n");
    
    csr_ptr = posx_to_cursor (trace, ev->x);
    if (!csr_ptr) return;

    /* csr_ptr is the cursor to be moved - calculate starting x */
    last_x = (csr_ptr->time - global->time)* global->res + global->xstart;
    
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
	/* if window was resized, must redraw it */
	if (event.type == Expose ||
	    event.type == ConfigureNotify) {
	    win_expose_cb (0,trace);
	    }
	
	/* button released - calculate cursor position and leave the loop */
	if (event.type == ButtonRelease) {
	    eb = (XButtonReleasedEvent *)&event;
	    time = posx_to_time_edge (trace, eb->x, eb->y);
	    break;
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
    cur_add (time, csr_ptr->color, 0);
    XtFree ((char *)csr_ptr);
    
    /* redraw the screen with new cursor position */
    redraw_all (trace);
    }

void    cur_delete_ev (w, trace, ev)
    Widget		w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    CURSOR	*csr_ptr;
    
    if (DTPRINT) printf ("In cur_delete_ev - trace=%d x=%d y=%d\n",trace,ev->x,ev->y);
    
    csr_ptr = posx_to_cursor (trace, ev->x);
    if (!csr_ptr) return;
    
    /* delete the cursor */
    cur_remove (csr_ptr);
    DFree (csr_ptr);
    
    /* redraw the screen with new cursors */
    redraw_all (trace);
    }

void    cur_highlight_ev (w, trace, ev)
    Widget		w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    CURSOR	*csr_ptr;
    
    if (DTPRINT) printf ("In cur_highlight_ev - trace=%d x=%d y=%d\n",trace,ev->x,ev->y);
    
    csr_ptr = posx_to_cursor (trace, ev->x);
    if (!csr_ptr) return;
    
    /* change color */
    csr_ptr->color = global->highlight_color;
    
    /* redraw the screen with new cursors */
    redraw_all (trace);
    }

