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
#include <X11/Xm.h>

#include "dinotrace.h"
#include "callbacks.h"

extern void highlight_signal();



void    remove_signal_from_queue(trace, sig_ptr)
    TRACE	*trace;
    SIGNAL_SB	*sig_ptr;	/* Signal to remove */
    /* Removes the signal from the current and any other ques that it is in */
{
    SIGNAL_SB	*next_sig_ptr, *prev_sig_ptr;
    
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
    if ( sig_ptr == trace->delsig ) {
	trace->delsig = sig_ptr->forward;
	}
    }

void    add_signal_to_queue(trace,sig_ptr,loc_sig_ptr,top_pptr)
    TRACE	*trace;
    SIGNAL_SB	*sig_ptr;	/* Signal to add */
    SIGNAL_SB	*loc_sig_ptr;	/* Pointer to signal ahead of one to add, NULL=1st */
    SIGNAL_SB	**top_pptr;	/* Pointer to where top of list is stored */
{
    SIGNAL_SB		*next_sig_ptr, *prev_sig_ptr;
    
    if (DTPRINT) printf("In add_signal_from_queue - trace=%d\n",trace);
    
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
    if ( next_sig_ptr == trace->delsig ) {
	trace->delsig = sig_ptr;
	}
    }


void    sig_add_cb(w,trace,cb)
    Widget		w;
    TRACE	*trace;
    XmSelectionBoxCallbackStruct *cb;
{
    int		i;
    SIGNAL_SB	*sig_ptr;
    Widget	list_wid;
    
    if (DTPRINT) printf("In sig_add_cb - trace=%d\n",trace);
    
    if (!trace->signal.customize) {
	XtSetArg(arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg(arglist[1], XmNwidth, 200);
	XtSetArg(arglist[2], XmNheight, 150);
	XtSetArg(arglist[3], XmNdialogTitle, XmStringCreateSimple("Signal Select"));
	XtSetArg(arglist[4], XmNlistVisibleItemCount, 5);
	XtSetArg(arglist[5], XmNlistLabelString, XmStringCreateSimple("Select Signal than Location"));
	trace->signal.customize = XmCreateSelectionDialog(trace->work,"",arglist,6);
	/* XtAddCallback(trace->signal.customize, XmNsingleCallback, sig_selected_cb, trace); */
	XtAddCallback(trace->signal.customize, XmNokCallback, sig_selected_cb, trace);
	XtAddCallback(trace->signal.customize, XmNapplyCallback, sig_selected_cb, trace);
	XtAddCallback(trace->signal.customize, XmNcancelCallback, sig_cancel_cb, trace);
	XtUnmanageChild( XmSelectionBoxGetChild (trace->signal.customize, XmDIALOG_HELP_BUTTON));
	XtUnmanageChild( XmSelectionBoxGetChild (trace->signal.customize, XmDIALOG_APPLY_BUTTON));
	XtUnmanageChild( XmSelectionBoxGetChild (trace->signal.customize, XmDIALOG_PROMPT_LABEL));
	XtUnmanageChild( XmSelectionBoxGetChild (trace->signal.customize, XmDIALOG_VALUE_TEXT));
	}
    else {
	XtUnmanageChild(trace->signal.customize);
	}
    
    /* loop thru signals on deleted queue and add to list */
    list_wid = XmSelectionBoxGetChild (trace->signal.customize, XmDIALOG_LIST);
    XmListDeleteAllItems (list_wid);
    for (sig_ptr = trace->delsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	XmListAddItem (list_wid, sig_ptr->xsigname, 0);
	}

    /* if there are signals deleted make OK button active */
    XtSetArg(arglist[0], XmNsensitive, (trace->delsig != NULL)?TRUE:FALSE);
    XtSetValues (XmSelectionBoxGetChild (trace->signal.customize, XmDIALOG_OK_BUTTON), arglist, 1);

    /* manage the popup on the screen */
    XtManageChild(trace->signal.customize);
    }

void    sig_selected_cb(w,trace,cb)
    Widget				w;
    TRACE			*trace;
    XmSelectionBoxCallbackStruct *cb;
{
    int			i;
    char		*sig_name;
    SIGNAL_SB		*sig_ptr;
    
    if (DTPRINT) printf("In sig_selected_cb - trace=%d\n",trace);
    
    if ( trace->delsig == NULL ) return;

    /* save the deleted signal selected number */
    for (sig_ptr = trace->delsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	if (XmStringCompare (cb->value, sig_ptr->xsigname)) break;
	}
    
    /* remove any previous events */
    remove_all_events(trace);
    
    if (sig_ptr && XmStringCompare (cb->value, sig_ptr->xsigname)) {
	trace->selected_sig_ptr = sig_ptr;
	/* process all subsequent button presses as signal adds */ 
	set_cursor (trace, DC_SIG_ADD);
	XtAddEventHandler(trace->work,ButtonPressMask,TRUE,add_signal,trace);

	/* unmanage the popup on the screen */
	XtUnmanageChild(trace->signal.customize);
	}
    else {
	trace->selected_sig_ptr = NULL;
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
    XtUnmanageChild(trace->signal.customize);
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
    trace->select_moved = TRUE;
    
    /* process all subsequent button presses as signal moves */ 
    XtAddEventHandler(trace->work,ButtonPressMask,TRUE,move_signal,trace);
    set_cursor (trace, DC_SIG_MOVE_1);
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
    XtAddEventHandler(trace->work,ButtonPressMask,TRUE,delete_signal,trace);
    }

void    sig_highlight_cb(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In sig_highlight_cb - trace=%d\n",trace);
    
    /* remove any previous events */
    remove_all_events(trace);
    
    /* process all subsequent button presses as signal deletions */ 
    set_cursor (trace, DC_SIG_HIGHLIGHT);
    XtAddEventHandler(trace->work,ButtonPressMask,TRUE,highlight_signal,trace);
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
    XtUnmanageChild(trace->signal.customize);
    
    /* ADD RESET CODE !!! */
    }

void    add_signal(w,trace,ev)
    Widget			w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    int			i,num,max_y;
    SIGNAL_SB		*sig_ptr,*prev_del_sig_ptr,*next_del_sig_ptr,*loc_sig_ptr;
    
    if (DTPRINT) printf("In add_signal - trace=%d\n",trace);
    
    /* return if there is no file */
    if (!trace->loaded || trace->selected_sig_ptr==NULL)
	return;
    
    /* make sure button has been clicked in in valid location of screen */
    max_y = MIN(trace->numsig - trace->numsigdel,trace->numsigvis);
    if ( ev->y < trace->ystart || ev->y > trace->ystart + max_y * trace->sighgt )
	return;
    
    /* figure out which signal has been selected to add new signal before */
    num = (int)((ev->y - trace->ystart) / trace->sighgt);
    
    /* obtain the signal location */
    loc_sig_ptr = &trace->dispsig;
    for (i=1; i<num;i++)
	loc_sig_ptr = loc_sig_ptr->forward;
    
    /* obtain the signal to add */
    sig_ptr = trace->selected_sig_ptr;
    
    /* remove signal from deleted queue */
    remove_signal_from_queue (trace, sig_ptr);
    trace->numsigdel--;
    
    /* remove signal from list box */
    XmListDeleteItem (XmSelectionBoxGetChild (trace->signal.customize, XmDIALOG_LIST),
		      sig_ptr->xsigname );
    
    /* add signal to signal queue in specified location */
    add_signal_to_queue(trace, sig_ptr, (num<2)?NULL:loc_sig_ptr, &trace->dispsig);
    
#if 0
    printf("Adding %d) %s after %s at %d\n",
	   selected_signal,sig_ptr->signame,loc_sig_ptr->signame,num);
    printf("Deleted Signals (%d)\n",trace->numsigdel);
    sig_ptr = trace->delsig;
    while (sig_ptr) {
	printf("%d) %s\n",i,sig_ptr->signame);
	sig_ptr = sig_ptr->forward;
	}
#endif
    
    /* remove any previous events */
    remove_all_events(trace);
    
    /* pop window back to top of stack */
    XtUnmanageChild( trace->signal.customize );
    XtManageChild( trace->signal.customize );
    
    /* redraw the screen */
    get_geometry(trace);
    XClearWindow(trace->display,trace->wind);
    draw(trace);
    }

void    move_signal(w,trace,ev)
    Widget			w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    int			i,num,max_y;
    SIGNAL_SB		*sig_ptr,*prev_sig_ptr,*next_sig_ptr;
    
    if (DTPRINT) printf("In move_signal - trace=%d\n",trace);
    
    /* return if there is no file */
    if ( !trace->loaded )
	return;
    
    /* return if there is less than 2 signals to move */
    if ( trace->numsig - trace->numsigdel < 2 )
	return;
    
    /* make sure button has been clicked in in valid location of screen */
    max_y = MIN(trace->numsig - trace->numsigdel,trace->numsigvis);
    if ( ev->y < trace->ystart || ev->y > trace->ystart + max_y * trace->sighgt )
	return;
    
    /* figure out which signal has been selected */
    num = (int)((ev->y - trace->ystart) / trace->sighgt);
    
    if ( trace->select_moved == TRUE )
	{
	/* set static pointer to first signal to move */
	trace->selected_sig_ptr = trace->dispsig;
	for (i=0; i<num; i++)
	    trace->selected_sig_ptr = trace->selected_sig_ptr->forward;
	
	/* next call will perform the move */
	trace->select_moved = FALSE;
	set_cursor (trace, DC_SIG_MOVE_2);
	}
    else
	{
	/* set pointer to signal to insert moved signal */
	sig_ptr = trace->dispsig;
	for (i=1; i<num; i++)
	    sig_ptr = sig_ptr->forward;
	
	/* if not the same signal perform the move */
	if ( sig_ptr != trace->selected_sig_ptr )
	    {
	    /* remove signal from the queue */
	    remove_signal_from_queue (trace, trace->selected_sig_ptr);
	    
	    /* add the signal to the new location */
	    add_signal_to_queue(trace, trace->selected_sig_ptr, (num<2)?NULL:sig_ptr, &trace->dispsig);
	    }
	
	/* guarantee that next button press will select signal */
	trace->select_moved = TRUE;
	set_cursor (trace, DC_SIG_MOVE_1);
	}
    
    /* redraw the screen */
    get_geometry(trace);
    XClearWindow(trace->display,trace->wind);
    draw(trace);
    }

void    delete_signal(w,trace,ev)
    Widget			w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    int			i,num,max_y;
    SIGNAL_SB		*sig_ptr,*prev_sig_ptr,*next_sig_ptr;
    
    if (DTPRINT) printf("In delete_signal - trace=%d\n",trace);
    
    /* return if there is no file */
    if ( !trace->loaded )
	return;
    
    /* return if there are no signals to delete */
    if ( trace->numsig - trace->numsigdel == 0 )
	return;
    
    /* make sure button has been clicked in in valid location of screen */
    max_y = MIN(trace->numsig - trace->numsigdel,trace->numsigvis);
    if ( ev->y < trace->ystart || ev->y > trace->ystart + max_y * trace->sighgt )
	return;
    
    /* figure out which signal has been selected */
    num = (int)((ev->y - trace->ystart) / trace->sighgt);

    if (DTPRINT) printf("In delete_signal - num=%d\n",num);
    
    /* set pointer to signal to 'delete' */
    sig_ptr = trace->dispsig;
    for (i=0; i<num; i++)
	sig_ptr = sig_ptr->forward;
    
    /* remove the signal from the queue */
    remove_signal_from_queue (trace, sig_ptr);
    
    /* add signal to deleted queue */
    add_signal_to_queue(trace,sig_ptr,NULL,&trace->delsig);
    
    /* add signame to list box */
    if ( trace->signal.customize != NULL ) {
	XmListAddItem (XmSelectionBoxGetChild (trace->signal.customize, XmDIALOG_LIST),
		       sig_ptr->xsigname, 0 );
	}
    
    /* increment the deleted signal counter */
    trace->numsigdel++;

    /*
    printf("ev->y=%d num=%d numsig=%d numsigvis=%d numsigdel=%d\n",
	   ev->y,num,trace->numsig,trace->numsigvis,trace->numsigdel);
    printf("Deleted Signals (%d)\n",trace->numsigdel);
    sig_ptr = trace->delsig;
    while (sig_ptr) {
	printf("%d) %s\n",i,sig_ptr->signame);
	sig_ptr = sig_ptr->forward;
	}
    */

    /* redraw the screen */
    get_geometry(trace);
    XClearWindow(trace->display,trace->wind);
    draw(trace);
    }


void    highlight_signal(w,trace,ev)
    Widget			w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    int			i,num,max_y;
    SIGNAL_SB		*sig_ptr,*prev_sig_ptr,*next_sig_ptr;
    
    if (DTPRINT) printf("In highlight_signal - trace=%d\n",trace);
    
    /* return if there is no file */
    if ( !trace->loaded )
	return;
    
    /* make sure button has been clicked in in valid location of screen */
    max_y = MIN(trace->numsig - trace->numsigdel,trace->numsigvis);
    if ( ev->y < trace->ystart || ev->y > trace->ystart + max_y * trace->sighgt )
	return;
    
    /* figure out which signal has been selected */
    num = (int)((ev->y - trace->ystart) / trace->sighgt);

    /* set pointer to signal to 'highlight' */
    sig_ptr = trace->dispsig;
    for (i=0; i<num; i++)
	sig_ptr = sig_ptr->forward;
    
    /* Change the color */
    if (sig_ptr->color == 0)
	sig_ptr->color = 1;
    else sig_ptr->color = 0;

    /* redraw the screen */
    get_geometry(trace);
    XClearWindow(trace->display,trace->wind);
    draw(trace);
    }


