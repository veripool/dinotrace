/******************************************************************************
 *
 * Filename:
 *     dt_signal.c
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
 *     AAG	 6-Nov-90	popped add signal widget to top of stack after
 *				adding signal by unmanage/managing widget
 *     AAG	29-Apr-91	Use X11 for Ultrix support
 *     WPS	11-Mar-93	Fixed move_signal ->backward being null bug
 */
static char rcsid[] = "$Id$";


#include <stdio.h>
#include <stdlib.h>
/*#include <descrip.h> - removed for Ultrix support... */
#include <sys/types.h>
#include <sys/stat.h>

#include <X11/Xlib.h>
#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/SelectioB.h>
#include <Xm/Text.h>
#include <Xm/List.h>
#include <Xm/BulletinB.h>
#include <Xm/Label.h>

#include "dinotrace.h"
#include "callbacks.h"

extern void 
    sig_sel_ok_cb(), sig_sel_apply_cb(),
    sig_add_sel_cb(), sig_sel_pattern_cb(),
    sig_sel_add_all_cb(), sig_sel_add_list_cb(),
    sig_sel_del_all_cb(), sig_sel_del_list_cb(), sig_sel_del_const_cb();


/****************************** UTILITIES ******************************/

void sig_free (trace, sig_ptr, select, recursive)
    /* Free a signal structure, and unlink all traces of it */
    TRACE	*trace;
    SIGNAL	*sig_ptr;	/* Pointer to signal to be deleted */
    Boolean	select;		/* True = selectively pick trace's signals from the list */
    Boolean	recursive;	/* True = recursively do the entire list */
{
    SIGNAL	*del_sig_ptr;
    TRACE	*trace_ptr;

    /* loop and free signal data and each signal structure */
    while (sig_ptr) {
	if (!select || sig_ptr->trace == trace) {
	    /* Check head pointers */
	    for (trace_ptr = global->trace_head; trace_ptr; trace_ptr = trace_ptr->next_trace) {
		if ( sig_ptr == trace_ptr->dispsig )
		    trace_ptr->dispsig = sig_ptr->forward;
		if ( sig_ptr == trace_ptr->firstsig )
		    trace_ptr->firstsig = sig_ptr->forward;
		}
	    if ( sig_ptr == global->delsig )
		global->delsig = sig_ptr->forward;

	    /* free the signal data */
	    del_sig_ptr = sig_ptr;

	    if (sig_ptr->forward)
		((SIGNAL *)(sig_ptr->forward))->backward = sig_ptr->backward;
	    if (sig_ptr->backward)
		((SIGNAL *)(sig_ptr->backward))->forward = sig_ptr->forward;
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

void    remove_signal_from_queue (trace, sig_ptr)
    TRACE	*trace;
    SIGNAL	*sig_ptr;	/* Signal to remove */
    /* Removes the signal from the current and any other ques that it is in */
{
    SIGNAL	*next_sig_ptr, *prev_sig_ptr;
    
    if (DTPRINT_ENTRY) printf ("In remove_signal_from_queue - trace=%d sig %d\n",trace,sig_ptr);
    
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
    /* if the signal is the deleted signal, change it */
    if ( sig_ptr == global->delsig ) {
	global->delsig = sig_ptr->forward;
	}

    if (!sig_ptr->deleted) trace->numsig--;
    }

#define ADD_LAST ((SIGNAL *)(-1))
void    add_signal_to_queue (trace,sig_ptr,loc_sig_ptr,top_pptr)
    TRACE	*trace;
    SIGNAL	*sig_ptr;	/* Signal to add */
    SIGNAL	*loc_sig_ptr;	/* Pointer to signal ahead of one to add, NULL=1st, ADD_LAST=last */
    SIGNAL	**top_pptr;	/* Pointer to where top of list is stored */
{
    SIGNAL	*next_sig_ptr, *prev_sig_ptr;
    
    if (DTPRINT_ENTRY) printf ("In add_signal_to_queue - trace=%d   loc=%d%s\n",trace, loc_sig_ptr, loc_sig_ptr==ADD_LAST?"=last":"");
    
    if (sig_ptr==loc_sig_ptr) {
	loc_sig_ptr = loc_sig_ptr->forward;
	}

    /*printf ("iPrev %d  next %d  sig %d  top %d\n", prev_sig_ptr, next_sig_ptr, sig_ptr, *top_pptr);*/

    /* Insert into first position? */
    if (loc_sig_ptr == NULL) {
	next_sig_ptr = *top_pptr;
	*top_pptr = sig_ptr;
	prev_sig_ptr = (next_sig_ptr)? next_sig_ptr->backward : NULL;
	}
    else if (loc_sig_ptr == ADD_LAST) {
	prev_sig_ptr = NULL;
	for (next_sig_ptr = *top_pptr; next_sig_ptr; next_sig_ptr = next_sig_ptr->forward) {
	    prev_sig_ptr = next_sig_ptr;
	    }
	if (!prev_sig_ptr) {
	    *top_pptr = sig_ptr;
	    }
	}
    else {
	next_sig_ptr = loc_sig_ptr->forward;
	prev_sig_ptr = loc_sig_ptr;
	}

    sig_ptr->forward = next_sig_ptr;
    sig_ptr->backward = prev_sig_ptr;
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
    /* if the signal is the deleted signal, change it */
    if ( next_sig_ptr && ( next_sig_ptr == global->delsig )) {
	global->delsig = sig_ptr;
	}
    /* if no display sig, but is regular first sig, make the display sig */
    if ( trace->firstsig && !trace->dispsig) {
	trace->dispsig = trace->firstsig;
	}
    }

SIGNAL *replicate_signal (trace, sig_ptr)
    TRACE	*trace;
    SIGNAL	*sig_ptr;	/* Signal to remove */
    /* Makes a duplicate copy of the signal */
{
    SIGNAL	*new_sig_ptr;
    
    if (DTPRINT_ENTRY) printf ("In replicate_signal - trace=%d\n",trace);
    
    /* Create new structure */
    new_sig_ptr = (SIGNAL *)XtMalloc (sizeof (SIGNAL));

    /* Copy data */
    memcpy (new_sig_ptr, sig_ptr, sizeof (SIGNAL));

    /* Erase new links */
    new_sig_ptr->forward = NULL;
    new_sig_ptr->backward = NULL;
    if (sig_ptr->copyof)
	new_sig_ptr->copyof = sig_ptr->copyof;
    else new_sig_ptr->copyof = sig_ptr;

    return (new_sig_ptr);
    }

/* Returns SIGNAL or NULL if not found */
SIGNAL *sig_find_signame (trace, signame)
    TRACE	*trace;
    char	*signame;
{
    SIGNAL	*sig_ptr;
    
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	if (!strcmp (sig_ptr->signame, signame)) return (sig_ptr);
	}
    return (NULL);
    }

/* Returns SIGNAL or NULL if not found */
SIGNAL *sig_wildmat_signame (trace, signame)
    TRACE	*trace;
    char	*signame;
{
    SIGNAL	*sig_ptr;
    
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	if (!wildmat (sig_ptr->signame, signame)) return (sig_ptr);
	}
    return (NULL);
    }

void	sig_wildmat_select (trace, pattern)
    /* Create list of selected signals */
    TRACE	  	*trace;		/* NULL= do all traces */
    char		*pattern;
{
    SIGNAL		*sig_ptr;
    SIGNAL_LIST		*siglst_ptr;
    Boolean		trace_list;
    
    trace_list = (trace == NULL);

    /* Erase existing selections */
    while (global->select_head) {
	siglst_ptr = global->select_head;
	global->select_head = global->select_head->forward;
	DFree (siglst_ptr);
	}

    if (trace_list) trace = global->trace_head;
    for (; trace; trace = (trace_list ? trace->next_trace : NULL)) {
	for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	    if (wildmat (sig_ptr->signame, pattern)) {
		/* printf ("Selected: %s\n", sig_ptr->signame); */
		siglst_ptr = XtNew (SIGNAL_LIST);
		siglst_ptr->trace = trace;
		siglst_ptr->signal = sig_ptr;
		siglst_ptr->forward = global->select_head;
		global->select_head = siglst_ptr;
		}
	    }
	}

    if (trace_list) {
	for (sig_ptr = global->delsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	    if (wildmat (sig_ptr->signame, pattern)) {
		/* printf ("Selected (DEL): %s\n", sig_ptr->signame); */
		siglst_ptr = XtNew (SIGNAL_LIST);
		siglst_ptr->trace = sig_ptr->trace;
		siglst_ptr->signal = sig_ptr;
		siglst_ptr->forward = global->select_head;
		global->select_head = siglst_ptr;
		}
	    }
	}
    }

void	sig_delete (trace, sig_ptr)
    TRACE	*trace;
    SIGNAL	*sig_ptr;	/* Signal to remove */
    /* Delete the given signal */
{
    if (sig_ptr) {
	remove_signal_from_queue (trace, sig_ptr);
	sig_ptr->deleted = TRUE;
	add_signal_to_queue (trace, sig_ptr, ADD_LAST, &global->delsig);
	}
    }

void	sig_move (old_trace, sig_ptr, new_trace, after_sig_ptr)
    TRACE	*old_trace;
    SIGNAL	*sig_ptr;	/* Signal to move */
    TRACE	*new_trace;
    SIGNAL	*after_sig_ptr;	/* Signal to place after or ADD_LAST */
{

    if (sig_ptr) {
	remove_signal_from_queue (old_trace, sig_ptr);
	sig_ptr->deleted = FALSE;
	add_signal_to_queue (new_trace, sig_ptr, after_sig_ptr, &new_trace->firstsig);
	new_trace->numsig++;
	}
#if 0
    printf ("Adding %s\n", sig_ptr->signame);
    sig_ptr = global->delsig;
    while (sig_ptr) {
	printf (" %s\n",sig_ptr->signame);
	sig_ptr = sig_ptr->forward;
	}

    printf ("Post\n");
    debug_integrity_check_cb (NULL, NULL, NULL);
    printf ("Done\n");
#endif
    }

void	sig_copy (old_trace, sig_ptr, new_trace, after_sig_ptr)
    TRACE	*old_trace;
    SIGNAL	*sig_ptr;	/* Signal to move */
    TRACE	*new_trace;
    SIGNAL	*after_sig_ptr;	/* Signal to place after or ADD_LAST */
{
    SIGNAL	*new_sig_ptr;

    if (sig_ptr) {
	if (sig_ptr->deleted) {
	    sig_move (old_trace, sig_ptr, new_trace, after_sig_ptr);
	    }
	else {
	    new_sig_ptr = replicate_signal (old_trace, sig_ptr);
	    sig_ptr->deleted = FALSE;
	    add_signal_to_queue (new_trace, new_sig_ptr, after_sig_ptr, &new_trace->firstsig);
	    new_trace->numsig++;
	    }
	}
    }

void	sig_update_search ()
{
    TRACE	*trace;
    SIGNAL	*sig_ptr;
    register int i;

    if (DTPRINT_ENTRY) printf ("In sig_update_search\n");

    /* Search every trace for the signals */
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	if (!trace->loaded) continue;

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

Boolean sig_is_constant (trace, sig_ptr)
    TRACE	*trace;
    SIGNAL	*sig_ptr;
{
    Boolean 	changes;
    SIGNAL_LW	*cptr;

    /* Delete it */
    changes=FALSE;
    for (cptr = (SIGNAL_LW *)sig_ptr->bptr ; cptr->sttime.time != EOT; cptr += sig_ptr->lws ) {
	if ((cptr->sttime.time != trace->start_time) && (cptr->sttime.time != trace->end_time)) {
	    changes = TRUE;
	    break;
	    }
	}
    return (!changes);
    }

/****************************** CONFIG FUNCTIONS ******************************/


void    sig_highlight_selected (color)
    int			color;
{
    SIGNAL		*sig_ptr;
    SIGNAL_LIST		*siglst_ptr;
    
    if (DTPRINT_ENTRY) printf ("In sig_highlight_selected\n");
    
    for (siglst_ptr = global->select_head; siglst_ptr; siglst_ptr = siglst_ptr->forward) {
	sig_ptr = siglst_ptr->signal;
	/* Change the color */
	sig_ptr->color = color;
	sig_ptr->search = 0;
	}

    draw_all_needed ();
    }

void    sig_move_selected (new_trace, after_pattern)
    /* also used for adding deleted signals */
    TRACE	  	*new_trace;
    char		*after_pattern;
{
    TRACE	  	*old_trace;
    SIGNAL		*sig_ptr, *next_sig_ptr, *after_sig_ptr;
    SIGNAL_LIST		*siglst_ptr;
    
    if (DTPRINT_ENTRY) printf ("In sig_move_selected aft='%s'\n", after_pattern);

    after_sig_ptr = sig_wildmat_signame (new_trace, after_pattern);
    if (! after_sig_ptr) after_sig_ptr = ADD_LAST;
    
    for (siglst_ptr = global->select_head; siglst_ptr; siglst_ptr = siglst_ptr->forward) {
	sig_ptr = siglst_ptr->signal;
	old_trace = siglst_ptr->trace;
	/* Move it */
	sig_move (old_trace, sig_ptr, new_trace, after_sig_ptr);
	}

    draw_all_needed ();
    }


void    sig_copy_selected (new_trace, after_pattern)
    TRACE	  	*new_trace;
    char		*after_pattern;
{
    TRACE	  	*old_trace;
    SIGNAL		*sig_ptr, *next_sig_ptr, *after_sig_ptr;
    SIGNAL		*new_sig_ptr;
    SIGNAL_LIST		*siglst_ptr;
    
    if (DTPRINT_ENTRY) printf ("In sig_copy_pattern - aft='%s'\n", after_pattern);

    after_sig_ptr = sig_wildmat_signame (new_trace, after_pattern);
    if (! after_sig_ptr) after_sig_ptr = ADD_LAST;
    
    for (siglst_ptr = global->select_head; siglst_ptr; siglst_ptr = siglst_ptr->forward) {
	sig_ptr = siglst_ptr->signal;
	old_trace = siglst_ptr->trace;
	sig_copy (old_trace, sig_ptr, new_trace, after_sig_ptr);
	}

    draw_all_needed ();
    }


void    sig_delete_selected (constant_flag)
    Boolean		constant_flag;		/* FALSE = only delete constants */
{
    TRACE	  	*trace;
    SIGNAL		*sig_ptr;
    SIGNAL_LIST		*siglst_ptr;
    
    if (DTPRINT_ENTRY) printf ("In sig_delete_selected\n");

    for (siglst_ptr = global->select_head; siglst_ptr; siglst_ptr = siglst_ptr->forward) {
	sig_ptr = siglst_ptr->signal;
	trace = siglst_ptr->trace;
	if  ( constant_flag || sig_is_constant (trace, sig_ptr)) {
	    sig_delete (trace, sig_ptr);
	    }
	}

    draw_all_needed ();
    }


void    sig_goto_pattern (trace, pattern)
    TRACE	  	*trace;
    char		*pattern;
{
    SIGNAL		*sig_ptr;
    int			numprt;
    int			inc;
    Boolean		on_screen, found;
    
    if (DTPRINT_ENTRY) printf ("In sig_goto_pattern - trace=%d pat='%s'\n",trace, pattern);
    
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
	    for (sig_ptr = trace->firstsig, numprt = 0; sig_ptr;
		 sig_ptr = sig_ptr->forward, numprt++) {
		if (wildmat (sig_ptr->signame, pattern)) {
		    trace->dispsig = sig_ptr;
		    trace->numsigstart = numprt;
		    found = TRUE;
		    break;
		    }
		}

	    if (found) {
		/* Rescroll found signal to the center of the screen */
		inc = (-trace->numsigvis) / 2;
		while ( (inc < 0) && trace->dispsig && trace->dispsig->backward ) {
		    trace->numsigstart--;
		    trace->dispsig = trace->dispsig->backward;
		    inc++;
		    }
		}
	    }
	}

    draw_all_needed ();
    }


/****************************** MENU OPTIONS ******************************/

void    sig_add_cb (w,trace,cb)
    Widget		w;
    TRACE	*trace;
    XmSelectionBoxCallbackStruct *cb;
{
    SIGNAL	*sig_ptr;
    Widget	list_wid;
    
    if (DTPRINT_ENTRY) printf ("In sig_add_cb - trace=%d\n",trace);
    
    if (!trace->signal.add) {
	XtSetArg (arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg (arglist[1], XmNwidth, 200);
	XtSetArg (arglist[2], XmNheight, 150);
	XtSetArg (arglist[3], XmNdialogTitle, XmStringCreateSimple ("Signal Select"));
	XtSetArg (arglist[4], XmNlistVisibleItemCount, 5);
	XtSetArg (arglist[5], XmNlistLabelString, XmStringCreateSimple ("Select Signal than Location"));
	trace->signal.add = XmCreateSelectionDialog (trace->work,"",arglist,6);
	/* XtAddCallback (trace->signal.add, XmNsingleCallback, sig_add_sel_cb, trace); */
	XtAddCallback (trace->signal.add, XmNokCallback, sig_add_sel_cb, trace);
	XtAddCallback (trace->signal.add, XmNapplyCallback, sig_add_sel_cb, trace);
	XtAddCallback (trace->signal.add, XmNcancelCallback, sig_cancel_cb, trace);
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
    for (sig_ptr = global->delsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	XmListAddItem (list_wid, sig_ptr->xsigname, 0);
	}

    /* if there are signals deleted make OK button active */
    XtSetArg (arglist[0], XmNsensitive, (global->delsig != NULL)?TRUE:FALSE);
    XtSetValues (XmSelectionBoxGetChild (trace->signal.add, XmDIALOG_OK_BUTTON), arglist, 1);

    /* manage the popup on the screen */
    XtManageChild (trace->signal.add);
    }

void    sig_add_sel_cb (w,trace,cb)
    Widget				w;
    TRACE			*trace;
    XmSelectionBoxCallbackStruct *cb;
{
    SIGNAL		*sig_ptr;
    
    if (DTPRINT_ENTRY) printf ("In sig_add_sel_cb - trace=%d\n",trace);
    
    if ( global->delsig == NULL ) return;

    /* save the deleted signal selected number */
    for (sig_ptr = global->delsig; sig_ptr; sig_ptr = sig_ptr->forward) {
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

void    sig_cancel_cb (w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT_ENTRY) printf ("In sig_cancel_cb - trace=%d\n",trace);
    
    /* remove any previous events */
    remove_all_events (trace);
    
    /* unmanage the popup on the screen */
    XtUnmanageChild (trace->signal.add);
    }

void    sig_mov_cb (w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT_ENTRY) printf ("In sig_mov_cb - trace=%d\n",trace);
    
    /* remove any previous events */
    remove_all_events (trace);
    
    /* guarantee next button press selects a signal to be moved */
    global->selected_sig = NULL;
    
    /* process all subsequent button presses as signal moves */ 
    add_event (ButtonPressMask, sig_move_ev);
    set_cursor (trace, DC_SIG_MOVE_1);
    }

void    sig_copy_cb (w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT_ENTRY) printf ("In sig_copy_cb - trace=%d\n",trace);
    
    /* remove any previous events */
    remove_all_events (trace);
    
    /* guarantee next button press selects a signal to be moved */
    global->selected_sig = NULL;
    
    /* process all subsequent button presses as signal moves */ 
    add_event (ButtonPressMask, sig_copy_ev);
    set_cursor (trace, DC_SIG_COPY_1);
    }

void    sig_del_cb (w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT_ENTRY) printf ("In sig_del_cb - trace=%d\n",trace);
    
    /* remove any previous events */
    remove_all_events (trace);
    
    /* process all subsequent button presses as signal deletions */ 
    set_cursor (trace, DC_SIG_DELETE);
    add_event (ButtonPressMask, sig_delete_ev);
    }

void    sig_highlight_cb (w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT_ENTRY) printf ("In sig_highlight_cb - trace=%d\n",trace);
    
    /* remove any previous events */
    remove_all_events (trace);
     
    /* Grab color number from the menu button pointer */
    global->highlight_color = submenu_to_color (trace, w, trace->menu.sig_highlight_pds);

    /* process all subsequent button presses as signal deletions */ 
    set_cursor (trace, DC_SIG_HIGHLIGHT);
    add_event (ButtonPressMask, sig_highlight_ev);
    }

void    sig_search_cb (w,trace,cb)
    Widget		w;
    TRACE	*trace;
    XmSelectionBoxCallbackStruct *cb;
{
    int		i;
    int		y=10;
    
    if (DTPRINT_ENTRY) printf ("In sig_search_cb - trace=%d\n",trace);
    
    if (!trace->signal.search) {
	XtSetArg (arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg (arglist[1], XmNdialogTitle, XmStringCreateSimple ("Signal Search Requester") );
	/* XtSetArg (arglist[2], XmNwidth, 500);
	   XtSetArg (arglist[3], XmNheight, 400); */
	trace->signal.search = XmCreateBulletinBoardDialog (trace->work,"search",arglist,2);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Color"));
	XtSetArg (arglist[1], XmNx, 5);
	XtSetArg (arglist[2], XmNy, y);
	trace->signal.label1 = XmCreateLabel (trace->signal.search,"label1",arglist,3);
	XtManageChild (trace->signal.label1);
	
	y += 15;
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Sig"));
	XtSetArg (arglist[1], XmNx, 5);
	XtSetArg (arglist[2], XmNy, y);
	trace->signal.label4 = XmCreateLabel (trace->signal.search,"label4",arglist,3);
	XtManageChild (trace->signal.label4);
	
	XtSetArg (arglist[0], XmNlabelString,
		 XmStringCreateSimple ("Search value, *? are wildcards" ));
	XtSetArg (arglist[1], XmNx, 60);
	XtSetArg (arglist[2], XmNy, y);
	trace->signal.label3 = XmCreateLabel (trace->signal.search,"label3",arglist,3);
	XtManageChild (trace->signal.label3);
	
	y += 25;

	for (i=0; i<MAX_SRCH; i++) {
	    /* enable button */
	    XtSetArg (arglist[0], XmNx, 15);
	    XtSetArg (arglist[1], XmNy, y);
	    XtSetArg (arglist[2], XmNselectColor, trace->xcolornums[i+1]);
	    XtSetArg (arglist[3], XmNlabelString, XmStringCreateSimple (""));
	    trace->signal.enable[i] = XmCreateToggleButton (trace->signal.search,"togglen",arglist,4);
	    XtManageChild (trace->signal.enable[i]);

	    /* create the file name text widget */
	    XtSetArg (arglist[0], XmNrows, 1);
	    XtSetArg (arglist[1], XmNcolumns, 30);
	    XtSetArg (arglist[2], XmNx, 60);
	    XtSetArg (arglist[3], XmNy, y);
	    XtSetArg (arglist[4], XmNresizeHeight, FALSE);
	    XtSetArg (arglist[5], XmNeditMode, XmSINGLE_LINE_EDIT);
	    trace->signal.text[i] = XmCreateText (trace->signal.search,"textn",arglist,6);
	    XtAddCallback (trace->signal.text[i], XmNactivateCallback, sig_search_ok_cb, trace);
	    XtManageChild (trace->signal.text[i]);
	    
	    y += 40;
	    }

	y+= 15;

	/* Create OK button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple (" OK ") );
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNy, y);
	trace->signal.ok = XmCreatePushButton (trace->signal.search,"ok",arglist,3);
	XtAddCallback (trace->signal.ok, XmNactivateCallback, sig_search_ok_cb, trace);
	XtManageChild (trace->signal.ok);
	
	/* create apply button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Apply") );
	XtSetArg (arglist[1], XmNx, 70);
	XtSetArg (arglist[2], XmNy, y);
	trace->signal.apply = XmCreatePushButton (trace->signal.search,"apply",arglist,3);
	XtAddCallback (trace->signal.apply, XmNactivateCallback, sig_search_apply_cb, trace);
	XtManageChild (trace->signal.apply);
	
	/* create cancel button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Cancel") );
	XtSetArg (arglist[1], XmNx, 140);
	XtSetArg (arglist[2], XmNy, y);
	trace->signal.cancel = XmCreatePushButton (trace->signal.search,"cancel",arglist,3);
	XtAddCallback (trace->signal.cancel, XmNactivateCallback, unmanage_cb, trace->signal.search);

	XtManageChild (trace->signal.cancel);
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
    XtManageChild (trace->signal.search);
    }

void    sig_search_ok_cb (w,trace,cb)
    Widget				w;
    TRACE			*trace;
    XmSelectionBoxCallbackStruct *cb;
{
    char		*strg;
    int			i;

    if (DTPRINT_ENTRY) printf ("In sig_search_ok_cb - trace=%d\n",trace);

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

    sig_update_search ();

    draw_all_needed ();
    }

void    sig_search_apply_cb (w,trace,cb)
    Widget				w;
    TRACE			*trace;
    XmSelectionBoxCallbackStruct *cb;
{
    if (DTPRINT_ENTRY) printf ("In sig_search_apply_cb - trace=%d\n",trace);

    sig_search_ok_cb (w,trace,cb);
    sig_search_cb (w,trace,cb);
    }

/****************************** EVENTS ******************************/

void    sig_add_ev (w,trace,ev)
    Widget			w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    SIGNAL		*sig_ptr;
    
    if (DTPRINT_ENTRY) printf ("In sig_add_ev - trace=%d\n",trace);
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
    if (global->delsig == NULL) {
	XtSetArg (arglist[0], XmNsensitive, FALSE);
	XtSetValues (XmSelectionBoxGetChild (trace->signal.add, XmDIALOG_OK_BUTTON), arglist, 1);
	}
    
    sig_move (trace, global->selected_sig, trace, sig_ptr);
    
    /* remove any previous events */
    remove_all_events (trace);
    
    /* pop window back to top of stack */
    XtUnmanageChild ( trace->signal.add );
    XtManageChild ( trace->signal.add );
    
    draw_all_needed ();
    }

void    sig_move_ev (w,trace,ev)
    Widget			w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    SIGNAL		*sig_ptr;
    
    if (DTPRINT_ENTRY) printf ("In sig_move_ev - trace=%d\n",trace);
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
    
    draw_all_needed ();
    }

void    sig_copy_ev (w,trace,ev)
    Widget			w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    SIGNAL		*sig_ptr;
    
    if (DTPRINT_ENTRY) printf ("In sig_copy_ev - trace=%d\n",trace);
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
    
    draw_all_needed ();
    }

void    sig_delete_ev (w,trace,ev)
    Widget			w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    SIGNAL		*sig_ptr;
    
    if (DTPRINT_ENTRY) printf ("In sig_delete_ev - trace=%d\n",trace);
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
    sig_delete (trace, sig_ptr);
    
    /* add signame to list box */
    if ( trace->signal.add != NULL ) {
	XmListAddItem (XmSelectionBoxGetChild (trace->signal.add, XmDIALOG_LIST),
		       sig_ptr->xsigname, 0 );
	}
    
    draw_all_needed ();
    }


void    sig_highlight_ev (w,trace,ev)
    Widget			w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    SIGNAL		*sig_ptr;
    
    if (DTPRINT_ENTRY) printf ("In sig_highlight_ev - trace=%d\n",trace);
    if (ev->type != ButtonPress || ev->button!=1) return;
    
    sig_ptr = posy_to_signal (trace, ev->y);
    if (!sig_ptr) return;
    
    /* Change the color */
    sig_ptr->color = global->highlight_color;
    sig_ptr->search = 0;

    draw_all_needed ();
    }


/****************************** SELECT OPTIONS ******************************/

void    sig_select_cb (w,trace,cb)
    Widget		w;
    TRACE	*trace;
    XmSelectionBoxCallbackStruct *cb;
{
    if (DTPRINT_ENTRY) printf ("In sig_select_cb - trace=%d\n",trace);
    
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
	XtAddCallback (trace->select.ok, XmNactivateCallback, sig_sel_ok_cb, trace);
	XtManageChild (trace->select.ok);

	/* Create apply button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Apply") );
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_POSITION );
	XtSetArg (arglist[2], XmNleftPosition, 45);
	XtSetArg (arglist[3], XmNbottomAttachment, XmATTACH_FORM );
	trace->select.apply = XmCreatePushButton (trace->select.select,"apply",arglist,4);
	XtAddCallback (trace->select.apply, XmNactivateCallback, sig_sel_apply_cb, trace);
	XtManageChild (trace->select.apply);

	/* create cancel button */
#ifdef broken
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Cancel") );
	XtSetArg (arglist[1], XmNrightAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNbottomAttachment, XmATTACH_FORM );
	trace->select.cancel = XmCreatePushButton (trace->select.select,"cancel",arglist,3);
	/* XtAddCallback (trace->select.cancel, XmNactivateCallback, unmanage_cb, trace->select.select); */
	XtAddCallback (trace->select.cancel, XmNactivateCallback, unmanage_cb, trace->select.select);
	XtManageChild (trace->select.cancel);
#endif

	/*** Add (deleted list) section ***/
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Add Signal Pattern"));
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_FORM );
	trace->select.label2 = XmCreateLabel (trace->select.select,"label2",arglist,3);
	XtManageChild (trace->select.label2);
	
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
	XtAddCallback (trace->select.add_pat, XmNactivateCallback, sig_sel_pattern_cb, trace);
	XtManageChild (trace->select.add_pat);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Add All") );
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNbottomAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNbottomWidget, trace->select.ok);
	trace->select.add_all = XmCreatePushButton (trace->select.select,"aall",arglist,4);
	XtRemoveAllCallbacks (trace->select.add_all, XmNactivateCallback);
	XtAddCallback (trace->select.add_all, XmNactivateCallback, sig_sel_add_all_cb, trace);
	XtManageChild (trace->select.add_all);

	XtSetArg (arglist[0], XmNlabelString, string_create_with_cr ("Select a signal to add it\nto the displayed signals."));
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopWidget, trace->select.add_pat);
	trace->select.label1 = XmCreateLabel (trace->select.select,"label1",arglist,4);
	XtManageChild (trace->select.label1);
	
	XtSetArg (arglist[0], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[1], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[2], XmNtopWidget, trace->select.label1);
	XtSetArg (arglist[3], XmNlistVisibleItemCount, 5);
	XtSetArg (arglist[4], XmNbottomAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[5], XmNbottomWidget, trace->select.add_all);
	XtSetArg (arglist[6], XmNrightAttachment, XmATTACH_POSITION );
	XtSetArg (arglist[7], XmNrightPosition, 50);
	XtSetArg (arglist[8], XmNlistSizePolicy, XmCONSTANT);
	XtSetArg (arglist[9], XmNselectionPolicy, XmEXTENDED_SELECT);
	XtSetArg (arglist[10], XmNitems, 0);
	trace->select.add_sigs = XmCreateScrolledList (trace->select.select,"",arglist,11);
	XtAddCallback (trace->select.add_sigs, XmNextendedSelectionCallback, sig_sel_add_list_cb, trace);
	XtManageChild (trace->select.add_sigs);

	/*** Delete (existing list) section ***/
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Delete Signal Pattern"));
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[2], XmNleftWidget, trace->select.add_sigs);
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_FORM );
	trace->select.label4 = XmCreateLabel (trace->select.select,"label4",arglist,4);
	XtManageChild (trace->select.label4);

	XtSetArg (arglist[0], XmNrows, 1);
	XtSetArg (arglist[1], XmNcolumns, 30);
	XtSetArg (arglist[1], XmNvalue, "*");
	XtSetArg (arglist[2], XmNresizeHeight, FALSE);
	XtSetArg (arglist[3], XmNeditMode, XmSINGLE_LINE_EDIT);
	XtSetArg (arglist[4], XmNleftAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[5], XmNleftWidget, trace->select.add_sigs);
	XtSetArg (arglist[6], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[7], XmNtopWidget, trace->select.label2);
	XtSetArg (arglist[8], XmNrightAttachment, XmATTACH_FORM );
	trace->select.delete_pat = XmCreateText (trace->select.select,"apat",arglist,9);
	XtAddCallback (trace->select.delete_pat, XmNactivateCallback, sig_sel_pattern_cb, trace);
	XtManageChild (trace->select.delete_pat);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Delete All") );
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[2], XmNleftWidget, trace->select.add_sigs);
	XtSetArg (arglist[3], XmNbottomAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNbottomWidget, trace->select.ok);
	trace->select.delete_all = XmCreatePushButton (trace->select.select,"dall",arglist,5);
	XtRemoveAllCallbacks (trace->select.delete_all, XmNactivateCallback);
	XtAddCallback (trace->select.delete_all, XmNactivateCallback, sig_sel_del_all_cb, trace);
	XtManageChild (trace->select.delete_all);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Delete Constant Valued") );
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[2], XmNleftWidget, trace->select.delete_all);
	XtSetArg (arglist[3], XmNbottomAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNbottomWidget, trace->select.ok);
	trace->select.delete_const = XmCreatePushButton (trace->select.select,"dcon",arglist,5);
	XtRemoveAllCallbacks (trace->select.delete_const, XmNactivateCallback);
	XtAddCallback (trace->select.delete_const, XmNactivateCallback, sig_sel_del_const_cb, trace);
	XtManageChild (trace->select.delete_const);

	XtSetArg (arglist[0], XmNlabelString, string_create_with_cr ("Select a signal to delete it\nfrom the displayed signals."));
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[2], XmNleftWidget, trace->select.add_sigs);
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNtopWidget, trace->select.delete_pat);
	trace->select.label3 = XmCreateLabel (trace->select.select,"label3",arglist,5);
	XtManageChild (trace->select.label3);
	
	XtSetArg (arglist[0], XmNrightAttachment, XmATTACH_FORM );
	XtSetArg (arglist[1], XmNlistVisibleItemCount, 5);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopWidget, trace->select.label3);
	XtSetArg (arglist[4], XmNbottomAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[5], XmNbottomWidget, trace->select.delete_all);
	XtSetArg (arglist[6], XmNleftAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[7], XmNleftWidget, trace->select.add_sigs);
	XtSetArg (arglist[8], XmNlistSizePolicy, XmCONSTANT);
	XtSetArg (arglist[9], XmNselectionPolicy, XmEXTENDED_SELECT);
	XtSetArg (arglist[10], XmNitems, 0);
	trace->select.delete_sigs = XmCreateScrolledList (trace->select.select,"",arglist,11);
	XtAddCallback (trace->select.delete_sigs, XmNextendedSelectionCallback, sig_sel_del_list_cb, trace);
	XtManageChild (trace->select.delete_sigs);
	}
    
    /* manage the popup on the screen */
    XtManageChild (trace->select.select);

    /* update patterns - leave under the "manage" or toolkit will complain */
    sig_sel_pattern_cb (NULL, trace, NULL);
    }

void    sig_sel_update_pattern (w, head_sig_ptr, pattern, xs_list, xs_sigs, xs_size)
    Widget	w;			/* List widget to update */
    SIGNAL	*head_sig_ptr;		/* Head signal in the list */
    char	*pattern;		/* Pattern to match to the list */
    XmString	**xs_list;		/* Static storage for string list */
    SIGNAL	***xs_sigs;		/* Static storage for signal list */
    int		*xs_size;
{
    SIGNAL	*sig_ptr;
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
	*xs_sigs = (SIGNAL **)XtMalloc (sel_count * sizeof (SIGNAL *));
	*xs_size = sel_count;
	}
    else if (*xs_size < sel_count) {
	*xs_list = (XmString *)XtRealloc ((char*)*xs_list, sel_count * sizeof (XmString));
	*xs_sigs = (SIGNAL **)XtRealloc ((char*)*xs_sigs, sel_count * sizeof (SIGNAL *));
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

void    sig_sel_pattern_cb (w,trace,cb)
    Widget			w;
    TRACE			*trace;
    XmAnyCallbackStruct		*cb;
    /* Called by creation or call back - create the list of signals with a possibly new pattern */
{
    char	*pattern;

    /* loop thru signals on deleted queue and add to list */
    pattern = XmTextGetString (trace->select.add_pat);
    sig_sel_update_pattern (trace->select.add_sigs, global->delsig, pattern,
			    &(trace->select.add_strings), &(trace->select.add_signals),
			    &(trace->select.add_size));

    /* loop thru signals on displayed queue and add to list */
    pattern = XmTextGetString (trace->select.delete_pat);
    sig_sel_update_pattern (trace->select.delete_sigs, trace->firstsig, pattern,
			    &(trace->select.del_strings), &(trace->select.del_signals),
			    &(trace->select.del_size));
    }

void    sig_sel_ok_cb (w,trace,cb)
    Widget			w;
    TRACE			*trace;
    XmAnyCallbackStruct		*cb;
{
    draw_all_needed ();
    }

void    sig_sel_apply_cb (w,trace,cb)
    Widget			w;
    TRACE			*trace;
    XmAnyCallbackStruct 	*cb;
{
    if (DTPRINT_ENTRY) printf ("In sig_sel_apply_cb - trace=%d\n",trace);

    sig_sel_ok_cb (w,trace,cb);
    sig_select_cb (w,trace,cb);
    }

void    sig_sel_add_all_cb (w,trace,cb)
    Widget			w;
    TRACE			*trace;
    XmAnyCallbackStruct 	*cb;
{
    SIGNAL	*sig_ptr;
    int		i;

    if (DTPRINT_ENTRY) printf ("In sig_sel_add_all_cb - trace=%d\n",trace);

    /* loop thru signals on deleted queue and add to list */
    for (i=0; 1; i++) {
	sig_ptr = (trace->select.add_signals)[i];
	if (!sig_ptr) break;

	/* Add it */
	sig_move (trace, sig_ptr, trace, ADD_LAST);
	}
    sig_sel_pattern_cb (NULL, trace, NULL);
    }

void    sig_sel_del_all_cb (w,trace,cb)
    Widget			w;
    TRACE			*trace;
    XmAnyCallbackStruct 	*cb;
{
    SIGNAL	*sig_ptr;
    int		i;

    if (DTPRINT_ENTRY) printf ("In sig_sel_del_all_cb - trace=%d\n",trace);

    /* loop thru signals on deleted queue and add to list */
    for (i=0; 1; i++) {
	sig_ptr = (trace->select.del_signals)[i];
	if (!sig_ptr) break;
	sig_delete (trace, sig_ptr);
	}
    sig_sel_pattern_cb (NULL, trace, NULL);
    }

void    sig_sel_del_const_cb (w,trace,cb)
    Widget			w;
    TRACE			*trace;
    XmAnyCallbackStruct 	*cb;
{
    SIGNAL	*sig_ptr;
    int		i;

    if (DTPRINT_ENTRY) printf ("In sig_sel_del_const_cb - trace=%d\n",trace);

    /* loop thru signals on deleted queue and add to list */
    for (i=0; 1; i++) {
	sig_ptr = (trace->select.del_signals)[i];
	if (!sig_ptr) break;

	/* Delete it */
	if (sig_is_constant (trace, sig_ptr)) {
	    sig_delete (trace, sig_ptr);
	    }
	}
    sig_sel_pattern_cb (NULL, trace, NULL);
    }

void    sig_sel_add_list_cb (w,trace,cb)
    Widget			w;
    TRACE			*trace;
    XmListCallbackStruct 	*cb;
{
    int		sel, i;
    SIGNAL	*sig_ptr;

    if (DTPRINT_ENTRY) printf ("In sig_sel_add_list_cb - trace=%d\n",trace);

    for (sel=0; sel < cb->selected_item_count; sel++) {
	i = (cb->selected_item_positions)[sel] - 1;
	sig_ptr = (trace->select.add_signals)[i];
	if (!sig_ptr) fprintf (stderr, "Unexpected null signal\n");
	/*if (DTPRINT) printf ("Pos %d sig '%s'\n", i, sig_ptr->signame);*/

	/* Add it */
	sig_move (trace, sig_ptr, trace, ADD_LAST);
	}
    sig_sel_pattern_cb (NULL, trace, NULL);
    }

void    sig_sel_del_list_cb (w,trace,cb)
    Widget			w;
    TRACE			*trace;
    XmListCallbackStruct	 *cb;
{
    int		sel, i;
    SIGNAL	*sig_ptr;

    if (DTPRINT_ENTRY) printf ("In sig_sel_del_list_cb - trace=%d\n",trace);

    for (sel=0; sel < cb->selected_item_count; sel++) {
	i = (cb->selected_item_positions)[sel] - 1;
	sig_ptr = (trace->select.del_signals)[i];
	if (!sig_ptr) fprintf (stderr, "Unexpected null signal\n");
	/*if (DTPRINT) printf ("Pos %d sig '%s'\n", i, sig_ptr->signame);*/

	/* Delete it */
	sig_delete (trace, sig_ptr);
	}
    sig_sel_pattern_cb (NULL, trace, NULL);
    }

/**********************************************************************/

#if defined(VMS) || defined(mips)
char *strdup(s)
    char *s;
{
    char *d;
    d=(char *)malloc(strlen(s)+1);
    strcpy (d, s);
    return (d);
    }
#endif

void sig_modify_en_signal (trace, en_sig_ptr, base_sig_ptr)
    TRACE	*trace;
    SIGNAL	*en_sig_ptr, *base_sig_ptr;
{
    SIGNAL	*new_sig_ptr;
    SIGNAL_LW	*base_cptr, *en_cptr;
    VALUE	new_value, base_value, en_value;
    int		has_ones;
    int		has_zeros;
    Boolean	first_last_data=TRUE;
    
    /*DTPRINT = (!strncmp(base_sig_ptr->signame, "k->MEMDATA", 10));*/

    if (DTPRINT_FILE) printf ("sig_modify_en_signal %s + %s -> %s\n", en_sig_ptr->signame,
			      base_sig_ptr->signame, base_sig_ptr->signame);

    if (en_sig_ptr->bits != base_sig_ptr->bits) {
	printf ("sig_modify_en_signal, can't combine different sizes of %s and %s!!!\n",
		base_sig_ptr->signame, base_sig_ptr->signame);
	return;
	}
    
    new_sig_ptr = replicate_signal (trace, base_sig_ptr);
    /* Forget this is a copy, and allocate new data storage space */
    new_sig_ptr->signame = strdup (new_sig_ptr->signame);
    new_sig_ptr->copyof = NULL;
    new_sig_ptr->bptr = (SIGNAL_LW *)XtMalloc (BLK_SIZE);
    new_sig_ptr->cptr = new_sig_ptr->bptr;
    new_sig_ptr->blocks = 1;

    base_cptr = (SIGNAL_LW *)(base_sig_ptr->bptr),
    en_cptr = (SIGNAL_LW *)(en_sig_ptr->bptr);
    cptr_to_value (base_cptr, &base_value);
    cptr_to_value (en_cptr, &en_value);

    while ((base_cptr->sttime.time != EOT) 
	   && (en_cptr->sttime.time != EOT)) {
	/*if (DTPRINT_FILE) {
	    printf ("BASE "); print_cptr (base_cptr);
	    printf ("EN "); print_cptr (en_cptr);
	    }*/
	 
	if (base_cptr->sttime.time == en_cptr->sttime.time) {
	    cptr_to_value (en_cptr, &en_value);
	    cptr_to_value (base_cptr, &base_value);
	    }
	else if (base_cptr->sttime.time < en_cptr->sttime.time) {
	    cptr_to_value (base_cptr, &base_value);
	    }
	else {
	    cptr_to_value (en_cptr, &en_value);
	    }

	/** start of determining new value **/
	switch (en_value.siglw.sttime.state) {
	  case STATE_0:
	    has_ones = 0;
	    has_zeros = 1;
	    break;
	  case STATE_1:
	    has_ones = 1;
	    has_zeros = 0;
	    break;
	  case STATE_B32:
	  case STATE_B64:
	  case STATE_B96:
	    has_ones =
		( en_value.number[0] & en_sig_ptr->value_mask[0]
		 | en_value.number[1] & en_sig_ptr->value_mask[1]
		 | en_value.number[2] & en_sig_ptr->value_mask[2] );
	    has_zeros =
		(  (~ en_value.number[0]) & en_sig_ptr->value_mask[0]
		 | (~ en_value.number[1]) & en_sig_ptr->value_mask[1]
		 | (~ en_value.number[2]) & en_sig_ptr->value_mask[2] );
	    break;
	    } /* switch */

	/* printf ("has0=%d has1=%d\n", has_zeros, has_ones); */

	new_value.siglw.number = base_value.siglw.number;
	new_value.number[0] = base_value.number[0];
	new_value.number[1] = base_value.number[1];
	new_value.number[2] = base_value.number[2];

	if (has_zeros) {
	    if (has_ones) {
		new_value.siglw.sttime.state = STATE_U;
		}
	    else {
		new_value.siglw.sttime.state = STATE_Z;
		}
	    }
	/** end of determining new value **/

	/* Calc time */
	if (base_cptr->sttime.time == en_cptr->sttime.time) {
	    new_value.siglw.sttime.time = base_cptr->sttime.time;
	    base_cptr += base_sig_ptr->lws;
	    en_cptr += en_sig_ptr->lws;
	    }
	else if (base_cptr->sttime.time < en_cptr->sttime.time) {
	    new_value.siglw.sttime.time = base_cptr->sttime.time;
	    base_cptr += base_sig_ptr->lws;
	    }
	else {
	    new_value.siglw.sttime.time = en_cptr->sttime.time;
	    en_cptr += en_sig_ptr->lws;
	    }

	if ((base_cptr->sttime.time == EOT) && (en_cptr->sttime.time == EOT)) {
	    first_last_data=TRUE;
	    }
	/*if (DTPRINT_FILE) {
	    printf ("%s Time %d, type = %d%s\n",
		    new_sig_ptr->signame,
		    new_value.siglw.sttime.time,
		    new_value.siglw.sttime.state,
		    (first_last_data?", LAST_DATA": ""));
	    }*/
	fil_add_cptr (new_sig_ptr, &new_value, !first_last_data);
	first_last_data=FALSE;
	}

    /* Add ending data & EOT marker */
    new_value.siglw.sttime.time = EOT;
    fil_add_cptr (new_sig_ptr, &new_value, FALSE);
    new_sig_ptr->cptr = new_sig_ptr->bptr;

    add_signal_to_queue (trace, new_sig_ptr, base_sig_ptr, &trace->dispsig);
    trace->numsig++;

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

void strcpy_overlap (d, s)
    char *d,*s;
{
    while (*d++ = *s++) ;
    }

void sig_modify_enables (trace)
    TRACE	*trace;
{
    SIGNAL	*sig_ptr, *en_sig_ptr, *base_sig_ptr, *new_sig_ptr;
    char	*tp, *nonenablename;
    Boolean	did_one=FALSE;
    
    for (sig_ptr = trace->firstsig; sig_ptr; ) {
	for (tp=sig_ptr->signame; *tp; tp++) {
	    if (tp[0]=='_' && tp[1]=='_'
		&& ( ( tp[2]=='e' && tp[3]=='n')
		    || (tp[2]=='E' && tp[3]=='N')
		    || ( tp[2]=='i' && tp[3]=='n' && tp[4]=='e' && tp[5]=='n')
		    || ( tp[2]=='I' && tp[3]=='N' && tp[4]=='E' && tp[5]=='N') ))
		break;
	    }
	if (*tp) {
	    /* Got enable! */

	    /* if (DTPRINT_FILE) printf ("Checking %s\n", sig_ptr->signame); */
	    nonenablename = strdup (sig_ptr->signame);

	    /* Chop _en from the middle of the name (sig__en<xx> -> sig<xx>)*/
	    if (tp[2]=='e' || tp[2]=='E') {
		strcpy_overlap (nonenablename + (tp - sig_ptr->signame), 
				nonenablename + (tp - sig_ptr->signame) + 4);
		}
	    else {
		/* (sig__inen<xx> -> sig__in<xx>)*/
		strcpy_overlap (nonenablename + (tp - sig_ptr->signame) + 4, 
				nonenablename + (tp - sig_ptr->signame) + 6);
		}
	    
	    base_sig_ptr = sig_find_signame (trace, nonenablename);

	    en_sig_ptr = sig_ptr;
	    sig_ptr = sig_ptr->forward;	/*** INC early as pointers may change! ***/
	    free (nonenablename);

	    if (base_sig_ptr) {
		sig_modify_en_signal (trace, en_sig_ptr, base_sig_ptr);
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


