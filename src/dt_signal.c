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
 *
 */


#include <stdio.h>
/*#include <descrip.h> - removed for Ultrix support... */

#include <X11/DECwDwtApplProg.h>
#include <X11/Xlib.h>

#include "dinotrace.h"
#include "callbacks.h"

static int		select_moved = TRUE,selected_signal;
static SIGNAL_SB	*moved_sig_ptr;



void
remove_signal_from_queue(ptr,sig_ptr)
DISPLAY_SB	*ptr;
SIGNAL_SB	*sig_ptr;
{
    SIGNAL_SB		*prev_sig_ptr,*next_sig_ptr;

    /* obtain the prev and next signals */
    prev_sig_ptr = sig_ptr->backward;
    next_sig_ptr = sig_ptr->forward;

    /* redirect the forward pointer */
    prev_sig_ptr->forward = sig_ptr->forward;

    /* if not the last signal redirect the backward pointer */
    if ( next_sig_ptr != NULL )
    {
	next_sig_ptr->backward = sig_ptr->backward;
    }

    /* if the first signal was removed change the startsig pointer */
    if ( sig_ptr == ptr->startsig )
    {
	ptr->startsig = next_sig_ptr;
    }
}

void
add_signal_to_queue(ptr,sig_ptr,loc_sig_ptr)
DISPLAY_SB	*ptr;
SIGNAL_SB	*sig_ptr;
SIGNAL_SB	*loc_sig_ptr;
{
    SIGNAL_SB		*next_sig_ptr;

    /* obtain the next signal */
    next_sig_ptr = loc_sig_ptr->forward;

    /* connect new signal to previous signal on queue */
    loc_sig_ptr->forward = sig_ptr;
    sig_ptr->backward = loc_sig_ptr;

    /* connect new signal to next on queue if one exists */
    sig_ptr->forward = next_sig_ptr;
    if ( next_sig_ptr != NULL )
	next_sig_ptr->backward = sig_ptr;

    /* if the signal was added in the beginning change the startsig pointer */
    if ( next_sig_ptr == ptr->startsig )
    {
	ptr->startsig = sig_ptr;
    }
}

void
sig_add_cb(w,ptr,cb)
Widget		w;
DISPLAY_SB	*ptr;
DwtSelectionCallbackStruct *cb;
{
    int		i;
    SIGNAL_SB	*sig_ptr;

    if (DTPRINT) printf("In sig_add_cb - ptr=%d\n",ptr);

    if (!ptr->signal.customize)
    {
	XtSetArg(arglist[0],DwtNdefaultPosition, TRUE);
	XtSetArg(arglist[1],DwtNwidth, 200);
	XtSetArg(arglist[2],DwtNheight, 150);
	XtSetArg(arglist[3],DwtNtitle, DwtLatin1String("Signal Select"));
	ptr->signal.customize = DwtDialogBoxPopupCreate(ptr->work,
							"",arglist,4);

	/* create label widget for text widget */
	XtSetArg(arglist[0], DwtNlabel,
		DwtLatin1String("Select Signal Then Location") );
	XtSetArg(arglist[1], DwtNx, 5);
	XtSetArg(arglist[2], DwtNy, 5);
	ptr->signal.label = DwtLabelCreate(ptr->signal.customize,"",arglist,3);
	XtManageChild(ptr->signal.label);

	/* create the list box */
	XtSetArg(arglist[0], DwtNx, 20);
	XtSetArg(arglist[1], DwtNy, 20);
	XtSetArg(arglist[2],DwtNvisibleItemsCount, 4);
	dino_cb[0].proc = sig_selected_cb;
	dino_cb[0].tag = ptr;
	XtSetArg(arglist[3], DwtNsingleCallback, dino_cb);
	ptr->signal.text = DwtListBoxCreate(ptr->signal.customize,"",arglist,4);
	XtManageChild(ptr->signal.text);

	/* Create OK button */
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String(" OK ") );
	XtSetArg(arglist[1], DwtNx, 20);
	XtSetArg(arglist[2], DwtNy, 90);
	dino_cb[0].proc = sig_ok_cb;
	dino_cb[0].tag = ptr;
	XtSetArg(arglist[3], DwtNactivateCallback, dino_cb);
	ptr->signal.b1 = DwtPushButtonCreate(ptr->signal.customize,
								"",arglist,4);
	XtManageChild(ptr->signal.b1);

	/* create cancel button */
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Cancel") );
	XtSetArg(arglist[1], DwtNx, 60);
	XtSetArg(arglist[2], DwtNy, 90);
	dino_cb[0].proc = sig_cancel_cb;
	dino_cb[0].tag = ptr;
	XtSetArg(arglist[3], DwtNactivateCallback, dino_cb);
	ptr->signal.b2 = DwtPushButtonCreate(ptr->signal.customize,"",arglist,4);
	XtManageChild(ptr->signal.b2);

	/* loop thru signals on deleted queue and add to list */
	sig_ptr = ptr->del.forward;
	for (i=0; i<ptr->numsigdel; i++)
	{
	    DwtListBoxAddItem(ptr->signal.text,
					DwtLatin1String(sig_ptr->signame),0);
	    sig_ptr = sig_ptr->forward;
	}

    }
    else
    {
	XtUnmanageChild(ptr->signal.customize);
    }

    /* if there are signals deleted make OK button active */
    if ( ptr->numsigdel == 0 )
	XtSetArg(arglist[0],DwtNsensitive, FALSE);
    else
	XtSetArg(arglist[0],DwtNsensitive, TRUE);
    XtSetValues(ptr->signal.b1,arglist,1);

    /* manage the popup on the screen */
    XtManageChild(ptr->signal.customize);

    return;
}

void
sig_selected_cb(w,ptr,cb)
Widget				w;
DISPLAY_SB			*ptr;
DwtListBoxCallbackStruct	*cb;
{
    int			i;
    char		*sig_name;

    if (DTPRINT) printf("In sig_selected_cb - ptr=%d\n",ptr);

    /* save the deleted signal selected number */
    selected_signal = cb->item_number-1;

    /* remove any previous events */
    remove_all_events(ptr);

    /* process all subsequent button presses as signal adds */ 
    XtAddEventHandler(ptr->work,ButtonPressMask,TRUE,add_signal,ptr);
}

void
sig_ok_cb(w,ptr,cb)
Widget			w;
DISPLAY_SB		*ptr;
DwtAnyCallbackStruct	*cb;
{
    char		*sig_name;

    if (DTPRINT) printf("In sig_ok_cb - ptr=%d\n",ptr);

    /* remove any previous events */
    remove_all_events(ptr);

    /* process all subsequent button presses as signal adds */ 
    XtAddEventHandler(ptr->work,ButtonPressMask,TRUE,add_signal,ptr);

    /* unmanage the popup on the screen */
    XtUnmanageChild(ptr->signal.customize);

    return;
}

void
sig_cancel_cb(w,ptr,cb)
Widget			w;
DISPLAY_SB		*ptr;
DwtAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In sig_cancel_cb - ptr=%d\n",ptr);

    /* remove any previous events */
    remove_all_events(ptr);

    /* unmanage the popup on the screen */
    XtUnmanageChild(ptr->signal.customize);
}

void
sig_mov_cb(w,ptr,cb)
Widget			w;
DISPLAY_SB		*ptr;
DwtAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In sig_mov_cb - ptr=%d\n",ptr);

    /* remove any previous events */
    remove_all_events(ptr);

    /* guarantee next button press selects a signal to be moved */
    select_moved = TRUE;

    /* process all subsequent button presses as signal moves */ 
    XtAddEventHandler(ptr->work,ButtonPressMask,TRUE,move_signal,ptr);

}

void
sig_del_cb(w,ptr,cb)
Widget			w;
DISPLAY_SB		*ptr;
DwtAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In sig_del_cb - ptr=%d\n",ptr);

    /* remove any previous events */
    remove_all_events(ptr);

    /* process all subsequent button presses as signal deletions */ 
    XtAddEventHandler(ptr->work,ButtonPressMask,TRUE,delete_signal,ptr);

}

void
sig_reset_cb(w,ptr,cb)
Widget			w;
DISPLAY_SB		*ptr;
DwtAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In sig_reset_cb - ptr=%d\n",ptr);

    /* remove any previous events */
    remove_all_events(ptr);

    /* unmanage the popup on the screen */
    XtUnmanageChild(ptr->signal.customize);

    /* ADD RESET CODE !!! */

    return;
}

void
add_signal(w,ptr,ev)
Widget			w;
DISPLAY_SB		*ptr;
XButtonPressedEvent	*ev;
{
    int			i,num,max_y;
    SIGNAL_SB		*sig_ptr,*prev_del_sig_ptr,*next_del_sig_ptr,*loc_sig_ptr;

    if (DTPRINT) printf("In add_signal - ptr=%d\n",ptr);

    /* return if there is no file */
    if ( ptr->filename[0] == '\0')
	return;

    /* make sure button has been clicked in in valid location of screen */
    max_y = MIN(ptr->numsig - ptr->numsigdel,ptr->numsigvis);
    if ( ev->y < ptr->ystart || ev->y > ptr->ystart + max_y * ptr->sighgt )
	return;

    /* figure out which signal has been selected to add new signal before */
    num = (int)((ev->y - ptr->ystart) / ptr->sighgt);

    /* obtain the signal location */
    loc_sig_ptr = &ptr->sig;
    for (i=0; i<num;i++)
	loc_sig_ptr = loc_sig_ptr->forward;

    /* obtain the signal to add */
    sig_ptr = ptr->del.forward;
    for (i=0; i<selected_signal;i++)
	sig_ptr = sig_ptr->forward;

    /* remove signal from deleted queue */
    remove_signal_from_queue(ptr,sig_ptr);
    ptr->numsigdel--;

    /* remove signal from list box */
    DwtListBoxDeleteItem(ptr->signal.text,DwtLatin1String(sig_ptr->signame) );

    /* add signal to signal queue in specified location */
    add_signal_to_queue(ptr,sig_ptr,loc_sig_ptr);

#if 0
 printf("Adding %d) %s after %s at %d\n",
 selected_signal,sig_ptr->signame,loc_sig_ptr->signame,num);
 printf("Deleted Signals (%d)\n",ptr->numsigdel);
 sig_ptr = ptr->del.forward;
 for (i=0; i<ptr->numsigdel;i++)
 {
     printf("%d) %s\n",i,sig_ptr->signame);
     sig_ptr = sig_ptr->forward;
 }
#endif

    /* remove any previous events */
    remove_all_events(ptr);

    /* pop window back to top of stack */
    XtUnmanageChild( ptr->signal.customize );
    XtManageChild( ptr->signal.customize );

    /* redraw the screen */
    get_geometry(ptr);
    XClearWindow(ptr->disp,ptr->wind);
    drawsig(ptr);
    draw(ptr);

    return;
}

void
move_signal(w,ptr,ev)
Widget			w;
DISPLAY_SB		*ptr;
XButtonPressedEvent	*ev;
{
    int			i,num,max_y;
    SIGNAL_SB		*sig_ptr,*prev_sig_ptr,*next_sig_ptr;

    if (DTPRINT) printf("In move_signal - ptr=%d\n",ptr);

    /* return if there is no file */
    if ( ptr->filename[0] == '\0')
	return;

    /* return if there is less than 2 signals to move */
    if ( ptr->numsig - ptr->numsigdel < 2 )
	return;

    /* make sure button has been clicked in in valid location of screen */
    max_y = MIN(ptr->numsig - ptr->numsigdel,ptr->numsigvis);
    if ( ev->y < ptr->ystart || ev->y > ptr->ystart + max_y * ptr->sighgt )
	return;

    /* figure out which signal has been selected */
    num = (int)((ev->y - ptr->ystart) / ptr->sighgt);

    if ( select_moved == TRUE )
    {
	/* set static pointer to first signal to move */
	moved_sig_ptr = ptr->startsig;
	for (i=0; i<num; i++)
	    moved_sig_ptr = moved_sig_ptr->forward;

	/* next call will perform the move */
	select_moved = FALSE;
    }
    else
    {
	/* set pointer to signal to insert moved signal */
	sig_ptr = ptr->startsig;
	sig_ptr = sig_ptr->backward;
	for (i=0; i<num; i++)
	    sig_ptr = sig_ptr->forward;

	/* if not the same signal perform the move */
	if ( sig_ptr != moved_sig_ptr )
	{
	    /* remove signal from the queue */
	    remove_signal_from_queue(ptr,moved_sig_ptr);

	    /* add the signal to the new location */
	    add_signal_to_queue(ptr,moved_sig_ptr,sig_ptr);
	}

	/* guarantee that next button press will select signal */
	select_moved = TRUE;
    }

    /* redraw the screen */
    get_geometry(ptr);
    XClearWindow(ptr->disp,ptr->wind);
    drawsig(ptr);
    draw(ptr);

    return;
}

void
delete_signal(w,ptr,ev)
Widget			w;
DISPLAY_SB		*ptr;
XButtonPressedEvent	*ev;
{
    int			i,num,max_y;
    SIGNAL_SB		*sig_ptr,*prev_sig_ptr,*next_sig_ptr;

    if (DTPRINT) printf("In delete_signal - ptr=%d\n",ptr);

    /* return if there is no file */
    if ( ptr->filename[0] == '\0')
	return;

    /* return if there are no signals to delete */
    if ( ptr->numsig - ptr->numsigdel == 0 )
	return;

    /* make sure button has been clicked in in valid location of screen */
    max_y = MIN(ptr->numsig - ptr->numsigdel,ptr->numsigvis);
    if ( ev->y < ptr->ystart || ev->y > ptr->ystart + max_y * ptr->sighgt )
	return;

    /* figure out which signal has been selected */
    num = (int)((ev->y - ptr->ystart) / ptr->sighgt);

    /* set pointer to signal to 'delete' */
    sig_ptr = ptr->startsig;
    for (i=0; i<num; i++)
	sig_ptr = sig_ptr->forward;

    /* remove the signal from the queue */
    remove_signal_from_queue(ptr,sig_ptr);

    /* add signal to deleted queue */
    add_signal_to_queue(ptr,sig_ptr,&ptr->del);

    /* add signame to list box */
    if ( ptr->signal.customize != NULL )
    {
	DwtListBoxAddItem(ptr->signal.text,DwtLatin1String(sig_ptr->signame),1);
    }

    /* increment the deleted signal counter */
    ptr->numsigdel++;
/*
printf("ev->y=%d num=%d numsig=%d numsigvis=%d numsigdel=%d\n",
ev->y,num,ptr->numsig,ptr->numsigvis,ptr->numsigdel);
printf("Deleted Signals (%d)\n",ptr->numsigdel);
sig_ptr = ptr->del.forward;
for (i=0; i<ptr->numsigdel;i++)
{
    printf("%d) %s\n",i,sig_ptr->signame);
    sig_ptr = sig_ptr->forward;
}
*/
    /* redraw the screen */
    get_geometry(ptr);
    XClearWindow(ptr->disp,ptr->wind);
    drawsig(ptr);
    draw(ptr);

    return;
}
