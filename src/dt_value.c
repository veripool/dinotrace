/******************************************************************************
 *
 * Filename:
 *     dt_value.c
 *
 * Subsystem:
 *     Dinotrace
 *
 * Version:
 *     Dinotrace V6.3
 *
 * Author:
 *     Wilson Snyder
 *
 * Abstract:
 *
 * Modification History:
 *     WPS	20-Jun-93	Created from dt_signal.c
 */


#include <stdio.h>
/*#include <descrip.h> - removed for Ultrix support... */
#include <math.h>

#include <X11/Xlib.h>
#include <Xm/Xm.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/SelectioB.h>
#include <Xm/Text.h>
#include <Xm/BulletinB.h>
#include <Xm/RowColumn.h>

#include "dinotrace.h"
#include "callbacks.h"


/****************************** UTILITIES ******************************/

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


/****************************** MENU OPTIONS ******************************/

void    val_examine_cb(w, trace, cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    
    if (DTPRINT) printf("In val_examine_cb - trace=%d\n",trace);
    
    /* remove any previous events */
    remove_all_events(trace);
    
    /* process all subsequent button presses as cursor moves */
    set_cursor (trace, DC_VAL_EXAM);
    add_event (ButtonPressMask, val_examine_ev);
    }


/****************************** EVENTS ******************************/

void    val_examine_popup (trace, x, y, ev)
    /* Create the popup menu for val_examine, based on cursor position x,y */
    TRACE		*trace;
    int			x,y;
    XButtonPressedEvent	*ev;
{
    int		time;
    SIGNAL	*sig_ptr;
    SIGNAL_LW	*cptr;
    char	strg[2000];
    char	strg2[2000];
    XmString	xs;
    int		value[3];
    int		val;
    int		rows, cols, bit, bit_value, row, col;
    
    time = posx_to_time (trace, x);
    sig_ptr = posy_to_signal (trace, y);
    
    if (trace->examine.popup) {
	XtUnmanageChild (trace->examine.popup);
	/* XtUnmanageChild (trace->examine.label); */
	trace->examine.popup = NULL;
	}
    
    if (time && sig_ptr) {
	/* Get information */
	cptr = (SIGNAL_LW *)(sig_ptr)->cptr;
	
	for (; (cptr->time != EOT) && (((cptr + sig_ptr->inc)->time) < time); cptr += sig_ptr->inc) ;
	
	strcpy (strg, sig_ptr->signame);
	
	if (cptr->time == EOT) {
	    strcat (strg, "\nValue at EOT:\n");
	    }
	else {
	    strcat (strg, "\nValue at times ");
	    time_to_string (trace, strg2, cptr->time, FALSE);
	    strcat (strg, strg2);
	    strcat (strg, " - ");
	    if (((cptr + sig_ptr->inc)->time) == EOT) {
		strcat (strg, "EOT:\n");
		}
	    else {
		time_to_string (trace, strg2, (cptr + sig_ptr->inc)->time, FALSE);
		strcat (strg, strg2);
		strcat (strg, ":\n");
		}
	    }
	
	value[0] = value[1] = value[2] = 0;
	switch (cptr->state) {
	  case STATE_1:
	    value[2] = 1;
	    break;
	    
	  case STATE_B32:
	    value[2] = *((unsigned int *)cptr+1);
	    break;
	
	  case STATE_B64:
	    value[1] = *((unsigned int *)cptr+1);
	    value[2] = *((unsigned int *)cptr+2);
	    break;

	  case STATE_B96:
	    value[0] = *((unsigned int *)cptr+1);
	    value[1] = *((unsigned int *)cptr+2);
	    value[2] = *((unsigned int *)cptr+3);
	    break;
	    } /* switch */

	switch (cptr->state) {
	  case STATE_0:
	  case STATE_1:
	    sprintf (strg2, "= %d\n", value[2]);
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
	  case STATE_B64:
	  case STATE_B96:
	    strcat (strg, "= ");
	    value_to_string (trace, strg2, value);
	    strcat (strg, strg2);
	    if ( (sig_ptr->decode != NULL) 
		&& (cptr->state == STATE_B32)
		&& (value[2] < MAXSTATENAMES)
                && (sig_ptr->decode->statename[value[2]][0] != '\0') ) {
		sprintf (strg2, " = %s\n", sig_ptr->decode->statename[value[2]] );
		strcat (strg, strg2);
		}
	    else strcat (strg, "\n");

	    /* Bitwise information */
	    rows = ceil (sqrt ((double)(sig_ptr->bits + 1)));
	    cols = ceil ((double)(rows) / 4.0) * 4;
	    rows = ceil ((double)(sig_ptr->bits + 1)/ (double)cols);

	    bit = 0;
	    for (row=rows - 1; row >= 0; row--) {
		for (col = cols - 1; col >= 0; col--) {
		    bit = (row * cols + col);

		    if (bit<32) bit_value = ( value[2] >> bit ) & 1;
			else if (bit<64) bit_value = ( value[1] >> (bit-32) ) & 1;
			    else  bit_value = ( value[0] >> (bit-64) ) & 1;

		    if ((bit>=0) && (bit <= sig_ptr->bits)) {
			sprintf (strg2, "<%02d>=%d ", bit + sig_ptr->index - sig_ptr->bits, bit_value);
			strcat (strg, strg2);
			}
		    }
		strcat (strg, "\n");
		}

            break;
            } /* Case */

	/* Debugging information */
	if (DTDEBUG) {
	    sprintf (strg2, "State %d\n", cptr->state);
	    strcat (strg, strg2);
	    sprintf (strg2, "Type %d   Inc %d   Blocks %d\n",
		     sig_ptr->type, sig_ptr->inc, sig_ptr->blocks);
	    strcat (strg, strg2);
	    sprintf (strg2, "Bits %d   Index %d  Srch_ena %d\n",
		     sig_ptr->bits, sig_ptr->index, sig_ptr->srch_ena);
	    strcat (strg, strg2);
	    sprintf (strg2, "Binary_type %d   Binary_Pos %d\n",
		     sig_ptr->binary_type, sig_ptr->binary_pos);
	    strcat (strg, strg2);
	    }
	
        /* Create */
        if (DTPRINT) printf ("\ttime = %d, signal = %s\n", time, sig_ptr->signame);

	XtSetArg(arglist[0], XmNentryAlignment, XmALIGNMENT_BEGINNING);
	trace->examine.popup = XmCreatePopupMenu (trace->main, "examinepopup", arglist, 1);

	xs = string_create_with_cr (strg);
	XtSetArg(arglist[0], XmNlabelString, xs);
	trace->examine.label = XmCreateLabel (trace->examine.popup,"popuplabel",arglist,1);
	XtManageChild (trace->examine.label);
	XmStringFree (xs);

	XmMenuPosition (trace->examine.popup, ev);
	XtManageChild (trace->examine.popup);
	}
    }
	
void    val_examine_ev(w, trace, ev)
    Widget		w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    XEvent	event;
    XMotionEvent *em;
    XButtonEvent *eb;
    int		update_pending = FALSE;
    
    if (DTPRINT) printf("In val_examine_ev\n");
    
    /* not sure why this has to be done but it must be done */
    XUngrabPointer(XtDisplay(trace->work),CurrentTime);
    
    /* select the events the widget will respond to */
    XSelectInput(XtDisplay(trace->work),XtWindow(trace->work),
		 ButtonReleaseMask|PointerMotionMask|StructureNotifyMask|ExposureMask);
    
    /* Create */
    val_examine_popup (trace, ev->x, ev->y, ev);
    
    /* loop and service events until button is released */
    while ( 1 ) {
	/* wait for next event */
	XNextEvent(XtDisplay(trace->work),&event);
	
	/* Mark an update as needed */
	if (event.type == MotionNotify) {
	    update_pending = TRUE;
	    }

	/* if window was exposed or resized, must redraw it */
	if (event.type == Expose ||
	    event.type == ConfigureNotify) {
	    win_expose_cb (0,trace);
	    }
	
	/* button released - calculate cursor position and leave the loop */
	if (event.type == ButtonRelease) {
	    break;
	    }

	/* If a update is needed, redraw the menu. */
	/* Do it later if events pending, otherwise dragging is SLOWWWW */
	if (update_pending && !XPending (global->display)) {
	    update_pending = FALSE;
	    em = (XMotionEvent *)&event;
	    val_examine_popup (trace, em->x, em->y, em);
	    }
	}
    
    /* unmanage popup */
    if (trace->examine.popup) {
	XtUnmanageChild (trace->examine.popup);
	XtUnmanageChild (trace->examine.label);
	trace->examine.popup = NULL;
	}

    /* reset the events the widget will respond to */
    XSelectInput(XtDisplay(trace->work),XtWindow(trace->work),
		 ButtonPressMask|StructureNotifyMask|ExposureMask);
    
    /* redraw the screen as popup may have mangled widgets */
    redraw_all (trace);
    }

void    val_search_cb(w,trace,cb)
    Widget		w;
    TRACE	*trace;
    XmSelectionBoxCallbackStruct *cb;
{
    int		i;
    int		y=10;
    char	strg[40];
    
    if (DTPRINT) printf("In val_search_cb - trace=%d\n",trace);
    
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
	    XtAddCallback(trace->signal.text[i], XmNactivateCallback, val_search_ok_cb, trace);
	    XtManageChild(trace->signal.text[i]);
	    
	    y += 40;
	    }

	y+= 15;

	/* Create OK button */
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple(" OK ") );
	XtSetArg(arglist[1], XmNx, 10);
	XtSetArg(arglist[2], XmNy, y);
	trace->signal.ok = XmCreatePushButton(trace->signal.search,"ok",arglist,3);
	XtAddCallback(trace->signal.ok, XmNactivateCallback, val_search_ok_cb, trace);
	XtManageChild(trace->signal.ok);
	
	/* create apply button */
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("Apply") );
	XtSetArg(arglist[1], XmNx, 70);
	XtSetArg(arglist[2], XmNy, y);
	trace->signal.apply = XmCreatePushButton(trace->signal.search,"apply",arglist,3);
	XtAddCallback(trace->signal.apply, XmNactivateCallback, val_search_apply_cb, trace);
	XtManageChild(trace->signal.apply);
	
	/* create cancel button */
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("Cancel") );
	XtSetArg(arglist[1], XmNx, 140);
	XtSetArg(arglist[2], XmNy, y);
	trace->signal.cancel = XmCreatePushButton(trace->signal.search,"cancel",arglist,3);
	XtAddCallback(trace->signal.cancel, XmNactivateCallback, val_search_cancel_cb, trace);
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

void    val_search_ok_cb(w,trace,cb)
    Widget				w;
    TRACE			*trace;
    XmSelectionBoxCallbackStruct *cb;
{
    char		*strg;
    int			i;

    if (DTPRINT) printf("In val_search_ok_cb - trace=%d\n",trace);

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

void    val_search_apply_cb(w,trace,cb)
    Widget				w;
    TRACE			*trace;
    XmSelectionBoxCallbackStruct *cb;
{
    if (DTPRINT) printf("In val_search_apply_cb - trace=%d\n",trace);

    val_search_ok_cb (w,trace,cb);
    val_search_cb (w,trace,cb);
    }

void    val_search_cancel_cb(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In val_search_cancel_cb - trace=%d\n",trace);
    
    /* unmanage the popup on the screen */
    XtUnmanageChild(trace->signal.search);
    }

