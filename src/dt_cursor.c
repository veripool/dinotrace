#ident "$Id$"
/******************************************************************************
 * dt_cursor.c --- cursor requestors, events, etc
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

#include "functions.h"

/****************************** UTILITIES ******************************/

void    cur_free (
    DCursor	*csr_ptr)	/* Cursor to remove */
{
    DFree (csr_ptr->note);
    DFree (csr_ptr);
}

void    cur_remove (
    /* Cursor is removed from list, but not freed! */
    DCursor	*csr_ptr)	/* Cursor to remove */
{
    DCursor	*next_csr_ptr, *prev_csr_ptr;
    
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

void cur_add (
    DTime	ctime,
    ColorNum	color,
    CursorType_t	type,
    const char *note)
{
    DCursor *new_csr_ptr;
    DCursor *prev_csr_ptr, *csr_ptr;
    
    if (DTPRINT_ENTRY) printf ("In cur_add - time=%d\n",ctime);
    
    new_csr_ptr = DNewCalloc (DCursor);
    new_csr_ptr->time = ctime;
    new_csr_ptr->color = color;
    new_csr_ptr->type = type;
    if (note && note[0]) new_csr_ptr->note = strdup(note);

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
	    if (note && note[0]) csr_ptr->note = strdup(note);
	}
	/* else Don't go over existing user one */
	/* Don't need new structure */
	cur_free (new_csr_ptr);
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


void cur_delete_of_type (
    CursorType_t	type)
{
    DCursor	*csr_ptr,*new_csr_ptr;

    for (csr_ptr = global->cursor_head; csr_ptr; ) {
	new_csr_ptr = csr_ptr;
	csr_ptr = csr_ptr->next;
	if (new_csr_ptr->type==type) {
	    cur_remove (new_csr_ptr);
	    cur_free (new_csr_ptr);
	}
    }
}

void cur_write (FILE *writefp, char *c)
{
    DCursor	*csr_ptr;
    char strg[MAXTIMELEN];

    for (csr_ptr = global->cursor_head; csr_ptr; csr_ptr = csr_ptr->next) {
	time_to_string (global->trace_head, strg, csr_ptr->time, FALSE);
	switch (csr_ptr->type) {
	case USER:
	    fprintf (writefp, "%scursor_add %s %d -user\n",c, strg, csr_ptr->color);
	    break;
	case CONFIG:
	    fprintf (writefp, "%scursor_add %s %d\n",c, strg, csr_ptr->color);
	    break;
	default:
	    break;
	}
    }
}

DTime cur_time_first (
    Trace 	*trace)
    /* Return time of the first cursor, or BOT if none */
{
    DCursor	*csr_ptr;

    csr_ptr = global->cursor_head;
    if (csr_ptr) {
	return (csr_ptr->time);
    }
    else {
	return (trace->start_time);
    }
}

DTime cur_time_last (
    Trace 	*trace)
    /* Return time of the last cursor, or EOT if none */
{
    DCursor	*csr_ptr;

    for (csr_ptr = global->cursor_head; csr_ptr && csr_ptr->next; csr_ptr = csr_ptr->next) ;
    if (csr_ptr) {
	return (csr_ptr->time);
    }
    else {
	return (trace->end_time);
    }
}

void cur_step (
    DTime	step
    )
    /* Move all cursors the specified distance */
{
    DCursor	*csr_ptr;
    DCursor	*nxt_csr_ptr;

    if (DTPRINT_ENTRY) printf ("In cur_step %d\n", step);
    for (csr_ptr = global->cursor_head; csr_ptr; csr_ptr = nxt_csr_ptr) {
	nxt_csr_ptr = csr_ptr->next;
	if (csr_ptr->type == USER) {
	    if (step >= 0 || csr_ptr->time >= -step) {
		csr_ptr->time += step;
	    } else {
		cur_remove (csr_ptr);
	    }
	}
    }
    draw_all_needed ();
}

/****************************** EXAMINE ******************************/

char *cur_examine_string (
    /* Return string with examine information in it */
    Trace	*trace,
    DCursor	*csr_ptr)
{
    static char	strg[2000];
    char	strg2[2000];
    
    if (DTPRINT_ENTRY) printf ("val_examine_popup_csr_string\n");

    strcpy (strg, "Cursor at Time ");
    time_to_string (trace, strg2, csr_ptr->time, FALSE);
    strcat (strg, strg2);
    strcat (strg, "\n");
    
    switch (csr_ptr->type) {
    case USER:
	strcat (strg, "Placed by you\n");
	break;
    case SEARCH:
	strcat (strg, "Placed by Value Search\n");
	break;
    case CONFIG:
	strcat (strg, "Placed by .dino file or Emacs\n");
	break;
    case SEARCHOLD:
    }
    if (csr_ptr->note) {
        strcat (strg, csr_ptr->note);
        strcat (strg, "\n");
    }
    return (strg);
}
	

/****************************** MENU OPTIONS ******************************/

void    cur_add_cb (
    Widget		w)
{
    Trace *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In cur_add_cb - trace=%p\n",trace);
    
    /* Grab color number from the menu button pointer */
    global->highlight_color = submenu_to_color (trace, w, trace->menu.cur_add_pds);

    /* process all subsequent button presses as cursor adds */
    remove_all_events (trace);
    set_cursor (DC_CUR_ADD);
    add_event (ButtonPressMask, cur_add_ev);
}

void    cur_mov_cb (
    Widget		w)
{
    Trace *trace = widget_to_trace(w);
    
    if (DTPRINT_ENTRY) printf ("In cur_mov_cb - trace=%p\n",trace);
    
    /* process all subsequent button presses as cursor moves */
    remove_all_events (trace);
    set_cursor (DC_CUR_MOVE);
    add_event (ButtonPressMask, cur_move_ev);
}

void    cur_del_cb (
    Widget		w)
{
    Trace *trace = widget_to_trace(w);
    
    if (DTPRINT_ENTRY) printf ("In cur_del_cb - trace=%p\n",trace);
    
    /* process all subsequent button presses as cursor deletes */
    remove_all_events (trace);
    set_cursor (DC_CUR_DELETE);
    add_event (ButtonPressMask, cur_delete_ev);
}

void    cur_clr_cb (
    Widget		w)
{
    Trace *trace = widget_to_trace(w);
    DCursor	*csr_ptr;
    
    if (DTPRINT_ENTRY) printf ("In cur_clr_cb.\n");
    
    /* clear the cursor array to zero */
    while ( global->cursor_head ) {
	csr_ptr = global->cursor_head;
	global->cursor_head = csr_ptr->next;
	cur_free (csr_ptr);
    }
    
    /* cancel the button actions */
    remove_all_events (trace);
    
    draw_all_needed ();
}

void    cur_highlight_cb (
    Widget		w)
{
    Trace *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In cur_highlight_cb - trace=%p\n",trace);
    
    /* Grab color number from the menu button pointer */
    global->highlight_color = submenu_to_color (trace, w, trace->menu.cur_highlight_pds);

    /* process all subsequent button presses as signal deletions */ 
    remove_all_events (trace);
    set_cursor (DC_CUR_HIGHLIGHT);
    add_event (ButtonPressMask, cur_highlight_ev);
}

void    cur_step_fwd_cb (
    Widget		w)
{
    Trace *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In cur_step_fwd_cb.\n");
    cur_step (grid_primary_period (trace));
}

void    cur_step_back_cb (
    Widget		w)
{
    Trace *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In cur_step_back_cb.\n");
    cur_step ( - grid_primary_period (trace));
}

/****************************** EVENTS ******************************/

void    cur_add_ev (
    Widget		w,
    Trace		*trace,
    XButtonPressedEvent	*ev)
{
    DTime	time;
    
    if (DTPRINT_ENTRY) printf ("In add cursor - trace=%p x=%d y=%d\n",trace,ev->x,ev->y);
    if (ev->type != ButtonPress || ev->button!=1) return;
    
    /* convert x value to a time value */
    time = posx_to_time_edge (trace, ev->x, ev->y);
    if (time<0) return;
    
    /* make the cursor */
    cur_add (time, global->highlight_color, USER, NULL);
    
    draw_all_needed ();
}

void    cur_move_ev (
    Widget		w,
    Trace		*trace,
    XButtonPressedEvent	*ev)
{
    DTime	time;
    Position	x1,x2,y1,y2;
    static	last_x = 0;
    XEvent	event;
    DCursor	*csr_ptr;
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
    y2 = trace->height - trace->ystart + global->sighgt;
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
	    y2 = trace->height - trace->ystart + global->sighgt;
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
    cur_add (time, csr_ptr->color, USER, csr_ptr->note);
    cur_free (csr_ptr);
    
    draw_all_needed ();
}

void    cur_delete_ev (
    Widget		w,
    Trace		*trace,
    XButtonPressedEvent	*ev)
{
    DCursor	*csr_ptr;
    
    if (DTPRINT_ENTRY) printf ("In cur_delete_ev - trace=%p x=%d y=%d\n",trace,ev->x,ev->y);
    if (ev->type != ButtonPress || ev->button!=1) return;
    
    csr_ptr = posx_to_cursor (trace, ev->x);
    if (!csr_ptr) return;
    
    /* delete the cursor */
    cur_remove (csr_ptr);
    cur_free (csr_ptr);
    
    draw_all_needed ();
}

void    cur_highlight_ev (
    Widget		w,
    Trace		*trace,
    XButtonPressedEvent	*ev)
{
    DCursor	*csr_ptr;
    
    if (DTPRINT_ENTRY) printf ("In cur_highlight_ev - trace=%p x=%d y=%d\n",trace,ev->x,ev->y);
    if (ev->type != ButtonPress || ev->button!=1) return;
    
    csr_ptr = posx_to_cursor (trace, ev->x);
    if (!csr_ptr) return;
    
    /* change color */
    csr_ptr->color = global->highlight_color;
    
    draw_all_needed ();
}

