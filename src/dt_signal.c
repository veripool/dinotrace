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


#include <stdio.h>
/*#include <descrip.h> - removed for Ultrix support... */

#include <X11/Xlib.h>
#include <Xm/Xm.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/SelectioB.h>
#include <Xm/Text.h>
#include <Xm/BulletinB.h>

#include "dinotrace.h"
#include "callbacks.h"



/****************************** UTILITIES ******************************/

void    remove_signal_from_queue(trace, sig_ptr)
    TRACE	*trace;
    SIGNAL	*sig_ptr;	/* Signal to remove */
    /* Removes the signal from the current and any other ques that it is in */
{
    SIGNAL	*next_sig_ptr, *prev_sig_ptr;
    
    if (DTPRINT) printf("In remove_signal_from_queue - trace=%d\n",trace);
    
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
    }

void    add_signal_to_queue(trace,sig_ptr,loc_sig_ptr,top_pptr)
    TRACE	*trace;
    SIGNAL	*sig_ptr;	/* Signal to add */
    SIGNAL	*loc_sig_ptr;	/* Pointer to signal ahead of one to add, NULL=1st */
    SIGNAL	**top_pptr;	/* Pointer to where top of list is stored */
{
    SIGNAL		*next_sig_ptr, *prev_sig_ptr;
    
    if (DTPRINT) printf("In add_signal_to_queue - trace=%d\n",trace);
    
    /* Insert into first position? */
    if (loc_sig_ptr == NULL) {
	next_sig_ptr = *top_pptr;
	*top_pptr = sig_ptr;
	prev_sig_ptr = (next_sig_ptr)? next_sig_ptr->backward : NULL;
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
    if ( next_sig_ptr == trace->dispsig ) {
        trace->dispsig = sig_ptr;
	}
    /* if the signal is the signal, change it */
    if ( next_sig_ptr == trace->firstsig ) {
	trace->firstsig = sig_ptr;
	}
    /* if the signal is the deleted signal, change it */
    if ( next_sig_ptr == global->delsig ) {
	global->delsig = sig_ptr;
	}
    }

SIGNAL *replicate_signal (trace, sig_ptr)
    TRACE	*trace;
    SIGNAL	*sig_ptr;	/* Signal to remove */
    /* Makes a duplicate copy of the signal */
{
    SIGNAL	*new_sig_ptr;
    
    if (DTPRINT) printf("In replicate_signal - trace=%d\n",trace);
    
    /* Create new structure */
    new_sig_ptr = (SIGNAL *)XtMalloc(sizeof(SIGNAL));

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

/****************************** MENU OPTIONS ******************************/

void    sig_add_cb(w,trace,cb)
    Widget		w;
    TRACE	*trace;
    XmSelectionBoxCallbackStruct *cb;
{
    SIGNAL	*sig_ptr;
    Widget	list_wid;
    
    if (DTPRINT) printf("In sig_add_cb - trace=%d\n",trace);
    
    if (!trace->signal.add) {
	XtSetArg(arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg(arglist[1], XmNwidth, 200);
	XtSetArg(arglist[2], XmNheight, 150);
	XtSetArg(arglist[3], XmNdialogTitle, XmStringCreateSimple("Signal Select"));
	XtSetArg(arglist[4], XmNlistVisibleItemCount, 5);
	XtSetArg(arglist[5], XmNlistLabelString, XmStringCreateSimple("Select Signal than Location"));
	trace->signal.add = XmCreateSelectionDialog(trace->work,"",arglist,6);
	/* XtAddCallback(trace->signal.add, XmNsingleCallback, sig_selected_cb, trace); */
	XtAddCallback(trace->signal.add, XmNokCallback, sig_selected_cb, trace);
	XtAddCallback(trace->signal.add, XmNapplyCallback, sig_selected_cb, trace);
	XtAddCallback(trace->signal.add, XmNcancelCallback, sig_cancel_cb, trace);
	XtUnmanageChild( XmSelectionBoxGetChild (trace->signal.add, XmDIALOG_HELP_BUTTON));
	XtUnmanageChild( XmSelectionBoxGetChild (trace->signal.add, XmDIALOG_APPLY_BUTTON));
	XtUnmanageChild( XmSelectionBoxGetChild (trace->signal.add, XmDIALOG_PROMPT_LABEL));
	XtUnmanageChild( XmSelectionBoxGetChild (trace->signal.add, XmDIALOG_VALUE_TEXT));
	}
    else {
	XtUnmanageChild(trace->signal.add);
	}
    
    /* loop thru signals on deleted queue and add to list */
    list_wid = XmSelectionBoxGetChild (trace->signal.add, XmDIALOG_LIST);
    XmListDeleteAllItems (list_wid);
    for (sig_ptr = global->delsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	XmListAddItem (list_wid, sig_ptr->xsigname, 0);
	}

    /* if there are signals deleted make OK button active */
    XtSetArg(arglist[0], XmNsensitive, (global->delsig != NULL)?TRUE:FALSE);
    XtSetValues (XmSelectionBoxGetChild (trace->signal.add, XmDIALOG_OK_BUTTON), arglist, 1);

    /* manage the popup on the screen */
    XtManageChild(trace->signal.add);
    }

void    sig_selected_cb(w,trace,cb)
    Widget				w;
    TRACE			*trace;
    XmSelectionBoxCallbackStruct *cb;
{
    SIGNAL		*sig_ptr;
    
    if (DTPRINT) printf("In sig_selected_cb - trace=%d\n",trace);
    
    if ( global->delsig == NULL ) return;

    /* save the deleted signal selected number */
    for (sig_ptr = global->delsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	if (XmStringCompare (cb->value, sig_ptr->xsigname)) break;
	}
    
    /* remove any previous events */
    remove_all_events(trace);
    
    if (sig_ptr && XmStringCompare (cb->value, sig_ptr->xsigname)) {
	global->selected_sig = sig_ptr;
	/* process all subsequent button presses as signal adds */ 
	set_cursor (trace, DC_SIG_ADD);
	add_event (ButtonPressMask, sig_add_ev);

	/* unmanage the popup on the screen */
	XtUnmanageChild(trace->signal.add);
	}
    else {
	global->selected_sig = NULL;
	}
    }

void    sig_cancel_cb(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In sig_cancel_cb - trace=%d\n",trace);
    
    /* remove any previous events */
    remove_all_events(trace);
    
    /* unmanage the popup on the screen */
    XtUnmanageChild(trace->signal.add);
    }

void    sig_mov_cb(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In sig_mov_cb - trace=%d\n",trace);
    
    /* remove any previous events */
    remove_all_events(trace);
    
    /* guarantee next button press selects a signal to be moved */
    global->selected_sig = NULL;
    
    /* process all subsequent button presses as signal moves */ 
    add_event (ButtonPressMask, sig_move_ev);
    set_cursor (trace, DC_SIG_MOVE_1);
    }

void    sig_copy_cb(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In sig_copy_cb - trace=%d\n",trace);
    
    /* remove any previous events */
    remove_all_events(trace);
    
    /* guarantee next button press selects a signal to be moved */
    global->selected_sig = NULL;
    
    /* process all subsequent button presses as signal moves */ 
    add_event (ButtonPressMask, sig_copy_ev);
    set_cursor (trace, DC_SIG_COPY_1);
    }

void    sig_del_cb(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In sig_del_cb - trace=%d\n",trace);
    
    /* remove any previous events */
    remove_all_events(trace);
    
    /* process all subsequent button presses as signal deletions */ 
    set_cursor (trace, DC_SIG_DELETE);
    add_event (ButtonPressMask, sig_delete_ev);
    }

void    sig_highlight_cb(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    int i;

    if (DTPRINT) printf("In sig_highlight_cb - trace=%d\n",trace);
    
    /* remove any previous events */
    remove_all_events(trace);
     
    global->highlight_color = 0;
    for (i=1; i<=MAX_SRCH; i++) {
	if (w == trace->menu.pdsubbutton[i + trace->menu.sig_highlight_pds]) {
	    global->highlight_color = i;
	    }
	}

    /* process all subsequent button presses as signal deletions */ 
    set_cursor (trace, DC_SIG_HIGHLIGHT);
    add_event (ButtonPressMask, sig_highlight_ev);
    }

void    sig_reset_cb(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In sig_reset_cb - trace=%d\n",trace);
    
    /* remove any previous events */
    remove_all_events(trace);
    
    /* unmanage the popup on the screen */
    XtUnmanageChild(trace->signal.add);
    
    /* ADD RESET CODE !!! */
    }

/****************************** EVENTS ******************************/

void    sig_add_ev(w,trace,ev)
    Widget			w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    SIGNAL		*sig_ptr;
    
    if (DTPRINT) printf("In sig_add_ev - trace=%d\n",trace);
    
    /* return if there is no file */
    if (!trace->loaded || global->selected_sig==NULL)
	return;
    
    /* make sure button has been clicked in in valid location of screen */
    sig_ptr = posy_to_signal (trace, ev->y);
    /* Null signal is OK */

    /* get previous signal */
    if (sig_ptr) sig_ptr = sig_ptr->backward;
    
    /* remove signal from deleted queue */
    remove_signal_from_queue (trace, global->selected_sig);
    
    /* remove signal from list box */
    XmListDeleteItem (XmSelectionBoxGetChild (trace->signal.add, XmDIALOG_LIST),
		      global->selected_sig->xsigname );
    if (global->delsig == NULL) {
	XtSetArg(arglist[0], XmNsensitive, FALSE);
	XtSetValues (XmSelectionBoxGetChild (trace->signal.add, XmDIALOG_OK_BUTTON), arglist, 1);
	}
    
    /* add signal to signal queue in specified location */
    add_signal_to_queue(trace, global->selected_sig, sig_ptr, &trace->dispsig);
    trace->numsig++;
    
#if 0
    printf("Adding %d) %s after %s\n",
	   selected_signal, global->selected_sig->signame,sig_ptr->signame);
    sig_ptr = global->delsig;
    while (sig_ptr) {
	printf(" %s\n",sig_ptr->signame);
	sig_ptr = sig_ptr->forward;
	}
#endif
    
    /* remove any previous events */
    remove_all_events(trace);
    
    /* pop window back to top of stack */
    XtUnmanageChild( trace->signal.add );
    XtManageChild( trace->signal.add );
    
    /* redraw the screen */
    redraw_all (trace);
    }

void    sig_move_ev(w,trace,ev)
    Widget			w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    SIGNAL		*sig_ptr;
    
    if (DTPRINT) printf("In sig_move_ev - trace=%d\n",trace);
    
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
	    /* remove signal from the queue */
	    remove_signal_from_queue (global->selected_trace, global->selected_sig);
	    global->selected_trace->numsig--;
	    
	    /* add the signal to the new location */
	    add_signal_to_queue(trace, global->selected_sig, sig_ptr, &trace->dispsig);
	    trace->numsig++;
	    }
	
	/* guarantee that next button press will select signal */
	global->selected_sig = NULL;
	set_cursor (trace, DC_SIG_MOVE_1);
	}
    
    /* redraw the screen */
    redraw_all (trace);
    }

void    sig_copy_ev(w,trace,ev)
    Widget			w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    SIGNAL		*sig_ptr, *new_sig_ptr;
    
    if (DTPRINT) printf("In sig_copy_ev - trace=%d\n",trace);
    
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
	    /* make copy of signal */
	    new_sig_ptr = replicate_signal (global->selected_trace, global->selected_sig);
	    
	    /* add the signal to the new location */
	    add_signal_to_queue(trace, new_sig_ptr, sig_ptr, &trace->dispsig);
	    trace->numsig++;
	    }
	
	/* guarantee that next button press will select signal */
	global->selected_sig = NULL;
	set_cursor (trace, DC_SIG_COPY_1);
	}
    
    /* redraw the screen */
    redraw_all (trace);
    }

void    sig_delete_ev(w,trace,ev)
    Widget			w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    SIGNAL		*sig_ptr;
    
    if (DTPRINT) printf("In sig_delete_ev - trace=%d\n",trace);
    
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
    remove_signal_from_queue (trace, sig_ptr);
    trace->numsig--;

    /* add signal to deleted queue */
    add_signal_to_queue(trace,sig_ptr,NULL,&global->delsig);
    
    /* add signame to list box */
    if ( trace->signal.add != NULL ) {
	XmListAddItem (XmSelectionBoxGetChild (trace->signal.add, XmDIALOG_LIST),
		       sig_ptr->xsigname, 0 );
	}
    
    /*
    printf("ev->y=%d numsig=%d numsigvis=%d\n",
	   ev->y,trace->numsig,trace->numsigvis);
    sig_ptr = global->delsig;
    while (sig_ptr) {
	printf(" %s\n",sig_ptr->signame);
	sig_ptr = sig_ptr->forward;
	}
    */

    /* redraw the screen */
    redraw_all (trace);
    }


void    sig_highlight_ev(w,trace,ev)
    Widget			w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    SIGNAL		*sig_ptr;
    
    if (DTPRINT) printf("In sig_highlight_ev - trace=%d\n",trace);
    
    sig_ptr = posy_to_signal (trace, ev->y);
    if (!sig_ptr) return;
    
    /* Change the color */
    sig_ptr->color = global->highlight_color;

    /* redraw the screen */
    redraw_all (trace);
    }


