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


/****************************** SEARCHING ******************************/

#pragma inline (value_to_string)
void    value_to_string (trace, strg, cptr)
    TRACE *trace;
    char *strg;
    unsigned int cptr[];
{
    if (cptr[0]) {
	if (trace->busrep == HBUS)
	    sprintf(strg,"%X %08X %08X", cptr[0], cptr[1], cptr[2]);
	else if (trace->busrep == OBUS)
	    sprintf(strg,"%o %o %o", cptr[0], cptr[1], cptr[2]);
	}
    else if (cptr[1]) {
	if (trace->busrep == HBUS)
	    sprintf(strg,"%X %08X", cptr[1], cptr[2]);
	else if (trace->busrep == OBUS)
	    sprintf(strg,"%o %o", cptr[1], cptr[2]);
	}
    else {
	if (trace->busrep == HBUS)
	    sprintf(strg,"%X", cptr[2]);
	else if (trace->busrep == OBUS)
	    sprintf(strg,"%o", cptr[2]);
	}
    }

void    string_to_value (trace, strg, cptr)
    TRACE *trace;
    char *strg;
    unsigned int cptr[];
{
    register char value;
    unsigned int MSO = (7<<29);		/* Most significant hex digit */
    unsigned int MSH = (15<<28);	/* Most significant octal digit */

    cptr[0] = cptr[1] = cptr[2] = 0;

    for (; *strg; strg++) {
	value = -1;
	if (*strg >= '0' && *strg <= '9')
	    value = *strg - '0';
	else if (*strg >= 'A' && *strg <= 'F')
	    value = *strg - ('A' - 10);
	else if (*strg >= 'a' && *strg <= 'f')
	    value = *strg - ('a' - 10);

	if (trace->busrep == HBUS && value >=0 && value <= 15) {
	    cptr[0] = (cptr[0]<<4) + ((cptr[1] & MSH)>>28);
	    cptr[1] = (cptr[1]<<4) + ((cptr[2] & MSH)>>28);
	    cptr[2] = (cptr[2]<<4) + value;
	    }
	else if (trace->busrep == OBUS && value >=0 && value <= 7) {
	    cptr[0] = (cptr[0]<<3) + ((cptr[1] & MSO)>>29);
	    cptr[1] = (cptr[1]<<3) + ((cptr[2] & MSO)>>29);
	    cptr[2] = (cptr[2]<<3) + value;
	    }
	}
    }


void    sig_search_cb(w,trace,cb)
    Widget		w;
    TRACE	*trace;
    XmSelectionBoxCallbackStruct *cb;
{
    int		i;
    int		y=10;
    char	strg[40];
    
    if (DTPRINT) printf("In sig_search_cb - trace=%d\n",trace);
    
    if (!trace->signal.search) {
	XtSetArg(arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg(arglist[1], XmNdialogTitle, XmStringCreateSimple ("Search Requester") );
	XtSetArg(arglist[2], XmNwidth, 500);
	XtSetArg(arglist[3], XmNheight, 400);
	trace->signal.search = XmCreateBulletinBoardDialog(trace->work,"search",arglist,4);
	
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("Color"));
	XtSetArg(arglist[1], XmNx, 5);
	XtSetArg(arglist[2], XmNy, y);
	trace->signal.label1 = XmCreateLabel(trace->signal.search,"label1",arglist,3);
	XtManageChild(trace->signal.label1);
	
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("Place"));
	XtSetArg(arglist[1], XmNx, 60);
	XtSetArg(arglist[2], XmNy, y);
	trace->signal.label2 = XmCreateLabel(trace->signal.search,"label2",arglist,3);
	XtManageChild(trace->signal.label2);
	
	y += 15;
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("Value"));
	XtSetArg(arglist[1], XmNx, 5);
	XtSetArg(arglist[2], XmNy, y);
	trace->signal.label4 = XmCreateLabel(trace->signal.search,"label4",arglist,3);
	XtManageChild(trace->signal.label4);
	
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("Cursor"));
	XtSetArg(arglist[1], XmNx, 60);
	XtSetArg(arglist[2], XmNy, y);
	trace->signal.label5 = XmCreateLabel(trace->signal.search,"label5",arglist,3);
	XtManageChild(trace->signal.label5);
	
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple
		 ( (trace->busrep == HBUS)? "Search value in HEX":"Search value in OCTAL" ) );
	XtSetArg(arglist[1], XmNx, 140);
	XtSetArg(arglist[2], XmNy, y);
	trace->signal.label3 = XmCreateLabel(trace->signal.search,"label3",arglist,3);
	XtManageChild(trace->signal.label3);
	
	y += 25;

	for (i=0; i<MAX_SRCH; i++) {
	    /* enable button */
	    XtSetArg(arglist[0], XmNx, 15);
	    XtSetArg(arglist[1], XmNy, y);
	    XtSetArg(arglist[2], XmNselectColor, trace->xcolornums[i+1]);
	    XtSetArg(arglist[3], XmNlabelString, XmStringCreateSimple (""));
	    trace->signal.enable[i] = XmCreateToggleButton(trace->signal.search,"togglen",arglist,4);
	    XtManageChild(trace->signal.enable[i]);

	    /* enable button */
	    XtSetArg(arglist[0], XmNx, 70);
	    XtSetArg(arglist[1], XmNy, y);
	    XtSetArg(arglist[2], XmNselectColor, trace->xcolornums[i+1]);
	    XtSetArg(arglist[3], XmNlabelString, XmStringCreateSimple (""));
	    trace->signal.cursor[i] = XmCreateToggleButton(trace->signal.search,"cursorn",arglist,4);
	    XtManageChild(trace->signal.cursor[i]);

	    /* create the file name text widget */
	    XtSetArg(arglist[0], XmNrows, 1);
	    XtSetArg(arglist[1], XmNcolumns, 30);
	    XtSetArg(arglist[2], XmNx, 120);
	    XtSetArg(arglist[3], XmNy, y);
	    XtSetArg(arglist[4], XmNresizeHeight, FALSE);
	    XtSetArg(arglist[5], XmNeditMode, XmSINGLE_LINE_EDIT);
	    trace->signal.text[i] = XmCreateText(trace->signal.search,"textn",arglist,6);
	    XtAddCallback(trace->signal.text[i], XmNactivateCallback, sig_search_ok_cb, trace);
	    XtManageChild(trace->signal.text[i]);
	    
	    y += 40;
	    }

	y+= 15;

	/* Create OK button */
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple(" OK ") );
	XtSetArg(arglist[1], XmNx, 10);
	XtSetArg(arglist[2], XmNy, y);
	trace->signal.ok = XmCreatePushButton(trace->signal.search,"ok",arglist,3);
	XtAddCallback(trace->signal.ok, XmNactivateCallback, sig_search_ok_cb, trace);
	XtManageChild(trace->signal.ok);
	
	/* create apply button */
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("Apply") );
	XtSetArg(arglist[1], XmNx, 70);
	XtSetArg(arglist[2], XmNy, y);
	trace->signal.apply = XmCreatePushButton(trace->signal.search,"apply",arglist,3);
	XtAddCallback(trace->signal.apply, XmNactivateCallback, sig_search_apply_cb, trace);
	XtManageChild(trace->signal.apply);
	
	/* create cancel button */
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("Cancel") );
	XtSetArg(arglist[1], XmNx, 140);
	XtSetArg(arglist[2], XmNy, y);
	trace->signal.cancel = XmCreatePushButton(trace->signal.search,"cancel",arglist,3);
	XtAddCallback(trace->signal.cancel, XmNactivateCallback, sig_search_cancel_cb, trace);
	XtManageChild(trace->signal.cancel);
	}
    
    /* Copy settings to local area to allow cancel to work */
    for (i=0; i<MAX_SRCH; i++) {
	/* Update with current search enables */
	XtSetArg (arglist[0], XmNset, (global->srch[i].color != 0));
	XtSetValues (trace->signal.enable[i], arglist, 1);

	/* Update with current cursor enables */
	XtSetArg (arglist[0], XmNset, (global->srch[i].cursor != 0));
	XtSetValues (trace->signal.cursor[i], arglist, 1);

	/* Update with current search values */
	value_to_string (trace, strg, global->srch[i].value);
	XmTextSetString (trace->signal.text[i], strg);
	}

    /* manage the popup on the screen */
    XtManageChild(trace->signal.search);
    }

void    sig_search_ok_cb(w,trace,cb)
    Widget				w;
    TRACE			*trace;
    XmSelectionBoxCallbackStruct *cb;
{
    char		*strg;
    int			i;

    if (DTPRINT) printf("In sig_search_ok_cb - trace=%d\n",trace);

    for (i=0; i<MAX_SRCH; i++) {
	/* Update with current search enables */
	if (XmToggleButtonGetState (trace->signal.enable[i]))
	    global->srch[i].color = i+1;
	else global->srch[i].color = 0;
	
	/* Update with current cursor enables */
	if (XmToggleButtonGetState (trace->signal.cursor[i]))
	    global->srch[i].cursor = i+1;
	else global->srch[i].cursor = 0;
	
	/* Update with current search values */
	strg = XmTextGetString (trace->signal.text[i]);
	string_to_value (trace, strg, global->srch[i].value);

	if (DTPRINT) {
	    char strg2[40];
	    value_to_string (trace, strg2, global->srch[i].value);
	    printf ("Search %d) %d   '%s' -> '%s'\n", i, global->srch[i].color, strg, strg2);
	    }
	}
    
    XtUnmanageChild(trace->signal.search);

    update_search ();

    /* redraw the display */
    redraw_all (trace);
    }

void    sig_search_apply_cb(w,trace,cb)
    Widget				w;
    TRACE			*trace;
    XmSelectionBoxCallbackStruct *cb;
{
    if (DTPRINT) printf("In sig_search_apply_cb - trace=%d\n",trace);

    sig_search_ok_cb (w,trace,cb);
    sig_search_cb (w,trace,cb);
    }

void    sig_search_cancel_cb(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In sig_search_cancel_cb - trace=%d\n",trace);
    
    /* unmanage the popup on the screen */
    XtUnmanageChild(trace->signal.search);
    }

