/******************************************************************************
 * DESCRIPTION: Dinotrace source: cursor requestors, events, etc
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

#include "functions.h"

#include <assert.h>

/****************************** UTILITIES ******************************/

void    cur_free (
    DCursor_t	*csr_ptr)	/* Cursor to remove */
{
    DFree (csr_ptr->note);
    DFree (csr_ptr);
}

void    cur_remove (
    /* Cursor is removed from list, but not freed! */
    DCursor_t	*csr_ptr)	/* Cursor to remove */
{
    DCursor_t	*next_csr_ptr, *prev_csr_ptr;

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

void cur_delete (
    DCursor_t *csr_ptr
    )
{
    if (csr_ptr->type == SIMVIEW) {
	/* Pass this event to SimView. */
	simview_cur_delete (csr_ptr->simview_id);
    }

    /* Delete it, unless this is a SimView cursor and a SimView response will delete it. */
    if (csr_ptr->type != SIMVIEW || global->simview_info_ptr->handshake == FALSE) {
	cur_remove (csr_ptr);
	cur_free (csr_ptr);
    }
}

DCursor_t *cur_add (
    /* Add a cursor, or replace an existing one at the same time as long as neither is
       a SIMVIEW type.  Return the new or conflicting cursor. */
    DTime_t	ctime,
    ColorNum_t	color,
    CursorType_t	type,
    const char *note)
{
    DCursor_t *new_csr_ptr;
    DCursor_t *prev_csr_ptr, *csr_ptr;

    if (DTPRINT_ENTRY) printf ("In cur_add - time=%d\n",ctime);

    /* No SIMVIEW cursors unless SimView is enabled. */
    assert (type != SIMVIEW || global->simview_info_ptr);

    new_csr_ptr = DNewCalloc (DCursor_t);
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

    /* Same as an existing cursor?  SimView cursors may overlap others (is this ok?) */
    if ((csr_ptr && csr_ptr->time == new_csr_ptr->time) &&
        (new_csr_ptr->type != SIMVIEW && csr_ptr->type != SIMVIEW)) {
        /* Do not create a new cursor */
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
        return (csr_ptr);
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
        return (new_csr_ptr);
    }
}

/* Create new cursor and handle simview cursors */
void cur_new (
    DTime_t ctime,
    ColorNum_t color,
    CursorType_t type,
    const char *note
    )
{
    DCursor_t *csr_ptr;
    static int simview_cur_id = 0;

    /* Add cursor, unless this is a SimView cursor, and a response from SimView will add it. */
    if (type != SIMVIEW || global->simview_info_ptr->handshake == FALSE) {
	csr_ptr = cur_add (ctime, color, type, note);
	/* Set cursor's id. */
	if (type == SIMVIEW) {
	    csr_ptr->simview_id = simview_cur_id;
	}
    }

    if (type == SIMVIEW) {
	/* Pass this event to SimView. */
	simview_cur_create (simview_cur_id, time_to_cyc_num(ctime), (int)color, global->color_names[color]);
	simview_cur_id++;
    }
}

void cur_move (
    DCursor_t *csr_ptr,
    DTime_t new_time
    )
{
    DCursor_t *new_csr_ptr;

    /* Don't move if this is a SimView cursor and a SimView response will move it. */
    if (csr_ptr->type != SIMVIEW || global->simview_info_ptr->handshake == FALSE) {

	/* Move by removing, and adding. */
	cur_remove (csr_ptr);
	new_csr_ptr = cur_add (new_time, csr_ptr->color, ((csr_ptr->type == SIMVIEW) ? SIMVIEW : USER), csr_ptr->note);
	if (csr_ptr->type == SIMVIEW) {
	    new_csr_ptr->simview_id = csr_ptr->simview_id;
	}

	/* A SIMVIEW cursor is the one added if and only if it was intended to be added. */
	assert ((csr_ptr->type == SIMVIEW) ^ (new_csr_ptr->type != SIMVIEW));

	cur_free (csr_ptr);
    } else {
	new_csr_ptr = csr_ptr;
    }
    if (new_csr_ptr->type == SIMVIEW) {
	/* Pass this event to SimView. */
	simview_cur_move (new_csr_ptr->simview_id, time_to_cyc_num(new_time));
    }
}


void    cur_note (
    DCursor_t*		csr_ptr,
    const char*		note)
{
    DFree(csr_ptr->note);
    csr_ptr->note = strdup(note);
}


void cur_delete_of_type (
    CursorType_t	type)
{
    DCursor_t	*csr_ptr,*new_csr_ptr;

    for (csr_ptr = global->cursor_head; csr_ptr; ) {
	new_csr_ptr = csr_ptr;
	csr_ptr = csr_ptr->next;
	if (new_csr_ptr->type==type) {
	    cur_delete (new_csr_ptr);
	}
    }
}

/* Get a SIMVIEW cursor by its id number. */
DCursor_t *cur_id_to_cursor (int id) {
    DCursor_t *cur;

    cur = global->cursor_head;
    while (cur && (cur->type != SIMVIEW || cur->simview_id != id)) {
	cur = cur->next;
    }

    return (cur);
}

void cur_write (FILE *writefp, const char *c)
{
    DCursor_t	*csr_ptr;
    char strg[MAXTIMELEN];

    for (csr_ptr = global->cursor_head; csr_ptr; csr_ptr = csr_ptr->next) {
	time_to_string (global->trace_head, strg, csr_ptr->time, FALSE);
	switch (csr_ptr->type) {
	case USER:
	    fprintf (writefp, "%scursor_add %s %d -user",c, strg, csr_ptr->color);
	    if (csr_ptr->note && csr_ptr->note[0]) fprintf (writefp, " \"%s\"", csr_ptr->note);
	    fprintf (writefp, "\n");
	    break;
	case CONFIG:
	    fprintf (writefp, "%scursor_add %s %d",c, strg, csr_ptr->color);
	    if (csr_ptr->note && csr_ptr->note[0]) fprintf (writefp, " \"%s\"", csr_ptr->note);
	    fprintf (writefp, "\n");
	    break;
	default:
	    break;
	}
    }
}

DTime_t cur_time_first (
    const Trace_t 	*trace)
    /* Return time of the first cursor, or BOT if none */
{
    DCursor_t	*csr_ptr;

    csr_ptr = global->cursor_head;
    if (csr_ptr) {
	return (csr_ptr->time);
    }
    else {
	return (trace->start_time);
    }
}

DTime_t cur_time_last (
    const Trace_t 	*trace)
    /* Return time of the last cursor, or EOT if none */
{
    DCursor_t	*csr_ptr;

    for (csr_ptr = global->cursor_head; csr_ptr && csr_ptr->next; csr_ptr = csr_ptr->next) ;
    if (csr_ptr) {
	return (csr_ptr->time);
    }
    else {
	return (trace->end_time);
    }
}

/* Used only by cur_step.  This steps one cursor.
 * Only USER and SIMVIEW cursors are moved.
 * (Note that SIMVIEW cursors may only notify SimView that a move is requested without
 *  performing the move.) */
static void cur_step_one (
    DTime_t       step,
    DCursor_t     *csr_ptr
    )
{
    if (csr_ptr->type == USER || csr_ptr->type == SIMVIEW) {
        if (step >= 0 || csr_ptr->time >= -step) {
            cur_move (csr_ptr, csr_ptr->time + step);
        } else {
            cur_delete (csr_ptr);
        }
    }
}

void cur_step (
    DTime_t	step
    )
    /* Move all USER and SIMVIEW cursors the specified distance */
{
    DCursor_t	*csr_ptr;
    DCursor_t	*nxt_csr_ptr;

    /* Since the cursor list changes as we step through it, we step
     * through it forward if cursors are moving back, and backward
     * if cursors are moving forward.  This ensures that we hit
     * every cursor exactly once.
     */

    if (DTPRINT_ENTRY) printf ("In cur_step %d\n", step);

    if (step < 0) {
        /* Step forward through cursors */
        for (csr_ptr = global->cursor_head; csr_ptr; csr_ptr = nxt_csr_ptr) {
	    nxt_csr_ptr = csr_ptr->next;
            cur_step_one (step, csr_ptr);
        }
    } else {
        /* Step backward through cursors */
        if (global->cursor_head) {
            /* Find last cursor */
            for (csr_ptr = global->cursor_head; csr_ptr->next; csr_ptr = csr_ptr->next) {
            }
            /* Step backward through cursors */
            for (; csr_ptr; csr_ptr = nxt_csr_ptr) {
                nxt_csr_ptr = csr_ptr->prev;
                cur_step_one (step, csr_ptr);
            }
        }
    }
    draw_all_needed ();
}

void cur_highlight (
    /* Set the highlight color for a cursor.  Communicate the change to SimView if necessary. */
    DCursor_t	*csr_ptr,
    ColorNum_t      color
    )
{
    csr_ptr->color = color;

    if (csr_ptr->type == SIMVIEW) {
	/* Pass this event to SimView. (note that highlights are always */
	/* controlled by Dinotrace, not SimView, and no handshaking is done). */
	simview_cur_highlight (csr_ptr->simview_id, color, global->color_names[color]);
    }
}

/****************************** EXAMINE ******************************/

char *cur_examine_string (
    /* Return string with examine information in it */
    Trace_t	*trace,
    DCursor_t	*csr_ptr)
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
    case SIMVIEW:
	strcat (strg, "Placed by simview\n");
	break;
    case SEARCHOLD:
	break;
    }
    if (csr_ptr->note) {
        strcat (strg, csr_ptr->note);
        strcat (strg, "\n");
    }
    return (strg);
}


/****************************** MENU OPTIONS ******************************/

static void    cur_add_internal (
    Widget		w,
    CursorType_t	type,
    ColorNum_t		colorOverride
    )
{
    Trace_t *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In cur_add_internal - trace=%p\n",trace);

    global->selected_curtype = type;

    /* Grab color number from the menu button pointer */
    global->highlight_color = submenu_to_color
	(trace, w, colorOverride,
	 (type == SIMVIEW) ? trace->menu.cur_add_simview_pds : trace->menu.cur_add_pds);

    /* process all subsequent button presses as cursor adds */
    remove_all_events (trace);
    set_cursor (DC_CUR_ADD);
    add_event (ButtonPressMask, cur_add_ev);
}

void    cur_add_cb (
    Widget		w)
{
    cur_add_internal (w, USER, 0);
}

void    cur_add_current_cb (
    Widget		w)
{
    cur_add_internal (w, USER, COLOR_CURRENT);
}

void    cur_add_next_cb (
    Widget		w)
{
    cur_add_internal (w, USER, COLOR_NEXT);
}

void    cur_add_simview_cb (
    Widget		w)
{
    cur_add_internal (w, SIMVIEW, 0);
}

void    cur_mov_cb (
    Widget		w)
{
    Trace_t *trace = widget_to_trace(w);

    if (DTPRINT_ENTRY) printf ("In cur_mov_cb - trace=%p\n",trace);

    /* process all subsequent button presses as cursor moves */
    remove_all_events (trace);
    set_cursor (DC_CUR_MOVE);
    add_event (ButtonPressMask, cur_move_ev);
}

void    cur_del_cb (
    Widget		w)
{
    Trace_t *trace = widget_to_trace(w);

    if (DTPRINT_ENTRY) printf ("In cur_del_cb - trace=%p\n",trace);

    /* process all subsequent button presses as cursor deletes */
    remove_all_events (trace);
    set_cursor (DC_CUR_DELETE);
    add_event (ButtonPressMask, cur_delete_ev);
}

void    cur_clr_cb (
    Widget		w)
{
    Trace_t *trace = widget_to_trace(w);
    DCursor_t	*csr_ptr, *next;

    if (DTPRINT_ENTRY) printf ("In cur_clr_cb.\n");

    /* delete every cursor */
    for (csr_ptr = global->cursor_head; csr_ptr != NULL; csr_ptr = next) {
	next = csr_ptr->next;
	cur_delete (csr_ptr);  /* Note, this may not delete the cursor if it is
				* a SimView cursor with handshaking. */
    }

    /* cancel the button actions */
    remove_all_events (trace);

    draw_all_needed ();
}

void    cur_highlight_cb (
    Widget		w)
{
    Trace_t *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In cur_highlight_cb - trace=%p\n",trace);

    /* Grab color number from the menu button pointer */
    global->highlight_color = submenu_to_color (trace, w, 0, trace->menu.cur_highlight_pds);

    /* process all subsequent button presses as signal highlights */
    remove_all_events (trace);
    set_cursor (DC_CUR_HIGHLIGHT);
    add_event (ButtonPressMask, cur_highlight_ev);
}

void    cur_step_fwd_cb (
    Widget		w)
{
    Trace_t *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In cur_step_fwd_cb.\n");
    cur_step (grid_primary_period (trace));
}

void    cur_step_back_cb (
    Widget		w)
{
    Trace_t *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In cur_step_back_cb.\n");
    cur_step ( - grid_primary_period (trace));
}

void    cur_note_cb (
    Widget		w)
{
    Trace_t *trace = widget_to_trace(w);

    if (DTPRINT_ENTRY) printf ("In cur_note_cb - trace=%p\n",trace);

    /* process all subsequent button presses as cursor moves */
    remove_all_events (trace);
    set_cursor (DC_CUR_NOTE);
    add_event (ButtonPressMask, cur_note_ev);
}

/****************************** EVENTS ******************************/

void    cur_add_ev (
    Widget		w,
    Trace_t		*trace,
    XButtonPressedEvent	*ev)
{
    DTime_t	time;

    if (DTPRINT_ENTRY) printf ("In cur_add_ev - trace=%p x=%d y=%d\n",trace,ev->x,ev->y);
    if (ev->type != ButtonPress || ev->button!=1) return;

    /* convert x value to a time value */
    time = posx_to_time_edge (trace, ev->x, ev->y);
    if (time<0) return;

    /* make the cursor */
    cur_new (time, global->highlight_color, global->selected_curtype, NULL);

    draw_all_needed ();
}

void    cur_move_ev (
    Widget		w,
    Trace_t		*trace,
    XButtonPressedEvent	*ev)
{
    DTime_t	time;
    Position	x1,x2,y1,y2;
    static	int last_x = 0;
    XEvent	event;
    DCursor_t	*csr_ptr;
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

    /* move the cursor */
    cur_move (csr_ptr, time);

    draw_all_needed ();
}

void    cur_delete_ev (
    Widget		w,
    Trace_t		*trace,
    XButtonPressedEvent	*ev)
{
    DCursor_t	*csr_ptr;

    if (DTPRINT_ENTRY) printf ("In cur_delete_ev - trace=%p x=%d y=%d\n",trace,ev->x,ev->y);
    if (ev->type != ButtonPress || ev->button!=1) return;

    csr_ptr = posx_to_cursor (trace, ev->x);
    if (!csr_ptr) return;

    /* delete the cursor */
    cur_delete (csr_ptr);

    draw_all_needed ();
}

void    cur_highlight_ev (
    Widget		w,
    Trace_t		*trace,
    XButtonPressedEvent	*ev)
{
    DCursor_t	*csr_ptr;

    if (DTPRINT_ENTRY) printf ("In cur_highlight_ev - trace=%p x=%d y=%d\n",trace,ev->x,ev->y);
    if (ev->type != ButtonPress || ev->button!=1) return;

    csr_ptr = posx_to_cursor (trace, ev->x);
    if (!csr_ptr) return;

    /* change color */
    cur_highlight (csr_ptr, global->highlight_color);

    draw_all_needed ();
}

void    cur_note_ev (
    Widget		w,
    Trace_t		*trace,
    XButtonPressedEvent	*ev)
{
    DCursor_t	*csr_ptr;

    if (DTPRINT_ENTRY) printf ("In cur_note_ev - trace=%p x=%d y=%d\n",trace,ev->x,ev->y);
    if (ev->type != ButtonPress || ev->button!=1) return;

    csr_ptr = posx_to_cursor (trace, ev->x);
    if (!csr_ptr) return;

    /* change cursor note */
    global->selected_cursor = csr_ptr;
    win_note(trace, "Note for Cursor", "", csr_ptr->note, TRUE);
}

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/* Hooks for SimView */
/**********************************************************************/

/* These hooks were added for a proprietary tool of Compaq Computer Corp. called SimView.
 * The hooks may be useful for other tools that wish to be controlled by Dinotrace cursors.
 */

#if !HAVE_SIMVIEW

/* Call at initialization when "-simview" arg is processed. */
void simview_init (char *arg) {
    printf ("The SimView library is not available, \"-simview\" should not be used.\n");
}
/* Call when a simview cursor is created/moved/etc. */
void simview_cur_create (int cur_num, double cyc_num, ColorNum_t color, char *color_name) {}
void simview_cur_move (int cur_num, double cyc_num) {}
void simview_cur_highlight (int cur_num, ColorNum_t color, char *color_name) {}
void simview_cur_delete (int cur_num) {}

#endif
