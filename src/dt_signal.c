#ident "$Id$"
/******************************************************************************
 * DESCRIPTION: Dinotrace source: signal handling, sarching, etc
 *
 * This file is part of Dinotrace.
 *
 * Author: Wilson Snyder <wsnyder@wsnyder.org>
 *
 * Code available from: http://www.veripool.com/dinotrace
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

extern void sig_sel_ok_cb(Widget w, Trace_t *trace, XmAnyCallbackStruct *cb);
extern void sig_sel_apply_cb(Widget w, Trace_t *trace, XmAnyCallbackStruct *cb);
extern void sig_add_sel_cb(Widget w, Trace_t *trace, XmSelectionBoxCallbackStruct *cb);
extern void sig_sel_add_all_cb(Widget w, Trace_t *trace, XmAnyCallbackStruct *cb);
extern void sig_sel_add_list_cb(Widget w, Trace_t *trace, XmListCallbackStruct *cb);
extern void sig_sel_del_all_cb(Widget w, Trace_t *trace, XmAnyCallbackStruct *cb);
extern void sig_sel_del_list_cb(Widget w, Trace_t *trace, XmListCallbackStruct *cb);
extern void sig_sel_pattern_cb (Widget, Trace_t*, XmAnyCallbackStruct*);
extern void sig_sel_del_const_cb(Widget, Trace_t*, XmAnyCallbackStruct*);
extern void sig_sel_del_const_xz_cb(Widget, Trace_t*, XmAnyCallbackStruct*);
extern void sig_sel_sort_cb(Widget, Trace_t*, XmAnyCallbackStruct*);
extern void sig_sel_sort_base_cb(Widget, Trace_t*, XmAnyCallbackStruct*);


/****************************** UTILITIES ******************************/

void sig_free (
    /* Free a signal structure, and unlink all traces of it */
    Trace_t	*trace,
    Signal_t	*sig_ptr,	/* Pointer to signal to be deleted */
    Boolean_t	select,		/* True = selectively pick trace's signals from the list */
    Boolean_t	recursive)	/* True = recursively do the entire list */
{
    Signal_t	*del_sig_ptr;
    Trace_t	*trace_ptr;

    /* loop and free signal data and each signal structure */
    while (sig_ptr) {
	if (!select || sig_ptr->trace == trace) {
	    /* Check head pointers, Including deleted */
	    for (trace_ptr = global->deleted_trace_head; trace_ptr; trace_ptr = trace_ptr->next_trace) {
		if ( sig_ptr == trace_ptr->dispsig )
		    trace_ptr->dispsig = sig_ptr->forward;
		if ( sig_ptr == trace_ptr->firstsig )
		    trace_ptr->firstsig = sig_ptr->forward;
		if ( sig_ptr == trace_ptr->lastsig )
		    trace_ptr->lastsig = sig_ptr->backward;
	    }

	    /* free the signal data */
	    del_sig_ptr = sig_ptr;

	    if (sig_ptr->forward)
		((Signal_t *)(sig_ptr->forward))->backward = sig_ptr->backward;
	    if (sig_ptr->backward)
		((Signal_t *)(sig_ptr->backward))->forward = sig_ptr->forward;
	    sig_ptr = sig_ptr->forward;

	    /* free the signal structure */
	    if (del_sig_ptr->copyof == NULL) {
		DFree (del_sig_ptr->bptr);
		DFree (del_sig_ptr->signame);
		DFree (del_sig_ptr->xsigname);
		DFree (del_sig_ptr->note);
	    }
	    DFree (del_sig_ptr);
	}
	else {
	    sig_ptr = sig_ptr->forward;
	}
	if (!recursive) sig_ptr=NULL;
    }
}


static void sig_remove_from_queue (
    Trace_t	*trace,
    Signal_t	*sig_ptr)	/* Signal to remove */
    /* Removes the signal from the current and any other ques that it is in */
{
    Signal_t	*next_sig_ptr, *prev_sig_ptr;

    if (DTPRINT_ENTRY) printf ("In sig_remove_from_queue - trace=%p sig %p\n",trace,sig_ptr);

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
    /* if the signal is the last signal, change it */
    if ( sig_ptr == trace->lastsig ) {
	trace->lastsig = sig_ptr->backward;
    }

    trace->numsig--;
}

#define ADD_LAST ((Signal_t *)(-1))
static void    sig_add_to_queue (
    Trace_t	*trace,
    Signal_t	*sig_ptr,	/* Signal to add */
    Signal_t	*loc_sig_ptr)	/* Pointer to signal ahead of one to add, NULL=1st, ADD_LAST=last */
{
    Signal_t	*next_sig_ptr, *prev_sig_ptr;

    if (DTPRINT_ENTRY) printf ("In sig_add_to_queue - trace=%p   loc=%p%s\n",trace,
			       loc_sig_ptr, loc_sig_ptr==ADD_LAST?"=last":"");

    if (sig_ptr==loc_sig_ptr) {
	loc_sig_ptr = loc_sig_ptr->forward;
    }

    /*printf ("iPrev %d  next %d  sig %d  top %d\n", prev_sig_ptr, next_sig_ptr, sig_ptr, *top_pptr);*/

    /* Insert into first position? */
    if (loc_sig_ptr == NULL) {
	next_sig_ptr = trace->firstsig;
	prev_sig_ptr = NULL;
    }
    else if (loc_sig_ptr == ADD_LAST) {
	next_sig_ptr = NULL;
	prev_sig_ptr = trace->lastsig;
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

    if (!prev_sig_ptr) trace->firstsig = sig_ptr;
    if (!next_sig_ptr) trace->lastsig = sig_ptr;

    /* if the signal is the first screen signal, change it */
    if ( next_sig_ptr && ( next_sig_ptr == trace->dispsig )) {
        trace->dispsig = sig_ptr;
    }
    /* if no display sig, but is regular first sig, make the display sig */
    if ( trace->firstsig && !trace->dispsig) {
	trace->dispsig = trace->firstsig;
    }
}

Signal_t *sig_replicate (
    Trace_t	*trace,
    Signal_t	*sig_ptr)	/* Signal to remove */
    /* Makes a duplicate copy of the signal */
{
    Signal_t	*new_sig_ptr;

    if (DTPRINT_ENTRY) printf ("In sig_replicate - trace=%p\n",trace);

    /* Create new structure */
    new_sig_ptr = XtNew (Signal_t);

    /* Copy data */
    memcpy (new_sig_ptr, sig_ptr, sizeof (Signal_t));

    /* Erase new links */
    new_sig_ptr->forward = NULL;
    new_sig_ptr->backward = NULL;
    if (sig_ptr->copyof)
	new_sig_ptr->copyof = sig_ptr->copyof;
    else new_sig_ptr->copyof = sig_ptr;
    new_sig_ptr->file_copy = FALSE;

    return (new_sig_ptr);
}

/* Returns basename of signal */
char *sig_basename (
    const Trace_t *trace,
    const Signal_t	*sig_ptr)
{
    char *basename;
    /* First is the basename with hierarchy and bus bits stripped */
    basename = strrchr ((sig_ptr->signame_buspos ?
			 sig_ptr->signame_buspos : sig_ptr->signame),
			trace->dfile.hierarchy_separator);
    if (!basename || basename[0]=='\0') basename = sig_ptr->signame;
    else basename++;
    return (basename);
}

/* Returns Signal or NULL if not found, starting at specified signal */
static Signal_t* sig_find_signame_start (
    Signal_t*	start_sig_ptr,
    const char*	signame)
{
    Signal_t*	sig_ptr;
    for (sig_ptr = start_sig_ptr; sig_ptr; sig_ptr = sig_ptr->forward) {
	if (!strcmp (sig_ptr->signame, signame)) return (sig_ptr);
    }
    return (NULL);
}

/* Returns Signal or NULL if not found, starting at specified signal
going backwards */
static Signal_t* sig_find_signame_start_backward (
    Signal_t* start_sig_ptr,
    const char*       signame)
{
    Signal_t* sig_ptr;
    for (sig_ptr = start_sig_ptr; sig_ptr; sig_ptr = sig_ptr->backward) {
	if (!strcmp (sig_ptr->signame, signame)) return (sig_ptr);
    }
    return (NULL);
}

/* Returns Signal or NULL if not found */
Signal_t* sig_find_signame (
    const Trace_t*	trace,
    const char*	signame)
{
    return sig_find_signame_start(trace->firstsig, signame);
}

/* Returns Signal or NULL if not found */
Signal_t *sig_wildmat_signame (
    const Trace_t	*trace,
    const char	*signame)
{
    Signal_t	*sig_ptr;

    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	if (wildmat (sig_ptr->signame, signame)) {
	    return (sig_ptr);
	}
    }
    return (NULL);
}

void	sig_wildmat_clear (void)
{
    SignalList_t *siglst_ptr;

    /* Erase existing selections */
    while (global->select_head) {
	siglst_ptr = global->select_head;
	global->select_head = global->select_head->forward;
	DFree (siglst_ptr);
    }

}

void	sig_wildmat_add (Trace_t *trace, Signal_t *sig_ptr)
{
    SignalList_t *siglst_ptr;

    /* printf ("Selected: %s\n", sig_ptr->signame); */
    siglst_ptr = XtNew (SignalList_t);
    siglst_ptr->trace = trace;
    siglst_ptr->signal = sig_ptr;
    siglst_ptr->forward = global->select_head;
    global->select_head = siglst_ptr;
}

void	sig_wildmat_select (
    /* Create list of selected signals */
    Trace_t	  	*trace,		/* NULL= do all traces */
    const char		*pattern)
{
    Signal_t		*sig_ptr;
    Boolean_t		trace_list;

    trace_list = (trace == NULL);

    sig_wildmat_clear();

    /*printf ("Pattern: %s\n", pattern);*/
    if (trace_list) trace = global->deleted_trace_head;
    for (; trace; trace = (trace_list ? trace->next_trace : NULL)) {
	for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	    if (wildmat (sig_ptr->signame, pattern)) {
		/*printf ("  Selected: %s\n", sig_ptr->signame);*/
		sig_wildmat_add (trace, sig_ptr);
	    }
	}
    }
}

void	sig_wildmat_select_color (
    /* Create list of signals that are not highlighed */
    Trace_t	  	*trace,
    int			color)
{
    Signal_t		*sig_ptr;

    sig_wildmat_clear();

    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	if (sig_ptr->color == color) {
	    sig_wildmat_add (trace, sig_ptr);
	}
    }
}

void	sig_move (
    Trace_t	*old_trace,
    Signal_t	*sig_ptr,	/* Signal to move */
    Trace_t	*new_trace,
    Signal_t	*after_sig_ptr)	/* Signal to place after or ADD_LAST */
{

    if (sig_ptr) {
	sig_remove_from_queue (old_trace, sig_ptr);
	sig_ptr->deleted = FALSE;
	sig_add_to_queue (new_trace, sig_ptr, after_sig_ptr);
    }

#if 0
    printf ("Adding %s\n", sig_ptr->signame);

    printf ("Post\n");
    debug_integrity_check_cb (NULL, NULL, NULL);
    printf ("Done\n");
#endif
}

static void	sig_delete (
    Trace_t	*trace,
    Signal_t	*sig_ptr,	/* Signal to remove */
    Boolean_t	preserve,	/* TRUE if should preserve deletion on rereading */
    Signal_t	*after_sig_ptr)	/* Signal to place after or ADD_LAST */
    /* Delete the given signal */
{
    if (sig_ptr) {
	sig_move (trace, sig_ptr, global->deleted_trace_head, after_sig_ptr);
	sig_ptr->deleted = TRUE;
	sig_ptr->deleted_preserve = preserve;
    }
}

void	sig_copy (
    Trace_t	*old_trace,
    Signal_t	*sig_ptr,	/* Signal to move */
    Trace_t	*new_trace,
    Signal_t	*after_sig_ptr)	/* Signal to place after or ADD_LAST */
{
    Signal_t	*new_sig_ptr;

    if (sig_ptr) {
	if (sig_ptr->deleted) {
	    sig_move (old_trace, sig_ptr, new_trace, after_sig_ptr);
	}
	else {
	    new_sig_ptr = sig_replicate (old_trace, sig_ptr);
	    sig_ptr->deleted = FALSE;
	    sig_add_to_queue (new_trace, new_sig_ptr, after_sig_ptr);
	}
    }
}

void	sig_update_search ()
{
    Trace_t	*trace;
    Signal_t	*sig_ptr;
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

#if 0 /* Unused */
static Boolean_t sig_is_same (
    const Signal_t	*siga_ptr,
    const Signal_t	*sigb_ptr)
{
    Value_t	*captr;
    Value_t	*cbptr;

    printf ("Check %s %s\n", siga_ptr->signame, sigb_ptr->signame);
    /* Is there a transition? */
    for (captr = siga_ptr->bptr,
	 cbptr = sigb_ptr->bptr;
	 CPTR_TIME(captr) != EOT
	     && CPTR_TIME(cbptr) != EOT;
	 captr = CPTR_NEXT(captr),
	 cbptr = CPTR_NEXT(cbptr)) {
	if (CPTR_TIME(cbptr) != CPTR_TIME(captr)
	    || !val_equal (captr, cbptr)) {
	    return (FALSE);
	}
    }

    if (CPTR_TIME(cbptr) != CPTR_TIME(captr)) return (FALSE);
    return (TRUE);
}

static void sig_count (
    const Trace_t *trace)
{
    Signal_t	*sig_ptr;
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	Value_t	*cptr;
	int cnt = 0;
	for (cptr = sig_ptr->bptr ; CPTR_TIME(cptr) != EOT; cptr = CPTR_NEXT(cptr)) {
	    cnt ++;
	}
    }
}
#endif

static Boolean_t sig_is_constant (
    const Trace_t	*trace,
    const Signal_t	*sig_ptr,
    Boolean_t	ignorexz)		/* TRUE = ignore xz */
{
    Boolean_t 	changes;
    Value_t	*cptr;
    Value_t	old_value;
    Boolean_t	old_got_value;

    /* Is there a transition? */
    changes=FALSE;
    old_got_value = FALSE;
    if (!sig_ptr || !sig_ptr->bptr) abort();
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
    Trace_t	*trace)
{
    Signal_t	*sig_ptr, *back_sig_ptr;

    if (DTPRINT_ENTRY) printf ("In print_sig_names\n");

    printf ("  Number of signals = %d\n", trace->numsig);

    /* loop thru each signal */
    back_sig_ptr = NULL;
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	printf (" Sig '%s'  ty=%d index=%d-%d, btyp=%x bpos=%d bcode=%d bits=%d copy=%s\n",
		sig_ptr->signame, sig_ptr->type,
		sig_ptr->msb_index ,sig_ptr->lsb_index,
		sig_ptr->file_type.flags, sig_ptr->file_pos, sig_ptr->file_code, sig_ptr->bits,
		(sig_ptr->copyof?"Y":"N")
		);
	if (sig_ptr->backward != back_sig_ptr) {
	    printf (" %%E, Backward link is to '%p' not '%p'\n", sig_ptr->backward, back_sig_ptr);
	}
	back_sig_ptr = sig_ptr;
    }

    /* Don't do a integrity check here, as sometimes all links aren't ready! */
    /* signalstates_dump (trace); */
}

/****************************** CONFIG FUNCTIONS ******************************/


void    sig_highlight_selected (
    int		color)
{
    Signal_t	*sig_ptr;
    SignalList_t	*siglst_ptr;

    if (DTPRINT_ENTRY) printf ("In sig_highlight_selected\n");

    for (siglst_ptr = global->select_head; siglst_ptr; siglst_ptr = siglst_ptr->forward) {
	sig_ptr = siglst_ptr->signal;
	/* Change the color */
	sig_ptr->color = color;
	sig_ptr->search = 0;
	if (sig_ptr->copyof) {
	    sig_ptr->copyof->color = color;
	    sig_ptr->copyof->search = 0;
	}
    }

    draw_all_needed ();
}

void    sig_radix_selected (
    Radix_t	*radix_ptr)
{
    Signal_t	*sig_ptr;
    SignalList_t	*siglst_ptr;

    if (DTPRINT_ENTRY) printf ("In sig_radix_selected\n");

    for (siglst_ptr = global->select_head; siglst_ptr; siglst_ptr = siglst_ptr->forward) {
	sig_ptr = siglst_ptr->signal;
	/* Change the color */
	sig_ptr->radix = radix_ptr;
    }

    draw_all_needed ();
}

void    sig_waveform_selected (
    Waveform_t	waveform)
{
    Signal_t	*sig_ptr;
    SignalList_t	*siglst_ptr;

    if (DTPRINT_ENTRY) printf ("In sig_waveform_selected\n");

    for (siglst_ptr = global->select_head; siglst_ptr; siglst_ptr = siglst_ptr->forward) {
	sig_ptr = siglst_ptr->signal;
	sig_ptr->waveform = waveform;
    }

    draw_all_needed ();
}

void    sig_move_selected (
    /* also used for adding deleted signals */
    Trace_t	*new_trace,
    const char	*after_pattern)
{
    Trace_t	*old_trace;
    Signal_t	*sig_ptr, *after_sig_ptr;
    SignalList_t	*siglst_ptr;

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
    const char	*new_name)
{
    Signal_t	*sig_ptr;
    SignalList_t	*siglst_ptr;

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
    Trace_t	*new_trace,
    const char	*after_pattern)
{
    Trace_t	*old_trace;
    Signal_t	*sig_ptr, *after_sig_ptr;
    SignalList_t	*siglst_ptr;

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
    Trace_t	*trace;
    Signal_t	*sig_ptr;
    SignalList_t	*siglst_ptr;


    if (DTPRINT_ENTRY) printf ("In sig_delete_selected %d %d\n", constant_flag, ignorexz);

    for (siglst_ptr = global->select_head; siglst_ptr; siglst_ptr = siglst_ptr->forward) {
	sig_ptr = siglst_ptr->signal;
	trace = siglst_ptr->trace;
	if (!sig_ptr || !sig_ptr->bptr) abort();
	if (sig_ptr==NULL) abort();
	if  ( constant_flag || sig_is_constant (trace, sig_ptr, ignorexz)) {
	    sig_delete (trace, sig_ptr, constant_flag, ADD_LAST );
	}
    }

    draw_needupd_sig_start ();
    draw_all_needed ();
}


void    sig_note (
    Signal_t* sig_ptr,
    const char *note)
{
    sig_ptr->note = strdup (note);
    /*draw_all_needed (); not visible, so don't bother*/
}

void    sig_note_selected (
    const char *note)
{
    Signal_t	*sig_ptr;
    SignalList_t	*siglst_ptr;

    if (DTPRINT_ENTRY) printf ("In sig_note_selected\n");

    for (siglst_ptr = global->select_head; siglst_ptr; siglst_ptr = siglst_ptr->forward) {
	sig_ptr = siglst_ptr->signal;
	sig_note (sig_ptr,note);
    }
    /*draw_all_needed (); not visible, so don't bother*/
}


void    sig_goto_pattern (
    Trace_t	*trace,
    const char	*pattern)
{
    Signal_t	*sig_ptr;
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
    const Trace_t	*trace,
    const Signal_t	*sig_ptr)
{
    static char	strg[2000];
    char	strg2[2000];

    if (DTPRINT_ENTRY) printf ("val_examine_popup_sig_string\n");

    strcpy (strg, sig_ptr->signame);

    sprintf (strg2, "\nRadix: %s\n", sig_ptr->radix->name);
    strcat (strg, strg2);
    if (sig_ptr->note) {
        strcat (strg, sig_ptr->note);
        strcat (strg, "\n");
    }

    /* Debugging information */
    if (DTDEBUG) {
	if (sig_ptr->copyof) {
	    sprintf (strg2, "\nCopy-of %s\n",
		     sig_ptr->copyof->signame);
	    strcat (strg, strg2);
	}
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
    Trace_t *trace = widget_to_trace(w);
    Signal_t	*sig_ptr;
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
    Trace_t	*trace,
    XmSelectionBoxCallbackStruct *cb)
{
    Signal_t	*sig_ptr;

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
	set_cursor (DC_SIG_ADD);
	add_event (ButtonPressMask, sig_add_ev);

	/* unmanage the popup on the screen - we'll pop it back later */
	XtUnmanageChild (trace->signal.add);
    }
    else {
	global->selected_sig = NULL;
    }
}

void    sig_cancel_cb (
    Widget	w)
{
    Widget	list_wid;
    Trace_t *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In sig_cancel_cb - trace=%p\n",trace);

    /* remove any previous events */
    remove_all_events (trace);

    /* unmanage the popup on the screen */
    XtUnmanageChild (trace->signal.add);

    /* cleanup list in case the trace gets reloaded */
    list_wid = XmSelectionBoxGetChild (trace->signal.add, XmDIALOG_LIST);
    XmListDeleteAllItems (list_wid);
}

void    sig_mov_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In sig_mov_cb - trace=%p\n",trace);

    /* guarantee next button press selects a signal to be moved */
    global->selected_sig = NULL;

    /* process all subsequent button presses as signal moves */
    remove_all_events (trace);
    add_event (ButtonPressMask, sig_move_ev);
    set_cursor (DC_SIG_MOVE_1);
}

void    sig_copy_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In sig_copy_cb - trace=%p\n",trace);

    /* guarantee next button press selects a signal to be moved */
    global->selected_sig = NULL;

    /* process all subsequent button presses as signal moves */
    remove_all_events (trace);
    add_event (ButtonPressMask, sig_copy_ev);
    set_cursor (DC_SIG_COPY_1);
}

void    sig_del_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In sig_del_cb - trace=%p\n",trace);

    /* process all subsequent button presses as signal deletions */
    remove_all_events (trace);
    set_cursor (DC_SIG_DELETE);
    add_event (ButtonPressMask, sig_delete_ev);
}

void    sig_radix_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    int radixnum = submenu_to_color (trace, w, 0, trace->menu.sig_radix_pds);
    if (DTPRINT_ENTRY) printf ("In sig_radix_cb - trace=%p radixnum=%d\n",trace, radixnum);

    /* Grab color number from the menu button pointer */
    global->selected_radix = global->radixs[radixnum];

    /* process all subsequent button presses as signal deletions */
    remove_all_events (trace);
    set_cursor (DC_SIG_RADIX);
    add_event (ButtonPressMask, sig_radix_ev);
}

void    sig_waveform_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    int subnum = submenu_to_color (trace, w, 0, trace->menu.sig_waveform_pds);

    if (DTPRINT_ENTRY) printf ("In sig_waveform_digital_cb - trace=%p sub=%d\n",trace, subnum);
    global->selected_waveform = (Waveform_t)subnum;

    /* process all subsequent button presses as signal deletions */
    remove_all_events (trace);
    set_cursor (DC_SIG_RADIX);
    add_event (ButtonPressMask, sig_waveform_ev);
}

static void    sig_highlight_internal (
    Widget	w,
    ColorNum_t	overrideColor)
{
    Trace_t *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In sig_highlight_cb - trace=%p\n",trace);

    /* Grab color number from the menu button pointer */
    global->highlight_color = submenu_to_color (trace, w, overrideColor, trace->menu.sig_highlight_pds);

    /* process all subsequent button presses as signal deletions */
    remove_all_events (trace);
    set_cursor (DC_SIG_HIGHLIGHT);
    add_event (ButtonPressMask, sig_highlight_ev);
}

void    sig_highlight_cb (
    Widget	w)
{
    sig_highlight_internal(w,0);
}

void    sig_highlight_current_cb (
    Widget	w)
{
    sig_highlight_internal(w,COLOR_CURRENT);
}

void    sig_highlight_next_cb (
    Widget	w)
{
    sig_highlight_internal(w,COLOR_NEXT);
}

void    sig_highlight_clear_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In sig_highlight_clear_cb - trace=%p\n",trace);

    sig_wildmat_select (NULL, "*");
    sig_highlight_selected (0);
}

void    sig_highlight_keep_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In sig_highlight_keep_cb - trace=%p\n",trace);

    sig_wildmat_select_color (trace, 0);
    sig_delete_selected (TRUE, TRUE);
}

void    sig_note_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In sig_note_cb %p\n",trace);

    /* process all subsequent button presses as signal deletions */
    remove_all_events (trace);
    set_cursor (DC_SIG_NOTE);
    add_event (ButtonPressMask, sig_note_ev);
}

void    sig_search_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    int		i;
    Widget above;

    if (DTPRINT_ENTRY) printf ("In sig_search_cb - trace=%p\n",trace);

    if (!trace->signal.dialog) {
	XtSetArg (arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg (arglist[1], XmNdialogTitle, XmStringCreateSimple ("Signal Search Requester") );
	XtSetArg (arglist[2], XmNverticalSpacing, 0);
	XtSetArg (arglist[3], XmNhorizontalSpacing, 10);
	trace->signal.dialog = XmCreateFormDialog (trace->work,"search",arglist,4);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Color"));
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_FORM );
	XtSetArg (arglist[3], XmNtopOffset, 10);
	trace->signal.label1 = XmCreateLabel (trace->signal.dialog,"label1",arglist,4);
	DManageChild (trace->signal.label1, trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Sig"));
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopWidget, dmanage_last);
	trace->signal.label4 = XmCreateLabel (trace->signal.dialog,"label4",arglist,4);
	DManageChild (trace->signal.label4, trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNlabelString,
		 XmStringCreateSimple ("Search value, *? are wildcards" ));
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[2], XmNleftWidget, trace->signal.label4 );
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNtopWidget, trace->signal.label1);
	XtSetArg (arglist[5], XmNverticalSpacing, 0);
	trace->signal.label3 = XmCreateLabel (trace->signal.dialog,"label3",arglist,6);
	DManageChild (trace->signal.label3, trace, MC_NOKEYS);

	above = trace->signal.label3;

	for (i=0; i<MAX_SRCH; i++) {
	    /* enable button */
	    XtSetArg (arglist[0], XmNleftAttachment, XmATTACH_FORM );
	    XtSetArg (arglist[1], XmNtopAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[2], XmNtopWidget, above);
	    XtSetArg (arglist[3], XmNselectColor, trace->xcolornums[i+1]);
	    XtSetArg (arglist[4], XmNlabelString, XmStringCreateSimple (" "));  /* Else openmotif makes small button*/
	    trace->signal.enable[i] = XmCreateToggleButton (trace->signal.dialog,"togglen",arglist,5);
	    DManageChild (trace->signal.enable[i], trace, MC_NOKEYS);

	    /* create the file name text widget */
	    XtSetArg (arglist[0], XmNrows, 1);
	    XtSetArg (arglist[1], XmNcolumns, 30);
	    XtSetArg (arglist[2], XmNleftAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[3], XmNleftWidget, trace->signal.enable[i]);
	    XtSetArg (arglist[4], XmNtopAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[5], XmNtopWidget, above);
	    XtSetArg (arglist[6], XmNresizeHeight, FALSE);
	    XtSetArg (arglist[7], XmNeditMode, XmSINGLE_LINE_EDIT);
	    XtSetArg (arglist[8], XmNrightAttachment, XmATTACH_FORM );
	    trace->signal.text[i] = XmCreateText (trace->signal.dialog,"textn",arglist,9);
	    DAddCallback (trace->signal.text[i], XmNactivateCallback, sig_search_ok_cb, trace);
	    DManageChild (trace->signal.text[i], trace, MC_NOKEYS);

	    above = trace->signal.text[i];
	}

	/* Ok/apply/cancel */
	ok_apply_cancel (&trace->signal.okapply, trace->signal.dialog,
			 dmanage_last,
			 (XtCallbackProc)sig_search_ok_cb, trace,
			 (XtCallbackProc)sig_search_apply_cb, trace,
			 NULL, NULL,
			 (XtCallbackProc)unmanage_cb, (Trace_t*)trace->signal.dialog);
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
    DManageChild (trace->signal.dialog, trace, MC_NOKEYS);
}

void    sig_search_ok_cb (
    Widget	w,
    Trace_t	*trace,
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

    XtUnmanageChild (trace->signal.dialog);

    draw_needupd_sig_search ();
    draw_needupd_sig_start ();
    draw_all_needed ();
}

void    sig_search_apply_cb (
    Widget	w,
    Trace_t	*trace,
    XmSelectionBoxCallbackStruct *cb)
{
    if (DTPRINT_ENTRY) printf ("In sig_search_apply_cb - trace=%p\n",trace);

    sig_search_ok_cb (w,trace,cb);
    sig_search_cb (trace->main);
}

/****************************** EVENTS ******************************/

void    sig_add_ev (
    Widget	w,
    Trace_t	*trace,
    XButtonPressedEvent	*ev)
{
    Signal_t		*sig_ptr;

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
    Trace_t	*trace,
    XButtonPressedEvent	*ev)
{
    Signal_t	*sig_ptr;

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
	set_cursor (DC_SIG_MOVE_2);
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
	set_cursor (DC_SIG_MOVE_1);
    }

    draw_needupd_sig_start ();
    draw_all_needed ();
}

void    sig_copy_ev (
    Widget	w,
    Trace_t	*trace,
    XButtonPressedEvent	*ev)
{
    Signal_t	*sig_ptr;

    if (DTPRINT_ENTRY) printf ("In sig_copy_ev - trace=%p\n",trace);
    if (ev->type != ButtonPress || ev->button!=1) return;

    /* make sure button has been clicked in in valid location of screen */
    sig_ptr = posy_to_signal (trace, ev->y);
    if (!sig_ptr) return;

    if ( global->selected_sig == NULL ) {
	/* next call will perform the copy */
	global->selected_sig = sig_ptr;
	global->selected_trace = trace;
	set_cursor (DC_SIG_COPY_2);
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
	set_cursor (DC_SIG_COPY_1);
    }

    draw_needupd_sig_start ();
    draw_all_needed ();
}

void    sig_delete_ev (
    Widget	w,
    Trace_t	*trace,
    XButtonPressedEvent	*ev)
{
    Signal_t	*sig_ptr;

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
    sig_delete (trace, sig_ptr, TRUE, ADD_LAST);

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
    Trace_t	*trace,
    XButtonPressedEvent	*ev)
{
    Signal_t	*sig_ptr;

    if (DTPRINT_ENTRY) printf ("In sig_highlight_ev - trace=%p\n",trace);
    if (ev->type != ButtonPress || ev->button!=1) return;

    sig_ptr = posy_to_signal (trace, ev->y);
    if (!sig_ptr) return;

    /* Change the color */
    sig_ptr->color = global->highlight_color;
    sig_ptr->search = 0;
    if (sig_ptr->copyof) {
	sig_ptr->copyof->color = global->highlight_color;
	sig_ptr->copyof->search = 0;
    }

    draw_all_needed ();
}


void    sig_radix_ev (
    Widget	w,
    Trace_t	*trace,
    XButtonPressedEvent	*ev)
{
    Signal_t	*sig_ptr;

    if (DTPRINT_ENTRY) printf ("In sig_radix_ev - trace=%p\n",trace);
    if (ev->type != ButtonPress || ev->button!=1) return;

    sig_ptr = posy_to_signal (trace, ev->y);
    if (!sig_ptr) return;

    /* Change the radix */
    sig_ptr->radix = global->selected_radix;

    draw_all_needed ();
}

void    sig_waveform_ev (
    Widget	w,
    Trace_t	*trace,
    XButtonPressedEvent	*ev)
{
    Signal_t	*sig_ptr;

    if (DTPRINT_ENTRY) printf ("In sig_waveform_ev - trace=%p\n",trace);
    if (ev->type != ButtonPress || ev->button!=1) return;

    sig_ptr = posy_to_signal (trace, ev->y);
    if (!sig_ptr) return;

    /* Change the radix */
    sig_ptr->waveform = global->selected_waveform;

    draw_all_needed ();
}

void    sig_note_ev (
    Widget	w,
    Trace_t	*trace,
    XButtonPressedEvent	*ev)
{
    Signal_t	*sig_ptr;

    if (DTPRINT_ENTRY) printf ("In sig_note_ev - trace=%p\n",trace);
    if (ev->type != ButtonPress || ev->button!=1) return;

    sig_ptr = posy_to_signal (trace, ev->y);
    if (!sig_ptr) return;

    /* Get the note */
    global->selected_sig = sig_ptr;
    win_note(trace, "Note for ",sig_ptr->signame, sig_ptr->note, FALSE);
}

/****************************** SELECT OPTIONS ******************************/

void    sig_select_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In sig_select_cb - trace=%p\n",trace);

    /* return if there is no file */
    if (!trace->loaded) return;

    if (!trace->select.dialog) {
	trace->select.del_strings=NULL;
	trace->select.add_strings=NULL;
	trace->select.del_signals=NULL;
	trace->select.add_signals=NULL;

	XtSetArg (arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg (arglist[1], XmNdialogTitle, XmStringCreateSimple ("Select Signal Requester") );
	XtSetArg (arglist[2], XmNheight, 400);
	XtSetArg (arglist[3], XmNverticalSpacing, 5);
	XtSetArg (arglist[4], XmNhorizontalSpacing, 10);
	XtSetArg (arglist[5], XmNresizable, FALSE);
	trace->select.dialog = XmCreateFormDialog (trace->work,"select",arglist,6);

	/*** BUTTONS ***/

	/* Create OK button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple (" OK ") );
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNbottomAttachment, XmATTACH_FORM );
	trace->select.okapply.ok = XmCreatePushButton (trace->select.dialog,"ok",arglist,3);
	DAddCallback (trace->select.okapply.ok, XmNactivateCallback, sig_sel_ok_cb, trace);
	DManageChild (trace->select.okapply.ok, trace, MC_NOKEYS);

	/* Create apply button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Apply") );
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_POSITION );
	XtSetArg (arglist[2], XmNleftPosition, 45);
	XtSetArg (arglist[3], XmNbottomAttachment, XmATTACH_FORM );
	trace->select.okapply.apply = XmCreatePushButton (trace->select.dialog,"apply",arglist,4);
	DAddCallback (trace->select.okapply.apply, XmNactivateCallback, sig_sel_apply_cb, trace);
	DManageChild (trace->select.okapply.apply, trace, MC_NOKEYS);

	/* cancel button is broken */

	/* Create Separator */
	XtSetArg (arglist[0], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[1], XmNrightAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNbottomAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNbottomWidget, trace->select.okapply.ok );
	XtSetArg (arglist[4], XmNbottomOffset, 10);
	trace->select.okapply.sep = XmCreateSeparator (trace->select.dialog, "sep",arglist,5);
	DManageChild (trace->select.okapply.sep, trace, MC_NOKEYS);

	/*** Add (deleted list) section ***/
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Add Signal Pattern"));
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_FORM );
	trace->select.label2 = XmCreateLabel (trace->select.dialog,"label2",arglist,3);
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
	trace->select.add_pat = XmCreateText (trace->select.dialog,"dpat",arglist,10);
	DAddCallback (trace->select.add_pat, XmNactivateCallback, sig_sel_pattern_cb, trace);
	DManageChild (trace->select.add_pat, trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Add All") );
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNbottomAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNbottomWidget, trace->select.okapply.sep);
	trace->select.add_all = XmCreatePushButton (trace->select.dialog,"aall",arglist,4);
	XtRemoveAllCallbacks (trace->select.add_all, XmNactivateCallback);
	DAddCallback (trace->select.add_all, XmNactivateCallback, sig_sel_add_all_cb, trace);
	DManageChild (trace->select.add_all, trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNlabelString, string_create_with_cr ("Select a signal to add it\nto the displayed signals."));
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopWidget, trace->select.add_pat);
	trace->select.label1 = XmCreateLabel (trace->select.dialog,"label1",arglist,4);
	DManageChild (trace->select.label1, trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[1], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[2], XmNtopWidget, trace->select.label1);
	XtSetArg (arglist[3], XmNbottomAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNbottomWidget, trace->select.add_all);
	XtSetArg (arglist[5], XmNrightAttachment, XmATTACH_POSITION );
	XtSetArg (arglist[6], XmNrightPosition, 50);
	trace->select.add_sigs_form = XmCreateForm (trace->select.dialog, "add_sigs_form", arglist, 7);
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
	trace->select.label4 = XmCreateLabel (trace->select.dialog,"label4",arglist,4);
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
	trace->select.delete_pat = XmCreateText (trace->select.dialog,"apat",arglist,9);
	DAddCallback (trace->select.delete_pat, XmNactivateCallback, sig_sel_pattern_cb, trace);
	DManageChild (trace->select.delete_pat, trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Sort Wholename") );
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[2], XmNleftWidget, trace->select.add_sigs_form);
	XtSetArg (arglist[3], XmNbottomAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNbottomWidget, trace->select.okapply.sep);
	trace->select.sort_full = XmCreatePushButton (trace->select.dialog,"dcon",arglist,5);
	XtRemoveAllCallbacks (trace->select.sort_full, XmNactivateCallback);
	DAddCallback (trace->select.sort_full, XmNactivateCallback, sig_sel_sort_cb, trace);
	DManageChild (trace->select.sort_full, trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Sort Basename") );
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[2], XmNleftWidget, trace->select.sort_full);
	XtSetArg (arglist[3], XmNbottomAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNbottomWidget, trace->select.okapply.sep);
	trace->select.sort_nopath = XmCreatePushButton (trace->select.dialog,"dconxz",arglist,5);
	XtRemoveAllCallbacks (trace->select.sort_nopath, XmNactivateCallback);
	DAddCallback (trace->select.sort_nopath, XmNactivateCallback, sig_sel_sort_base_cb, trace);
	DManageChild (trace->select.sort_nopath, trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Delete Constants") );
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[2], XmNleftWidget, trace->select.add_sigs_form);
	XtSetArg (arglist[3], XmNbottomAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNbottomWidget, trace->select.sort_full);
	trace->select.delete_const = XmCreatePushButton (trace->select.dialog,"dcon",arglist,5);
	XtRemoveAllCallbacks (trace->select.delete_const, XmNactivateCallback);
	DAddCallback (trace->select.delete_const, XmNactivateCallback, sig_sel_del_const_cb, trace);
	DManageChild (trace->select.delete_const, trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Delete Const or X/Z") );
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[2], XmNleftWidget, trace->select.delete_const);
	XtSetArg (arglist[3], XmNbottomAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNbottomWidget, trace->select.sort_full);
	trace->select.delete_const_xz = XmCreatePushButton (trace->select.dialog,"dconxz",arglist,5);
	XtRemoveAllCallbacks (trace->select.delete_const_xz, XmNactivateCallback);
	DAddCallback (trace->select.delete_const_xz, XmNactivateCallback, sig_sel_del_const_xz_cb, trace);
	DManageChild (trace->select.delete_const_xz, trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Delete All") );
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[2], XmNleftWidget, trace->select.add_sigs_form);
	XtSetArg (arglist[3], XmNbottomAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNbottomWidget, trace->select.delete_const);
	trace->select.delete_all = XmCreatePushButton (trace->select.dialog,"dall",arglist,5);
	XtRemoveAllCallbacks (trace->select.delete_all, XmNactivateCallback);
	DAddCallback (trace->select.delete_all, XmNactivateCallback, sig_sel_del_all_cb, trace);
	DManageChild (trace->select.delete_all, trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNlabelString, string_create_with_cr ("Select a signal to delete it\nfrom the displayed signals."));
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[2], XmNleftWidget, trace->select.add_sigs_form);
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNtopWidget, trace->select.delete_pat);
	trace->select.label3 = XmCreateLabel (trace->select.dialog,"label3",arglist,5);
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
	trace->select.delete_sigs = XmCreateScrolledList (trace->select.dialog,"",arglist,11);
	DAddCallback (trace->select.delete_sigs, XmNextendedSelectionCallback, sig_sel_del_list_cb, trace);
	DManageChild (trace->select.delete_sigs, trace, MC_NOKEYS);
    }

    /* manage the popup on the screen */
    DManageChild (trace->select.dialog, trace, MC_NOKEYS);

    /* update patterns - leave under the "manage" or toolkit will complain */
    sig_sel_pattern_cb (NULL, trace, NULL);
}

static void    sig_sel_update_pattern (
    Widget	w,			/* List widget to update */
    Signal_t	*head_sig_ptr,		/* Head signal in the list */
    char	*pattern,		/* Pattern to match to the list */
    XmString	**xs_list,		/* Static storage for string list */
    Signal_t	***xs_sigs,		/* Static storage for signal list */
    uint_t	*xs_size)
{
    Signal_t	*sig_ptr;
    uint_t	sel_count;

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
	*xs_sigs = (Signal_t **)XtMalloc (sel_count * sizeof (Signal_t *));
	*xs_size = sel_count;
    }
    else if (*xs_size < sel_count) {
	*xs_list = (XmString *)XtRealloc ((char*)*xs_list, sel_count * sizeof (XmString));
	*xs_sigs = (Signal_t **)XtRealloc ((char*)*xs_sigs, sel_count * sizeof (Signal_t *));
	*xs_size = sel_count;
    }

    /* go through the list again and make the array */
    sel_count = 0;
    for (sig_ptr = head_sig_ptr; sig_ptr; sig_ptr = sig_ptr->forward) {
	if ((pattern[0] == '*' && pattern[1] == '\0')
	    || wildmat (sig_ptr->signame, pattern)) {
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
    Trace_t	*trace,
    XmAnyCallbackStruct		*cb)
    /* Called by creation or call back - create the list of signals with a possibly new pattern */
{
    char	*pattern;
    int		prev_cursor;

    /* This may take a while */
    prev_cursor = last_set_cursor ();
    set_cursor (DC_BUSY);

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

    set_cursor (prev_cursor);
}

void    sig_sel_ok_cb (
    Widget	w,
    Trace_t	*trace,
    XmAnyCallbackStruct		*cb)
{
    draw_needupd_sig_start ();
    draw_all_needed ();
}

void    sig_sel_apply_cb (
    Widget	w,
    Trace_t	*trace,
    XmAnyCallbackStruct 	*cb)
{
    if (DTPRINT_ENTRY) printf ("In sig_sel_apply_cb - trace=%p\n",trace);

    sig_sel_ok_cb (w,trace,cb);
    sig_select_cb (trace->main);
}

void    sig_sel_add_all_cb (
    Widget	w,
    Trace_t	*trace,
    XmAnyCallbackStruct 	*cb)
{
    Signal_t	*sig_ptr;
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
    Trace_t	*trace,
    XmAnyCallbackStruct 	*cb)
{
    Signal_t	*sig_ptr;
    int		i;

    if (DTPRINT_ENTRY) printf ("In sig_sel_del_all_cb - trace=%p\n",trace);

    /* loop thru signals on deleted queue and add to list */
    for (i=0; 1; i++) {
	sig_ptr = (trace->select.del_signals)[i];
	if (!sig_ptr) break;
	sig_delete (trace, sig_ptr, TRUE, ADD_LAST);
    }
    sig_sel_pattern_cb (NULL, trace, NULL);
}

void    sig_sel_del_const (
    Trace_t	*trace,
    Boolean_t	ignorexz)		/* TRUE = ignore xz */
{
    Signal_t	*sig_ptr;
    int		i;

    if (DTPRINT_ENTRY) printf ("In sig_sel_del_const_cb - trace=%p\n",trace);

    /* loop thru signals on deleted queue and add to list */
    for (i=0; 1; i++) {
	sig_ptr = (trace->select.del_signals)[i];
	if (!sig_ptr) break;

	/* Delete it */
	if (sig_is_constant (trace, sig_ptr, ignorexz)) {
	    sig_delete (trace, sig_ptr, FALSE, ADD_LAST);
	}
    }
    sig_sel_pattern_cb (NULL, trace, NULL);
}

void    sig_sel_del_const_xz_cb (
    Widget	w,
    Trace_t	*trace,
    XmAnyCallbackStruct 	*cb)
{
    sig_sel_del_const (trace, TRUE);
}

void    sig_sel_del_const_cb (
    Widget	w,
    Trace_t	*trace,
    XmAnyCallbackStruct 	*cb)
{
    sig_sel_del_const (trace, FALSE);
}

void    sig_sel_add_list_cb (
    Widget	w,
    Trace_t	*trace,
    XmListCallbackStruct 	*cb)
{
    int		sel, i;
    Signal_t	*sig_ptr;

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
    Trace_t	*trace,
    XmListCallbackStruct	 *cb)
{
    int		sel, i;
    Signal_t	*sig_ptr;

    if (DTPRINT_ENTRY) printf ("In sig_sel_del_list_cb - trace=%p\n",trace);

    for (sel=0; sel < cb->selected_item_count; sel++) {
	i = (cb->selected_item_positions)[sel] - 1;
	sig_ptr = (trace->select.del_signals)[i];
	if (!sig_ptr) fprintf (stderr, "Unexpected null signal\n");
	/*if (DTPRINT) printf ("Pos %d sig '%s'\n", i, sig_ptr->signame);*/

	/* Delete it */
	sig_delete (trace, sig_ptr, TRUE, ADD_LAST);
    }
    sig_sel_pattern_cb (NULL, trace, NULL);
}

/**********************************************************************/

typedef int (*qsort_compar) (const void *, const void *);
static int sig_sel_sort_cmp (
    Signal_t	**siga_pptr,
    Signal_t	**sigb_pptr)
{
    return (strcmp((*siga_pptr)->key, (*sigb_pptr)->key));
}

void	sig_sel_sort (
    Trace_t	*trace,
    Boolean_t	usebasename)
{
    Signal_t	*sig_ptr;
    int i;
    int nel = 0;
    if (DTPRINT_ENTRY) printf ("In sig_sel_sort - trace=%p\n",trace);

    /* loop thru signals on deleted queue and add to list */
    for (i=0; 1; i++) {
	sig_ptr = (trace->select.del_signals)[i];
	if (!sig_ptr) break;
	/* Add to sort list */
	nel = i+1;
	sig_ptr->key = usebasename ? sig_basename (trace, sig_ptr)
	    : sig_ptr->signame;
	/*printf ("sortn %s\n", sig_ptr->key);*/
    }

    /* sortem */
    qsort ((trace->select.del_signals), nel, sizeof (Signal_t *), (qsort_compar)sig_sel_sort_cmp);

    /* reextract */
    for (i=0; i<nel; i++) {
	sig_ptr = (trace->select.del_signals)[i];
	/*printf ("sorted %s\n", sig_ptr->key);*/
	sig_move (trace, sig_ptr, trace, ADD_LAST);
    }

    sig_sel_apply_cb (NULL, trace, NULL);
}

void    sig_sel_sort_cb (
    Widget	w,
    Trace_t	*trace,
    XmAnyCallbackStruct 	*cb)
{
    sig_sel_sort (trace, FALSE);
}

void    sig_sel_sort_base_cb (
    Widget	w,
    Trace_t	*trace,
    XmAnyCallbackStruct 	*cb)
{
    sig_sel_sort (trace, TRUE);
}

/**********************************************************************/
/**********************************************************************/
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
    Trace_t	*trace)
{
    Signal_t	*sig_ptr;
    Trace_t	*trace_ptr;

    if (DTPRINT_ENTRY) printf ("In sig_cross_preserve - trace=%p\n",trace);

    /* Save the current trace into a new preserved place */
    assert (global->preserved_trace==NULL);

    if (! global->save_ordering) {
	/* Will be freed by sig_free call later inside trace reading */
	return;
    }

    global->preserved_trace = XtNew (Trace_t);
    memcpy (global->preserved_trace, trace, sizeof (Trace_t));

    /* Other signals, AND deleted signals */
    for (trace_ptr = global->deleted_trace_head; trace_ptr; trace_ptr = trace_ptr->next_trace) {
	for (sig_ptr = trace_ptr->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {

	    if (sig_ptr->trace == trace) {
		/* change to point to the new preserved structure */
		sig_ptr->trace = global->preserved_trace;
		sig_ptr->new_trace_sig = NULL;
	    }
	}
    }

    /* Tell old trace that it no longer has any signals */
    trace->firstsig = NULL;
    trace->lastsig = NULL;
    trace->dispsig = NULL;
    trace->numsigstart = 0;
}

static void sig_hash_name (
    /* Assign signal names a hash value for faster lookup */
    Trace_t	*trace,
    uint_t	hashsize,
    Signal_t	**hash
    )
{
    Signal_t *sig_ptr;
    Signal_t **change_pptr;
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	uint_t hashval = 0;
	char *tp = sig_ptr->signame;
	while (*tp) hashval = (hashval*33) + *tp++;
	hashval = hashval % hashsize;
	sig_ptr->signame_hash = hashval;
	if (hash) {
	    change_pptr = &hash[hashval];
	    /* Add to the end of the hash list so that signals w/ identical name will retain ordering */
	    /* Verilog_next is overloaded for hash chain */
	    while (*change_pptr) { change_pptr = &((*change_pptr)->verilog_next); }
	    *change_pptr = sig_ptr;
	}
	/* Clear link in prep for next operation */
	sig_ptr->new_trace_sig = NULL;
	sig_ptr->preserve_match = 0;
    }
}

static void sig_cross_sigmatch (
    /* Look through each signal in the old list, attempt to match with
       signal from the new_trace.  When found create new_trace_sig link to/from old & new signal */
    Trace_t	*new_trace,
    Trace_t	*old_trace)
{
    Signal_t	*new_sig_ptr;		/* New signal now searching on */
    Signal_t	*old_sig_ptr;
    Signal_t    **hash;
    uint_t	hashsize;

    if (!old_trace->firstsig || !new_trace->firstsig) return;	/* Empty */
    if (old_trace == new_trace) return;

    /* Speed up name comparison by hashing names */
    hashsize = new_trace->numsig*2;
    hash = (Signal_t**)XtCalloc(hashsize, (unsigned) sizeof(Signal_t *));

    sig_hash_name (new_trace, hashsize, hash);	/* Also sets new_trace_sig = NULL */
    sig_hash_name (old_trace, hashsize, NULL);

    /* Look at each old signal */
    for (old_sig_ptr = old_trace->firstsig; old_sig_ptr; old_sig_ptr = old_sig_ptr->forward) {
	/* See if have new signal in hash bucket */
	for (new_sig_ptr = hash[old_sig_ptr->signame_hash];
	     new_sig_ptr; new_sig_ptr = new_sig_ptr->verilog_next) {
	    char *os = old_sig_ptr->signame;
	    char *ns = new_sig_ptr->signame;
	    /*printf ("Compare %p %s == %p %s\n",
	      old_sig_ptr, old_sig_ptr->signame, new_sig_ptr, new_sig_ptr->signame);*/
	    /* Do our own strcmp; much faster */
	    while (*os && *os++ == *ns++);
	    if (*os == '\0' && *ns == '\0'
		&& !new_sig_ptr->preserve_match) {
		/* Match */
		if (DTPRINT_FILE) printf ("Matched %s %d %p == %s %d %p\n",
					  old_sig_ptr->signame, old_sig_ptr->file_pos, old_sig_ptr,
					  new_sig_ptr->signame, new_sig_ptr->file_pos, new_sig_ptr);
		old_sig_ptr->new_trace_sig = new_sig_ptr;
		new_sig_ptr->preserve_match = 1;  /* So duplicate sig names work ok */
		/* Save color, etc */
		new_sig_ptr->color = old_sig_ptr->color;
		new_sig_ptr->search = old_sig_ptr->search;
		new_sig_ptr->radix = old_sig_ptr->radix;
		new_sig_ptr->waveform = old_sig_ptr->waveform;
		break;  /* On to next old signal */
	    }
	}
    }
    DFree(hash);
}

void sig_cross_restore (
    /* Try to make new trace's signal placement match the old placement */
    Trace_t	*trace)		/* New trace */
{
    Signal_t	*old_sig_ptr;
    Trace_t	*trace_ptr;
    Signal_t	*new_sig_ptr;	/* Pointer to new trace's signal */
    Signal_t	*next_sig_ptr;

    Signal_t	*newlist_firstsig;
    Signal_t	**back_sig_pptr;

    Signal_t	*new_dispsig;

    if (DTPRINT_ENTRY) printf ("In sig_cross_restore - trace=%p\n",trace);

    if (global->save_ordering && global->preserved_trace) {
	/* Preserve colors, etc */

	/* Establish links */
	sig_cross_sigmatch (trace, global->deleted_trace_head);
	sig_cross_sigmatch (trace, global->preserved_trace);
	for (trace_ptr = global->deleted_trace_head; trace_ptr; trace_ptr = trace_ptr->next_trace) {
	    sig_cross_sigmatch (trace, trace_ptr);
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
		    sig_delete (trace, new_sig_ptr, TRUE, NULL/*killorder-faster*/);
		}
	    }
	}

	/* Remember scrolling position before move signals around */
	new_dispsig = NULL;	/* But, don't set it yet, the pointer may move */
	if (global->preserved_trace->dispsig) {
	}
	if (global->preserved_trace->dispsig && global->preserved_trace->dispsig->new_trace_sig) {
	    new_dispsig = global->preserved_trace->dispsig->new_trace_sig;
	}

	/* Other signals */
	if (DTPRINT_PRESERVE) printf ("Preserve: Copy/move to other\n");
	for (trace_ptr = global->trace_head; trace_ptr; trace_ptr = trace_ptr->next_trace) {
	    for (old_sig_ptr = global->preserved_trace->firstsig; old_sig_ptr; old_sig_ptr = old_sig_ptr->forward) {
		if (    (old_sig_ptr->trace == global->preserved_trace)	/* Sig from old trace */
		    &&  (trace_ptr != global->preserved_trace) ) {	/* Not in old trace */
		    if (NULL != (new_sig_ptr = old_sig_ptr->new_trace_sig)) {
			if (old_sig_ptr->copyof && !old_sig_ptr->file_copy) {
			    /* Copy to other */
			    if (DTPRINT_PRESERVE) printf ("Preserve: Please copy-to-other %s\n", old_sig_ptr->signame);
			    sig_copy (trace, new_sig_ptr, trace_ptr, old_sig_ptr);
			}
			else {
			    /* Move to other */
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
	global->preserved_trace->lastsig = NULL;
	global->preserved_trace->dispsig = NULL;
	if (DTPRINT_PRESERVE) printf ("Preserve: %d\n", __LINE__);
	while (old_sig_ptr) {
	    if (DTPRINT_PRESERVE && old_sig_ptr) printf ("Preserve: %s\n", old_sig_ptr->signame);

	    if (old_sig_ptr->trace != global->preserved_trace) {	/* NOT sig from old trace */
		/* Copy or moved from a third independant trace file into this view,
		   cheat by just relinking into new structure */
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
	trace->numsigstart = 0;
	old_sig_ptr = NULL;
	for (new_sig_ptr = newlist_firstsig; new_sig_ptr; new_sig_ptr = new_sig_ptr->new_forward_sig) {
	    trace->numsig++;
	    new_sig_ptr->deleted = FALSE;
	    new_sig_ptr->forward = new_sig_ptr->new_forward_sig;
	    new_sig_ptr->backward = old_sig_ptr;
	    old_sig_ptr = new_sig_ptr;
	}
	trace->lastsig = old_sig_ptr;

	/* Restore scrolling position */
	trace->dispsig = (new_dispsig) ? new_dispsig : trace->firstsig;
	vscroll_new (trace,0);

	if (DTPRINT_PRESERVE) printf ("Preserve: Done\n");
    }

    /* Free information for the preserved trace */
    free_data (global->preserved_trace);
    DFree (global->preserved_trace);

    if (DTDEBUG) debug_integrity_check_cb (NULL);
}



/****************************** Enable COMBINING ******************************/

static void sig_modify_en_signal (
    Trace_t	*trace,
    Signal_t	*en_sig_ptr,
    Signal_t	*base_sig_ptr,
    Boolean_t	is_cosmos)
{
    Signal_t	*new_sig_ptr;
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

    val_zero (&new_value);

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

	val_copy (&new_value, &base_value);

	/* Determine new value */
	/* Cosmos	enable means force 0/1	-> U */
	/* Verilator	enable means force 0/1	-> Z */
	switch (en_value.siglw.stbits.state) {
	  case STATE_0:	/* Leave value normal */
	    break;
	  case STATE_1:	/* Single bit, disable it */
	    new_value.siglw.stbits.state = is_cosmos ? STATE_U : STATE_Z;
	    break;

	    /* STATE_1 can't occur below in the base because we know */
	    /* the width of the enable is the same as the width of the base */
	    /* and STATE_1 can only occur with 1 bits, B32 with 2 or more bits */
	  case STATE_B32:	/* Some bitmask of U's */
	    switch (base_value.siglw.stbits.state) {
	      case STATE_0:
		new_value.number[0] = 0;
		/* FALLTHRU */
	      case STATE_B32:
	      default:
		new_value.siglw.stbits.state = STATE_F32;
		if (is_cosmos) new_value.number[0] &= ~en_value.number[0]; /*U*/
		else new_value.number[0] |= en_value.number[0]; /*Z*/
		new_value.number[1] = en_value.number[0];
		break;
	      case STATE_B128:
		new_value.siglw.stbits.state = STATE_F128;
		if (is_cosmos) new_value.number[0] &= ~en_value.number[0]; /*U*/
		else new_value.number[0] |= en_value.number[0]; /*Z*/
		new_value.number[4] = en_value.number[0];
		break;
	    } /* en=B32, switch base_state */
	    break;

	  case STATE_B128:	/* Some bitmask of U's */
	    switch (base_value.siglw.stbits.state) {
	      case STATE_0:
		new_value.number[0] = 0;
		/* FALLTHRU */
	      case STATE_B32:
	      default:
		/* Make it look like the base value is a B128 */
		new_value.number[1] = 0;
		new_value.number[2] = 0;
		new_value.number[3] = 0;
		/* FALLTHRU */
	      case STATE_B128:
		new_value.siglw.stbits.state = STATE_F128;
		if (is_cosmos) new_value.number[0] &= ~en_value.number[0]; /*U*/
		else new_value.number[0] |= en_value.number[0]; /*Z*/
		if (is_cosmos) new_value.number[1] &= ~en_value.number[1]; /*U*/
		else new_value.number[1] |= en_value.number[1]; /*Z*/
		if (is_cosmos) new_value.number[2] &= ~en_value.number[2]; /*U*/
		else new_value.number[2] |= en_value.number[2]; /*Z*/
		if (is_cosmos) new_value.number[3] &= ~en_value.number[3]; /*U*/
		else new_value.number[3] |= en_value.number[3]; /*Z*/
		new_value.number[4] = en_value.number[0];
		new_value.number[5] = en_value.number[1];
		new_value.number[6] = en_value.number[2];
		new_value.number[7] = en_value.number[3];
		break;
	    } /* en=B32, switch base_state */
	    break;
	} /* switch */
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

	if (new_value.number[4]
	    && (new_value.number[4] == (new_sig_ptr->value_mask[0] & 0xffffffff))
	    && (new_value.number[5] == (new_sig_ptr->value_mask[1] & 0xffffffff))
	    && (new_value.number[6] == (new_sig_ptr->value_mask[2] & 0xffffffff))
	    && (new_value.number[7] == (new_sig_ptr->value_mask[3] & 0xffffffff))
	    && (new_value.number[0] == (new_sig_ptr->value_mask[0] & 0xffffffff))
	    && (new_value.number[1] == (new_sig_ptr->value_mask[1] & 0xffffffff))
	    && (new_value.number[2] == (new_sig_ptr->value_mask[2] & 0xffffffff))
	    && (new_value.number[3] == (new_sig_ptr->value_mask[3] & 0xffffffff))) {
	    new_value.siglw.stbits.state = STATE_Z;
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

    sig_add_to_queue (trace, new_sig_ptr, base_sig_ptr);

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

static Boolean_t sig_wordcmp(const char* str, const char* word) {
    /* Return true if this signal ends in the specified pattern*/
    while (*word) {
	if (!*str || *word != *str) return FALSE;
	word++; str++;
    }
    /* Next character must end a word*/
    return (!*str || (!isalnum(*str) && *str!='_'));
}

void sig_modify_enables (
    Trace_t	*trace)
{
    Signal_t	*sig_ptr, *en_sig_ptr, *base_sig_ptr;
    char	*tp, *nonenablename;
    Boolean_t	did_one=FALSE;
    Boolean_t	is_cosmos=FALSE;

    for (sig_ptr = trace->firstsig; sig_ptr; ) {
	for (tp=sig_ptr->signame; *tp; tp++) {
	    if (tp[0]=='_' && tp[1]=='_'
		&& (sig_wordcmp(tp+2, "en")
		    || sig_wordcmp(tp+2, "EN")
		    || sig_wordcmp(tp+2, "inen")
		    || sig_wordcmp(tp+2, "INEN")
		    || sig_wordcmp(tp+2, "cos")
		    || sig_wordcmp(tp+2, "COS")))
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

	    en_sig_ptr = sig_ptr;

	    /* Start search right before this signal, in case there are duplicate signames */
	    base_sig_ptr = en_sig_ptr;
	    if (base_sig_ptr->backward) base_sig_ptr=base_sig_ptr->backward;
	    base_sig_ptr = sig_find_signame_start_backward (base_sig_ptr, nonenablename);
	    if (!base_sig_ptr) base_sig_ptr = sig_find_signame (trace, nonenablename);

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


