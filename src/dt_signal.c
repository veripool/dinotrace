#ident "$Id$"
/******************************************************************************
 * dt_signal.c --- signal handling, searching, etc
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

#include <assert.h>

#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/SelectioB.h>
#include <Xm/Text.h>
#include <Xm/List.h>
#include <Xm/BulletinB.h>
#include <Xm/Label.h>
#include <Xm/Separator.h>
#include <Xm/Form.h>

#include "functions.h"

/**********************************************************************/

extern void 
    sig_sel_ok_cb(), sig_sel_apply_cb(),
    sig_add_sel_cb(), sig_sel_pattern_cb(),
    sig_sel_add_all_cb(), sig_sel_add_list_cb(),
    sig_sel_del_all_cb(), sig_sel_del_list_cb(), sig_sel_del_const_cb();


/****************************** UTILITIES ******************************/

void sig_free (
    /* Free a signal structure, and unlink all traces of it */
    Trace	*trace,
    Signal	*sig_ptr,	/* Pointer to signal to be deleted */
    Boolean_t	select,		/* True = selectively pick trace's signals from the list */
    Boolean_t	recursive)	/* True = recursively do the entire list */
{
    Signal	*del_sig_ptr;
    Trace	*trace_ptr;

    /* loop and free signal data and each signal structure */
    while (sig_ptr) {
	if (!select || sig_ptr->trace == trace) {
	    /* Check head pointers, Including deleted */
	    for (trace_ptr = global->deleted_trace_head; trace_ptr; trace_ptr = trace_ptr->next_trace) {
		if ( sig_ptr == trace_ptr->dispsig )
		    trace_ptr->dispsig = sig_ptr->forward;
		if ( sig_ptr == trace_ptr->firstsig )
		    trace_ptr->firstsig = sig_ptr->forward;
	    }

	    /* free the signal data */
	    del_sig_ptr = sig_ptr;

	    if (sig_ptr->forward)
		((Signal *)(sig_ptr->forward))->backward = sig_ptr->backward;
	    if (sig_ptr->backward)
		((Signal *)(sig_ptr->backward))->forward = sig_ptr->forward;
	    sig_ptr = sig_ptr->forward;
	
	    /* free the signal structure */
	    if (del_sig_ptr->copyof == NULL) {
		DFree (del_sig_ptr->bptr);
		DFree (del_sig_ptr->signame);
		DFree (del_sig_ptr->xsigname);
	    }
	    DFree (del_sig_ptr);
	}
	else {
	    sig_ptr = sig_ptr->forward;
	}
	if (!recursive) sig_ptr=NULL;
    }
}


void    remove_signal_from_queue (
    Trace	*trace,
    Signal	*sig_ptr)	/* Signal to remove */
    /* Removes the signal from the current and any other ques that it is in */
{
    Signal	*next_sig_ptr, *prev_sig_ptr;
    
    if (DTPRINT_ENTRY) printf ("In remove_signal_from_queue - trace=%p sig %p\n",trace,sig_ptr);
    
    /* redirect the forward pointer */
    prev_sig_ptr = sig_ptr->backward;
    if (prev_sig_ptr) {
	prev_sig_ptr->forward = sig_ptr->forward;
    }
    
    /* if not the last signal redirect the backward pointer */
    next_sig_ptr = sig_ptr->forward;
    if ( next_sig_ptr != NULL ) {
	next_sig_ptr->backward = sig_ptr->backward;
    }

    /* if the signal is the first screen signal, change it */
    if ( sig_ptr == trace->dispsig ) {
        trace->dispsig = sig_ptr->forward;
    }
    /* if the signal is the first signal, change it */
    if ( sig_ptr == trace->firstsig ) {
	trace->firstsig = sig_ptr->forward;
    }

    trace->numsig--;
}

#define ADD_LAST ((Signal *)(-1))
void    add_signal_to_queue (
    Trace	*trace,
    Signal	*sig_ptr,	/* Signal to add */
    Signal	*loc_sig_ptr)	/* Pointer to signal ahead of one to add, NULL=1st, ADD_LAST=last */
{
    Signal	*next_sig_ptr, *prev_sig_ptr;
    
    if (DTPRINT_ENTRY) printf ("In add_signal_to_queue - trace=%p   loc=%p%s\n",trace,
			       loc_sig_ptr, loc_sig_ptr==ADD_LAST?"=last":"");
    
    if (sig_ptr==loc_sig_ptr) {
	loc_sig_ptr = loc_sig_ptr->forward;
    }

    /*printf ("iPrev %d  next %d  sig %d  top %d\n", prev_sig_ptr, next_sig_ptr, sig_ptr, *top_pptr);*/

    /* Insert into first position? */
    if (loc_sig_ptr == NULL) {
	next_sig_ptr = trace->firstsig;
	trace->firstsig = sig_ptr;
	prev_sig_ptr = (next_sig_ptr)? next_sig_ptr->backward : NULL;
    }
    else if (loc_sig_ptr == ADD_LAST) {
	prev_sig_ptr = NULL;
	for (next_sig_ptr = trace->firstsig; next_sig_ptr; next_sig_ptr = next_sig_ptr->forward) {
	    prev_sig_ptr = next_sig_ptr;
	}
	if (!prev_sig_ptr) {
	    trace->firstsig = sig_ptr;
	}
    }
    else {
	next_sig_ptr = loc_sig_ptr->forward;
	prev_sig_ptr = loc_sig_ptr;
    }

    sig_ptr->forward = next_sig_ptr;
    sig_ptr->backward = prev_sig_ptr;
    trace->numsig++;
    /* restore signal next in list */
    if (next_sig_ptr) {
	next_sig_ptr->backward = sig_ptr;
    }

    /* restore signal earlier in list */
    if (prev_sig_ptr) {
	prev_sig_ptr->forward = sig_ptr;
    }

    /* if the signal is the first screen signal, change it */
    if ( next_sig_ptr && ( next_sig_ptr == trace->dispsig )) {
        trace->dispsig = sig_ptr;
    }
    /* if the signal is the signal, change it */
    if ( next_sig_ptr && ( next_sig_ptr == trace->firstsig )) {
	trace->firstsig = sig_ptr;
    }
    /* if no display sig, but is regular first sig, make the display sig */
    if ( trace->firstsig && !trace->dispsig) {
	trace->dispsig = trace->firstsig;
    }
}

Signal *sig_replicate (
    Trace	*trace,
    Signal	*sig_ptr)	/* Signal to remove */
    /* Makes a duplicate copy of the signal */
{
    Signal	*new_sig_ptr;
    
    if (DTPRINT_ENTRY) printf ("In sig_replicate - trace=%p\n",trace);
    
    /* Create new structure */
    new_sig_ptr = XtNew (Signal);

    /* Copy data */
    memcpy (new_sig_ptr, sig_ptr, sizeof (Signal));

    /* Erase new links */
    new_sig_ptr->forward = NULL;
    new_sig_ptr->backward = NULL;
    if (sig_ptr->copyof)
	new_sig_ptr->copyof = sig_ptr->copyof;
    else new_sig_ptr->copyof = sig_ptr;

    return (new_sig_ptr);
}

/* Returns Signal or NULL if not found */
Signal *sig_find_signame (
    Trace	*trace,
    char	*signame)
{
    Signal	*sig_ptr;
    
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	if (!strcmp (sig_ptr->signame, signame)) return (sig_ptr);
    }
    return (NULL);
}


/* Returns Signal or NULL if not found */
Signal *sig_wildmat_signame (
    Trace	*trace,
    char	*signame)
{
    Signal	*sig_ptr;
    
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	if (wildmat (sig_ptr->signame, signame)) {
	    return (sig_ptr);
	}
    }
    return (NULL);
}

void	sig_wildmat_select (
    /* Create list of selected signals */
    Trace	  	*trace,		/* NULL= do all traces */
    char		*pattern)
{
    Signal		*sig_ptr;
    SignalList		*siglst_ptr;
    Boolean_t		trace_list;
    
    trace_list = (trace == NULL);

    /* Erase existing selections */
    while (global->select_head) {
	siglst_ptr = global->select_head;
	global->select_head = global->select_head->forward;
	DFree (siglst_ptr);
    }

    if (trace_list) trace = global->deleted_trace_head;
    for (; trace; trace = (trace_list ? trace->next_trace : NULL)) {
	for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	    if (wildmat (sig_ptr->signame, pattern)) {
		/* printf ("Selected: %s\n", sig_ptr->signame); */
		siglst_ptr = XtNew (SignalList);
		siglst_ptr->trace = trace;
		siglst_ptr->signal = sig_ptr;
		siglst_ptr->forward = global->select_head;
		global->select_head = siglst_ptr;
	    }
	}
    }
}

void	sig_move (
    Trace	*old_trace,
    Signal	*sig_ptr,	/* Signal to move */
    Trace	*new_trace,
    Signal	*after_sig_ptr)	/* Signal to place after or ADD_LAST */
{

    if (sig_ptr) {
	remove_signal_from_queue (old_trace, sig_ptr);
	sig_ptr->deleted = FALSE;
	add_signal_to_queue (new_trace, sig_ptr, after_sig_ptr);
    }
    
#if 0
    printf ("Adding %s\n", sig_ptr->signame);

    printf ("Post\n");
    debug_integrity_check_cb (NULL, NULL, NULL);
    printf ("Done\n");
#endif
}

void	sig_delete (
    Trace	*trace,
    Signal	*sig_ptr,	/* Signal to remove */
    Boolean_t	preserve)	/* TRUE if should preserve deletion on rereading */
    /* Delete the given signal */
{
    if (sig_ptr) {
	sig_move (trace, sig_ptr, global->deleted_trace_head, ADD_LAST);
	sig_ptr->deleted = TRUE;
	sig_ptr->deleted_preserve = preserve;
    }
}

void	sig_copy (
    Trace	*old_trace,
    Signal	*sig_ptr,	/* Signal to move */
    Trace	*new_trace,
    Signal	*after_sig_ptr)	/* Signal to place after or ADD_LAST */
{
    Signal	*new_sig_ptr;

    if (sig_ptr) {
	if (sig_ptr->deleted) {
	    sig_move (old_trace, sig_ptr, new_trace, after_sig_ptr);
	}
	else {
	    new_sig_ptr = sig_replicate (old_trace, sig_ptr);
	    sig_ptr->deleted = FALSE;
	    add_signal_to_queue (new_trace, new_sig_ptr, after_sig_ptr);
	}
    }
}

void	sig_update_search ()
{
    Trace	*trace;
    Signal	*sig_ptr;
    register int i;

    if (DTPRINT_ENTRY) printf ("In sig_update_search\n");

    /* Search every trace for the signals, including deleted */
    for (trace = global->deleted_trace_head; trace; trace = trace->next_trace) {
	/* See what signals match the search and highlight as appropriate */
	for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	    if (sig_ptr->color && sig_ptr->search) {
		sig_ptr->color = sig_ptr->search = 0;
	    }
	    for (i=0; i<MAX_SRCH; i++) {
		if (!sig_ptr->color &&
		    global->sig_srch[i].color &&
		    wildmat (sig_ptr->signame, global->sig_srch[i].string)) {
		    sig_ptr->search = i+1;
		    sig_ptr->color = global->sig_srch[i].color;
		}
	    }
	}
    }
}

Boolean_t sig_is_constant (
    Trace	*trace,
    Signal	*sig_ptr,
    Boolean_t	ignorexz)		/* TRUE = ignore xz */
{
    Boolean_t 	changes;
    Value_t	*cptr;
    Value_t	old_value;
    Boolean_t	old_got_value;

    /* Is there a transition? */
    changes=FALSE;
    old_got_value = FALSE;
    for (cptr = sig_ptr->bptr ; CPTR_TIME(cptr) != EOT; cptr = CPTR_NEXT(cptr)) {
	if (CPTR_TIME(cptr) == trace->end_time) {
	    break;
	}

	if (ignorexz & ((cptr->siglw.stbits.state == STATE_U)
			| (cptr->siglw.stbits.state == STATE_Z))) {
	    continue;
	}

	if (!old_got_value) {
	    val_copy (&old_value, cptr);
	    old_got_value = TRUE;
	    continue;
	}

	if (!val_equal (cptr, &old_value)) {
	    changes = TRUE;
	    break;
	}
    }
    return (!changes);
}

void    sig_print_names (
    Trace	*trace)
{
    Signal	*sig_ptr, *back_sig_ptr;

    if (DTPRINT_ENTRY) printf ("In print_sig_names\n");

    printf ("  Number of signals = %d\n", trace->numsig);

    /* loop thru each signal */
    back_sig_ptr = NULL;
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	printf (" Sig '%s'  ty=%d index=%d-%d tempbit %d btyp=%d bpos=%d bits=%d\n",
		sig_ptr->signame, sig_ptr->type,
		sig_ptr->msb_index ,sig_ptr->lsb_index, sig_ptr->bit_index,
		sig_ptr->file_type.flags, sig_ptr->file_pos, sig_ptr->bits
		);
	if (sig_ptr->backward != back_sig_ptr) {
	    printf (" %%E, Backward link is to '%p' not '%p'\n", sig_ptr->backward, back_sig_ptr);
	}
	back_sig_ptr = sig_ptr;
    }
    
    /* Don't do a integrity check here, as sometimes all links aren't ready! */
    /* print_signal_states (trace); */
}

/****************************** CONFIG FUNCTIONS ******************************/


void    sig_highlight_selected (
    int		color)
{
    Signal	*sig_ptr;
    SignalList	*siglst_ptr;
    
    if (DTPRINT_ENTRY) printf ("In sig_highlight_selected\n");
    
    for (siglst_ptr = global->select_head; siglst_ptr; siglst_ptr = siglst_ptr->forward) {
	sig_ptr = siglst_ptr->signal;
	/* Change the color */
	sig_ptr->color = color;
	sig_ptr->search = 0;
    }

    draw_all_needed ();
}

void    sig_base_selected (
    Base_t	*base_ptr)
{
    Signal	*sig_ptr;
    SignalList	*siglst_ptr;
    
    if (DTPRINT_ENTRY) printf ("In sig_base_selected\n");
    
    for (siglst_ptr = global->select_head; siglst_ptr; siglst_ptr = siglst_ptr->forward) {
	sig_ptr = siglst_ptr->signal;
	/* Change the color */
	sig_ptr->base = base_ptr;
    }

    draw_all_needed ();
}

void    sig_move_selected (
    /* also used for adding deleted signals */
    Trace	*new_trace,
    char	*after_pattern)
{
    Trace	*old_trace;
    Signal	*sig_ptr, *after_sig_ptr;
    SignalList	*siglst_ptr;
    
    if (DTPRINT_ENTRY) printf ("In sig_move_selected aft='%s'\n", after_pattern);

    after_sig_ptr = sig_wildmat_signame (new_trace, after_pattern);
    if (! after_sig_ptr) after_sig_ptr = ADD_LAST;
    
    for (siglst_ptr = global->select_head; siglst_ptr; siglst_ptr = siglst_ptr->forward) {
	sig_ptr = siglst_ptr->signal;
	old_trace = siglst_ptr->trace;
	/* Move it */
	sig_move (old_trace, sig_ptr, new_trace, after_sig_ptr);
    }

    draw_needupd_sig_start ();
    draw_all_needed ();
}


void    sig_rename_selected (
    /* also used for adding deleted signals */
    char	*new_name)
{
    Signal	*sig_ptr;
    SignalList	*siglst_ptr;
    
    if (DTPRINT_ENTRY) printf ("In sig_rename_selected new='%s'\n", new_name);

    siglst_ptr = global->select_head;
    if (siglst_ptr) {
	sig_ptr = siglst_ptr->signal;
	strcpy (sig_ptr->signame, new_name);
	sig_ptr->xsigname = XmStringCreateSimple (sig_ptr->signame);
    }

    draw_needupd_sig_start ();
    draw_all_needed ();
}


void    sig_copy_selected (
    Trace	*new_trace,
    char	*after_pattern)
{
    Trace	*old_trace;
    Signal	*sig_ptr, *after_sig_ptr;
    SignalList	*siglst_ptr;
    
    if (DTPRINT_ENTRY) printf ("In sig_copy_pattern - aft='%s'\n", after_pattern);

    after_sig_ptr = sig_wildmat_signame (new_trace, after_pattern);
    if (! after_sig_ptr) after_sig_ptr = ADD_LAST;
    
    for (siglst_ptr = global->select_head; siglst_ptr; siglst_ptr = siglst_ptr->forward) {
	sig_ptr = siglst_ptr->signal;
	old_trace = siglst_ptr->trace;
	sig_copy (old_trace, sig_ptr, new_trace, after_sig_ptr);
    }

    draw_needupd_sig_start ();
    draw_all_needed ();
}


void    sig_delete_selected (
    Boolean_t	constant_flag,		/* FALSE = only delete constants */
    Boolean_t	ignorexz)		/* TRUE = if deleting constants, ignore xz */
{
    Trace	*trace;
    Signal	*sig_ptr;
    SignalList	*siglst_ptr;
    
    if (DTPRINT_ENTRY) printf ("In sig_delete_selected %d %d\n", constant_flag, ignorexz);

    for (siglst_ptr = global->select_head; siglst_ptr; siglst_ptr = siglst_ptr->forward) {
	sig_ptr = siglst_ptr->signal;
	trace = siglst_ptr->trace;
	if  ( constant_flag || sig_is_constant (trace, sig_ptr, ignorexz)) {
	    sig_delete (trace, sig_ptr, constant_flag );
	}
    }

    draw_needupd_sig_start ();
    draw_all_needed ();
}


void    sig_goto_pattern (
    Trace	*trace,
    char	*pattern)
{
    Signal	*sig_ptr;
    uint_t	numprt;
    int		inc;
    Boolean_t	on_screen, found;
    
    if (DTPRINT_ENTRY) printf ("In sig_goto_pattern - trace=%p pat='%s'\n",trace, pattern);
    
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	/* Is this signal already on the screen? */
	on_screen = FALSE;
	for (sig_ptr = trace->dispsig, numprt = 0; sig_ptr && numprt<trace->numsigvis;
	     sig_ptr = sig_ptr->forward, numprt++) {
	    if (wildmat (sig_ptr->signame, pattern)) {
		on_screen = TRUE;
		break;
	    }
	}

	if (!on_screen) {
	    /* Align starting signal to search position */
	    found = FALSE;
	    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
		if (wildmat (sig_ptr->signame, pattern)) {
		    trace->dispsig = sig_ptr;
		    found = TRUE;
		    break;
		}
	    }

	    if (found) {
		/* Rescroll found signal to the center of the screen */
		inc = (-trace->numsigvis) / 2;
		while ( (inc < 0) && trace->dispsig && trace->dispsig->backward ) {
		    trace->dispsig = trace->dispsig->backward;
		    inc++;
		}
	    }
	    vscroll_new (trace,0);	/* Realign time */
	}
    }
    draw_all_needed ();
}


/****************************** EXAMINE ******************************/

char *sig_examine_string (
    /* Return string with examine information in it */
    Trace	*trace,
    Signal	*sig_ptr)
{
    static char	strg[2000];
    char	strg2[2000];
    
    if (DTPRINT_ENTRY) printf ("val_examine_popup_sig_string\n");

    strcpy (strg, sig_ptr->signame);

    sprintf (strg2, "\nBase: %s\n", sig_ptr->base->name);
    strcat (strg, strg2);
	
    /* Debugging information */
    if (DTDEBUG) {
	sprintf (strg2, "\nType %d   Blocks %ld\n",
		 sig_ptr->type, sig_ptr->blocks);
	strcat (strg, strg2);
	sprintf (strg2, "Bits %d   Index %d - %d\n",
		 sig_ptr->bits, sig_ptr->msb_index, sig_ptr->lsb_index);
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
	
/****************************** MENU OPTIONS ******************************/

void    sig_add_cb (
    Widget	w)
{
    Trace *trace = widget_to_trace(w);
    Signal	*sig_ptr;
    Widget	list_wid;
    
    if (DTPRINT_ENTRY) printf ("In sig_add_cb - trace=%p\n",trace);
    
    if (!trace->signal.add) {
	XtSetArg (arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg (arglist[1], XmNwidth, 200);
	XtSetArg (arglist[2], XmNheight, 150);
	XtSetArg (arglist[3], XmNdialogTitle, XmStringCreateSimple ("Signal Select"));
	XtSetArg (arglist[4], XmNlistVisibleItemCount, 5);
	XtSetArg (arglist[5], XmNlistLabelString, XmStringCreateSimple ("Select Signal than Location"));
	trace->signal.add = XmCreateSelectionDialog (trace->work,"",arglist,6);
	/* DAddCallback (trace->signal.add, XmNsingleCallback, sig_add_sel_cb, trace); */
	DAddCallback (trace->signal.add, XmNokCallback, sig_add_sel_cb, trace);
	DAddCallback (trace->signal.add, XmNapplyCallback, sig_add_sel_cb, trace);
	DAddCallback (trace->signal.add, XmNcancelCallback, sig_cancel_cb, trace);
	XtUnmanageChild ( XmSelectionBoxGetChild (trace->signal.add, XmDIALOG_HELP_BUTTON));
	XtUnmanageChild ( XmSelectionBoxGetChild (trace->signal.add, XmDIALOG_APPLY_BUTTON));
	XtUnmanageChild ( XmSelectionBoxGetChild (trace->signal.add, XmDIALOG_PROMPT_LABEL));
	XtUnmanageChild ( XmSelectionBoxGetChild (trace->signal.add, XmDIALOG_VALUE_TEXT));
    }
    else {
	XtUnmanageChild (trace->signal.add);
    }
    
    /* loop thru signals on deleted queue and add to list */
    list_wid = XmSelectionBoxGetChild (trace->signal.add, XmDIALOG_LIST);
    XmListDeleteAllItems (list_wid);
    for (sig_ptr = global->deleted_trace_head->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	XmListAddItem (list_wid, sig_ptr->xsigname, 0);
    }

    /* if there are signals deleted make OK button active */
    XtSetArg (arglist[0], XmNsensitive, (global->deleted_trace_head->firstsig != NULL)?TRUE:FALSE);
    XtSetValues (XmSelectionBoxGetChild (trace->signal.add, XmDIALOG_OK_BUTTON), arglist, 1);

    /* manage the popup on the screen */
    DManageChild (trace->signal.add, trace, MC_NOKEYS);
}

void    sig_add_sel_cb (
    Widget	w,
    Trace	*trace,
    XmSelectionBoxCallbackStruct *cb)
{
    Signal	*sig_ptr;
    
    if (DTPRINT_ENTRY) printf ("In sig_add_sel_cb - trace=%p\n",trace);
    
    if ( global->deleted_trace_head->firstsig == NULL ) return;

    /* save the deleted signal selected number */
    for (sig_ptr = global->deleted_trace_head->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	if (XmStringCompare (cb->value, sig_ptr->xsigname)) break;
    }
    
    /* remove any previous events */
    remove_all_events (trace);
    
    if (sig_ptr && XmStringCompare (cb->value, sig_ptr->xsigname)) {
	global->selected_sig = sig_ptr;
	/* process all subsequent button presses as signal adds */ 
	set_cursor (trace, DC_SIG_ADD);
	add_event (ButtonPressMask, sig_add_ev);

	/* unmanage the popup on the screen */
	XtUnmanageChild (trace->signal.add);
    }
    else {
	global->selected_sig = NULL;
    }
}

void    sig_cancel_cb (
    Widget	w)
{
    Trace *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In sig_cancel_cb - trace=%p\n",trace);
    
    /* remove any previous events */
    remove_all_events (trace);
    
    /* unmanage the popup on the screen */
    XtUnmanageChild (trace->signal.add);
}

void    sig_mov_cb (
    Widget	w)
{
    Trace *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In sig_mov_cb - trace=%p\n",trace);
    
    /* guarantee next button press selects a signal to be moved */
    global->selected_sig = NULL;
    
    /* process all subsequent button presses as signal moves */ 
    remove_all_events (trace);
    add_event (ButtonPressMask, sig_move_ev);
    set_cursor (trace, DC_SIG_MOVE_1);
}

void    sig_copy_cb (
    Widget	w)
{
    Trace *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In sig_copy_cb - trace=%p\n",trace);
    
    /* guarantee next button press selects a signal to be moved */
    global->selected_sig = NULL;
    
    /* process all subsequent button presses as signal moves */ 
    remove_all_events (trace);
    add_event (ButtonPressMask, sig_copy_ev);
    set_cursor (trace, DC_SIG_COPY_1);
}

void    sig_del_cb (
    Widget	w)
{
    Trace *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In sig_del_cb - trace=%p\n",trace);
    
    /* process all subsequent button presses as signal deletions */ 
    remove_all_events (trace);
    set_cursor (trace, DC_SIG_DELETE);
    add_event (ButtonPressMask, sig_delete_ev);
}

void    sig_base_cb (
    Widget	w)
{
    Trace *trace = widget_to_trace(w);
    int basenum = submenu_to_color (trace, w, trace->menu.sig_base_pds);
    if (DTPRINT_ENTRY) printf ("In sig_base_cb - trace=%p basenum=%d\n",trace, basenum);
    
    /* Grab color number from the menu button pointer */
    global->selected_base = global->bases[basenum];

    /* process all subsequent button presses as signal deletions */ 
    remove_all_events (trace);
    set_cursor (trace, DC_SIG_BASE);
    add_event (ButtonPressMask, sig_base_ev);
}

void    sig_highlight_cb (
    Widget	w)
{
    Trace *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In sig_highlight_cb - trace=%p\n",trace);
    
    /* Grab color number from the menu button pointer */
    global->highlight_color = submenu_to_color (trace, w, trace->menu.sig_highlight_pds);

    /* process all subsequent button presses as signal deletions */ 
    remove_all_events (trace);
    set_cursor (trace, DC_SIG_HIGHLIGHT);
    add_event (ButtonPressMask, sig_highlight_ev);
}

void    sig_search_cb (
    Widget	w)
{
    Trace *trace = widget_to_trace(w);
    int		i;
    Widget above;
    
    if (DTPRINT_ENTRY) printf ("In sig_search_cb - trace=%p\n",trace);
    
    if (!trace->signal.search) {
	XtSetArg (arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg (arglist[1], XmNdialogTitle, XmStringCreateSimple ("Signal Search Requester") );
	trace->signal.search = XmCreateBulletinBoardDialog (trace->work,"search",arglist,2);
	
	XtSetArg (arglist[0], XmNverticalSpacing, 0);
	trace->signal.form = XmCreateForm (trace->signal.search, "form", arglist, 1);
	DManageChild (trace->signal.form, trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Color"));
	XtSetArg (arglist[1], XmNx, 5);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_FORM );
	trace->signal.label1 = XmCreateLabel (trace->signal.form,"label1",arglist,3);
	DManageChild (trace->signal.label1, trace, MC_NOKEYS);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Sig"));
	XtSetArg (arglist[1], XmNx, 5);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopWidget, trace->signal.label1);
	trace->signal.label4 = XmCreateLabel (trace->signal.form,"label4",arglist,4);
	DManageChild (trace->signal.label4, trace, MC_NOKEYS);
	
	XtSetArg (arglist[0], XmNlabelString,
		 XmStringCreateSimple ("Search value, *? are wildcards" ));
	XtSetArg (arglist[1], XmNx, 60);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopWidget, trace->signal.label1);
	XtSetArg (arglist[4], XmNverticalSpacing, 0);
	trace->signal.label3 = XmCreateLabel (trace->signal.form,"label3",arglist,5);
	DManageChild (trace->signal.label3, trace, MC_NOKEYS);
	
	above = trace->signal.label3;

	for (i=0; i<MAX_SRCH; i++) {
	    /* enable button */
	    XtSetArg (arglist[0], XmNx, 15);
	    XtSetArg (arglist[1], XmNtopAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[2], XmNtopWidget, above);
	    XtSetArg (arglist[3], XmNselectColor, trace->xcolornums[i+1]);
	    XtSetArg (arglist[4], XmNlabelString, XmStringCreateSimple (""));
	    trace->signal.enable[i] = XmCreateToggleButton (trace->signal.form,"togglen",arglist,5);
	    DManageChild (trace->signal.enable[i], trace, MC_NOKEYS);

	    /* create the file name text widget */
	    XtSetArg (arglist[0], XmNrows, 1);
	    XtSetArg (arglist[1], XmNcolumns, 30);
	    XtSetArg (arglist[2], XmNx, 60);
	    XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[4], XmNtopWidget, above);
	    XtSetArg (arglist[5], XmNresizeHeight, FALSE);
	    XtSetArg (arglist[6], XmNeditMode, XmSINGLE_LINE_EDIT);
	    trace->signal.text[i] = XmCreateText (trace->signal.form,"textn",arglist,7);
	    DAddCallback (trace->signal.text[i], XmNactivateCallback, sig_search_ok_cb, trace);
	    DManageChild (trace->signal.text[i], trace, MC_NOKEYS);

	    above = trace->signal.text[i];
	}

	/* Create Separator */
	XtSetArg (arglist[0], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[1], XmNtopWidget, above);
	XtSetArg (arglist[2], XmNtopOffset, 10);
	XtSetArg (arglist[3], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[4], XmNrightAttachment, XmATTACH_FORM );
	trace->signal.sep = XmCreateSeparator (trace->signal.form, "sep",arglist,5);
	DManageChild (trace->signal.sep, trace, MC_NOKEYS);
	
	/* Create OK button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple (" OK ") );
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopWidget, trace->signal.sep);
	XtSetArg (arglist[4], XmNtopOffset, 10);
	trace->signal.ok = XmCreatePushButton (trace->signal.form,"ok",arglist,5);
	DAddCallback (trace->signal.ok, XmNactivateCallback, sig_search_ok_cb, trace);
	DManageChild (trace->signal.ok, trace, MC_NOKEYS);
	
	/* create apply button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Apply") );
	XtSetArg (arglist[1], XmNx, 70);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopWidget, trace->signal.sep);
	XtSetArg (arglist[4], XmNtopOffset, 10);
	trace->signal.apply = XmCreatePushButton (trace->signal.form,"apply",arglist,5);
	DAddCallback (trace->signal.apply, XmNactivateCallback, sig_search_apply_cb, trace);
	DManageChild (trace->signal.apply, trace, MC_NOKEYS);
	
	/* create cancel button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Cancel") );
	XtSetArg (arglist[1], XmNx, 140);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopWidget, trace->signal.sep);
	XtSetArg (arglist[4], XmNtopOffset, 10);
	trace->signal.cancel = XmCreatePushButton (trace->signal.form,"cancel",arglist,5);
	DAddCallback (trace->signal.cancel, XmNactivateCallback, unmanage_cb, trace->signal.search);

	DManageChild (trace->signal.cancel, trace, MC_NOKEYS);
    }
    
    /* Copy settings to local area to allow cancel to work */
    for (i=0; i<MAX_SRCH; i++) {
	/* Update with current search enables */
	XtSetArg (arglist[0], XmNset, (global->sig_srch[i].color != 0));
	XtSetValues (trace->signal.enable[i], arglist, 1);

	/* Update with current search values */
	XmTextSetString (trace->signal.text[i], global->sig_srch[i].string);
    }

    /* manage the popup on the screen */
    DManageChild (trace->signal.search, trace, MC_NOKEYS);
}

void    sig_search_ok_cb (
    Widget	w,
    Trace	*trace,
    XmSelectionBoxCallbackStruct *cb)
{
    char	*strg;
    int		i;

    if (DTPRINT_ENTRY) printf ("In sig_search_ok_cb - trace=%p\n",trace);

    for (i=0; i<MAX_SRCH; i++) {
	/* Update with current search enables */
	if (XmToggleButtonGetState (trace->signal.enable[i]))
	    global->sig_srch[i].color = i+1;
	else global->sig_srch[i].color = 0;
	
	/* Update with current search values */
	strg = XmTextGetString (trace->signal.text[i]);
	strcpy (global->sig_srch[i].string, strg);
    }
    
    XtUnmanageChild (trace->signal.search);

    draw_needupd_sig_search ();
    draw_needupd_sig_start ();
    draw_all_needed ();
}

void    sig_search_apply_cb (
    Widget	w,
    Trace	*trace,
    XmSelectionBoxCallbackStruct *cb)
{
    if (DTPRINT_ENTRY) printf ("In sig_search_apply_cb - trace=%p\n",trace);

    sig_search_ok_cb (w,trace,cb);
    sig_search_cb (trace->main);
}

/****************************** EVENTS ******************************/

void    sig_add_ev (
    Widget	w,
    Trace	*trace,
    XButtonPressedEvent	*ev)
{
    Signal		*sig_ptr;
    
    if (DTPRINT_ENTRY) printf ("In sig_add_ev - trace=%p\n",trace);
    if (ev->type != ButtonPress || ev->button!=1) return;
    
    /* return if there is no file */
    if (!trace->loaded || global->selected_sig==NULL)
	return;
    
    /* make sure button has been clicked in in valid location of screen */
    sig_ptr = posy_to_signal (trace, ev->y);
    /* Null signal is OK */

    /* get previous signal */
    if (sig_ptr) sig_ptr = sig_ptr->backward;
    
    /* remove signal from list box */
    XmListDeleteItem (XmSelectionBoxGetChild (trace->signal.add, XmDIALOG_LIST),
		      global->selected_sig->xsigname );
    if (global->deleted_trace_head->firstsig == NULL) {
	XtSetArg (arglist[0], XmNsensitive, FALSE);
	XtSetValues (XmSelectionBoxGetChild (trace->signal.add, XmDIALOG_OK_BUTTON), arglist, 1);
    }
    
    sig_move (global->deleted_trace_head, global->selected_sig, trace, sig_ptr);
    
    /* remove any previous events */
    remove_all_events (trace);
    
    /* pop window back to top of stack */
    XtUnmanageChild ( trace->signal.add );
    DManageChild ( trace->signal.add , trace, MC_NOKEYS);
    
    draw_needupd_sig_start ();
    draw_all_needed ();
}

void    sig_move_ev (
    Widget	w,
    Trace	*trace,
    XButtonPressedEvent	*ev)
{
    Signal	*sig_ptr;
    
    if (DTPRINT_ENTRY) printf ("In sig_move_ev - trace=%p\n",trace);
    if (ev->type != ButtonPress || ev->button!=1) return;
    
    /* return if there is no file */
    if ( !trace->loaded )
	return;
    
    /* return if there is less than 2 signals to move */
    if ( trace->numsig < 2 )
	return;
    
    /* make sure button has been clicked in in valid location of screen */
    sig_ptr = posy_to_signal (trace, ev->y);
    if (!sig_ptr) return;
   
    if ( global->selected_sig == NULL ) {
	/* next call will perform the move */
	global->selected_sig = sig_ptr;
	global->selected_trace = trace;
	set_cursor (trace, DC_SIG_MOVE_2);
    }
    else {
	/* get previous signal */
	if (sig_ptr) sig_ptr = sig_ptr->backward;
    
	/* if not the same signal perform the move */
	if ( sig_ptr != global->selected_sig ) {
	    sig_move (global->selected_trace, global->selected_sig, trace, sig_ptr);
	}
	
	/* guarantee that next button press will select signal */
	global->selected_sig = NULL;
	set_cursor (trace, DC_SIG_MOVE_1);
    }
    
    draw_needupd_sig_start ();
    draw_all_needed ();
}

void    sig_copy_ev (
    Widget	w,
    Trace	*trace,
    XButtonPressedEvent	*ev)
{
    Signal	*sig_ptr;
    
    if (DTPRINT_ENTRY) printf ("In sig_copy_ev - trace=%p\n",trace);
    if (ev->type != ButtonPress || ev->button!=1) return;
    
    /* make sure button has been clicked in in valid location of screen */
    sig_ptr = posy_to_signal (trace, ev->y);
    if (!sig_ptr) return;
   
    if ( global->selected_sig == NULL ) {
	/* next call will perform the copy */
	global->selected_sig = sig_ptr;
	global->selected_trace = trace;
	set_cursor (trace, DC_SIG_COPY_2);
    }
    else {
	/* get previous signal */
	if (sig_ptr) sig_ptr = sig_ptr->backward;
    
	/* if not the same signal perform the move */
	if ( sig_ptr != global->selected_sig ) {
	    sig_copy (global->selected_trace, global->selected_sig, trace, sig_ptr);
	}
	
	/* guarantee that next button press will select signal */
	global->selected_sig = NULL;
	set_cursor (trace, DC_SIG_COPY_1);
    }
    
    draw_needupd_sig_start ();
    draw_all_needed ();
}

void    sig_delete_ev (
    Widget	w,
    Trace	*trace,
    XButtonPressedEvent	*ev)
{
    Signal	*sig_ptr;
    
    if (DTPRINT_ENTRY) printf ("In sig_delete_ev - trace=%p\n",trace);
    if (ev->type != ButtonPress || ev->button!=1) return;
    
    /* return if there is no file */
    if ( !trace->loaded )
	return;
    
    /* return if there are no signals to delete */
    if ( trace->numsig == 0 )
	return;
    
    /* find the signal */
    sig_ptr = posy_to_signal (trace, ev->y);
    if (!sig_ptr) return;
    
    /* remove the signal from the queue */
    sig_delete (trace, sig_ptr, TRUE);
    
    /* add signame to list box */
    if ( trace->signal.add != NULL ) {
	XmListAddItem (XmSelectionBoxGetChild (trace->signal.add, XmDIALOG_LIST),
		       sig_ptr->xsigname, 0 );
    }
    
    draw_needupd_sig_start ();
    draw_all_needed ();
}


void    sig_highlight_ev (
    Widget	w,
    Trace	*trace,
    XButtonPressedEvent	*ev)
{
    Signal	*sig_ptr;
    
    if (DTPRINT_ENTRY) printf ("In sig_highlight_ev - trace=%p\n",trace);
    if (ev->type != ButtonPress || ev->button!=1) return;
    
    sig_ptr = posy_to_signal (trace, ev->y);
    if (!sig_ptr) return;
    
    /* Change the color */
    sig_ptr->color = global->highlight_color;
    sig_ptr->search = 0;

    draw_all_needed ();
}


void    sig_base_ev (
    Widget	w,
    Trace	*trace,
    XButtonPressedEvent	*ev)
{
    Signal	*sig_ptr;
    
    if (DTPRINT_ENTRY) printf ("In sig_base_ev - trace=%p\n",trace);
    if (ev->type != ButtonPress || ev->button!=1) return;
    
    sig_ptr = posy_to_signal (trace, ev->y);
    if (!sig_ptr) return;
    
    /* Change the base */
    sig_ptr->base = global->selected_base;

    draw_all_needed ();
}

/****************************** SELECT OPTIONS ******************************/

void    sig_select_cb (
    Widget	w)
{
    Trace *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In sig_select_cb - trace=%p\n",trace);
    
    /* return if there is no file */
    if (!trace->loaded) return;
    
    if (!trace->select.select) {
	trace->select.del_strings=NULL;
	trace->select.add_strings=NULL;
	trace->select.del_signals=NULL;
	trace->select.add_signals=NULL;
    
	XtSetArg (arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg (arglist[1], XmNdialogTitle, XmStringCreateSimple ("Select Signal Requester") );
	XtSetArg (arglist[2], XmNheight, 400);
	XtSetArg (arglist[3], XmNverticalSpacing, 7);
	XtSetArg (arglist[4], XmNhorizontalSpacing, 10);
	XtSetArg (arglist[5], XmNresizable, FALSE);
	trace->select.select = XmCreateFormDialog (trace->work,"select",arglist,6);
	
	/*** BUTTONS ***/

	/* Create OK button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple (" OK ") );
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNbottomAttachment, XmATTACH_FORM );
	trace->select.ok = XmCreatePushButton (trace->select.select,"ok",arglist,3);
	DAddCallback (trace->select.ok, XmNactivateCallback, sig_sel_ok_cb, trace);
	DManageChild (trace->select.ok, trace, MC_NOKEYS);

	/* Create apply button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Apply") );
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_POSITION );
	XtSetArg (arglist[2], XmNleftPosition, 45);
	XtSetArg (arglist[3], XmNbottomAttachment, XmATTACH_FORM );
	trace->select.apply = XmCreatePushButton (trace->select.select,"apply",arglist,4);
	DAddCallback (trace->select.apply, XmNactivateCallback, sig_sel_apply_cb, trace);
	DManageChild (trace->select.apply, trace, MC_NOKEYS);

	/* create cancel button */
#if 0 /*broken*/
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Cancel") );
	XtSetArg (arglist[1], XmNrightAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNbottomAttachment, XmATTACH_FORM );
	trace->select.cancel = XmCreatePushButton (trace->select.select,"cancel",arglist,3);
	/* DAddCallback (trace->select.cancel, XmNactivateCallback, unmanage_cb, trace->select.select); */
	DAddCallback (trace->select.cancel, XmNactivateCallback, unmanage_cb, trace->select.select);
	DManageChild (trace->select.cancel, trace, MC_NOKEYS);
#endif

	/* Create Separator */
	XtSetArg (arglist[0], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[1], XmNrightAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNbottomAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNbottomWidget, trace->select.ok );
	XtSetArg (arglist[4], XmNbottomOffset, 10);
	trace->select.sep = XmCreateSeparator (trace->select.select, "sep",arglist,5);
	DManageChild (trace->select.sep, trace, MC_NOKEYS);

	/*** Add (deleted list) section ***/
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Add Signal Pattern"));
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_FORM );
	trace->select.label2 = XmCreateLabel (trace->select.select,"label2",arglist,3);
	DManageChild (trace->select.label2, trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNrows, 1);
	XtSetArg (arglist[1], XmNcolumns, 30);
	XtSetArg (arglist[2], XmNresizeHeight, FALSE);
	XtSetArg (arglist[3], XmNeditMode, XmSINGLE_LINE_EDIT);
	XtSetArg (arglist[4], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[5], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[6], XmNtopWidget, trace->select.label2);
	XtSetArg (arglist[7], XmNrightAttachment, XmATTACH_POSITION );
	XtSetArg (arglist[8], XmNrightPosition, 50);
	XtSetArg (arglist[9], XmNvalue, "*");
	trace->select.add_pat = XmCreateText (trace->select.select,"dpat",arglist,10);
	DAddCallback (trace->select.add_pat, XmNactivateCallback, sig_sel_pattern_cb, trace);
	DManageChild (trace->select.add_pat, trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Add All") );
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNbottomAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNbottomWidget, trace->select.sep);
	trace->select.add_all = XmCreatePushButton (trace->select.select,"aall",arglist,4);
	XtRemoveAllCallbacks (trace->select.add_all, XmNactivateCallback);
	DAddCallback (trace->select.add_all, XmNactivateCallback, sig_sel_add_all_cb, trace);
	DManageChild (trace->select.add_all, trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNlabelString, string_create_with_cr ("Select a signal to add it\nto the displayed signals."));
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopWidget, trace->select.add_pat);
	trace->select.label1 = XmCreateLabel (trace->select.select,"label1",arglist,4);
	DManageChild (trace->select.label1, trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[1], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[2], XmNtopWidget, trace->select.label1);
	XtSetArg (arglist[3], XmNbottomAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNbottomWidget, trace->select.add_all);
	XtSetArg (arglist[5], XmNrightAttachment, XmATTACH_POSITION );
	XtSetArg (arglist[6], XmNrightPosition, 50);
	trace->select.add_sigs_form = XmCreateForm (trace->select.select, "add_sigs_form", arglist, 7);
	DManageChild (trace->select.add_sigs_form, trace, MC_NOKEYS);

	/* This is under its own form to prevent child attachment errors */
	XtSetArg (arglist[0], XmNlistVisibleItemCount, 5);
	XtSetArg (arglist[1], XmNlistSizePolicy, XmCONSTANT);
	XtSetArg (arglist[2], XmNselectionPolicy, XmEXTENDED_SELECT);
	XtSetArg (arglist[3], XmNitems, 0);
	XtSetArg (arglist[4], XmNrightAttachment, XmATTACH_FORM );
	XtSetArg (arglist[5], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[6], XmNtopAttachment, XmATTACH_FORM );
	XtSetArg (arglist[7], XmNbottomAttachment, XmATTACH_FORM );
	trace->select.add_sigs = XmCreateScrolledList (trace->select.add_sigs_form,"add_sigs",arglist,8);
	DAddCallback (trace->select.add_sigs, XmNextendedSelectionCallback, sig_sel_add_list_cb, trace);
	DManageChild (trace->select.add_sigs, trace, MC_NOKEYS);

	/*** Delete (existing list) section ***/
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Delete Signal Pattern"));
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[2], XmNleftWidget, trace->select.add_sigs_form);
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_FORM );
	trace->select.label4 = XmCreateLabel (trace->select.select,"label4",arglist,4);
	DManageChild (trace->select.label4, trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNrows, 1);
	XtSetArg (arglist[1], XmNcolumns, 30);
	XtSetArg (arglist[1], XmNvalue, "*");
	XtSetArg (arglist[2], XmNresizeHeight, FALSE);
	XtSetArg (arglist[3], XmNeditMode, XmSINGLE_LINE_EDIT);
	XtSetArg (arglist[4], XmNleftAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[5], XmNleftWidget, trace->select.add_sigs_form);
	XtSetArg (arglist[6], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[7], XmNtopWidget, trace->select.label2);
	XtSetArg (arglist[8], XmNrightAttachment, XmATTACH_FORM );
	trace->select.delete_pat = XmCreateText (trace->select.select,"apat",arglist,9);
	DAddCallback (trace->select.delete_pat, XmNactivateCallback, sig_sel_pattern_cb, trace);
	DManageChild (trace->select.delete_pat, trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Delete All") );
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[2], XmNleftWidget, trace->select.add_sigs_form);
	XtSetArg (arglist[3], XmNbottomAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNbottomWidget, trace->select.sep);
	trace->select.delete_all = XmCreatePushButton (trace->select.select,"dall",arglist,5);
	XtRemoveAllCallbacks (trace->select.delete_all, XmNactivateCallback);
	DAddCallback (trace->select.delete_all, XmNactivateCallback, sig_sel_del_all_cb, trace);
	DManageChild (trace->select.delete_all, trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Delete Constant Valued") );
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[2], XmNleftWidget, trace->select.delete_all);
	XtSetArg (arglist[3], XmNbottomAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNbottomWidget, trace->select.sep);
	trace->select.delete_const = XmCreatePushButton (trace->select.select,"dcon",arglist,5);
	XtRemoveAllCallbacks (trace->select.delete_const, XmNactivateCallback);
	DAddCallback (trace->select.delete_const, XmNactivateCallback, sig_sel_del_const_cb, trace);
	DManageChild (trace->select.delete_const, trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNlabelString, string_create_with_cr ("Select a signal to delete it\nfrom the displayed signals."));
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[2], XmNleftWidget, trace->select.add_sigs_form);
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNtopWidget, trace->select.delete_pat);
	trace->select.label3 = XmCreateLabel (trace->select.select,"label3",arglist,5);
	DManageChild (trace->select.label3, trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNrightAttachment, XmATTACH_FORM );
	XtSetArg (arglist[1], XmNlistVisibleItemCount, 5);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopWidget, trace->select.label3);
	XtSetArg (arglist[4], XmNbottomAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[5], XmNbottomWidget, trace->select.delete_all);
	XtSetArg (arglist[6], XmNleftAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[7], XmNleftWidget, trace->select.add_sigs_form);
	XtSetArg (arglist[8], XmNlistSizePolicy, XmCONSTANT);
	XtSetArg (arglist[9], XmNselectionPolicy, XmEXTENDED_SELECT);
	XtSetArg (arglist[10], XmNitems, 0);
	trace->select.delete_sigs = XmCreateScrolledList (trace->select.select,"",arglist,11);
	DAddCallback (trace->select.delete_sigs, XmNextendedSelectionCallback, sig_sel_del_list_cb, trace);
	DManageChild (trace->select.delete_sigs, trace, MC_NOKEYS);
    }
    
    /* manage the popup on the screen */
    DManageChild (trace->select.select, trace, MC_NOKEYS);

    /* update patterns - leave under the "manage" or toolkit will complain */
    sig_sel_pattern_cb (NULL, trace, NULL);
}

void    sig_sel_update_pattern (
    Widget	w,			/* List widget to update */
    Signal	*head_sig_ptr,		/* Head signal in the list */
    char	*pattern,		/* Pattern to match to the list */
    XmString	**xs_list,		/* Static storage for string list */
    Signal	***xs_sigs,		/* Static storage for signal list */
    int		*xs_size)
{
    Signal	*sig_ptr;
    int		sel_count;

    /* loop thru signals on deleted queue and add to list */
    sel_count = 0;
    for (sig_ptr = head_sig_ptr; sig_ptr; sig_ptr = sig_ptr->forward) {
	if (wildmat (sig_ptr->signame, pattern)) {
	    sel_count++;
	}
    }
    sel_count++;	/* Space for null termination */

    /* Make sure that we have somewhere to store the signals */
    if (! *xs_list) {
	*xs_list = (XmString *)XtMalloc (sel_count * sizeof (XmString));
	*xs_sigs = (Signal **)XtMalloc (sel_count * sizeof (Signal *));
	*xs_size = sel_count;
    }
    else if (*xs_size < sel_count) {
	*xs_list = (XmString *)XtRealloc ((char*)*xs_list, sel_count * sizeof (XmString));
	*xs_sigs = (Signal **)XtRealloc ((char*)*xs_sigs, sel_count * sizeof (Signal *));
	*xs_size = sel_count;
    }

    /* go through the list again and make the array */
    sel_count = 0;
    for (sig_ptr = head_sig_ptr; sig_ptr; sig_ptr = sig_ptr->forward) {
	if (wildmat (sig_ptr->signame, pattern)) {
	    (*xs_list)[sel_count] = sig_ptr->xsigname;
	    (*xs_sigs)[sel_count] = sig_ptr;
	    sel_count++;
	}
    }
    /* Mark list end */
    (*xs_list)[sel_count] = NULL;
    (*xs_sigs)[sel_count] = NULL;

    /* update the widget */
    XtSetArg (arglist[0], XmNitemCount, sel_count);
    XtSetArg (arglist[1], XmNitems, *xs_list);
    XtSetValues (w, arglist, 2);
}

void    sig_sel_pattern_cb (
    Widget	w,
    Trace	*trace,
    XmAnyCallbackStruct		*cb)
    /* Called by creation or call back - create the list of signals with a possibly new pattern */
{
    char	*pattern;

    /* loop thru signals on deleted queue and add to list */
    pattern = XmTextGetString (trace->select.add_pat);
    sig_sel_update_pattern (trace->select.add_sigs, global->deleted_trace_head->firstsig, pattern,
			    &(trace->select.add_strings), &(trace->select.add_signals),
			    &(trace->select.add_size));

    /* loop thru signals on displayed queue and add to list */
    pattern = XmTextGetString (trace->select.delete_pat);
    sig_sel_update_pattern (trace->select.delete_sigs, trace->firstsig, pattern,
			    &(trace->select.del_strings), &(trace->select.del_signals),
			    &(trace->select.del_size));
}

void    sig_sel_ok_cb (
    Widget	w,
    Trace	*trace,
    XmAnyCallbackStruct		*cb)
{
    draw_needupd_sig_start ();
    draw_all_needed ();
}

void    sig_sel_apply_cb (
    Widget	w,
    Trace	*trace,
    XmAnyCallbackStruct 	*cb)
{
    if (DTPRINT_ENTRY) printf ("In sig_sel_apply_cb - trace=%p\n",trace);

    sig_sel_ok_cb (w,trace,cb);
    sig_select_cb (trace->main);
}

void    sig_sel_add_all_cb (
    Widget	w,
    Trace	*trace,
    XmAnyCallbackStruct 	*cb)
{
    Signal	*sig_ptr;
    int		i;

    if (DTPRINT_ENTRY) printf ("In sig_sel_add_all_cb - trace=%p\n",trace);

    /* loop thru signals on deleted queue and add to list */
    for (i=0; 1; i++) {
	sig_ptr = (trace->select.add_signals)[i];
	if (!sig_ptr) break;

	/* Add it */
	sig_move (global->deleted_trace_head, sig_ptr, trace, ADD_LAST);
    }
    sig_sel_pattern_cb (NULL, trace, NULL);
}

void    sig_sel_del_all_cb (
    Widget	w,
    Trace	*trace,
    XmAnyCallbackStruct 	*cb)
{
    Signal	*sig_ptr;
    int		i;

    if (DTPRINT_ENTRY) printf ("In sig_sel_del_all_cb - trace=%p\n",trace);

    /* loop thru signals on deleted queue and add to list */
    for (i=0; 1; i++) {
	sig_ptr = (trace->select.del_signals)[i];
	if (!sig_ptr) break;
	sig_delete (trace, sig_ptr, TRUE);
    }
    sig_sel_pattern_cb (NULL, trace, NULL);
}

void    sig_sel_del_const_cb (
    Widget	w,
    Trace	*trace,
    XmAnyCallbackStruct 	*cb)
{
    Signal	*sig_ptr;
    int		i;

    if (DTPRINT_ENTRY) printf ("In sig_sel_del_const_cb - trace=%p\n",trace);

    /* loop thru signals on deleted queue and add to list */
    for (i=0; 1; i++) {
	sig_ptr = (trace->select.del_signals)[i];
	if (!sig_ptr) break;

	/* Delete it */
	if (sig_is_constant (trace, sig_ptr, FALSE)) {
	    sig_delete (trace, sig_ptr, FALSE);
	}
    }
    sig_sel_pattern_cb (NULL, trace, NULL);
}

void    sig_sel_add_list_cb (
    Widget	w,
    Trace	*trace,
    XmListCallbackStruct 	*cb)
{
    int		sel, i;
    Signal	*sig_ptr;

    if (DTPRINT_ENTRY) printf ("In sig_sel_add_list_cb - trace=%p\n",trace);

    for (sel=0; sel < cb->selected_item_count; sel++) {
	i = (cb->selected_item_positions)[sel] - 1;
	sig_ptr = (trace->select.add_signals)[i];
	if (!sig_ptr) fprintf (stderr, "Unexpected null signal\n");
	/*if (DTPRINT) printf ("Pos %d sig '%s'\n", i, sig_ptr->signame);*/

	/* Add it */
	sig_move (global->deleted_trace_head, sig_ptr, trace, ADD_LAST);
    }
    sig_sel_pattern_cb (NULL, trace, NULL);
}

void    sig_sel_del_list_cb (
    Widget	w,
    Trace	*trace,
    XmListCallbackStruct	 *cb)
{
    int		sel, i;
    Signal	*sig_ptr;

    if (DTPRINT_ENTRY) printf ("In sig_sel_del_list_cb - trace=%p\n",trace);

    for (sel=0; sel < cb->selected_item_count; sel++) {
	i = (cb->selected_item_positions)[sel] - 1;
	sig_ptr = (trace->select.del_signals)[i];
	if (!sig_ptr) fprintf (stderr, "Unexpected null signal\n");
	/*if (DTPRINT) printf ("Pos %d sig '%s'\n", i, sig_ptr->signame);*/

	/* Delete it */
	sig_delete (trace, sig_ptr, TRUE);
    }
    sig_sel_pattern_cb (NULL, trace, NULL);
}

/****************************** PRESERVATION  ****************************************/

/* Preserving signal ordering and other information across trace reading
   sig_cross_preserve	
        Make new trace structure with old data
   	Clear out transition data (to save memory space)

   read new trace information

   sig_cross_restore
        Erase copy of old trace
	Erase old signal information
*/

#define new_trace_sig	verilog_next	/* Use existing unused field (not current trace) */ 
#define new_forward_sig	verilog_next	/* Use existing unused field (in  current trace) */ 

void sig_cross_preserve (
    /* This is pre-cleanup when preserving signal information for a new trace to be read */
    Trace	*trace)
{
    Signal	*sig_ptr;
    Trace	*trace_ptr;

    if (DTPRINT_ENTRY) printf ("In sig_cross_preserve - trace=%p\n",trace);

    /* Save the current trace into a new preserved place */
    assert (global->preserved_trace==NULL);

    if (! global->save_ordering) {
	/* Will be freed by sig_free call later inside trace reading */
	return;
    }

    global->preserved_trace = XtNew (Trace);
    memcpy (global->preserved_trace, trace, sizeof (Trace));
    
    /* Other signals, AND deleted signals */
    for (trace_ptr = global->deleted_trace_head; trace_ptr; trace_ptr = trace_ptr->next_trace) {
	for (sig_ptr = trace_ptr->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {

	    if (sig_ptr->trace == trace) {
		/* free the signal data to save memory */
		DFree (sig_ptr->bptr);
		DFree (sig_ptr->xsigname);

		/* change to point to the new preserved structure */
		sig_ptr->trace = global->preserved_trace;
		sig_ptr->new_trace_sig = NULL;
	    }
	}
    }

    /* Tell old trace that it no longer has any signals */
    trace->firstsig = NULL;
    trace->dispsig = NULL;
}

void sig_cross_sigmatch (
    /* Look through each signal in the old list, attempt to match with
       signal from the new_trace.  When found create new_trace_sig link to/from old & new signal */
    Trace	*new_trace,
    Signal	*old_sig_ptr)
{
    register Signal	*new_sig_ptr;		/* New signal now searching on */
    register Signal	*start_sig_ptr;		/* Signal search began with */
    register char	*old_signame;		/* Searching for this signame */
    register int	old_bits;		/* Searching for this # bits */

    /* Beginning */
    new_sig_ptr = start_sig_ptr = new_trace->firstsig;
    if (!new_sig_ptr || !old_sig_ptr) return;	/* Empty */

    /* First search */
    old_signame = old_sig_ptr->signame;
    old_bits = old_sig_ptr->bits;

    while (1) {
	/*printf ("Compare %s == %s\n", old_sig_ptr->signame, new_sig_ptr->signame);*/
	if (new_sig_ptr->bits == old_bits	/* Redundant, but speeds strcmp */
	    && !strcmp (new_sig_ptr->signame, old_signame)) {
	    /* Match */
	    old_sig_ptr->new_trace_sig = new_sig_ptr;
	    /* Save color, etc */
	    new_sig_ptr->color = old_sig_ptr->color;
	    new_sig_ptr->search = old_sig_ptr->search;
	    /*printf ("Matched %s == %s\n", old_sig_ptr->signame, new_sig_ptr->signame);*/
	    
	    /* Start search for next signal */
nextsig:    
	    do {
		old_sig_ptr = old_sig_ptr->forward;
	    } while (old_sig_ptr && old_sig_ptr->trace != global->preserved_trace);

	    if (!old_sig_ptr) return;
	    old_signame = old_sig_ptr->signame;
	    old_bits = old_sig_ptr->bits;
	    start_sig_ptr = new_sig_ptr;
	}

	/* Search in circular link */
	/* This is much faster because usually the signal ordering will be the same old & new */
	new_sig_ptr = new_sig_ptr->forward ? new_sig_ptr->forward : new_trace->firstsig;
	if (new_sig_ptr == start_sig_ptr) {
	    /* No match, went around list once */
	    /*printf ("MisMatched %s\n", old_sig_ptr->signame);*/
	    old_sig_ptr->new_trace_sig = NULL;
	    goto nextsig;
	}
    }
}

void sig_cross_restore (
    /* Try to make new trace's signal placement match the old placement */
    Trace	*trace)		/* New trace */
{
    Signal	*old_sig_ptr;
    Trace	*trace_ptr;
    Signal	*new_sig_ptr;	/* Pointer to new trace's signal */
    Signal	*next_sig_ptr;

    Signal	*newlist_firstsig;
    Signal	**back_sig_pptr;

    Signal	*new_dispsig;

    if (DTPRINT_ENTRY) printf ("In sig_cross_restore - trace=%p\n",trace);
    
    if (global->save_ordering && global->preserved_trace) {
	/* Preserve colors, etc */

	/* Establish links */
	sig_cross_sigmatch (trace, global->deleted_trace_head->firstsig);
	sig_cross_sigmatch (trace, global->preserved_trace->firstsig);
	for (trace_ptr = global->deleted_trace_head; trace_ptr; trace_ptr = trace_ptr->next_trace) {
	    sig_cross_sigmatch (trace, trace_ptr->firstsig);
	}
	if (DTPRINT_PRESERVE) printf ("Preserve: Match done\n");

	/* Deleted signals */
	if (DTPRINT_PRESERVE) printf ("Preserve: Deleting signals\n");
	for (old_sig_ptr = global->deleted_trace_head->firstsig; old_sig_ptr; old_sig_ptr = old_sig_ptr->forward) {
	    if (old_sig_ptr->trace == global->preserved_trace) {
		/* Signal was deleted in preserved trace, so delete new one too */
		new_sig_ptr = old_sig_ptr->new_trace_sig;
		if (old_sig_ptr->deleted_preserve && (NULL != new_sig_ptr)) {
		    if (DTPRINT_PRESERVE) printf ("Preserve: Please delete %s\n", old_sig_ptr->signame);
		    sig_delete (trace, new_sig_ptr, TRUE);
		}
	    }
	}

	/* Remember scrolling position before move signals around */
	new_dispsig = NULL;	/* But, don't set it yet, the pointer may move */
	if (global->preserved_trace->dispsig && global->preserved_trace->dispsig->new_trace_sig) {
	    new_dispsig = global->preserved_trace->dispsig->new_trace_sig;
	}

	/* Other signals */
	if (DTPRINT_PRESERVE) printf ("Preserve: Copy/move to other\n");
	for (trace_ptr = global->trace_head; trace_ptr; trace_ptr = trace_ptr->next_trace) {
	    for (old_sig_ptr = trace_ptr->firstsig; old_sig_ptr; old_sig_ptr = old_sig_ptr->forward) {
		
		if (    (old_sig_ptr->trace == global->preserved_trace)	/* Sig from old trace */
		    &&  (trace_ptr != global->preserved_trace) ) {	/* Not in old trace */
		    if (old_sig_ptr->copyof) {
			/* Copy to other */
			if (NULL != (new_sig_ptr = old_sig_ptr->new_trace_sig)) {
			    if (DTPRINT_PRESERVE) printf ("Preserve: Please copy-to-other %s\n", old_sig_ptr->signame);
			    sig_copy (trace, new_sig_ptr, trace_ptr, old_sig_ptr);
			}
		    }
		    else {
			/* Move to other */
			if (NULL != (new_sig_ptr = old_sig_ptr->new_trace_sig)) {
			    if (DTPRINT_PRESERVE) printf ("Preserve: Please move-to-other %s\n", old_sig_ptr->signame);
			    sig_move (trace, new_sig_ptr, trace_ptr, old_sig_ptr);
			}
		    }
		}
	    }
	}
	if (DTPRINT_PRESERVE && global->preserved_trace->firstsig) printf ("Preserve: %d %s\n", __LINE__, global->preserved_trace->firstsig->signame);

	/* Zero new_forward_sigs in prep of making new list */
	for (new_sig_ptr = trace->firstsig; new_sig_ptr; new_sig_ptr = new_sig_ptr->forward) {
	    new_sig_ptr->new_forward_sig = NULL;
	    new_sig_ptr->preserve_done = FALSE;		/* Temp flag to indicate has been moved to new link structure */
	}

	/* Our trace, reestablish signal ordering and copying */
	if (DTPRINT_PRESERVE) printf ("Preserve: Ordering\n");
	trace_ptr = trace;
	newlist_firstsig = NULL;
	back_sig_pptr = &(newlist_firstsig);
	old_sig_ptr = global->preserved_trace->firstsig;	/* This will trash old list, kill heads now */
	global->preserved_trace->firstsig = NULL;
	global->preserved_trace->dispsig = NULL;
	if (DTPRINT_PRESERVE) printf ("Preserve: %d\n", __LINE__);
	while (old_sig_ptr) {
	    if (DTPRINT_PRESERVE && old_sig_ptr) printf ("Preserve: %s\n", old_sig_ptr->signame);
		
	    if (old_sig_ptr->trace != global->preserved_trace) {	/* NOT sig from old trace */
		/* Copy or moved from other, cheat by just relinking into new structure */
		if (DTPRINT_PRESERVE) printf ("Preserve: Please cp/mv-to-new %s\n", old_sig_ptr->signame);
		new_sig_ptr = old_sig_ptr;
		new_sig_ptr->preserve_done = TRUE;
		*back_sig_pptr = new_sig_ptr;
		back_sig_pptr = &(new_sig_ptr->new_forward_sig);
		/* Next OLD */
		next_sig_ptr = old_sig_ptr->forward;
		old_sig_ptr = next_sig_ptr;
	    }
	    else {	/* In same trace */
		new_sig_ptr = old_sig_ptr->new_trace_sig;
		if (new_sig_ptr && !(new_sig_ptr->preserve_done)) {
		    if (DTPRINT_PRESERVE) printf ("Preserve: %d\n", __LINE__);
		    /* Has equivelent */
		    new_sig_ptr->preserve_done = TRUE;
		    *back_sig_pptr = new_sig_ptr;
		    back_sig_pptr = &(new_sig_ptr->new_forward_sig);
		}
		
		/* Don't need old_sig any more */
		/* Next OLD */
		if (DTPRINT_PRESERVE) printf ("Preserve: %d\n", __LINE__);
		next_sig_ptr = old_sig_ptr->forward;
		sig_free (trace_ptr, old_sig_ptr, FALSE, FALSE);
		old_sig_ptr = next_sig_ptr;
	    }
	}
	*back_sig_pptr = NULL;		/* Final link */

	/* Insert any new signals that don't have links */
	if (DTPRINT_PRESERVE) printf ("Preserve: Inserting leftovers\n");
	old_sig_ptr = newlist_firstsig;			/* This will trash old list, kill heads now */
	back_sig_pptr = &(newlist_firstsig);
	for (new_sig_ptr = trace->firstsig; new_sig_ptr; new_sig_ptr = new_sig_ptr->forward) {
	    if (DTPRINT_PRESERVE) printf ("Preserve: %d new %s del%d lk %s nxt %s \n", __LINE__, new_sig_ptr->signame, new_sig_ptr->preserve_done,
					  DeNullSignal(new_sig_ptr->new_forward_sig),  DeNullSignal(new_sig_ptr->forward) );
	    if (! new_sig_ptr->preserve_done) {
		/* TRUE insertion into list */
		new_sig_ptr->preserve_done = TRUE;
		new_sig_ptr->new_forward_sig = *back_sig_pptr;
		*back_sig_pptr = new_sig_ptr;
	    }
	    /* Either way, new_forward is now pointing to something */
	    back_sig_pptr = &(new_sig_ptr->new_forward_sig);	/* Insert next signal after this one */
	}

	/* Relink forward & backwards links - may have been trashed above */
	if (DTPRINT_PRESERVE) printf ("Preserve: Relinking\n");
	trace->firstsig = newlist_firstsig;
	trace->numsig = 0;
	old_sig_ptr = NULL;
	for (new_sig_ptr = newlist_firstsig; new_sig_ptr; new_sig_ptr = new_sig_ptr->new_forward_sig) {
	    trace->numsig++;
	    new_sig_ptr->deleted = FALSE;
	    new_sig_ptr->forward = new_sig_ptr->new_forward_sig;
	    new_sig_ptr->backward = old_sig_ptr;
	    old_sig_ptr = new_sig_ptr;
	}

	/* Restore scrolling position */
	trace->dispsig = (new_dispsig) ? new_dispsig : trace->firstsig;
	/* vscroll_new called later in file_cb */

	if (DTPRINT_PRESERVE) printf ("Preserve: Done\n");
    }

    /* Free information for the preserved trace */
    free_data (global->preserved_trace);
    DFree (global->preserved_trace);
}



/****************************** Enable COMBINING ******************************/

void sig_modify_en_signal (
    Trace	*trace,
    Signal	*en_sig_ptr,
    Signal	*base_sig_ptr,
    Boolean_t	is_cosmos)
{
    Signal	*new_sig_ptr;
    Value_t	*base_cptr, *en_cptr;
    Value_t	new_value, base_value, en_value;
    int		has_ones;
    int		has_zeros;
    Boolean_t	first_last_data=TRUE;
    
    /*DTPRINT = (!strncmp(base_sig_ptr->signame, "k->MEMDATA", 10));*/

    if (DTPRINT_FILE) printf ("sig_modify_en_signal %s + %s -> %s\n", en_sig_ptr->signame,
			      base_sig_ptr->signame, base_sig_ptr->signame);

    if (en_sig_ptr->bits != base_sig_ptr->bits) {
	printf ("sig_modify_en_signal, can't combine different sizes of %s and %s!!!\n",
		base_sig_ptr->signame, base_sig_ptr->signame);
	return;
    }
    
    new_sig_ptr = sig_replicate (trace, base_sig_ptr);
    /* Forget this is a copy, and allocate new data storage space */
    new_sig_ptr->signame = strdup (new_sig_ptr->signame);
    new_sig_ptr->copyof = NULL;
    new_sig_ptr->blocks = BLK_SIZE;
    new_sig_ptr->bptr = (Value_t *)XtMalloc ((new_sig_ptr->blocks*sizeof(uint_t))
						+ (sizeof(Value_t)*2 + 2));
    val_zero (new_sig_ptr->bptr);	/* So we know is empty */
    new_sig_ptr->cptr = new_sig_ptr->bptr;

    base_cptr = base_sig_ptr->bptr;
    en_cptr = en_sig_ptr->bptr;
    val_copy (&base_value, base_cptr);
    val_copy (&en_value, en_cptr);

    has_ones = has_zeros = 0;
    while ((CPTR_TIME(base_cptr) != EOT) 
	   && (CPTR_TIME(en_cptr) != EOT)) {
	/*if (DTPRINT_FILE) {
	    printf ("BASE "); print_cptr (base_cptr);
	    printf ("EN "); print_cptr (en_cptr);
	    }*/
	 
	if (CPTR_TIME(base_cptr) == CPTR_TIME(en_cptr)) {
	    val_copy (&en_value, en_cptr);
	    val_copy (&base_value, base_cptr);
	}
	else if (CPTR_TIME(base_cptr) < CPTR_TIME(en_cptr)) {
	    val_copy (&base_value, base_cptr);
	}
	else {
	    val_copy (&en_value, en_cptr);
	}

	/** start of determining new value **/
	switch (en_value.siglw.stbits.state) {
	case STATE_0:
	    has_ones = 0;
	    has_zeros = 1;
	    break;
	case STATE_1:
	    has_ones = 1;
	    has_zeros = 0;
	    break;
	case STATE_B32:
	case STATE_B128:
	    has_ones =
		( (en_value.number[0] & en_sig_ptr->value_mask[0])
		 | (en_value.number[1] & en_sig_ptr->value_mask[1])
		 | (en_value.number[2] & en_sig_ptr->value_mask[2])
		 | (en_value.number[3] & en_sig_ptr->value_mask[3]) );
	    has_zeros =
		(  ((~ en_value.number[0]) & en_sig_ptr->value_mask[0])
		 | ((~ en_value.number[1]) & en_sig_ptr->value_mask[1])
		 | ((~ en_value.number[2]) & en_sig_ptr->value_mask[2])
		 | ((~ en_value.number[3]) & en_sig_ptr->value_mask[3]) );
	    break;
	} /* switch */

	/* printf ("has0=%d has1=%d\n", has_zeros, has_ones); */

	val_copy (&new_value, &base_value);

	if (is_cosmos) {
	    if (has_ones) {
		/* Cosmos	enable means force U	-> U */
		new_value.siglw.stbits.state = STATE_U;
	    }
	}
	else {
	    if (has_zeros) {
		if (has_ones) {
		    /* Non-cosmos	mixed enables	-> U/F32/F128 */
		    new_value.siglw.stbits.state = STATE_U;
		    /*WPSFIX*/
		}
		else {
		    /* Non-cosmos	zero enables	-> Z */
		    new_value.siglw.stbits.state = STATE_Z;
		}
	    }
	}
	/** end of determining new value **/

	/* Calc time */
	if (CPTR_TIME(base_cptr) == CPTR_TIME(en_cptr)) {
	    new_value.time = CPTR_TIME(base_cptr);
	    base_cptr = CPTR_NEXT(base_cptr);
	    en_cptr = CPTR_NEXT(en_cptr);
	}
	else if (CPTR_TIME(base_cptr) < CPTR_TIME(en_cptr)) {
	    new_value.time = CPTR_TIME(base_cptr);
	    base_cptr = CPTR_NEXT(base_cptr);
	}
	else {
	    new_value.time = CPTR_TIME(en_cptr);
	    en_cptr = CPTR_NEXT(en_cptr);
	}

	if ((CPTR_TIME(base_cptr) == EOT) && (CPTR_TIME(en_cptr) == EOT)) {
	    first_last_data=TRUE;
	}
	/*if (DTPRINT_FILE) {
	    printf ("%s Time %d, type = %d%s\n",
		    new_sig_ptr->signame,
		    CPTR_TIME(&new_value),
		    new_value.siglw.stbits.state,
		    (first_last_data?", LAST_DATA": ""));
		    }*/
	fil_add_cptr (new_sig_ptr, &new_value, first_last_data);
	first_last_data=FALSE;
    }

    /* Add ending data & EOT marker */
    new_value.time = EOT;
    fil_add_cptr (new_sig_ptr, &new_value, TRUE);
    new_sig_ptr->cptr = new_sig_ptr->bptr;

    add_signal_to_queue (trace, new_sig_ptr, base_sig_ptr);

    /*if (DTPRINT_FILE) {
	print_sig_info (new_sig_ptr);
	printf ("\n");
	}*/

    if (! global->save_enables) {
	sig_free (trace, base_sig_ptr, FALSE, FALSE);
	sig_free (trace, en_sig_ptr, FALSE, FALSE);
	trace->numsig-=2;
    }
}

void strcpy_overlap (
    char *d,
    char *s)
{
    while ((*d++ = *s++)) ;
}

void sig_modify_enables (
    Trace	*trace)
{
    Signal	*sig_ptr, *en_sig_ptr, *base_sig_ptr;
    char	*tp, *nonenablename;
    Boolean_t	did_one=FALSE;
    Boolean_t	is_cosmos=FALSE;

    for (sig_ptr = trace->firstsig; sig_ptr; ) {
	for (tp=sig_ptr->signame; *tp; tp++) {
	    if (tp[0]=='_' && tp[1]=='_'
		&& ( ( tp[2]=='e' && tp[3]=='n')
		    || (tp[2]=='E' && tp[3]=='N')
		    || ( tp[2]=='i' && tp[3]=='n' && tp[4]=='e' && tp[5]=='n')
		    || ( tp[2]=='I' && tp[3]=='N' && tp[4]=='E' && tp[5]=='N')
		    || ( tp[2]=='c' && tp[3]=='o' && tp[4]=='s')
		    || ( tp[2]=='C' && tp[3]=='O' && tp[4]=='S') ))
		break;
	}
	if (*tp) {
	    /* Got enable! */

	    /* if (DTPRINT_FILE) printf ("Checking %s\n", sig_ptr->signame); */
	    nonenablename = strdup (sig_ptr->signame);

	    /* Chop _en from the middle of the name (sig__en<xx> -> sig<xx>)*/
	    if (tp[2]=='e' || tp[2]=='E') {
		is_cosmos = FALSE;
		strcpy_overlap (nonenablename + (tp - sig_ptr->signame), 
				nonenablename + (tp - sig_ptr->signame) + 4);
	    }
	    else if (tp[2]=='c' || tp[2]=='C') {
		is_cosmos = TRUE;
		strcpy_overlap (nonenablename + (tp - sig_ptr->signame), 
				nonenablename + (tp - sig_ptr->signame) + 5);
	    }
	    else {
		/* (sig__inen<xx> -> sig__in<xx>)*/
		is_cosmos = FALSE;
		strcpy_overlap (nonenablename + (tp - sig_ptr->signame) + 4, 
				nonenablename + (tp - sig_ptr->signame) + 6);
	    }
	    
	    base_sig_ptr = sig_find_signame (trace, nonenablename);

	    en_sig_ptr = sig_ptr;

	    /* Point to next signal, as we will be deleting several, */
	    /* make sure we don't point at one being deleted */
	    sig_ptr = sig_ptr->forward;
	    while (sig_ptr && ((sig_ptr == en_sig_ptr) || (sig_ptr==base_sig_ptr))) {
		sig_ptr = sig_ptr->forward;
	    }

	    free (nonenablename);

	    if (base_sig_ptr) {
		sig_modify_en_signal (trace, en_sig_ptr, base_sig_ptr, is_cosmos);
		did_one = TRUE;
	    }
	}
	else {
	    sig_ptr = sig_ptr->forward;
	}
    }

    if (did_one) {
	if (DTPRINT_FILE) printf ("Done sig_modify_enables\n");
	/*read_mark_cptr_end (trace);*/
    }
}


