#ident "$Id$"
/******************************************************************************
 * dt_util.c --- misc utilities
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
 * gratefuly have agreed to share it, and thus the bas version has been
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

#include <Xm/MessageB.h>
#include <Xm/SelectioB.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <Xm/RowColumn.h>

#include "functions.h"

/**********************************************************************/

extern void prompt_ok_cb(), fil_ok_cb();


void upcase_string (char *tp)
{
    for (;*tp;tp++) *tp = toupper (*tp);
}

/* get normal string from XmString */
char *extract_first_xms_segment (
    XmString	cs)
{
    XmStringContext	context;
    XmStringCharSet	charset;
    XmStringDirection	direction;
    Boolean		separator;
    char		*primitive_string;

    XmStringInitContext (&context,cs);
    XmStringGetNextSegment (context,&primitive_string,
			   &charset,&direction,&separator);
    XmStringFreeContext (context);
    return (primitive_string);
}


XmString string_create_with_cr (
    char	*msg)
{
    XmString	xsout,xsnew,xsfree;
    char	*ptr,*nptr;
    char	aline[2000];

    /* create string w/ seperators */
    xsout = NULL;
    for (ptr=msg; ptr && *ptr; ptr = nptr) {
	nptr = strchr (ptr, '\n');
	if (nptr) {
	    strncpy (aline, ptr, nptr-ptr);
	    aline[nptr-ptr] = '\0';
	    nptr++;
	    xsnew = XmStringCreateSimple (aline);
	}
	else {
	    xsnew = XmStringCreateSimple (ptr);
	}

	if (xsout) {
	    xsout = XmStringConcat (xsfree=xsout, XmStringSeparatorCreate());
	    XmStringFree (xsfree);
	    if (!XmStringEmpty (xsnew)) {
		xsout = XmStringConcat (xsfree=xsout, xsnew);
		XmStringFree (xsfree);
	    }
	    XmStringFree (xsnew);
	}
	else {
	    xsout = xsnew;
	}
    }
    return (xsout);
}
    

char *date_string (
    time_t	time_num)	/* Time to parse, or 0 for current time */
{
    static char	date_str[50];
    struct tm *timestr;
    
    if (!time_num) {
	time_num = time (NULL);
    }
    timestr = localtime (&time_num);
    strcpy (date_str, asctime (timestr));
    if (date_str[strlen (date_str)-1]=='\n')
	date_str[strlen (date_str)-1]='\0';

    return (date_str);
}


#if ! HAVE_STRDUP
char *strdup(char *s)
{
    char *d;
    d=(char *)malloc(strlen(s)+1);
    strcpy (d, s);
    return (d);
}
#endif

#define FGETS_SIZE_INC	1024	/* Characters to increase length by */

void fgets_dynamic (
    /* fgets a line, with dynamically allocated line storage */
    char **line_pptr,	/* & Line pointer */
    uint_t *length_ptr,	/* & Length of the buffer */
    FILE *readfp)
{
    if (*length_ptr == 0) {
	*length_ptr = FGETS_SIZE_INC;
	*line_pptr = XtMalloc (*length_ptr);
    }

    fgets ((*line_pptr), (*length_ptr), readfp);
    
    /* Did fgets overflow the string? */
    while (*((*line_pptr) + *length_ptr - 2)!='\n'
	   && strlen(*line_pptr)==(*length_ptr - 1)) {
	/* Alloc more */
	*length_ptr += FGETS_SIZE_INC;
	*line_pptr = XtRealloc (*line_pptr, *length_ptr);
	/* Read remainder */
	fgets (((*line_pptr) + *length_ptr - 2) + 1 - FGETS_SIZE_INC,
	       FGETS_SIZE_INC+1, readfp);
    }
}

/* Keep only the directory portion of a file spec */
void file_directory (
    char	*strg)
{
    char 	*pchar;
    
#ifdef VMS
    if ((pchar=strrchr (strg,']')) != NULL )
	* (pchar+1) = '\0';
    else
	if ((pchar=strrchr (strg,':')) != NULL )
	    * (pchar+1) = '\0';
	else
	    strg[0] = '\0';
#else
    if ((pchar=strrchr (strg,'/')) != NULL )
	* (pchar+1) = '\0';
    else
	strg[0] = '\0';
#endif
}


/******************************************************************************/
/* X-windows stuff */

void    unmanage_cb (
    /* Generic unmanage routine, usually for cancel buttons */
    /* NOTE that you pass the widget in the tag area, not the trace!! */
    Widget	w,
    Widget	tag,
    XmAnyCallbackStruct *cb)
{
    if (DTPRINT_ENTRY) printf ("In unmanage_cb\n");
    
    /* unmanage the widget */
    XtUnmanageChild (tag);
}

void    cancel_all_events_cb (
    Widget	w)
{
    Trace *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In cancel_all_events - trace=%p\n",trace);
    
    /* remove all events */
    remove_all_events (trace);
    
    /* unmanage any widgets left around */
    if ( trace->signal.add != NULL )
	XtUnmanageChild (trace->signal.add);
}

void    update_scrollbar (
    Widget	w,
    int 	value,
    int		inc,
    int		min,
    int		max,
    int		size)
{
    if (DTPRINT_ENTRY) printf ("In update_scrollbar - %d %d %d %d %d\n",
			value, inc, min, max, size);

    if (min >= max) {
	XtSetArg (arglist[0], XmNsensitive, FALSE);
	min=0; max=1; inc=1; size=1; value=0;
    }
    else {
	XtSetArg (arglist[0], XmNsensitive, TRUE);
    }

    if (inc < 1) inc=1;
    if (size < 1) size=1;
    if (value > (max - size)) value = max - size;
    if (value < min) value = min;

    if (size > (max - min)) size = max - min;

    XtSetArg (arglist[1], XmNvalue, value);
    XtSetArg (arglist[2], XmNincrement, inc);
    XtSetArg (arglist[3], XmNsliderSize, size);
    XtSetArg (arglist[4], XmNminimum, min);
    XtSetArg (arglist[5], XmNmaximum, max);
    XtSetValues (w, arglist, 6);
}


void 	add_event (
    int		type,
    void	(*callback)())
{
    Trace	*trace;

    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	XtAddEventHandler (trace->work, type, TRUE,
			   (XtEventHandler)callback, trace);
    }
}

void    remove_event (
    int		type,
    void	(*callback)())
{
    Trace	*trace;
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	XtRemoveEventHandler (trace->work, type, TRUE,
			      (XtEventHandler)callback, trace);
    }
}


void    remove_all_events (
    Trace	*trace)
{
    if (DTPRINT_ENTRY) printf ("In remove_all_events - trace=%p\n",trace);
    
    /* remove all possible events due to res options */ 
    remove_event (ButtonPressMask, res_zoom_click_ev);
	
    /* remove all possible events due to cursor options */ 
    remove_event (ButtonPressMask, cur_add_ev);
    remove_event (ButtonPressMask, cur_move_ev);
    remove_event (ButtonPressMask, cur_delete_ev);
    remove_event (ButtonPressMask, cur_highlight_ev);
    
    /* remove all possible events due to signal options */ 
    remove_event (ButtonPressMask, sig_add_ev);
    remove_event (ButtonPressMask, sig_move_ev);
    remove_event (ButtonPressMask, sig_copy_ev);
    remove_event (ButtonPressMask, sig_delete_ev);
    remove_event (ButtonPressMask, sig_highlight_ev);
    
    /* remove all possible events due to nvalue options */ 
    remove_event (ButtonPressMask, val_examine_ev);
    remove_event (ButtonPressMask, val_highlight_ev);
    
    global->selected_sig = NULL;

    /* Set the cursor back to normal */
    set_cursor (trace, DC_NORMAL);
}

ColorNum submenu_to_color (
    Trace	*trace,		/* Display information */
    Widget	w,
    int		base)		/* Loaded when the menu was loaded */
{
    ColorNum color;

    /* Grab color number from the menu button pointer (+2 for current and next buttons) */
    for (color=0; color<=(MAX_SRCH+2); color++) {
	if (w == trace->menu.pdsubbutton[color + base]) {
	    break;
	}
    }

    /* Duplicate code in config_read_color */
    if (color==COLOR_CURRENT) color = global->highlight_color;
    if (color==COLOR_NEXT) {
	if ((++global->highlight_color) > MAX_SRCH) global->highlight_color = 1;  /* skips black */
	color = global->highlight_color;
    }

    return (color);
}

int option_to_number (
    Widget	w,		/* foo.options widget */
    Widget	*entry0_ptr,	/* &foo.pulldownbutton[0] */
    int		maxnumber)	/* number to consider (INCLUDING!) */
{
    int			tempi;
    Widget		clicked;

    /* Get menu */
    XtSetArg (arglist[0], XmNmenuHistory, &clicked);
    XtGetValues (w, arglist, 1);
    for (tempi=maxnumber; tempi>0; tempi--) {
	if (entry0_ptr[tempi]==clicked) break;
    }
    return (tempi);
}

/* Find the trace for this widget; widget must be created with DManageChild */
Trace	*widget_to_trace (
    Widget	w)
{
    Trace	*trace;		/* Display information */
    
    XtSetArg (arglist[0], XmNuserData, &trace);
    XtGetValues (w, arglist, 1);

    if (!trace) printf ("widget_to_trace failed lookup.\n");
    return (trace);
}

void new_time_sigs (
    Signal	*sig_first_ptr)
{
    Signal	*sig_ptr;	

    for (sig_ptr = sig_first_ptr; sig_ptr; sig_ptr = sig_ptr->forward) {
	/* if (DTPRINT)
	   printf ("next time=%d\n", (* (SignalLW *)((sig_ptr->cptr)+sig_ptr->lws)).sttime.time);
	   */
	
	if ( !sig_ptr->cptr ) {
	    sig_ptr->cptr = sig_ptr->bptr;
	}

	if (global->time >= (* (SignalLW *)(sig_ptr->cptr)).sttime.time ) {
	    while (((* (SignalLW *)(sig_ptr->cptr)).sttime.time != EOT) &&
		   (global->time > (* (SignalLW *)((sig_ptr->cptr)+sig_ptr->lws)).sttime.time)) {
		(sig_ptr->cptr) += sig_ptr->lws;
	    }
	}
	else {
	    while ((sig_ptr->cptr > sig_ptr->bptr) &&
		   (global->time < (* (SignalLW *)(sig_ptr->cptr)).sttime.time)) {
		(sig_ptr->cptr) -= sig_ptr->lws;
	    }
	}
    }
}

void new_time (
    Trace 	*trace)
{
    if ( global->time > (trace->end_time - TIME_WIDTH (trace)) ) {
        if (DTPRINT_ENTRY) printf ("At end of trace...\n");
        global->time = trace->end_time - TIME_WIDTH (trace);
    }
    
    if ( global->time < trace->start_time ) {
        if (DTPRINT_ENTRY) printf ("At beginning of trace...\n");
        global->time = trace->start_time;
    }

    /* Update beginning of all traces, DELETED TOO */
    for (trace = global->deleted_trace_head; trace; trace = trace->next_trace) {
	new_time_sigs (trace->firstsig);
    }

    /* Update windows */
    draw_all_needed ();
}

/* Get window size, calculate what fits on the screen and update scroll bars */
void get_geometry (
    Trace	*trace)
{
    int		x,y;
    uint_t	width,height,dret;
    Dimension m_time_height = global->time_font->ascent + global->time_font->descent;
    
    XGetGeometry ( global->display, XtWindow (trace->work), (Window *)&dret,
		 &x, &y, &width, &height, &dret, &dret);
    
    trace->width = MIN(width, (uint_t)MAXSCREENWIDTH-2);

    /* See comment in dinotrace.h about y layout */
    trace->height = height;
    trace->ygridtime = Y_TOP_BOTTOM + m_time_height;
    trace->ystart = trace->ygridtime + Y_TEXT_SPACE + MAX(Y_GRID_TOP,Y_CURSOR_TOP);

    /* if there are cursors showing, leave room */
    trace->ycursortimeabs = trace->height - Y_TOP_BOTTOM; 
    if ( (global->cursor_head != NULL) &&
	 trace->cursor_vis) {
	trace->ycursortimerel = trace->ycursortimeabs - Y_TEXT_SPACE - m_time_height;
	trace->yend = trace->ycursortimerel - Y_TEXT_SPACE - m_time_height - Y_GRID_BOTTOM;
    } else {
	trace->ycursortimerel = trace->ycursortimeabs;
	trace->yend = trace->ycursortimeabs - Y_GRID_BOTTOM;
    }
    
    /* calulate the number of signals possibly visible on the screen */
    trace->numsigvis = (trace->yend - trace->ystart) / trace->sighgt;
    
    /* Correct starting position to be reasonable */
    if (global->namepos > (global->namepos_hier + global->namepos_base - global->namepos_visible))
	global->namepos = (global->namepos_hier + global->namepos_base - global->namepos_visible);

    /* Move horiz scrollbar left edge to start position */
    XtUnmanageChild (trace->hscroll);
    XtSetArg (arglist[0], XmNleftAttachment, XmATTACH_FORM );
    XtSetArg (arglist[1], XmNleftOffset, global->xstart - XSTART_MARGIN);
    XtSetValues (trace->hscroll,arglist,2);
    XtManageChild (trace->hscroll);	/* Don't need DManage... already set up */

    update_scrollbar (trace->hscroll, global->time,
		      trace->grid[0].period,
		      trace->start_time, trace->end_time, 
		      (int)TIME_WIDTH(trace) );

    update_scrollbar (trace->command.namescroll, global->namepos,
		      (global->namepos_hier + global->namepos_base - global->namepos_visible)/10,
		      0, (global->namepos_hier + global->namepos_base),
		      global->namepos_visible);

    update_scrollbar (trace->vscroll, trace->numsigstart, 1,
		      0, trace->numsig, MIN (trace->numsigvis, trace->numsig - trace->numsigstart)); 
    
    if (DTPRINT_ENTRY) printf ("In get_geometry: x=%d y=%d width=%d height=%d\n",
			x,y,width,height);
}

void get_data_popup (
    Trace	*trace,
    char	*string,
    int		type)
{
    if (DTPRINT_ENTRY) 
	printf ("In get_data trace=%p string=%s type=%d\n",trace,string,type);
    
    if (trace->prompt_popup == NULL) {
	/* only need to create the widget once - now just manage/unmanage */
	/* create the dialog box popup window */
	/* XtSetArg (arglist[0], XmNstyle, DwtModal ); */
	XtSetArg (arglist[0], XmNdefaultPosition, TRUE);
	/* XtSetArg (arglist[1], XmNheight, DIALOG_HEIGHT); */
	/* no width so autochosen XtSetArg (arglist[2], XmNwidth, DIALOG_WIDTH); */
	trace->prompt_popup = XmCreatePromptDialog (trace->work,"", arglist, 1);
	DAddCallback (trace->prompt_popup, XmNokCallback, prompt_ok_cb, trace);
	DAddCallback (trace->prompt_popup, XmNcancelCallback, (XtCallbackProc)unmanage_cb, trace->prompt_popup);

	XtUnmanageChild ( XmSelectionBoxGetChild (trace->prompt_popup, XmDIALOG_HELP_BUTTON));
    }
    
    XtSetArg (arglist[0], XmNdialogTitle, XmStringCreateSimple (string) );
    XtSetArg (arglist[1], XmNtextString, XmStringCreateSimple ("") );
    XtSetValues (trace->prompt_popup,arglist,2);
    
    /* manage the popup window */
    trace->prompt_type = type;
    DManageChild (trace->prompt_popup, trace, MC_NOKEYS);
}

void    prompt_ok_cb (
    Widget		w,
    Trace		*trace,
    XmSelectionBoxCallbackStruct *cb)
{
    char	*valstr;
    DTime	restime;
    
    if (DTPRINT_ENTRY) printf ("In prompt_ok_cb type=%d\n",trace->prompt_type);
    
    /* Get value */
    valstr = extract_first_xms_segment (cb->value);
    restime = string_to_time (trace, valstr);

    /* unmanage the popup window */
    XtUnmanageChild (trace->prompt_popup);
    
    if (restime <= 0) {
	sprintf (message,"Value %d out of range", restime);
	dino_error_ack (trace,message);
	return;
    }

    /* do stuff depending on type of data */
    switch (trace->prompt_type)
	{
      case IO_RES:
	/* change the resolution string on display */
	new_res (trace, RES_SCALE / (float)restime);
	break;
	
      default:
	printf ("Error - bad type %d\n",trace->prompt_type);
	}
    }


void dino_message_ack (
    Trace	*trace,
    int		type,	/* See dino_warning_ack macros, 1=warning */
    char	*msg)
{
    Arg		msg_arglist[10];	/* Local copy so don't munge global args */
    XmString	xsout;
    
    if (DTPRINT) printf ("In dino_message_ack msg=%s\n",msg);
    
    /* May be called before the window was opened, if so ignore the message */
    if (!trace->work) return;

    /* create the widget if it hasn't already been */
    if (!trace->message)
	{
	XtSetArg (msg_arglist[0], XmNdefaultPosition, TRUE);
	switch (type) {
	  case 2:
	    trace->message = XmCreateInformationDialog (trace->work, "info", msg_arglist, 1);
	    break;
	  case 1:
	    trace->message = XmCreateWarningDialog (trace->work, "warning", msg_arglist, 1);
	    break;
	  default:
	    trace->message = XmCreateErrorDialog (trace->work, "error", msg_arglist, 1);
	    break;
	}
	DAddCallback (trace->message, XmNokCallback, (XtCallbackProc)unmanage_cb, trace->message);
	XtUnmanageChild ( XmMessageBoxGetChild (trace->message, XmDIALOG_CANCEL_BUTTON));
	XtUnmanageChild ( XmMessageBoxGetChild (trace->message, XmDIALOG_HELP_BUTTON));
	}

    /* create string w/ seperators */
    xsout = string_create_with_cr (msg);
    
    /* change the label value and location */
    XtSetArg (msg_arglist[0], XmNmessageString, xsout);
    XtSetArg (msg_arglist[1], XmNdialogTitle, XmStringCreateSimple ("Dinotrace Message") );
    XtSetValues (trace->message,msg_arglist,2);
    
    /* manage the widget */
    DManageChild (trace->message, trace, MC_NOKEYS);
}

SignalLW *cptr_at_time (
    /* Return the CPTR for the given time */
    Signal	*sig_ptr,
    DTime	ctime)
{
    SignalLW	*cptr;
    for (cptr = (SignalLW *)(sig_ptr->cptr);
	 (cptr->sttime.time != EOT) && (((cptr + sig_ptr->lws)->sttime.time) <= ctime);
	     cptr += sig_ptr->lws) ;
    return (cptr);
}


/******************************************************************************
 *
 *			Dinotrace Debugging Routines
 *
 *****************************************************************************/

void    cptr_to_string (
    SignalLW	*cptr,
    char	*strg)
{
    switch (cptr->sttime.state) {
      case STATE_1:
	strg[0] = '1';
	strg[1] = '\0';
	return;
	
      case STATE_0:
	strg[0] = '0';
	strg[1] = '\0';
	return;
	
      case STATE_U:
	strg[0] = 'U';
	strg[1] = '\0';
	return;
	
      case STATE_Z:
	strg[0] = 'Z';
	strg[1] = '\0';
	return;
	
      case STATE_B32:
	sprintf (strg, "%x", (cptr+1)->number);
	return;
	
      case STATE_B128:
	sprintf (strg, "%x_%08x_%08x_%08x",
		 (cptr+4)->number,
		 (cptr+3)->number,
		 (cptr+2)->number,
		 (cptr+1)->number);
	return;
	
      default:
	strg[0] = '?';
	return;
    }
}

void    print_cptr (
    SignalLW	*cptr)
{
    char strg[1000];
    uint_t	value[4];

    cptr_to_search_value (cptr, value);
    value_to_string (global->trace_head, strg, value, '_');
    printf ("%s at time %d\n",strg,cptr->sttime.time);
}

void    debug_print_screen_traces_cb (
    Widget	w)
{
    Trace *trace = widget_to_trace(w);
    uint_t	i,num;
    Signal	*sig_ptr;
    
    printf ("There are up to %d signals currently visible.\n",trace->numsigvis);
    printf ("Which signal do you wish to view [0-%d]: ",trace->numsigvis-1);
    scanf ("%d",&num);
    if ( num > trace->numsigvis-1 ) {
	printf ("Illegal Value.\n");
	return;
    }
    
    sig_ptr = (Signal *)trace->dispsig;
    for (i=0; i<num; i++) {
	sig_ptr = (Signal *)sig_ptr->forward;	
    }
    
    print_sig_info (sig_ptr);
}

void    print_sig_info (
    Signal	*sig_ptr)
{
    SignalLW	*cptr;
    
    cptr = (SignalLW *)sig_ptr->bptr;
    
    printf ("Signal %s starts ",sig_ptr->signame);
    print_cptr (cptr);
    
    for ( ; cptr->sttime.time != EOT; cptr += sig_ptr->lws ) {
	/*printf ("%x %x %x %x %x    ", (cptr)->number, (cptr+1)->number, (cptr+2)->number,
		(cptr+3)->number, (cptr+4)->number);*/
	print_cptr (cptr);
    }
}

void    debug_signal_integrity (
    Trace	*trace,
    Signal	*sig_ptr,
    char	*list_name,
    Boolean	del)
{
    SignalLW	*cptr;
    DTime	last_time;
    uint_t	nsigstart, nsig;
    Boolean	hitstart;

    if (!sig_ptr) return;

    if (sig_ptr->backward != NULL) {
	printf ("%s, Backward pointer should be NULL on signal %s\n", list_name, sig_ptr->signame);
    }

    nsig = 0; nsigstart = 0;
    hitstart = FALSE;
    for (; sig_ptr; sig_ptr=sig_ptr->forward) {
	/* Number of signals check */
	nsig++;
	if (!hitstart) {
	    nsigstart++;
	    if (!del && sig_ptr == trace->dispsig) hitstart=TRUE;
	}

	/* flags */
	if (sig_ptr->deleted != del) {
	    printf ("%s, Bad deleted flag on %s!\n", list_name, sig_ptr->signame);
	}

	if (sig_ptr->forward && sig_ptr->forward->backward != sig_ptr) {
	    printf ("%s, Bad backward link on signal %s\n", list_name, sig_ptr->signame);
	}

	/* Change data */
	last_time = -1;
	for (cptr = (SignalLW *)sig_ptr->bptr; cptr->sttime.time != EOT; cptr += sig_ptr->lws) {
	    if ( cptr->sttime.time == last_time ) {
		printf ("%s, Double time change at signal %s time %d\n", list_name, sig_ptr->signame, cptr->sttime.time);
	    }
	    if ( cptr->sttime.time < last_time ) {
		printf ("%s, Reverse time change at signal %s time %d\n", list_name, sig_ptr->signame, cptr->sttime.time);
	    }
	    last_time = cptr->sttime.time;
	}
	if (last_time != sig_ptr->trace->end_time) {
	    printf ("%s, Doesn't end at right time, signal %s time %d\n", list_name, sig_ptr->signame, cptr->sttime.time);
	}
    }

    if (!del) {
	if (nsig != trace->numsig) {
	    printf ("%s, Numsigs is wrong, %d!=%d\n", list_name, nsig, trace->numsig);
	}
	if (!hitstart) {
	    printf ("%s, Never found display starting point\n", list_name);
	}
	if (nsigstart) nsigstart--;
	if (nsigstart != trace->numsigstart) {
	    printf ("%s, Numsigstart is wrong, %d!=%d\n", list_name, nsigstart, trace->numsigstart);
        }
    }
}

void    debug_integrity_check_cb (
    Widget	w)
{
    Trace *trace;
    for (trace = global->deleted_trace_head; trace; trace = trace->next_trace) {
	debug_signal_integrity (trace, trace->firstsig, trace->filename, 
				(trace==global->deleted_trace_head));
    }
}


void    debug_increase_debugtemp_cb (
    Widget	w)
{
    DebugTemp++;
    printf ("New DebugTemp = %d\n", DebugTemp);
    draw_all_needed ();
}

void    debug_decrease_debugtemp_cb (
    Widget		w)
{
    DebugTemp--;
    printf ("New DebugTemp = %d\n", DebugTemp);
    draw_all_needed ();
}


void change_title (
    Trace	*trace)
{
    char title[300],icontitle[300],*pchar;
    
    /*
     ** Change the name on title bar to filename
     */
    strcpy (title,DTVERSION);
    if (trace->loaded) {
	strcat (title," - ");
	strcat (title,trace->filename);
    }
    
    /* For icon title drop extension and directory */
    if (trace->loaded) {
	strcpy (icontitle, trace->filename);
#ifdef VMS
	if ((pchar=strrchr (trace->filename,']')) != NULL )
	    strcpy (icontitle, pchar+1);
	else
	    if ((pchar=strrchr (trace->filename,':')) != NULL )
		strcpy (icontitle, pchar+1);
	if ((pchar=strchr (icontitle,'.')) != NULL )
	    * (pchar) = '\0';
	if ((pchar=strchr (icontitle,';')) != NULL )
	    * (pchar) = '\0';
	/* Tack on the version number */
	if ((pchar=strrchr (trace->filename,';')) != NULL )
	    strcat (icontitle, pchar);
#else
	if ((pchar=strrchr (trace->filename,'/')) != NULL )
	    strcpy (icontitle, pchar+1);
#endif
    }
    else {
	strcpy (icontitle, DTVERSION);
    }
    
    XtSetArg (arglist[0], XmNtitle, title);
    XtSetArg (arglist[1], XmNiconName, icontitle);
    XtSetValues (trace->toplevel,arglist,2);
}

#pragma inline (posx_to_time)
DTime	posx_to_time (
    /* convert x value to a time value, return -1 if invalid click */
    Trace 	*trace,
    Position 	x)
{
    /* check if button has been clicked on trace portion of screen */
    if ( !trace->loaded || x < global->xstart || x > trace->width - XMARGIN )
	return (-1);
    
    return (((x) + global->time * global->res - global->xstart) / global->res);
}


DTime	posx_to_time_edge (
    /* convert x value to a time value, return -1 if invalid click */
    /* allow clicking to a edge if it is enabled */
    Trace 	*trace,
    Position	x,
    Position	y)
{
    DTime	xtime;
    Signal 	*sig_ptr;
    SignalLW	*cptr;
    DTime	left_time, right_time;

    xtime = posx_to_time (trace,x);
    if (xtime<0 || !global->click_to_edge) return (xtime);

    /* If no signal at this position return time directly */
    if (! (sig_ptr = posy_to_signal (trace,y))) return (xtime);

    /* Find time where signal changes */
    for (cptr = (SignalLW *)(sig_ptr->cptr);
	 (cptr->sttime.time != EOT) && (((cptr + sig_ptr->lws)->sttime.time) < xtime);
	 cptr += sig_ptr->lws) ;
    left_time = cptr->sttime.time;
    if (left_time == EOT) return (xtime);
    cptr += sig_ptr->lws;
    right_time = cptr->sttime.time;

    /*
    if (DTPRINT) {
	printf ("Time %d, signal %s changes %d - %d, pixels %f,%f\n", xtime, sig_ptr->signame, 
		left_time, right_time,
		(float)(ABS (left_time - xtime) * global->res),
		(float)(ABS (right_time - xtime) * global->res)
		);
		}
    */
    
    /* See if edge is "close" on screen */
    if ((right_time != EOT) && ( (xtime - left_time) > (right_time - xtime) ))
	left_time = right_time;
    if ( (abs (left_time - xtime) * global->res) > CLICKCLOSE )	return (xtime);
    
    /* close enough */
    return (left_time);
}


#pragma inline (posy_to_signal)
Signal	*posy_to_signal (
    /* convert y value to a signal pointer, return NULL if invalid click */
    Trace	*trace,
    Position 	y)
{
    Signal	*sig_ptr;
    int num,i,max_y;

    /* return if there is no file */
    if ( !trace->loaded )
	return (NULL);
    
    /* make sure button has been clicked in in valid location of screen */
    max_y = MIN (trace->numsig,trace->numsigvis);
    if ( y < trace->ystart || y > trace->ystart + max_y * trace->sighgt )
	return (NULL);
    
    /* figure out which signal has been selected */
    num = (int)((y - trace->ystart) / trace->sighgt);

    /* set pointer to signal to 'highlight' */
    sig_ptr = trace->dispsig;
    for (i=0; sig_ptr && i<num; i++)
	sig_ptr = sig_ptr->forward;

    return (sig_ptr);
}


#pragma inline (posx_to_cursor)
DCursor *posx_to_cursor (
    /* convert x value to the index of the nearest cursor, return NULL if invalid click */
    Trace	*trace,
    Position	x)
{
    DCursor 	*csr_ptr;
    DTime 	xtime;

    /* check if there are any cursors */
    if (!global->cursor_head) {
	return (NULL);
    }
    
    xtime = posx_to_time (trace, x);
    if (xtime<0) return (NULL);
    
    /* find the closest cursor */
    csr_ptr = global->cursor_head;
    while ( (xtime > csr_ptr->time) && csr_ptr->next ) {
	csr_ptr = csr_ptr->next;
    }
    
    /* i is between cursors[i-1] and cursors[i] - determine which is closest */
    if ( csr_ptr->prev && ( (csr_ptr->time - xtime) > (xtime - csr_ptr->prev->time) ) ) {
	csr_ptr = csr_ptr->prev;
    }

    return (csr_ptr);
}


#pragma inline (time_to_cursor)
DCursor *time_to_cursor (
    /* convert specific time value to the index of the nearest cursor, return NULL if none */
    /* Unlike posx_to_cursor, this will not return a "close" one */
    DTime	xtime)
{
    DCursor	*csr_ptr;

    /* find the closest cursor */
    csr_ptr = global->cursor_head;
    while ( csr_ptr && (xtime > csr_ptr->time) ) {
	csr_ptr = csr_ptr->next;
    }
    
    if (csr_ptr && (xtime == csr_ptr->time))
	return (csr_ptr);
    else return (NULL);
}

#pragma inline (string_to_time)
DTime string_to_time (
    /* convert integer to time value */
    Trace	*trace,
    char	*strg)
{
    double	f_time;

    f_time = atof (strg);
    if (f_time < 0) return (0);

    if (trace->timerep == TIMEREP_CYC) {
	return ((DTime)(f_time * trace->grid[0].period
			+ trace->grid[0].alignment % trace->grid[0].period));
    }

    /* First convert to picoseconds */
    f_time *= trace->timerep;

    /* Then convert to internal units and return */
    f_time /= global->time_precision;

    return ((DTime)f_time);
}

#pragma inline (time_to_string)
void time_to_string (
    /* convert specific time value into the string passed in */
    Trace	*trace,
    char	*strg,
    DTime	ctime,
    Boolean	relative)	/* true = time is relative, so don't adjust */
{
    double	f_time, remain;
    int		decimals;

    f_time = (double)ctime;

    if (trace->timerep == TIMEREP_CYC) {
	if (!relative) {
	    /* Adjust within one cycle so that grids are on .0 boundaries */
	    f_time -= (double)(trace->grid[0].alignment % trace->grid[0].period);	/* Want integer remainder */
	}

	decimals = 1;
	f_time = f_time / ((double)trace->grid[0].period);
    }
    else {
	/* Convert time to picoseconds, Preserve fall through order in case statement */
	f_time *= global->time_precision;
	f_time /= trace->timerep;

	if (global->time_precision >= TIMEREP_US) decimals = 0;
	else if (global->time_precision >= TIMEREP_NS) decimals = 3;
	else decimals = 6;

	if (trace->timerep >= TIMEREP_US) decimals -= 0;
	else if (trace->timerep >= TIMEREP_NS) decimals -= 3;
	else decimals -= 6;
    }

    remain = f_time - floor(f_time);
    if (remain < 0.000001) {
	sprintf (strg, "%d", (int)f_time);
    }
    else {
	if (global->time_format[0]) {
	    /* User specified format */
	    sprintf (strg, global->time_format, f_time);
	}
	else {	/* Default format */
	    if (decimals >= 6) sprintf (strg, "%0.06f", f_time);
	    else if (decimals >= 3) sprintf (strg, "%0.03f", f_time);
	    else if (decimals >= 1) sprintf (strg, "%0.01f", f_time);
	    else sprintf (strg, "%f", f_time);
	}
    }
}

#pragma inline (time_units_to_string)
char *time_units_to_string (
    /* find units for the given time represetation */
    TimeRep	timerep,
    Boolean	showvalue)	/* Show value instead of "units" */
{
    static char	units[MAXTIMELEN];

    /* Can't switch as not a integral expression. */
    if (timerep==TIMEREP_CYC)		return ("cycles");
    if (timerep==TIMEREP_PS)		return ("ps");
    if (timerep==TIMEREP_US)		return ("us");
    if (timerep==TIMEREP_NS)		return ("ns");

    if (showvalue) {
	sprintf (units, "%0.0f", timerep);
	return (units);
    }
    else return ("units");
}

#pragma inline (time_units_to_multiplier)
DTime time_units_to_multiplier (
    /* find units for the given time represetation */
    TimeRep	timerep)
{
    return (timerep);
    /*
    switch (timerep) {
      case TIMEREP_CYC:
	return (-1);
      case TIMEREP_PS:
	return (1);
      case TIMEREP_NS:
      default:
	return (1000);
      case TIMEREP_US:
	return (1000000);
	}
	*/
}

