#ident "$Id$"
/******************************************************************************
 * dinotrace.c --- main routine and documentation
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

#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/SelectioB.h>
#include <Xm/Text.h>
#include <Xm/BulletinB.h>
#include <Xm/RowColumn.h>
#include <Xm/RowColumnP.h>
#include <Xm/Label.h>
#include <Xm/LabelP.h>

#include "functions.h"

/****************************** UTILITIES ******************************/

void    value_to_string (
    Trace *trace,
    char *strg,
    uint_t cptr[],
    char seperator)		/* What to print between the values */
{
    if (cptr[3]) {
	if (trace->busrep == BUSREP_HEX_UN)
	    sprintf (strg,"%x%c%08x%c%08x%c%08x", cptr[3], seperator, cptr[2], seperator, cptr[1], seperator, cptr[0]);
	else if (trace->busrep == BUSREP_OCT_UN)
	    sprintf (strg,"%o%c%010o%c%010o%c%010o", cptr[3], seperator, cptr[2], seperator, cptr[1], seperator, cptr[0]);
	else
	    sprintf (strg,"%d%c%010d%c%010d%c%010d", cptr[3], seperator, cptr[2], seperator, cptr[1], seperator, cptr[0]);
    }
    else if (cptr[2]) {
	if (trace->busrep == BUSREP_HEX_UN)
	    sprintf (strg,"%x%c%08x%c%08x", cptr[2], seperator, cptr[1], seperator, cptr[0]);
	else if (trace->busrep == BUSREP_OCT_UN)
	    sprintf (strg,"%o%c%010o%c%010o", cptr[2], seperator, cptr[1], seperator, cptr[0]);
	else
	    sprintf (strg,"%d%c%010d%c%010d", cptr[2], seperator, cptr[1], seperator, cptr[0]);
    }
    else if (cptr[1]) {
	if (trace->busrep == BUSREP_HEX_UN)
	    sprintf (strg,"%x%c%08x", cptr[1], seperator, cptr[0]);
	else if (trace->busrep == BUSREP_OCT_UN)
	    sprintf (strg,"%o%c%010o", cptr[1], seperator, cptr[0]);
	else
	    sprintf (strg,"%d%c%010d", cptr[1], seperator, cptr[0]);
    }
    else {
	if (trace->busrep == BUSREP_HEX_UN)
	    sprintf (strg,"%x", cptr[0]);
	else if (trace->busrep == BUSREP_OCT_UN)
	    sprintf (strg,"%o", cptr[0]);
	else
	    sprintf (strg,"%d", cptr[0]);
    }
}

void    string_to_value (
    Trace *trace,
    char *strg,
    uint_t cptr[])
{
    register char value;
    uint_t MSO = (7<<29);		/* Most significant hex digit */
    uint_t MSH = (15<<28);	/* Most significant octal digit */
    register char *cp;

    cptr[0] = cptr[1] = cptr[2] = cptr[3] = 0;

    for (cp=strg; *cp; cp++) {
	value = -1;
	if (*cp >= '0' && *cp <= '9')
	    value = *cp - '0';
	else if (*cp >= 'A' && *cp <= 'F')
	    value = *cp - ('A' - 10);
	else if (*cp >= 'a' && *cp <= 'f')
	    value = *cp - ('a' - 10);

	if (trace->busrep == BUSREP_HEX_UN && value >=0 && value <= 15) {
	    cptr[3] = (cptr[3]<<4) + ((cptr[2] & MSH)>>28);
	    cptr[2] = (cptr[2]<<4) + ((cptr[1] & MSH)>>28);
	    cptr[1] = (cptr[1]<<4) + ((cptr[0] & MSH)>>28);
	    cptr[0] = (cptr[0]<<4) + value;
	}
	else if (trace->busrep == BUSREP_OCT_UN && value >=0 && value <= 7) {
	    cptr[3] = (cptr[3]<<3) + ((cptr[2] & MSO)>>29);
	    cptr[2] = (cptr[2]<<3) + ((cptr[1] & MSO)>>29);
	    cptr[1] = (cptr[1]<<3) + ((cptr[0] & MSO)>>29);
	    cptr[0] = (cptr[0]<<3) + value;
	}
	else if (trace->busrep == BUSREP_DEC_UN && value >=0 && value <= 9) {
	    /* This may be buggy for large numbers */
	    cptr[3] = (cptr[3]*10) + ((cp>=(strg+30))?cp[-30]:0);
	    cptr[2] = (cptr[2]*10) + ((cp>=(strg+20))?cp[-20]:0);
	    cptr[1] = (cptr[1]*10) + ((cp>=(strg+10))?cp[-10]:0);
	    cptr[0] = (cptr[0]*10) + value;
	}
    }
}

void    cptr_to_search_value (
    SignalLW	*cptr,
    uint_t value[])
{
    value[0] = value[1] = value[2] = value[3] = 0;
    switch (cptr->sttime.state) {
      case STATE_0:
      case STATE_U:
      case STATE_Z:
	break;

      case STATE_1:
	value[0] = 1;
	break;
	
      case STATE_B32:
	value[0] = *((uint_t *)cptr+1);
	break;
	
      case STATE_B128:
	value[0] = *((uint_t *)cptr+1);
	value[1] = *((uint_t *)cptr+2);
	value[2] = *((uint_t *)cptr+3);
	value[3] = *((uint_t *)cptr+4);
	break;
    } /* switch */
}

void	val_update_search ()
{
    Trace	*trace;
    Signal	*sig_ptr;
    SignalLW	*cptr;
    int		cursorize;
    register int i;
    DCursor	*csr_ptr;
    Boolean	any_enabled;
    Boolean	matches[MAX_SRCH];	/* Cache the wildmat for each bit, so searching is faster */

    if (DTPRINT_ENTRY) printf ("In val_update_search\n");

    /* If no searches are enabled, skip the cptr loop.  This saves */
    /* 3% of the first reading time on large traces */
    any_enabled = FALSE;
    for (i=0; i<MAX_SRCH; i++) {
	if (global->val_srch[i].color != 0) any_enabled = TRUE;
	if (global->val_srch[i].cursor != 0) any_enabled = TRUE;
    }

    /* Mark all cursors that are a result of a search as old (-1) */
    for (csr_ptr = global->cursor_head; csr_ptr; csr_ptr = csr_ptr->next) {
	if (csr_ptr->type==SEARCH) csr_ptr->type = SEARCHOLD;
    }

    /* Search every trace for the value, mark the signal if it has it to speed up displaying */
    for (trace = global->deleted_trace_head; trace; trace = trace->next_trace) {
	for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	    if (sig_ptr->lws == 1) {
		/* Single bit signal, don't search for values */
		continue;
	    }
	    
	    if (any_enabled) {
		cursorize=0;
		for (i=0; i<MAX_SRCH; i++) {
		    matches[i] = 0;
		    sig_ptr->srch_ena[i] = FALSE;
		}

		cptr = (SignalLW *)(sig_ptr->bptr);
		for (; (cptr->sttime.time != EOT); cptr += sig_ptr->lws) {
		    switch (cptr->sttime.state) {
		      case STATE_B32:
			for (i=0; i<MAX_SRCH; i++) {
			    if ( ( global->val_srch[i].value[0]== *((uint_t *)cptr+1) )
				&& ( global->val_srch[i].value[1] == 0) 
				&& ( global->val_srch[i].value[2] == 0)
				&& ( global->val_srch[i].value[3] == 0)
				&& ( matches[i] || wildmat (sig_ptr->signame, global->val_srch[i].signal))  ) {
				matches[i] = TRUE;
				if ( global->val_srch[i].color != 0)  sig_ptr->srch_ena[i] = TRUE;
				if ( global->val_srch[i].cursor != 0) cursorize = global->val_srch[i].cursor;
				/* don't break, because if same value on two lines, one with cursor and one without will fail */
			    }
			}
			break;
			
		      case STATE_B128:
			for (i=0; i<MAX_SRCH; i++) {
			    if ( ( global->val_srch[i].value[0]== *((uint_t *)cptr+1) )
				&& ( global->val_srch[i].value[1]== *((uint_t *)cptr+2) )
				&& ( global->val_srch[i].value[2]== *((uint_t *)cptr+3) )
				&& ( global->val_srch[i].value[3]== *((uint_t *)cptr+4) )
				&& ( matches[i] || wildmat (sig_ptr->signame, global->val_srch[i].signal))  ) {
				matches[i] = TRUE;
				if ( global->val_srch[i].color != 0)  sig_ptr->srch_ena[i] = TRUE;
				if ( global->val_srch[i].cursor != 0) cursorize = global->val_srch[i].cursor;
			    }
			}
			break;
		    } /* switch */
		    
		    if (cursorize) {
			if (NULL != (csr_ptr = time_to_cursor (cptr->sttime.time))) {
			    if (csr_ptr->type == SEARCHOLD) {
				/* mark the old cursor as new so won't be deleted */
				csr_ptr->type = SEARCH;
			    }
			}
			else {
			    /* Make new cursor at this location */
			    cur_add (cptr->sttime.time, cursorize, SEARCH);
			}
			cursorize = 0;
		    }
		    
		} /* for cptr */
	    } /* if enabled */
	} /* for sig */
    } /* for trace */

    /* Delete all old cursors */
    cur_delete_of_type (SEARCHOLD);
}

/****************************** STATES ******************************/

void	val_states_update ()
{
    Trace	*trace;
    Signal	*sig_ptr;

    if (DTPRINT_ENTRY) printf ("In update_signal_states\n");

     for (trace = global->deleted_trace_head; trace; trace = trace->next_trace) {
	 for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	     if (NULL != (sig_ptr->decode = find_signal_state (trace, sig_ptr->signame))) {
		 /* if (DTPRINT_FILE) printf ("Signal %s is patterned\n",sig_ptr->signame); */
	     }
	 }
     }
}


/****************************** MENU OPTIONS ******************************/

void    val_examine_cb (
    Widget	w)
{
    Trace *trace = widget_to_trace(w);
    
    if (DTPRINT_ENTRY) printf ("In val_examine_cb - trace=%p\n",trace);
    
    /* remove any previous events */
    remove_all_events (trace);
    
    /* process all subsequent button presses as cursor moves */
    set_cursor (trace, DC_VAL_EXAM);
    add_event (ButtonPressMask, val_examine_ev);
}

void    val_highlight_cb (
    Widget	w)
{
    Trace *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In val_highlight_cb - trace=%p\n",trace);
    
    /* remove any previous events */
    remove_all_events (trace);
     
    /* Grab color number from the menu button pointer */
    global->highlight_color = submenu_to_color (trace, w, trace->menu.val_highlight_pds);

    /* process all subsequent button presses as signal deletions */ 
    set_cursor (trace, DC_VAL_HIGHLIGHT);
    add_event (ButtonPressMask, val_highlight_ev);
}


/****************************** EVENTS ******************************/

char *val_examine_popup_sig_string (
    /* Return string with examine information in it */
    Trace	*trace,
    Signal	*sig_ptr)
{
    static char	strg[2000];
    char	strg2[2000];
    
    if (DTPRINT_ENTRY) printf ("val_examine_popup_sig_string\n");

    strcpy (strg, sig_ptr->signame);
	
    /* Debugging information */
    if (DTDEBUG) {
	sprintf (strg2, "\nType %d   Lws %d   Blocks %ld\n",
		 sig_ptr->type, sig_ptr->lws, sig_ptr->blocks);
	strcat (strg, strg2);
	sprintf (strg2, "Bits %d   Index %d - %d  Srch_ena %p\n",
		 sig_ptr->bits, sig_ptr->msb_index, sig_ptr->lsb_index, sig_ptr->srch_ena);
	strcat (strg, strg2);
	sprintf (strg2, "File_type %x  File_Pos %d-%d  Mask %08x\n",
		 sig_ptr->file_type.flags, sig_ptr->file_pos, sig_ptr->file_end_pos, sig_ptr->pos_mask);
	strcat (strg, strg2);
	sprintf (strg2, "Value_mask %08x %08x %08x %08x\n",
		 sig_ptr->value_mask[3], sig_ptr->value_mask[2], sig_ptr->value_mask[1], sig_ptr->value_mask[0]);
	strcat (strg, strg2);
    }
    return (strg);
}
	
char *val_examine_popup_cptr_string (
    /* Return string with examine information in it */
    Trace	*trace,
    Signal	*sig_ptr,
    DTime	time)
{
    SignalLW	*cptr;
    static char	strg[2000];
    char	strg2[2000];
    uint_t	value[4];
    int		rows, cols, bit, row, col, par;
    uint_t	bit_value;
    char	*format;
    
    if (DTPRINT_ENTRY) printf ("\ttime = %d, signal = %s\n", time, sig_ptr->signame);

    /* Get information */
    cptr = cptr_at_time (sig_ptr, time);
    
    strcpy (strg, sig_ptr->signame);
	
    if (cptr->sttime.time == EOT) {
	strcat (strg, "\nValue at EOT:\n");
    }
    else {
	strcat (strg, "\nValue at times ");
	time_to_string (trace, strg2, cptr->sttime.time, FALSE);
	strcat (strg, strg2);
	strcat (strg, " - ");
	if (((cptr + sig_ptr->lws)->sttime.time) == EOT) {
	    strcat (strg, "EOT:\n");
	}
	else {
	    time_to_string (trace, strg2, (cptr + sig_ptr->lws)->sttime.time, FALSE);
	    strcat (strg, strg2);
	    strcat (strg, ":\n");
	}
    }
    
    cptr_to_search_value (cptr, value);
    
    switch (cptr->sttime.state) {
    case STATE_0:
    case STATE_1:
	sprintf (strg2, "= %d\n", value[0]);
	strcat (strg, strg2);
	break;
	
    case STATE_Z:
	sprintf (strg2, "= Z\n");
	strcat (strg, strg2);
	break;
	
    case STATE_U:
	sprintf (strg2, "= U\n");
	strcat (strg, strg2);
	break;
	
    case STATE_B32:
    case STATE_B128:
	strcat (strg, "= ");
	value_to_string (trace, strg2, value, ' ');
	strcat (strg, strg2);
	if ( (sig_ptr->decode != NULL) 
	     && (cptr->sttime.state == STATE_B32)
	     && (value[0] < sig_ptr->decode->numstates)
	     && (sig_ptr->decode->statename[value[0]][0] != '\0') ) {
	    sprintf (strg2, " = %s\n", sig_ptr->decode->statename[value[0]] );
	    strcat (strg, strg2);
	}
	else strcat (strg, "\n");
	
	/* Bitwise information */
	rows = ceil (sqrt ((double)(sig_ptr->bits + 1)));
	cols = ceil ((double)(rows) / 4.0) * 4;
	rows = ceil ((double)(sig_ptr->bits + 1)/ (double)cols);
	
	format = "<%01d>=%d ";
	if (sig_ptr->bits >= 10)  format = "<%02d>=%d ";
	if (sig_ptr->bits >= 100) format = "<%03d>=%d ";
	
	bit = 0;
	for (row=rows - 1; row >= 0; row--) {
	    for (col = cols - 1; col >= 0; col--) {
		bit = (row * cols + col);
		
		if (bit<32) bit_value = ( value[0] >> bit ) & 1;
		else if (bit<64) bit_value = ( value[1] >> (bit-32) ) & 1;
		else if (bit<96) bit_value = ( value[2] >> (bit-64) ) & 1;
		else  bit_value = ( value[3] >> (bit-96) ) & 1;
		
		if ((bit>=0) && (bit <= sig_ptr->bits)) {
		    sprintf (strg2, format, sig_ptr->msb_index +
			     ((sig_ptr->msb_index >= sig_ptr->lsb_index)
			      ? (bit - sig_ptr->bits) : (sig_ptr->bits - bit)),
			     bit_value);
		    strcat (strg, strg2);
		    if (col==4 || col==8) strcat (strg, "  ");
		}
	    }
	    strcat (strg, "\n");
	}
	
	if (sig_ptr->bits > 2) {
	    par = 0;
	    for (bit=0; bit<=sig_ptr->bits; bit++) {
		if (bit<32) bit_value = ( value[0] >> bit ) & 1;
		else if (bit<64) bit_value = ( value[1] >> (bit-32) ) & 1;
		else if (bit<96) bit_value = ( value[2] >> (bit-64) ) & 1;
		else  bit_value = ( value[3] >> (bit-96) ) & 1;
		
		par ^= bit_value;
	    }
	    if (par) strcat (strg, "Odd Parity\n");
	    else  strcat (strg, "Even Parity\n");
	}
	
	break;
    } /* Case */
    return (strg);
}
	
void    val_examine_popup (
    /* Create the popup menu for val_examine, based on cursor position x,y */
    Trace	*trace,
    XButtonPressedEvent	*ev)
{
    DTime	time;
    Signal	*sig_ptr;
    char	*strg = "No information here";
    XmString	xs;
    
    time = posx_to_time (trace, ev->x);
    sig_ptr = posy_to_signal (trace, ev->y);
    if (DTPRINT_ENTRY) printf ("In val_examine_popup %d\n", __LINE__);
    
    if (trace->examine.popup && XtIsManaged(trace->examine.popup)) {
	XtUnmanageChild (trace->examine.popup);
	XtDestroyWidget (trace->examine.popup);
	trace->examine.popup = NULL;
    }
      
    if (sig_ptr) {
	/* Get information */
	if (time>=0) {
	    strg = val_examine_popup_cptr_string (trace, sig_ptr, time);
	} else {
	    strg = val_examine_popup_sig_string (trace, sig_ptr);
	}
    }
	
    XtSetArg (arglist[0], XmNentryAlignment, XmALIGNMENT_BEGINNING);
    trace->examine.popup = XmCreatePopupMenu (trace->main, "examinepopup", arglist, 1);
  
    xs = string_create_with_cr (strg);
    XtSetArg (arglist[0], XmNlabelString, xs);
    trace->examine.label = XmCreateLabel (trace->examine.popup,"popuplabel",arglist,1);
    XtManageChild (trace->examine.label);
    XmStringFree (xs);

    XmMenuPosition (trace->examine.popup, ev);
    XtManageChild (trace->examine.popup);

    /* We definately shouldn't have to force exposure of the popup. */
    /* However, the reality is lessTif doesn't seem to draw the text unless we do */
    /* This is a unknown bug in lessTif; it works fine on the true Motif */
    (xmRowColumnClassRec.core_class.expose) (trace->examine.popup, (XEvent*) ev, NULL);
    (xmLabelClassRec.core_class.expose) (trace->examine.label, (XEvent*) ev, NULL);
}
	
void    val_examine_unpopup_act (
    /* callback or action! */
    Widget		w)
{
    Trace	*trace;		/* Display information */
    
    if (DTPRINT_ENTRY) printf ("In val_examine_unpopup_act\n");
    
    if (!(trace = widget_to_trace (w))) return;

    /* unmanage popup */
    if (trace->examine.popup) {
	XtUnmanageChild (trace->examine.popup);
	XtUnmanageChild (trace->examine.label);
	trace->examine.popup = NULL;
    }
    /* redraw the screen as popup may have mangled widgets */
    /* draw_all_needed ();*/
}

char *events[40] = {"","", "KeyPress", "KeyRelease", "ButtonPress", "ButtonRelease", "MotionNotify",
			"EnterNotify", "LeaveNotify", "FocusIn", "FocusOut", "KeymapNotify", "Expose", "GraphicsExpose",
			"NoExpose", "VisibilityNotify", "CreateNotify", "DestroyNotify", "UnmapNotify", "MapNotify",
			"MapRequest", "ReparentNotify", "ConfigureNotify", "ConfigureRequest", "GravityNotify",
			"ResizeRequest", "CirculateNotify", "CirculateRequest", "PropertyNotify", "SelectionClear",
			"SelectionRequest", "SelectionNotify", "ColormapNotify", "ClientMessage", "MappingNotify",
		    "LASTEvent"};

void    val_examine_ev (
    Widget		w,
    Trace		*trace,
    XButtonPressedEvent	*ev)
{
    XEvent	event;
    XButtonPressedEvent *em;
    int		update_pending = FALSE;
    
    if (DTPRINT_ENTRY) printf ("In val_examine_ev, button=%d state=%d\n", ev->button, ev->state);
    if (ev->type != ButtonPress) return;	/* Used for both button 1 & 2. */
    
    /* not sure why this has to be done but it must be done */
    XSync (global->display, FALSE);
    XUngrabPointer (XtDisplay (trace->work),CurrentTime);

    /* select the events the widget will respond to */
    XSelectInput (XtDisplay (trace->work),XtWindow (trace->work),
		 ButtonReleaseMask|PointerMotionMask|StructureNotifyMask|ExposureMask);
    
    /* Create */
    val_examine_popup (trace, ev);
    
    /* loop and service events until button is released */
    while ( 1 ) {
	/* wait for next event */
	XNextEvent (global->display, &event);
	
	/* Mark an update as needed */
	if (event.type == MotionNotify) {
	    update_pending = TRUE;
	}

	/* if window was exposed, must redraw it */
	if (event.type == Expose) win_expose_cb (0,trace);
	/* if window was resized, must redraw it */
	if (event.type == ConfigureNotify) win_resize_cb (0,trace);
	
	/* button released - calculate cursor position and leave the loop */
	if (event.type == ButtonRelease || event.type == ButtonPress) {
	    /* ButtonPress in case user is freaking out, some strange X behavior caused the ButtonRelease to be lost */
	    break;
	}

	/* If a update is needed, redraw the menu. */
	/* Do it later if events pending, otherwise dragging is SLOWWWW */
	if (update_pending && !XPending (global->display)) {
	    update_pending = FALSE;
	    em = (XButtonPressedEvent *)&event;
	    val_examine_popup (trace, em);
	}

	if (global->redraw_needed && !XtAppPending (global->appcontext)) {
	    draw_perform();
	}
    }
    
    /* reset the events the widget will respond to */
    XSync (global->display, FALSE);
    XSelectInput (XtDisplay (trace->work),XtWindow (trace->work),
		 ButtonPressMask|StructureNotifyMask|ExposureMask);
    
    /* unmanage popup */
    val_examine_unpopup_act (trace->work);
    draw_needed (trace);
}

void    val_examine_popup_act (
    Widget		w,
    XButtonPressedEvent	*ev,
    String		params,
    Cardinal		*num_params)
{
    Trace	*trace;		/* Display information */
    int		prev_cursor;
    
    if (ev->type != ButtonPress) return;

    if (DTPRINT_ENTRY) printf ("In val_examine_popup_ev, button=%d state=%d\n", ev->button, ev->state);

    if (!(trace = widget_to_trace (w))) return;

    /* Create */
    prev_cursor = last_set_cursor ();
    set_cursor (trace, DC_VAL_EXAM);

    val_examine_ev (w, trace, ev);

    set_cursor (trace, prev_cursor);
}

void    val_search_widget_update (
    Trace	*trace)
{
    VSearchNum search_pos;
    char	strg[MAXVALUELEN];

    /* Copy settings to local area to allow cancel to work */
    for (search_pos=0; search_pos<MAX_SRCH; search_pos++) {
	/* Update with current search enables */
	XtSetArg (arglist[0], XmNset, (global->val_srch[search_pos].color != 0));
	XtSetValues (trace->value.enable[search_pos], arglist, 1);

	/* Update with current cursor enables */
	XtSetArg (arglist[0], XmNset, (global->val_srch[search_pos].cursor != 0));
	XtSetValues (trace->value.cursor[search_pos], arglist, 1);

	/* Update with current search values */
	value_to_string (trace, strg, global->val_srch[search_pos].value, ' ');
	XmTextSetString (trace->value.text[search_pos], strg);

	/* Update with current signal values */
	XmTextSetString (trace->value.signal[search_pos], global->val_srch[search_pos].signal);
    }
}

void    val_search_cb (
    Widget	w)
{
    Trace *trace = widget_to_trace(w);
    int		i;
    
    if (DTPRINT_ENTRY) printf ("In val_search_cb - trace=%p\n",trace);
    
    if (!trace->value.search) {
	XtSetArg (arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg (arglist[1], XmNdialogTitle, XmStringCreateSimple ("Value Search Requester") );
	/* XtSetArg (arglist[2], XmNwidth, 500);
	   XtSetArg (arglist[3], XmNheight, 400); */
	trace->value.search = XmCreateBulletinBoardDialog (trace->work,"search",arglist,2);

	trace->value.form = XmCreateForm (trace->value.search, "form", arglist, 0);
	DManageChild (trace->value.form, trace, MC_NOKEYS);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Color"));
	XtSetArg (arglist[1], XmNx, 5);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_FORM );
	XtSetArg (arglist[3], XmNtopOffset, 5);
	trace->value.label1 = XmCreateLabel (trace->value.form,"label1",arglist,4);
	DManageChild (trace->value.label1, trace, MC_NOKEYS);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Place"));
	XtSetArg (arglist[1], XmNx, 60);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_FORM );
	XtSetArg (arglist[3], XmNtopOffset, 5);
	trace->value.label2 = XmCreateLabel (trace->value.form,"label2",arglist,4);
	DManageChild (trace->value.label2, trace, MC_NOKEYS);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Value"));
	XtSetArg (arglist[1], XmNx, 5);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopOffset, 1);
	XtSetArg (arglist[4], XmNtopWidget, trace->value.label1);
	trace->value.label4 = XmCreateLabel (trace->value.form,"label4",arglist,5);
	DManageChild (trace->value.label4, trace, MC_NOKEYS);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Cursor"));
	XtSetArg (arglist[1], XmNx, 60);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopOffset, 1);
	XtSetArg (arglist[4], XmNtopWidget, trace->value.label2);
	trace->value.label5 = XmCreateLabel (trace->value.form,"label5",arglist,5);
	DManageChild (trace->value.label5, trace, MC_NOKEYS);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple
		 ( (trace->busrep == BUSREP_HEX_UN)? "Search value in HEX":
		  ( (trace->busrep == BUSREP_OCT_UN) ? "Search value in OCTAL" : "Search value in DECIMAL" ) ) );
	XtSetArg (arglist[1], XmNx, 140);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopOffset, 1);
	XtSetArg (arglist[4], XmNtopWidget, trace->value.label1);
	trace->value.label3 = XmCreateLabel (trace->value.form,"label3",arglist,5);
	DManageChild (trace->value.label3, trace, MC_NOKEYS);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Signal Wildcard"));
	XtSetArg (arglist[1], XmNx, 500);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopOffset, 1);
	XtSetArg (arglist[4], XmNtopWidget, trace->value.label1);
	trace->value.label6 = XmCreateLabel (trace->value.form,"label6",arglist,5);
	DManageChild (trace->value.label6, trace, MC_NOKEYS);
	
	for (i=0; i<MAX_SRCH; i++) {
	    /* enable button */
	    XtSetArg (arglist[0], XmNx, 15);
	    XtSetArg (arglist[1], XmNselectColor, trace->xcolornums[i+1]);
	    XtSetArg (arglist[2], XmNlabelString, XmStringCreateSimple (""));
	    XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[4], XmNtopOffset, 5);
	    XtSetArg (arglist[5], XmNtopWidget, ((i==0)?(trace->value.label4):(trace->value.signal[i-1])));
	    trace->value.enable[i] = XmCreateToggleButton (trace->value.form,"togglen",arglist,6);
	    DManageChild (trace->value.enable[i], trace, MC_NOKEYS);

	    /* enable button */
	    XtSetArg (arglist[0], XmNselectColor, trace->xcolornums[i+1]);
	    XtSetArg (arglist[1], XmNlabelString, XmStringCreateSimple (""));
	    XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[3], XmNtopOffset, 5);
	    XtSetArg (arglist[4], XmNtopWidget, ((i==0)?(trace->value.label4):(trace->value.signal[i-1])));
	    XtSetArg (arglist[5], XmNleftAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[6], XmNleftOffset, 20);
	    XtSetArg (arglist[7], XmNleftWidget, trace->value.enable[i]);
	    trace->value.cursor[i] = XmCreateToggleButton (trace->value.form,"cursorn",arglist,9);
	    DManageChild (trace->value.cursor[i], trace, MC_NOKEYS);

	    /* create the file name text widget */
	    XtSetArg (arglist[0], XmNrows, 1);
	    XtSetArg (arglist[1], XmNcolumns, MAXVALUELEN);
	    XtSetArg (arglist[2], XmNresizeHeight, FALSE);
	    XtSetArg (arglist[3], XmNeditMode, XmSINGLE_LINE_EDIT);
	    XtSetArg (arglist[4], XmNtopAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[5], XmNtopOffset, 5);
	    XtSetArg (arglist[6], XmNtopWidget, ((i==0)?(trace->value.label4):(trace->value.signal[i-1])));
	    XtSetArg (arglist[7], XmNleftAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[8], XmNleftOffset, 20);
	    XtSetArg (arglist[9], XmNleftWidget, trace->value.cursor[i]);
	    trace->value.text[i] = XmCreateText (trace->value.form,"textn",arglist,10);
	    DAddCallback (trace->value.text[i], XmNactivateCallback, val_search_ok_cb, trace);
	    DManageChild (trace->value.text[i], trace, MC_NOKEYS);
	    
	    /* create the signal text widget */
	    XtSetArg (arglist[0], XmNrows, 1);
	    XtSetArg (arglist[1], XmNcolumns, 30);
	    XtSetArg (arglist[2], XmNresizeHeight, FALSE);
	    XtSetArg (arglist[3], XmNeditMode, XmSINGLE_LINE_EDIT);
	    XtSetArg (arglist[4], XmNtopAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[5], XmNtopOffset, 5);
	    XtSetArg (arglist[6], XmNtopWidget, ((i==0)?(trace->value.label4):(trace->value.signal[i-1])));
	    XtSetArg (arglist[7], XmNleftAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[8], XmNleftOffset, 20);
	    XtSetArg (arglist[9], XmNleftWidget, trace->value.text[i]);
	    trace->value.signal[i] = XmCreateText (trace->value.form,"texts",arglist,10);
	    DAddCallback (trace->value.signal[i], XmNactivateCallback, val_search_ok_cb, trace);
	    DManageChild (trace->value.signal[i], trace, MC_NOKEYS);
	}

	/* Create OK button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple (" OK ") );
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopOffset, 5);
	XtSetArg (arglist[4], XmNtopWidget, trace->value.signal[MAX_SRCH-1]);
	trace->value.ok = XmCreatePushButton (trace->value.form,"ok",arglist,5);
	DAddCallback (trace->value.ok, XmNactivateCallback, val_search_ok_cb, trace);
	DManageChild (trace->value.ok, trace, MC_NOKEYS);
	
	/* create apply button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Apply") );
	XtSetArg (arglist[1], XmNx, 70);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopOffset, 5);
	XtSetArg (arglist[4], XmNtopWidget, trace->value.signal[MAX_SRCH-1]);
	trace->value.apply = XmCreatePushButton (trace->value.form,"apply",arglist,5);
	DAddCallback (trace->value.apply, XmNactivateCallback, val_search_apply_cb, trace);
	DManageChild (trace->value.apply, trace, MC_NOKEYS);
	
	/* create cancel button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Cancel") );
	XtSetArg (arglist[1], XmNx, 140);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopOffset, 5);
	XtSetArg (arglist[4], XmNtopWidget, trace->value.signal[MAX_SRCH-1]);
	trace->value.cancel = XmCreatePushButton (trace->value.form,"cancel",arglist,5);
	DAddCallback (trace->value.cancel, XmNactivateCallback, unmanage_cb, trace->value.search);
	DManageChild (trace->value.cancel, trace, MC_NOKEYS);
    }
    
    /* Copy settings to local area to allow cancel to work */
    val_search_widget_update (trace);

    /* manage the popup on the screen */
    DManageChild (trace->value.search, trace, MC_NOKEYS);
}

void    val_search_ok_cb (
    Widget	w,
    Trace	*trace,
    XmSelectionBoxCallbackStruct *cb)
{
    char		*strg;
    int			i;

    if (DTPRINT_ENTRY) printf ("In val_search_ok_cb - trace=%p\n",trace);

    for (i=0; i<MAX_SRCH; i++) {
	/* Update with current search enables */
	global->val_srch[i].color = (XmToggleButtonGetState (trace->value.enable[i])) ? i+1 : 0;
	
	/* Update with current cursor enables */
	global->val_srch[i].cursor = (XmToggleButtonGetState (trace->value.cursor[i])) ? i+1 : 0;
	
	/* Update with current search values */
	strg = XmTextGetString (trace->value.text[i]);
	string_to_value (trace, strg, global->val_srch[i].value);

	/* Update with current search values */
	strg = XmTextGetString (trace->value.signal[i]);
	strcpy (global->val_srch[i].signal, strg);

	if (DTPRINT_SEARCH) {
	    char strg2[MAXVALUELEN];
	    value_to_string (trace, strg2, global->val_srch[i].value, '_');
	    printf ("Search %d) %d   '%s' -> '%s'\n", i, global->val_srch[i].color, strg, strg2);
	}
    }
    
    XtUnmanageChild (trace->value.search);

    draw_needupd_val_search ();
    draw_all_needed ();
}

void    val_search_apply_cb (
    Widget	w,
    Trace	*trace,
    XmSelectionBoxCallbackStruct *cb)
{
    if (DTPRINT_ENTRY) printf ("In val_search_apply_cb - trace=%p\n",trace);

    val_search_ok_cb (w,trace,cb);
    val_search_cb (trace->main);
}

void    val_highlight_ev (
    Widget	w,
    Trace	*trace,
    XButtonPressedEvent	*ev)
{
    DTime	time;
    Signal	*sig_ptr;
    SignalLW	*cptr;
    VSearchNum	search_pos;
    
    if (DTPRINT_ENTRY) printf ("In val_highlight_ev - trace=%p\n",trace);
    if (ev->type != ButtonPress || ev->button!=1) return;
    
    time = posx_to_time (trace, ev->x);
    sig_ptr = posy_to_signal (trace, ev->y);
    if (!sig_ptr && time<=0) return;
    cptr = cptr_at_time (sig_ptr, time);
    if (!cptr) return;
    
    /* Change the color */
    if (global->highlight_color > 0) {
	search_pos = global->highlight_color - 1;
	cptr_to_search_value (cptr, global->val_srch[search_pos].value);
	strcpy (global->val_srch[search_pos].signal, sig_ptr->signame);
	if (!global->val_srch[search_pos].color
	    && !global->val_srch[search_pos].cursor ) {
	    /* presume user really wants color if neither is on */
	    global->val_srch[search_pos].color = search_pos + 1;
	}
    }

    /* If search requester is on the screen, update it too */
    if (trace->value.search && XtIsManaged (trace->value.search)) {
	val_search_widget_update (trace);
    }

    /* redraw the screen */
    draw_needupd_val_search ();
    draw_all_needed ();
}


/****************************** ANNOTATION ******************************/

void    val_annotate_cb (
    Widget	w)
{
    Trace *trace = widget_to_trace(w);
    int i;

    if (DTPRINT_ENTRY) printf ("In val_annotate_cb - trace=%p\n",trace);
    
    if (!trace->annotate.dialog) {
	XtSetArg (arglist[0],XmNdefaultPosition, TRUE);
	XtSetArg (arglist[1],XmNdialogTitle, XmStringCreateSimple ("Annotate Menu"));
	trace->annotate.dialog = XmCreateBulletinBoardDialog (trace->work, "annotate",arglist,2);
	
	/* create label widget for text widget */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("File Name") );
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNy, 3);
	trace->annotate.label1 = XmCreateLabel (trace->annotate.dialog,"",arglist,3);
	DManageChild (trace->annotate.label1, trace, MC_NOKEYS);
	
	/* create the file name text widget */
	XtSetArg (arglist[0], XmNrows, 1);
	XtSetArg (arglist[1], XmNcolumns, 30);
	XtSetArg (arglist[2], XmNx, 10);
	XtSetArg (arglist[3], XmNy, 35);
	XtSetArg (arglist[4], XmNresizeHeight, FALSE);
	XtSetArg (arglist[5], XmNeditMode, XmSINGLE_LINE_EDIT);
	trace->annotate.text = XmCreateText (trace->annotate.dialog,"",arglist,6);
	DManageChild (trace->annotate.text, trace, MC_NOKEYS);
	DAddCallback (trace->annotate.text, XmNactivateCallback, val_annotate_ok_cb, trace);
	
	/* Cursor enables */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Include which user (solid) cursor colors:") );
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNy, 75);
	trace->annotate.label2 = XmCreateLabel (trace->annotate.dialog,"",arglist,3);
	DManageChild (trace->annotate.label2, trace, MC_NOKEYS);
	
	for (i=0; i<=MAX_SRCH; i++) {
	    /* enable button */
	    XtSetArg (arglist[0], XmNx, 15+30*i);
	    XtSetArg (arglist[1], XmNy, 100);
	    XtSetArg (arglist[2], XmNselectColor, trace->xcolornums[i]);
	    XtSetArg (arglist[3], XmNlabelString, XmStringCreateSimple (""));
	    trace->annotate.cursors[i] = XmCreateToggleButton (trace->annotate.dialog,"togglenc",arglist,4);
	    DManageChild (trace->annotate.cursors[i], trace, MC_NOKEYS);
	}

	/* Cursor enables */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Include which auto (dotted) cursor colors:") );
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNy, 145);
	trace->annotate.label4 = XmCreateLabel (trace->annotate.dialog,"",arglist,3);
	DManageChild (trace->annotate.label4, trace, MC_NOKEYS);
	
	for (i=0; i<=MAX_SRCH; i++) {
	    /* enable button */
	    XtSetArg (arglist[0], XmNx, 15+30*i);
	    XtSetArg (arglist[1], XmNy, 170);
	    XtSetArg (arglist[2], XmNselectColor, trace->xcolornums[i]);
	    XtSetArg (arglist[3], XmNlabelString, XmStringCreateSimple (""));
	    trace->annotate.cursors_dotted[i] = XmCreateToggleButton (trace->annotate.dialog,"togglencd",arglist,4);
	    DManageChild (trace->annotate.cursors_dotted[i], trace, MC_NOKEYS);
	}

	/* Signal Enables */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Include which signal colors:") );
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNy, 215);
	trace->annotate.label3 = XmCreateLabel (trace->annotate.dialog,"",arglist,3);
	DManageChild (trace->annotate.label3, trace, MC_NOKEYS);
	
	for (i=1; i<=MAX_SRCH; i++) {
	    /* enable button */
	    XtSetArg (arglist[0], XmNx, 15+30*i);
	    XtSetArg (arglist[1], XmNy, 240);
	    XtSetArg (arglist[2], XmNselectColor, trace->xcolornums[i]);
	    XtSetArg (arglist[3], XmNlabelString, XmStringCreateSimple (""));
	    trace->annotate.signals[i] = XmCreateToggleButton (trace->annotate.dialog,"togglen",arglist,4);
	    DManageChild (trace->annotate.signals[i], trace, MC_NOKEYS);
	}

	/* Create OK button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple (" OK ") );
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNy, 280);
	trace->annotate.ok = XmCreatePushButton (trace->annotate.dialog,"ok",arglist,3);
	DAddCallback (trace->annotate.ok, XmNactivateCallback, val_annotate_ok_cb, trace);
	DManageChild (trace->annotate.ok, trace, MC_NOKEYS);
	
	/* create apply button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Apply") );
	XtSetArg (arglist[1], XmNx, 70);
	XtSetArg (arglist[2], XmNy, 280);
	trace->annotate.apply = XmCreatePushButton (trace->annotate.dialog,"apply",arglist,3);
	DAddCallback (trace->annotate.apply, XmNactivateCallback, val_annotate_apply_cb, trace);
	DManageChild (trace->annotate.apply, trace, MC_NOKEYS);
	
	/* create cancel button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Cancel") );
	XtSetArg (arglist[1], XmNx, 140);
	XtSetArg (arglist[2], XmNy, 280);
	trace->annotate.cancel = XmCreatePushButton (trace->annotate.dialog,"cancel",arglist,3);
	DAddCallback (trace->annotate.cancel, XmNactivateCallback, unmanage_cb, trace->annotate.dialog);

	DManageChild (trace->annotate.cancel, trace, MC_NOKEYS);
    }
    
    /* reset file name */
    XtSetArg (arglist[0], XmNvalue, global->anno_filename);
    XtSetValues (trace->annotate.text,arglist,1);
    
    /* reset enables */
    for (i=0; i<=MAX_SRCH; i++) {
	XtSetArg (arglist[0], XmNset, (global->anno_ena_cursor[i] != 0));
	XtSetValues (trace->annotate.cursors[i], arglist, 1);

	XtSetArg (arglist[0], XmNset, (global->anno_ena_cursor_dotted[i] != 0));
	XtSetValues (trace->annotate.cursors_dotted[i], arglist, 1);
    }
    for (i=1; i<=MAX_SRCH; i++) {
	XtSetArg (arglist[0], XmNset, (global->anno_ena_signal[i] != 0));
	XtSetValues (trace->annotate.signals[i], arglist, 1);
    }

    /* manage the popup on the screen */
    DManageChild (trace->annotate.dialog, trace, MC_NOKEYS);
    global->anno_poppedup = TRUE;
}

void    val_annotate_ok_cb (
    Widget	w,
    Trace	*trace,
    XmAnyCallbackStruct *cb)
{
    char		*strg;
    int			i;

    if (DTPRINT_ENTRY) printf ("In sig_search_ok_cb - trace=%p\n",trace);

    /* Update with current search enables */
    for (i=0; i<=MAX_SRCH; i++) {
	global->anno_ena_cursor[i] = XmToggleButtonGetState (trace->annotate.cursors[i]);
	global->anno_ena_cursor_dotted[i] = XmToggleButtonGetState (trace->annotate.cursors_dotted[i]);
    }
    for (i=1; i<=MAX_SRCH; i++) {
	global->anno_ena_signal[i] = XmToggleButtonGetState (trace->annotate.signals[i]);
    }
	
    /* Update with current search values */
    strg = XmTextGetString (trace->annotate.text);
    strcpy (global->anno_filename, strg);

    XtUnmanageChild (trace->annotate.dialog);

    val_annotate_do_cb (w,trace,cb);
}

void    val_annotate_apply_cb (
    Widget	w,
    Trace	*trace,
    XmAnyCallbackStruct	*cb)
{
    if (DTPRINT_ENTRY) printf ("In sig_search_apply_cb - trace=%p\n",trace);

    val_annotate_ok_cb (w,trace,cb);
    val_annotate_cb (trace->main);
}

void    val_annotate_do_cb (
    Widget	w,
    Trace	*trace,
    XmAnyCallbackStruct	*cb)
{
    int		i;
    Signal	*sig_ptr;
    SignalLW	*cptr;
    FILE	*dump_fp;
    DCursor 	*csr_ptr;		/* Current cursor being printed */
    char	strg[1000];
    int		csr_num, csr_num_incl;
    int		value[4];
    
    /* Initialize requestor before first usage? */
    /*
    if (! global->anno_poppedup) {
	val_annotate_cb (w,trace,cb);
	}*/

    if (DTPRINT_ENTRY) printf ("In val_annotate_cb - trace=%p  file=%s\n",trace,global->anno_filename);

    draw_update ();

    /* Socket connection */
#if HAVE_SOCKETS
    socket_create ();
#endif

    if (! (dump_fp=fopen (global->anno_filename, "w"))) {
	sprintf (message,"Bad Filename: %s\n", global->anno_filename);
	dino_error_ack (trace,message);
	return;
    }
	
    /* Socket info */
    if (!global->anno_socket[0] || strchr (global->anno_socket, '*') ) {
	fprintf (dump_fp, "(setq dinotrace-socket-name nil)\n");
    }
    else {
	fprintf (dump_fp, "(setq dinotrace-socket-name \"%s\")\n\n",
		 global->anno_socket);
    }

    /* Trace info */
    fprintf (dump_fp, "(setq dinotrace-traces '(\n");
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	if (trace->loaded) {
	    fprintf (dump_fp, "	[\"%s\"\t\"%s\"]\n",
		     trace->filename,
		     date_string (trace->filestat.st_ctime));
	}
    }
    fprintf (dump_fp, "\t))\n");

#define colornum_to_name(_color_)  (((_color_)==0)?"":global->color_names[(_color_)])
    /* Cursor info */
    fprintf (dump_fp, "(setq dinotrace-cursors [\n");
    for (csr_ptr = global->cursor_head; csr_ptr; csr_ptr = csr_ptr->next) {
	if ((csr_ptr->type==USER) ? global->anno_ena_cursor[csr_ptr->color] : global->anno_ena_cursor_dotted[csr_ptr->color] ) {
	    time_to_string (global->trace_head, strg, csr_ptr->time, FALSE);
	    fprintf (dump_fp, "\t[\"%s\"\t%d\t\"%s\"\tnil]\n", strg,
		     csr_ptr->color, colornum_to_name(csr_ptr->color));
	}
    }
    fprintf (dump_fp, "\t])\n");

    /* Value search info */
    fprintf (dump_fp, "(setq dinotrace-value-searches '(\n");
    for (i=1; i<=MAX_SRCH; i++) {
	if (global->val_srch[i-1].color) {
	    value_to_string (global->trace_head, strg, global->val_srch[i-1].value, '_');
	    fprintf (dump_fp, "\t[\"%s\"\t%d\t\"%s\"]\n", strg,
		     i, colornum_to_name(i));
	}
    }
    fprintf (dump_fp, "\t))\n");

    /* Signal color info */
    /* 0's never actually used, but needed in the array so aref will work in emacs */
    fprintf (dump_fp, "(setq dinotrace-signal-colors [\n");
    for (i=0; i<=MAX_SRCH; i++) {
	fprintf (dump_fp, "\t[%d\t\"%s\"\tnil]\n", i, (i==0 || !global->color_names[i])?"":global->color_names[i]);
    }
    fprintf (dump_fp, "\t])\n");

    /* Find number of cursors that will be included */
    csr_num_incl = 0;
    for (csr_ptr = global->cursor_head; csr_ptr; csr_ptr = csr_ptr->next) {
	if ((csr_ptr->type==USER) ? global->anno_ena_cursor[csr_ptr->color] : global->anno_ena_cursor_dotted[csr_ptr->color] ) {
	    csr_num_incl++;
	}
    }

    /* Signal values */
    fprintf (dump_fp, "(setq dinotrace-signal-values '(\n");
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	    fprintf (dump_fp, "\t(\"%s\"\t", sig_ptr->signame);
	    if (global->anno_ena_signal[sig_ptr->color]) fprintf (dump_fp, "%d\t(", sig_ptr->color);
	    else     fprintf (dump_fp, "nil\t(");
	    cptr = (SignalLW *)sig_ptr->cptr;

	    csr_num=0;
	    for (csr_ptr = global->cursor_head; csr_ptr; csr_ptr = csr_ptr->next) {

		if ((csr_ptr->type==USER) ? global->anno_ena_cursor[csr_ptr->color] : global->anno_ena_cursor_dotted[csr_ptr->color] ) {
		    csr_num++;

		    /* Note grabs value to right of cursor */
		    while ( (cptr->sttime.time <= csr_ptr->time)
			   && (cptr->sttime.time != EOT)) {
			cptr += sig_ptr->lws;
		    }
		    if ( (cptr->sttime.time > csr_ptr->time)
			&& ( cptr != (SignalLW *)sig_ptr->cptr)) {
			cptr -= sig_ptr->lws;
		    }

		    cptr_to_search_value (cptr, value);
		    value_to_string (trace, strg, value, '_');
		    
		    /* First value must have `, last must have ', commas in middle */
		    if (csr_num==1 && csr_num==csr_num_incl)
			fprintf (dump_fp, "\"`%s'\"\t", strg);
		    else if (csr_num==1)
			fprintf (dump_fp, "\"`%s\"\t", strg);
		    else if (csr_num==csr_num_incl)
			fprintf (dump_fp, "\",%s'\"\t", strg);
		    else
			fprintf (dump_fp, "\",%s\"\t", strg);
		}
	    }
	    fprintf (dump_fp, "))\n");
	}
    }
    fprintf (dump_fp, "\t))\n");
    
    fclose (dump_fp);
}

