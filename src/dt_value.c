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
static char rcsid[] = "$Id$";


#include <stdio.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <X11/Xlib.h>
#include <Xm/Xm.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/SelectioB.h>
#include <Xm/Text.h>
#include <Xm/BulletinB.h>
#include <Xm/RowColumn.h>
#include <Xm/Label.h>

#include "dinotrace.h"
#include "callbacks.h"


/****************************** UTILITIES ******************************/

void    value_to_string (trace, strg, cptr, seperator)
    TRACE *trace;
    char *strg;
    unsigned int cptr[];
    char seperator;		/* What to print between the values */
{
    if (cptr[2]) {
	if (trace->busrep == HBUS)
	    sprintf (strg,"%X%c%08X%c%08X", cptr[2], seperator, cptr[1], seperator, cptr[0]);
	else if (trace->busrep == OBUS)
	    sprintf (strg,"%o%c%o%c%o", cptr[2], seperator, cptr[1], seperator, cptr[0]);
	}
    else if (cptr[1]) {
	if (trace->busrep == HBUS)
	    sprintf (strg,"%X%c%08X", cptr[1], seperator, cptr[0]);
	else if (trace->busrep == OBUS)
	    sprintf (strg,"%o%c%o", cptr[1], seperator, cptr[0]);
	}
    else {
	if (trace->busrep == HBUS)
	    sprintf (strg,"%X", cptr[0]);
	else if (trace->busrep == OBUS)
	    sprintf (strg,"%o", cptr[0]);
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
	    cptr[2] = (cptr[2]<<4) + ((cptr[1] & MSH)>>28);
	    cptr[1] = (cptr[1]<<4) + ((cptr[0] & MSH)>>28);
	    cptr[0] = (cptr[0]<<4) + value;
	    }
	else if (trace->busrep == OBUS && value >=0 && value <= 7) {
	    cptr[2] = (cptr[2]<<3) + ((cptr[1] & MSO)>>29);
	    cptr[1] = (cptr[1]<<3) + ((cptr[0] & MSO)>>29);
	    cptr[0] = (cptr[0]<<3) + value;
	    }
	}
    }

void    cptr_to_search_value (cptr, value)
    SIGNAL_LW	*cptr;
    unsigned int value[];
{
    value[0] = value[1] = value[2] = 0;
    switch (cptr->sttime.state) {
      case STATE_1:
	value[0] = 1;
	break;
	
      case STATE_B32:
	value[0] = *((unsigned int *)cptr+1);
	break;
	
      case STATE_B64:
	value[0] = *((unsigned int *)cptr+1);
	value[1] = *((unsigned int *)cptr+2);
	break;

      case STATE_B96:
	value[0] = *((unsigned int *)cptr+1);
	value[1] = *((unsigned int *)cptr+2);
	value[2] = *((unsigned int *)cptr+3);
	break;
	} /* switch */
    }

void	val_update_search ()
{
    TRACE	*trace;
    SIGNAL	*sig_ptr;
    SIGNAL_LW	*cptr;
    int		found, cursorize;
    register int i;
    CURSOR	*csr_ptr,*new_csr_ptr;

    if (DTPRINT_ENTRY) printf ("In val_update_search\n");

    /* Mark all cursors that are a result of a search as old (-1) */
    for (csr_ptr = global->cursor_head; csr_ptr; csr_ptr = csr_ptr->next) {
	if (csr_ptr->type==SEARCH) csr_ptr->type = SEARCHOLD;
	}

    /* Search every trace for the value, mark the signal if it has it to speed up displaying */
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	/* don't do anything if no file is loaded */
	if (!trace->loaded) continue;
	
	for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	    if (sig_ptr->lws == 1) {
		/* Single bit signal, don't search for values */
		continue;
		}
	    
	    found=0;
	    cursorize=0;
	    cptr = (SIGNAL_LW *)(sig_ptr->bptr);
	    
	    for (; (cptr->sttime.time != EOT); cptr += sig_ptr->lws) {
		switch (cptr->sttime.state) {
		  case STATE_B32:
		    for (i=0; i<MAX_SRCH; i++) {
			if ( ( global->val_srch[i].value[0]== *((unsigned int *)cptr+1) )
			    && ( global->val_srch[i].value[1] == 0) 
			    && ( global->val_srch[i].value[2] == 0) ) {
			    found |= ( global->val_srch[i].color != 0) ;
			    if ( global->val_srch[i].cursor != 0) cursorize = global->val_srch[i].cursor;
			    /* don't break, because if same value on two lines, one with cursor and one without will fail */
			    }
			}
		    break;
		    
		  case STATE_B64:
		    for (i=0; i<MAX_SRCH; i++) {
			if ( ( global->val_srch[i].value[0]== *((unsigned int *)cptr+1) )
			    && ( global->val_srch[i].value[1]== *((unsigned int *)cptr+2) )
			    && ( global->val_srch[i].value[2] == 0) ) {
			    found |= ( global->val_srch[i].color != 0) ;
			    if ( global->val_srch[i].cursor != 0) cursorize = global->val_srch[i].cursor;
			    }
			}
		    break;
		    
		  case STATE_B96:
		    for (i=0; i<MAX_SRCH; i++) {
			if ( ( global->val_srch[i].value[0]== *((unsigned int *)cptr+1) )
			    && ( global->val_srch[i].value[1]== *((unsigned int *)cptr+2) )
			    && ( global->val_srch[i].value[2]== *((unsigned int *)cptr+3) ) ) {
			    found |= ( global->val_srch[i].color != 0) ;
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
	    
	    sig_ptr->srch_ena = found;
	    if (found && DTPRINT_FILE) printf ("Signal %s matches search string.\n", sig_ptr->signame);
	    
	    } /* for sig */
	} /* for trace */

    /* Delete all old cursors */
    cur_delete_of_type (SEARCHOLD);
    }

/****************************** MENU OPTIONS ******************************/

void    val_examine_cb (w, trace, cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    
    if (DTPRINT_ENTRY) printf ("In val_examine_cb - trace=%d\n",trace);
    
    /* remove any previous events */
    remove_all_events (trace);
    
    /* process all subsequent button presses as cursor moves */
    set_cursor (trace, DC_VAL_EXAM);
    add_event (ButtonPressMask, val_examine_ev);
    }

void    val_highlight_cb (w,trace,cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT_ENTRY) printf ("In val_highlight_cb - trace=%d\n",trace);
    
    /* remove any previous events */
    remove_all_events (trace);
     
    /* Grab color number from the menu button pointer */
    global->highlight_color = submenu_to_color (trace, w, trace->menu.val_highlight_pds);

    /* process all subsequent button presses as signal deletions */ 
    set_cursor (trace, DC_VAL_HIGHLIGHT);
    add_event (ButtonPressMask, val_highlight_ev);
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
    int		rows, cols, bit, bit_value, row, col, par;
    
    time = posx_to_time (trace, x);
    sig_ptr = posy_to_signal (trace, y);
    
    if (trace->examine.popup) {
	XtUnmanageChild (trace->examine.popup);
	/* XtUnmanageChild (trace->examine.label); */
	trace->examine.popup = NULL;
	}
    
    if (time && sig_ptr) {
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
	  case STATE_B64:
	  case STATE_B96:
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

	    bit = 0;
	    for (row=rows - 1; row >= 0; row--) {
		for (col = cols - 1; col >= 0; col--) {
		    bit = (row * cols + col);

		    if (bit<32) bit_value = ( value[0] >> bit ) & 1;
			else if (bit<64) bit_value = ( value[1] >> (bit-32) ) & 1;
			    else  bit_value = ( value[2] >> (bit-64) ) & 1;

		    if ((bit>=0) && (bit <= sig_ptr->bits)) {
			sprintf (strg2, "<%02d>=%d ", sig_ptr->msb_index +
				 ((sig_ptr->msb_index >= sig_ptr->lsb_index)
				  ? (bit - sig_ptr->bits) : (sig_ptr->bits - bit)),
				 bit_value);
			strcat (strg, strg2);
			if (col==4) strcat (strg, "  ");
			}
		    }
		strcat (strg, "\n");
		}

	    if (sig_ptr->bits > 2) {
		par = 0;
		for (bit=0; bit<=sig_ptr->bits; bit++) {
		    if (bit<32) bit_value = ( value[0] >> bit ) & 1;
		    else if (bit<64) bit_value = ( value[1] >> (bit-32) ) & 1;
		    else  bit_value = ( value[2] >> (bit-64) ) & 1;

		    par ^= bit_value;
		    }
		if (par) strcat (strg, "Odd Parity\n");
		else  strcat (strg, "Even Parity\n");
		}

            break;
            } /* Case */

	/* Debugging information */
	if (DTDEBUG) {
	    sprintf (strg2, "\nState %d\n", cptr->sttime.state);
	    strcat (strg, strg2);
	    sprintf (strg2, "Type %d   Lws %d   Blocks %d\n",
		     sig_ptr->type, sig_ptr->lws, sig_ptr->blocks);
	    strcat (strg, strg2);
	    sprintf (strg2, "Bits %d   Index %d - %d  Srch_ena %d\n",
		     sig_ptr->bits, sig_ptr->msb_index, sig_ptr->lsb_index, sig_ptr->srch_ena);
	    strcat (strg, strg2);
	    sprintf (strg2, "File_type %d   File_Pos %d-%d  Mask %08lx\n",
		     sig_ptr->file_type.flags, sig_ptr->file_pos, sig_ptr->file_end_pos, sig_ptr->pos_mask);
	    strcat (strg, strg2);
	    sprintf (strg2, "Value_mask %08lx %08lx %08lx\n",
		     sig_ptr->value_mask[2], sig_ptr->value_mask[1], sig_ptr->value_mask[0]);
	    strcat (strg, strg2);
	    }
	
        /* Create */
        if (DTPRINT_ENTRY) printf ("\ttime = %d, signal = %s\n", time, sig_ptr->signame);

	XtSetArg (arglist[0], XmNentryAlignment, XmALIGNMENT_BEGINNING);
	trace->examine.popup = XmCreatePopupMenu (trace->main, "examinepopup", arglist, 1);

	xs = string_create_with_cr (strg);
	XtSetArg (arglist[0], XmNlabelString, xs);
	trace->examine.label = XmCreateLabel (trace->examine.popup,"popuplabel",arglist,1);
	XtManageChild (trace->examine.label);
	XmStringFree (xs);

	XmMenuPosition (trace->examine.popup, ev);
	XtManageChild (trace->examine.popup);
	}
    }
	
void    val_examine_unpopup_act (w)
    /* callback or action! */
    Widget		w;
{
    TRACE	*trace;		/* Display information */
    
    if (DTPRINT_ENTRY) printf ("In val_examine_unpopup_act\n");
    
    if (!(trace = widget_to_trace (w))) return;

    /* unmanage popup */
    if (trace->examine.popup) {
	XtUnmanageChild (trace->examine.popup);
	XtUnmanageChild (trace->examine.label);
	trace->examine.popup = NULL;
	}
    /* redraw the screen as popup may have mangled widgets */
    /* draw_all_needed (trace);*/
    }

char *events[40] = {"","", "KeyPress", "KeyRelease", "ButtonPress", "ButtonRelease", "MotionNotify",
			"EnterNotify", "LeaveNotify", "FocusIn", "FocusOut", "KeymapNotify", "Expose", "GraphicsExpose",
			"NoExpose", "VisibilityNotify", "CreateNotify", "DestroyNotify", "UnmapNotify", "MapNotify",
			"MapRequest", "ReparentNotify", "ConfigureNotify", "ConfigureRequest", "GravityNotify",
			"ResizeRequest", "CirculateNotify", "CirculateRequest", "PropertyNotify", "SelectionClear",
			"SelectionRequest", "SelectionNotify", "ColormapNotify", "ClientMessage", "MappingNotify",
			"LASTEvent"};

void    val_examine_ev (w, trace, ev)
    Widget		w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    XEvent	event;
    XMotionEvent *em;
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
    val_examine_popup (trace, ev->x, ev->y, ev);
    
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
	    em = (XMotionEvent *)&event;
	    val_examine_popup (trace, em->x, em->y, em);
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
    }

void    val_examine_popup_act (w, ev, params, num_params)
    Widget		w;
    XButtonPressedEvent	*ev;
    String		params;
    Cardinal		*num_params;
{
    TRACE	*trace;		/* Display information */
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

void    val_search_widget_update (trace)
    TRACE	*trace;
{
    VSearchNum search_pos;
    char	strg[40];

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
	}
    }

void    val_search_cb (w,trace,cb)
    Widget		w;
    TRACE	*trace;
    XmSelectionBoxCallbackStruct *cb;
{
    int		i;
    int		y=10;
    
    if (DTPRINT_ENTRY) printf ("In val_search_cb - trace=%d\n",trace);
    
    if (!trace->value.search) {
	XtSetArg (arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg (arglist[1], XmNdialogTitle, XmStringCreateSimple ("Value Search Requester") );
	/* XtSetArg (arglist[2], XmNwidth, 500);
	   XtSetArg (arglist[3], XmNheight, 400); */
	trace->value.search = XmCreateBulletinBoardDialog (trace->work,"search",arglist,2);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Color"));
	XtSetArg (arglist[1], XmNx, 5);
	XtSetArg (arglist[2], XmNy, y);
	trace->value.label1 = XmCreateLabel (trace->value.search,"label1",arglist,3);
	XtManageChild (trace->value.label1);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Place"));
	XtSetArg (arglist[1], XmNx, 60);
	XtSetArg (arglist[2], XmNy, y);
	trace->value.label2 = XmCreateLabel (trace->value.search,"label2",arglist,3);
	XtManageChild (trace->value.label2);
	
	y += 15;
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Value"));
	XtSetArg (arglist[1], XmNx, 5);
	XtSetArg (arglist[2], XmNy, y);
	trace->value.label4 = XmCreateLabel (trace->value.search,"label4",arglist,3);
	XtManageChild (trace->value.label4);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Cursor"));
	XtSetArg (arglist[1], XmNx, 60);
	XtSetArg (arglist[2], XmNy, y);
	trace->value.label5 = XmCreateLabel (trace->value.search,"label5",arglist,3);
	XtManageChild (trace->value.label5);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple
		 ( (trace->busrep == HBUS)? "Search value in HEX":"Search value in OCTAL" ) );
	XtSetArg (arglist[1], XmNx, 140);
	XtSetArg (arglist[2], XmNy, y);
	trace->value.label3 = XmCreateLabel (trace->value.search,"label3",arglist,3);
	XtManageChild (trace->value.label3);
	
	y += 25;

	for (i=0; i<MAX_SRCH; i++) {
	    /* enable button */
	    XtSetArg (arglist[0], XmNx, 15);
	    XtSetArg (arglist[1], XmNy, y);
	    XtSetArg (arglist[2], XmNselectColor, trace->xcolornums[i+1]);
	    XtSetArg (arglist[3], XmNlabelString, XmStringCreateSimple (""));
	    trace->value.enable[i] = XmCreateToggleButton (trace->value.search,"togglen",arglist,4);
	    XtManageChild (trace->value.enable[i]);

	    /* enable button */
	    XtSetArg (arglist[0], XmNx, 70);
	    XtSetArg (arglist[1], XmNy, y);
	    XtSetArg (arglist[2], XmNselectColor, trace->xcolornums[i+1]);
	    XtSetArg (arglist[3], XmNlabelString, XmStringCreateSimple (""));
	    trace->value.cursor[i] = XmCreateToggleButton (trace->value.search,"cursorn",arglist,4);
	    XtManageChild (trace->value.cursor[i]);

	    /* create the file name text widget */
	    XtSetArg (arglist[0], XmNrows, 1);
	    XtSetArg (arglist[1], XmNcolumns, 30);
	    XtSetArg (arglist[2], XmNx, 120);
	    XtSetArg (arglist[3], XmNy, y);
	    XtSetArg (arglist[4], XmNresizeHeight, FALSE);
	    XtSetArg (arglist[5], XmNeditMode, XmSINGLE_LINE_EDIT);
	    trace->value.text[i] = XmCreateText (trace->value.search,"textn",arglist,6);
	    XtAddCallback (trace->value.text[i], XmNactivateCallback, val_search_ok_cb, trace);
	    XtManageChild (trace->value.text[i]);
	    
	    y += 40;
	    }

	y+= 15;

	/* Create OK button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple (" OK ") );
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNy, y);
	trace->value.ok = XmCreatePushButton (trace->value.search,"ok",arglist,3);
	XtAddCallback (trace->value.ok, XmNactivateCallback, val_search_ok_cb, trace);
	XtManageChild (trace->value.ok);
	
	/* create apply button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Apply") );
	XtSetArg (arglist[1], XmNx, 70);
	XtSetArg (arglist[2], XmNy, y);
	trace->value.apply = XmCreatePushButton (trace->value.search,"apply",arglist,3);
	XtAddCallback (trace->value.apply, XmNactivateCallback, val_search_apply_cb, trace);
	XtManageChild (trace->value.apply);
	
	/* create cancel button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Cancel") );
	XtSetArg (arglist[1], XmNx, 140);
	XtSetArg (arglist[2], XmNy, y);
	trace->value.cancel = XmCreatePushButton (trace->value.search,"cancel",arglist,3);
	XtAddCallback (trace->value.cancel, XmNactivateCallback, unmanage_cb, trace->value.search);
	XtManageChild (trace->value.cancel);
	}
    
    /* Copy settings to local area to allow cancel to work */
    val_search_widget_update (trace);

    /* manage the popup on the screen */
    XtManageChild (trace->value.search);
    }

void    val_search_ok_cb (w,trace,cb)
    Widget				w;
    TRACE			*trace;
    XmSelectionBoxCallbackStruct *cb;
{
    char		*strg;
    int			i;

    if (DTPRINT_ENTRY) printf ("In val_search_ok_cb - trace=%d\n",trace);

    for (i=0; i<MAX_SRCH; i++) {
	/* Update with current search enables */
	global->val_srch[i].color = (XmToggleButtonGetState (trace->value.enable[i])) ? i+1 : 0;
	
	/* Update with current cursor enables */
	global->val_srch[i].cursor = (XmToggleButtonGetState (trace->value.cursor[i])) ? i+1 : 0;
	
	/* Update with current search values */
	strg = XmTextGetString (trace->value.text[i]);
	string_to_value (trace, strg, global->val_srch[i].value);

	if (DTPRINT_SEARCH) {
	    char strg2[40];
	    value_to_string (trace, strg2, global->val_srch[i].value, '_');
	    printf ("Search %d) %d   '%s' -> '%s'\n", i, global->val_srch[i].color, strg, strg2);
	    }
	}
    
    XtUnmanageChild (trace->value.search);

    val_update_search ();

    draw_all_needed (trace);
    }

void    val_search_apply_cb (w,trace,cb)
    Widget				w;
    TRACE			*trace;
    XmSelectionBoxCallbackStruct *cb;
{
    if (DTPRINT_ENTRY) printf ("In val_search_apply_cb - trace=%d\n",trace);

    val_search_ok_cb (w,trace,cb);
    val_search_cb (w,trace,cb);
    }

void    val_highlight_ev (w,trace,ev)
    Widget		w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    int		time;
    SIGNAL	*sig_ptr;
    SIGNAL_LW	*cptr;
    VSearchNum	search_pos;
    
    if (DTPRINT_ENTRY) printf ("In val_highlight_ev - trace=%d\n",trace);
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
    val_update_search ();
    draw_all_needed (trace);
    }


/****************************** ANNOTATION ******************************/

void    val_annotate_cb (w,trace,cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
    
{
    int i;

    if (DTPRINT_ENTRY) printf ("In val_annotate_cb - trace=%d\n",trace);
    
    if (!trace->annotate.dialog) {
	XtSetArg (arglist[0],XmNdefaultPosition, TRUE);
	XtSetArg (arglist[1],XmNdialogTitle, XmStringCreateSimple ("Annotate Menu"));
	trace->annotate.dialog = XmCreateBulletinBoardDialog (trace->work, "annotate",arglist,2);
	
	/* create label widget for text widget */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("File Name") );
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNy, 3);
	trace->annotate.label1 = XmCreateLabel (trace->annotate.dialog,"",arglist,3);
	XtManageChild (trace->annotate.label1);
	
	/* create the file name text widget */
	XtSetArg (arglist[0], XmNrows, 1);
	XtSetArg (arglist[1], XmNcolumns, 30);
	XtSetArg (arglist[2], XmNx, 10);
	XtSetArg (arglist[3], XmNy, 35);
	XtSetArg (arglist[4], XmNresizeHeight, FALSE);
	XtSetArg (arglist[5], XmNeditMode, XmSINGLE_LINE_EDIT);
	trace->annotate.text = XmCreateText (trace->annotate.dialog,"",arglist,6);
	XtManageChild (trace->annotate.text);
	XtAddCallback (trace->annotate.text, XmNactivateCallback, val_annotate_ok_cb, trace);
	
	/* Cursor enables */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Include which user (solid) cursor colors:") );
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNy, 75);
	trace->annotate.label2 = XmCreateLabel (trace->annotate.dialog,"",arglist,3);
	XtManageChild (trace->annotate.label2);
	
	for (i=0; i<=MAX_SRCH; i++) {
	    /* enable button */
	    XtSetArg (arglist[0], XmNx, 15+30*i);
	    XtSetArg (arglist[1], XmNy, 100);
	    XtSetArg (arglist[2], XmNselectColor, trace->xcolornums[i]);
	    XtSetArg (arglist[3], XmNlabelString, XmStringCreateSimple (""));
	    trace->annotate.cursors[i] = XmCreateToggleButton (trace->annotate.dialog,"togglenc",arglist,4);
	    XtManageChild (trace->annotate.cursors[i]);
	    }

	/* Cursor enables */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Include which auto (dotted) cursor colors:") );
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNy, 145);
	trace->annotate.label4 = XmCreateLabel (trace->annotate.dialog,"",arglist,3);
	XtManageChild (trace->annotate.label4);
	
	for (i=0; i<=MAX_SRCH; i++) {
	    /* enable button */
	    XtSetArg (arglist[0], XmNx, 15+30*i);
	    XtSetArg (arglist[1], XmNy, 170);
	    XtSetArg (arglist[2], XmNselectColor, trace->xcolornums[i]);
	    XtSetArg (arglist[3], XmNlabelString, XmStringCreateSimple (""));
	    trace->annotate.cursors_dotted[i] = XmCreateToggleButton (trace->annotate.dialog,"togglencd",arglist,4);
	    XtManageChild (trace->annotate.cursors_dotted[i]);
	    }

	/* Signal Enables */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Include which signal colors:") );
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNy, 215);
	trace->annotate.label3 = XmCreateLabel (trace->annotate.dialog,"",arglist,3);
	XtManageChild (trace->annotate.label3);
	
	for (i=1; i<=MAX_SRCH; i++) {
	    /* enable button */
	    XtSetArg (arglist[0], XmNx, 15+30*i);
	    XtSetArg (arglist[1], XmNy, 240);
	    XtSetArg (arglist[2], XmNselectColor, trace->xcolornums[i]);
	    XtSetArg (arglist[3], XmNlabelString, XmStringCreateSimple (""));
	    trace->annotate.signals[i] = XmCreateToggleButton (trace->annotate.dialog,"togglen",arglist,4);
	    XtManageChild (trace->annotate.signals[i]);
	    }

	/* Create OK button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple (" OK ") );
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNy, 280);
	trace->annotate.ok = XmCreatePushButton (trace->annotate.dialog,"ok",arglist,3);
	XtAddCallback (trace->annotate.ok, XmNactivateCallback, val_annotate_ok_cb, trace);
	XtManageChild (trace->annotate.ok);
	
	/* create apply button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Apply") );
	XtSetArg (arglist[1], XmNx, 70);
	XtSetArg (arglist[2], XmNy, 280);
	trace->annotate.apply = XmCreatePushButton (trace->annotate.dialog,"apply",arglist,3);
	XtAddCallback (trace->annotate.apply, XmNactivateCallback, val_annotate_apply_cb, trace);
	XtManageChild (trace->annotate.apply);
	
	/* create cancel button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Cancel") );
	XtSetArg (arglist[1], XmNx, 140);
	XtSetArg (arglist[2], XmNy, 280);
	trace->annotate.cancel = XmCreatePushButton (trace->annotate.dialog,"cancel",arglist,3);
	XtAddCallback (trace->annotate.cancel, XmNactivateCallback, unmanage_cb, trace->annotate.dialog);

	XtManageChild (trace->annotate.cancel);
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
    XtManageChild (trace->annotate.dialog);
    global->anno_poppedup = TRUE;
    }

void    val_annotate_ok_cb (w,trace,cb)
    Widget				w;
    TRACE			*trace;
    XmSelectionBoxCallbackStruct *cb;
{
    char		*strg;
    int			i;

    if (DTPRINT_ENTRY) printf ("In sig_search_ok_cb - trace=%d\n",trace);

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

void    val_annotate_apply_cb (w,trace,cb)
    Widget				w;
    TRACE			*trace;
    XmSelectionBoxCallbackStruct *cb;
{
    if (DTPRINT_ENTRY) printf ("In sig_search_apply_cb - trace=%d\n",trace);

    val_annotate_ok_cb (w,trace,cb);
    val_annotate_cb (w,trace,cb);
    }

void    val_annotate_do_cb (w,trace,cb)
    Widget	w;
    TRACE	*trace;
    XmAnyCallbackStruct	*cb;
{
    int		i;
    SIGNAL	*sig_ptr;
    SIGNAL_LW	*cptr;
    FILE	*dump_fp;
    CURSOR 	*csr_ptr;		/* Current cursor being printed */
    char	strg[100];
    int		csr_num, csr_num_incl;
    
    /* Initialize requestor before first usage? */
    /*
    if (! global->anno_poppedup) {
	val_annotate_cb (w,trace,cb);
	}*/

    if (DTPRINT_ENTRY) printf ("In val_annotate_cb - trace=%d  file=%s\n",trace,global->anno_filename);

    /* Socket connection */
#ifndef VMS
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
	    cptr = (SIGNAL_LW *)sig_ptr->cptr;

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
			&& ( cptr != (SIGNAL_LW *)sig_ptr->cptr)) {
			cptr -= sig_ptr->lws;
			}

		    cptr_to_string (cptr, strg);
		    
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

