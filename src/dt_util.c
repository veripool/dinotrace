#ident "$Id$"
/******************************************************************************
 * DESCRIPTION: Dinotrace source: misc utilities
 *
 * This file is part of Dinotrace.  
 *
 * Author: Wilson Snyder <wsnyder@wsnyder.org> or <wsnyder@iname.com>
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

#include <Xm/MessageB.h>
#include <Xm/SelectioB.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/Separator.h>
#include <Xm/ToggleB.h>
#include <Xm/RowColumn.h>

#include "functions.h"

/**********************************************************************/

extern void prompt_ok_cb(), fil_ok_cb();


void upcase_string (char *tp)
{
    for (;*tp;tp++) *tp = toupper (*tp);
}

void strcpy_overlap (
    /* Copy strings, maybe overlapping (strcpy would be unhappy) */
    char *d,
    char *s)	/* NOT const, may be overlapping */
{
    while ((*d++ = *s++)) ;
}

/* Pattern matching from GNU (see wildmat.c) */
/* Triple procedures, all inlined, unrolls loop much better */
static int wildmatii(
    const char	*s,	/* Buffer */
    const char	*p)	/* Pattern */
{
    for ( ; *p; s++, p++) {
	if (*p!='*') {
	    /* df is magic conversion to lower case */
	    if (((*s & 0xdf)!=(*p & 0xdf)) && *p != '?')
		return(FALSE);
	}
	else {
	    /* Trailing star matches everything. */
	    if (!*++p) return (TRUE);
	    while (wildmat(s, p) == FALSE)
		if (*++s == '\0')
		    return(FALSE);
	    return(TRUE);
	}
    }
    return(*s == '\0' || *s == '[');
}
#ifdef VMS
#pragma inline (wildmatii)
#endif

static int wildmati(
    const char	*s,	/* Buffer */
    const char	*p)	/* Pattern */
{
    for ( ; *p; s++, p++) {
	if (*p!='*') {
	    /* df is magic conversion to lower case */
	    if (((*s & 0xdf)!=(*p & 0xdf)) && *p != '?')
		return(FALSE);
	}
	else {
	    /* Trailing star matches everything. */
	    if (!*++p) return (TRUE);
	    while (wildmatii(s, p) == FALSE)
		if (*++s == '\0')
		    return(FALSE);
	    return(TRUE);
	}
    }
    return(*s == '\0' || *s == '[');
}
#ifdef VMS
#pragma inline (wildmati)
#endif

int wildmat(
    const char	*s,	/* Buffer */
    const char	*p)	/* Pattern */
{
    for ( ; *p; s++, p++) {
	if (*p!='*') {
	    /* df is magic conversion to lower case */
	    /* Not portable, but makes ~10% difference overall performance! */
	    if (((*s & 0xdf)!=(*p & 0xdf)) && *p != '?')
		return(FALSE);
	}
	else {
	    /* Trailing star matches everything. */
	    if (!*++p) return (TRUE);
	    while (wildmati(s, p) == FALSE)
		if (*++s == '\0')
		    return(FALSE);
	    return(TRUE);
	}
    }
    return(*s == '\0' || *s == '[');
}
#ifdef VMS
#pragma inline (wildmat)
#endif

/* get normal string from XmString */
char *extract_first_xms_segment (
    XmString	cs)
{
    XmStringContext	context;
    XmStringCharSet	charset;
    XmStringDirection	direction;
    Boolean_t		separator;
    char		*primitive_string;

    XmStringInitContext (&context,cs);
    XmStringGetNextSegment (context,&primitive_string,
			   &charset,&direction,&separator);
    XmStringFreeContext (context);
    return (primitive_string);
}


XmString string_create_with_cr (
    const char	*msg)
{
    XmString	xsout,xsnew,xsfree;
    const char	*ptr,*nptr;
    char	aline[3000];

    /* create string w/ separators */
    xsout = NULL;
    for (ptr=msg; ptr && *ptr; ptr = nptr) {
	nptr = strchr (ptr, '\n');
	if (nptr) {
	    strncpy (aline, ptr, nptr-ptr);
	    aline[nptr-ptr] = '\0';
	    nptr++;
	}
	else {
	    strcpy (aline, ptr);
	}
	xsnew = XmStringCreateSimple (aline);

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
char *strdup(const char *s)
{
    char *d;
    d=(char *)malloc(strlen(s)+1);
    strcpy (d, s);
    return (d);
}
#endif

#if ! HAVE_STRCASECMP
int strcasecmp(const char *a, const char *b)
{
    for (;*a && *b; a++, b++) {
	if (*a == *b) continue;	/* Bypass toupper if we can */
	if (toupper(*a) == toupper(*b)) continue;
	break;
    }
    return (*a != *b);	/* Both null if match, return 0 */
}
#endif

#define FGETS_SIZE_INC	10240	/* Characters to increase length by (set small for testing) */

void fgets_dynamic_extend (
    /* dynamically extend storage */
    char **line_pptr,	/* & Line pointer */
    uint_t *length_ptr,	/* & Length of the buffer */
    uint_t newlen)
{
    if (*length_ptr == 0) {
	*length_ptr = newlen;
	*line_pptr = XtMalloc (*length_ptr + 1);
    }
    if (*length_ptr < newlen) {
	*length_ptr = newlen;
	*line_pptr = XtRealloc (*line_pptr, *length_ptr + 1);
    }
}
void fgets_dynamic (
    /* fgets a line, with dynamically allocated line storage */
    char **line_pptr,	/* & Line pointer */
    uint_t *length_ptr,	/* & Length of the buffer */
    FILE *readfp)
{
    if (*length_ptr == 0) {
	fgets_dynamic_extend (line_pptr, length_ptr, FGETS_SIZE_INC);
    }

    fgets ((*line_pptr), (*length_ptr), readfp);
    
    /* Did fgets overflow the string? */
    while (*((*line_pptr) + *length_ptr - 2)!='\n'
	   && strlen(*line_pptr)==(*length_ptr - 1)) {
	/* Alloc more */
	fgets_dynamic_extend (line_pptr, length_ptr, *length_ptr + FGETS_SIZE_INC);
	/* Read remainder */
	fgets (((*line_pptr) + *length_ptr - 2) + 1 - FGETS_SIZE_INC,
	       FGETS_SIZE_INC+1, readfp);
    }
}

/* Load char with dinodisk directory, with appropriate / or : tacked on */
/* "" if not defined */
void dinodisk_directory (char *filename)
{
#ifdef VMS
    strcpy (filename, "DINODISK:");
#else
    char *pchar;
    pchar = getenv ("DINODISK");
    if (pchar == NULL) {
	filename[0] = '\0'; return;
    }
    strcpy (filename, pchar);
    strcat (filename, "/");
#endif
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
    Trace_t *trace = widget_to_trace(w);
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
    if (DTPRINT_ENTRY) printf ("In update_scrollbar - Val %d Inc %d mn %d MX %d sz %d\n",
			       value, inc, min, max, size);

    if (min >= max) {
	XtSetArg (arglist[0], XmNsensitive, FALSE);
	min=0; max=1; inc=1; size=1; value=0;
    }
    else {
	XtSetArg (arglist[0], XmNsensitive, TRUE);
    }

    if (inc < 1) inc=1;
    if (size > (max - min)) size = max - min;
    if (size < 1) size=1;
    if (value > (max - size)) value = max - size;
    if (value < min) value = min;

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
    Trace_t	*trace;

    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	XtInsertEventHandler (trace->work, type, TRUE,
			      (XtEventHandler)callback, trace, XtListHead);
    }
}

static void    remove_event (
    int		type,
    void	(*callback)())
{
    Trace_t	*trace;
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	XtRemoveEventHandler (trace->work, type, TRUE,
			      (XtEventHandler)callback, trace);
    }
}


void    remove_all_events (
    Trace_t	*trace)
{
    if (DTPRINT_ENTRY) printf ("In remove_all_events - trace=%p\n",trace);
    
    /* remove all possible events due to res options */ 
    remove_event (ButtonPressMask, res_zoom_click_ev);
	
    /* remove all possible events due to cursor options */ 
    remove_event (ButtonPressMask, cur_add_ev);
    remove_event (ButtonPressMask, cur_move_ev);
    remove_event (ButtonPressMask, cur_delete_ev);
    remove_event (ButtonPressMask, cur_note_ev);
    remove_event (ButtonPressMask, cur_highlight_ev);
    
    /* remove all possible events due to signal options */ 
    remove_event (ButtonPressMask, sig_add_ev);
    remove_event (ButtonPressMask, sig_move_ev);
    remove_event (ButtonPressMask, sig_copy_ev);
    remove_event (ButtonPressMask, sig_note_ev);
    remove_event (ButtonPressMask, sig_delete_ev);
    remove_event (ButtonPressMask, sig_highlight_ev);
    remove_event (ButtonPressMask, sig_radix_ev);
    remove_event (ButtonPressMask, sig_waveform_ev);
    
    /* remove all possible events due to value options */ 
    remove_event (ButtonPressMask, val_examine_ev);
    remove_event (ButtonPressMask, val_highlight_ev);
    
    global->selected_sig = NULL;

    /* Set the cursor back to normal */
    set_cursor (DC_NORMAL);
}

ColorNum_t submenu_to_color (
    Trace_t	*trace,		/* Display information */
    Widget	w,
    int		base)		/* Loaded when the menu was loaded */
{
    ColorNum_t color;

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
Trace_t	*widget_to_trace (
    Widget	w)
{
    Trace_t	*trace;		/* Display information */
    
    XtSetArg (arglist[0], XmNuserData, &trace);
    XtGetValues (w, arglist, 1);

    if (!trace) printf ("widget_to_trace failed lookup.\n");
    return (trace);
}

/* Adjust starting times on signals to new point */
static void new_time_sigs (
    Signal_t	*sig_first_ptr)
{
    Signal_t	*sig_ptr;	

    for (sig_ptr = sig_first_ptr; sig_ptr; sig_ptr = sig_ptr->forward) {
	Value_t	*cptr = sig_ptr->cptr;
	if ( !cptr ) {
	    cptr = sig_ptr->bptr;
	}
	/*if (DTPRINT) {printf ("sig %s move: %p   %p\n",
			      sig_ptr->signame, sig_ptr->cptr, sig_ptr->bptr);
			      print_sig_info (sig_ptr);}*/

	if (global->time >= CPTR_TIME(cptr) ) {
	    /* Step forward till right segment is found */
	    while ((CPTR_TIME(cptr) != EOT) &&
		   (global->time > CPTR_TIME(CPTR_NEXT(cptr)))) {
		cptr = CPTR_NEXT(cptr);
	    }
	}
	else {
	    /* Step backward till right segment is found */
	    while ((cptr > sig_ptr->bptr) &&
		   (global->time < CPTR_TIME(cptr))) {
		cptr = CPTR_PREV(cptr);
	    }
	}
	sig_ptr->cptr = cptr;
    }
}

void new_time (
    Trace_t 	*trace)
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
    Trace_t	*trace)
{
    int		x,y;
    uint_t	width,height,dret,depth;
    uint_t	pixmap_width,pixmap_height;
    Dimension m_time_height = global->time_font->ascent;
    Window	root;
    
    float old_res_per_pixel = global->res / (trace->width - global->xstart - XMARGIN);

    XGetGeometry ( global->display, trace->pixmap, &root,
		   &x, &y, &pixmap_width, &pixmap_height, &dret, &depth);

    XGetGeometry ( global->display, XtWindow (trace->work), &root,
		   &x, &y, &width, &height, &dret, &depth);
    
    if (pixmap_width != width || pixmap_height != height) {
	/* Reallocate a new pixmap of the right size */
	XFreePixmap (global->display, trace->pixmap);
	trace->pixmap = XCreatePixmap (global->display, XtWindow (trace->work),
				       width, height, depth);
    }

    /* Update width and resolution */
    trace->width = MIN(width, (uint_t)MAXSCREENWIDTH-2);
    global->res = old_res_per_pixel * (trace->width - global->xstart - XMARGIN);

    /* See comment in dinotrace.h about y layout */
    trace->height = height;
    trace->ygridtime = Y_TOP_BOTTOM + m_time_height;
    trace->ystart = trace->ygridtime + Y_TEXT_SPACE + MAX(Y_GRID_TOP,Y_CURSOR_TOP);

    /* if there are cursors showing, leave room */
    trace->ycursortimeabs = trace->height - Y_TOP_BOTTOM; 
    if ( (global->cursor_head != NULL) &&
	 global->cursor_vis) {
	trace->ycursortimerel = trace->ycursortimeabs - Y_TEXT_SPACE - m_time_height;
	trace->yend = trace->ycursortimerel - Y_TEXT_SPACE - m_time_height - Y_GRID_BOTTOM;
    } else {
	trace->ycursortimerel = trace->ycursortimeabs;
	trace->yend = trace->ycursortimeabs - Y_GRID_BOTTOM;
    }
    
    /* calulate the number of signals possibly visible on the screen */
    trace->numsigvis = (trace->yend - trace->ystart) / global->sighgt;
    
    /*update_scrollb (w, value, inc, min, max, size) */
    update_scrollbar (trace->hscroll, global->time,
		      grid_primary_period (trace),
		      trace->start_time, trace->end_time, 
		      (int)TIME_WIDTH(trace) );

    draw_namescroll (trace);

    update_scrollbar (trace->vscroll, trace->numsigstart, 1,
		      0, trace->numsig, MIN (trace->numsigvis, trace->numsig - trace->numsigstart)); 
    
    if (DTPRINT_ENTRY) printf ("In get_geometry: x=%d y=%d width=%d height=%d\n",
			x,y,width,height);
}

void get_data_popup (
    Trace_t	*trace,
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
    Widget	w,
    Trace_t	*trace,
    XmSelectionBoxCallbackStruct *cb)
{
    char	*valstr;
    DTime_t	restime;
    
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
    switch (trace->prompt_type) {
    case IO_RES:
	/* change the resolution string on display */
	new_res (trace, RES_SCALE / (float)restime);
	break;
	
    default:
	printf ("Error - bad type %d\n",trace->prompt_type);
    }
}


void dino_message_ack (
    Trace_t	*trace,
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

    /* create string w/ separators */
    xsout = string_create_with_cr (msg);
    
    /* change the label value and location */
    XtSetArg (msg_arglist[0], XmNmessageString, xsout);
    XtSetArg (msg_arglist[1], XmNdialogTitle, XmStringCreateSimple ("Dinotrace Message") );
    XtSetValues (trace->message,msg_arglist,2);
    
    /* manage the widget */
    DManageChild (trace->message, trace, MC_NOKEYS);
}

void	ok_apply_cancel (
    OkApplyWidgets_t *wid_ptr,
    Widget form,		/* Form to add widgets under */
    Widget above,		/* Widget above this one */
    XtCallbackProc ok_cb,	/* Callbacks to activate on each button */
    Trace_t *ok_trace,		 
    XtCallbackProc apply_cb,
    Trace_t *apply_trace,		 
    XtCallbackProc defaults_cb,
    Trace_t *defaults_trace,		 
    XtCallbackProc cancel_cb,
    Trace_t *cancel_trace		/* Often widget to unmanage */
    )
{
    Trace_t *trace = ok_trace;

    /* Create Separator */
    XtSetArg (arglist[0], XmNtopAttachment, XmATTACH_WIDGET );
    XtSetArg (arglist[1], XmNtopWidget, above );
    XtSetArg (arglist[2], XmNleftAttachment, XmATTACH_FORM );
    XtSetArg (arglist[3], XmNrightAttachment, XmATTACH_FORM );
    wid_ptr->sep = XmCreateSeparator (form, "sep",arglist,4);
    DManageChild (wid_ptr->sep, trace, MC_NOKEYS);
	
    if (ok_cb) {
        /* Create OK button */
        XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple (" OK ") );
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNleftOffset, 10);
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNtopWidget, wid_ptr->sep );
	XtSetArg (arglist[5], XmNbottomOffset, 10);
	XtSetArg (arglist[6], XmNbottomAttachment, XmATTACH_FORM);
	wid_ptr->ok = XmCreatePushButton (form,"ok",arglist,7);
	DAddCallback (wid_ptr->ok, XmNactivateCallback, ok_cb, ok_trace);
	DManageChild (wid_ptr->ok, trace, MC_NOKEYS);
    }
	
    if (apply_cb) {
        /* create apply button */
        XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Apply") );
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_POSITION );
	XtSetArg (arglist[2], XmNleftPosition, (defaults_cb)?35:45);
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNtopWidget, wid_ptr->sep );
	XtSetArg (arglist[5], XmNbottomOffset, 10);
	XtSetArg (arglist[6], XmNbottomAttachment, XmATTACH_FORM);
	wid_ptr->apply = XmCreatePushButton (form,"apply",arglist,7);
	DAddCallback (wid_ptr->apply, XmNactivateCallback, apply_cb, apply_trace);
	DManageChild (wid_ptr->apply, trace, MC_NOKEYS);
    }
	
    if (defaults_cb) {
        /* create defaults button */
        XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Defaults") );
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_POSITION );
	XtSetArg (arglist[2], XmNleftPosition, (apply_cb)?55:45);
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNtopWidget, wid_ptr->sep );
	XtSetArg (arglist[5], XmNbottomOffset, 10);
	XtSetArg (arglist[6], XmNbottomAttachment, XmATTACH_FORM);
	wid_ptr->defaults = XmCreatePushButton (form,"defaults",arglist,7);
	DAddCallback (wid_ptr->defaults, XmNactivateCallback, defaults_cb, defaults_trace);
	DManageChild (wid_ptr->defaults, trace, MC_NOKEYS);
    }
	
    if (cancel_cb) {
        /* create cancel button */
        XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Cancel") );
	XtSetArg (arglist[1], XmNrightAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNrightOffset, 10);
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNtopWidget, wid_ptr->sep );
	XtSetArg (arglist[5], XmNbottomOffset, 10);
	XtSetArg (arglist[6], XmNbottomAttachment, XmATTACH_FORM);
	wid_ptr->cancel = XmCreatePushButton (form,"cancel",arglist,7);
	DAddCallback (wid_ptr->cancel, XmNactivateCallback, cancel_cb, cancel_trace);
	DManageChild (wid_ptr->cancel, trace, MC_NOKEYS);
    }
}

/******************************************************************************
 *
 *			Dinotrace Debugging Routines
 *
 *****************************************************************************/

void    print_cptr (
    Value_t	*value_ptr)
{
    char strg[1000];
    val_to_string (global->radixs[0], strg, value_ptr, 0, FALSE, FALSE);
    if (CPTR_TIME(value_ptr)==EOT) {
	printf ("%s at EOT\n", strg);
    }
    else {
	printf ("%s at time %d\n", strg, CPTR_TIME(value_ptr));
    }
}

void    print_sig_info_internal (
    Signal_t	*sig_ptr,
    Value_t	*cptr,
    DTime_t	end_time)
{
    const char *states[8] = {"F128", "B128", "F32 ", "B32",
			     "Z  ", "U  ", "1  ", "0  "};
    printf ("Signal %s, cptr %p  bptr %p\n",sig_ptr->signame, sig_ptr->cptr, sig_ptr->bptr);
    
    for ( ; cptr->siglw.stbits.size > 0; cptr = CPTR_NEXT(cptr) ) {
	printf ("%p: %08x T:%08d  %s  %08x %08x %08x %08x    ", cptr,
		cptr->siglw.number, cptr->time, states[cptr->siglw.stbits.state],
		cptr->number[0],cptr->number[1],
		cptr->number[2],cptr->number[3]);
	print_cptr (cptr);
	/*printf ("s %d  sp %d\n", cptr->siglw.stbits.size, cptr->siglw.stbits.size_prev);*/
	if (end_time && CPTR_TIME(cptr) > end_time) break;
    }
}

void    print_sig_info (
    Signal_t	*sig_ptr)
{
    print_sig_info_internal (sig_ptr, sig_ptr->bptr, 0);
}

void    debug_print_screen_traces_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    uint_t	i,num;
    Signal_t	*sig_ptr;
    
    printf ("There are up to %d signals currently visible.\n",trace->numsigvis);
    printf ("Which signal do you wish to view [0-%d]: ",trace->numsigvis-1);
    scanf ("%d",&num);
    if ( num > trace->numsigvis-1 ) {
	printf ("Illegal Value.\n");
	return;
    }
    
    sig_ptr = (Signal_t *)trace->dispsig;
    for (i=0; i<num; i++) {
	sig_ptr = (Signal_t *)sig_ptr->forward;	
    }
    
    print_sig_info_internal (sig_ptr, sig_ptr->cptr, global->time + TIME_WIDTH (trace));
}

void    debug_signal_integrity (
    Trace_t	*trace,
    Signal_t	*sig_ptr,
    char	*list_name,
    Boolean_t	del)
{
    Value_t	*cptr;
    Value_t	*cptr_last;
    Signal_t	*last_sig_ptr=NULL;
    DTime_t	last_time;
    uint_t	nsigstart, nsig;
    Boolean_t	hitstart;
    Boolean_t	dumpsig;

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
	dumpsig = FALSE;
	if (sig_ptr->deleted != del) {
	    printf ("%%E, %s, Bad deleted flag on %s!\n", list_name, sig_ptr->signame);
	    dumpsig = TRUE;
	}

	if (sig_ptr->forward && sig_ptr->forward->backward != sig_ptr) {
	    printf ("%%E, %s, Bad backward link on signal %s\n", list_name, sig_ptr->signame);
	    dumpsig = TRUE;
	}

	/* Change data */
	last_time = -1;
	cptr_last = NULL;
	for (cptr = sig_ptr->bptr; CPTR_TIME(cptr) != EOT; cptr = CPTR_NEXT(cptr)) {
	    if ( cptr_last == cptr) {
		printf ("%%E, %s, size field messed up, infinite loop, signal %s time %d\n",
			list_name, sig_ptr->signame, CPTR_TIME(cptr));
		dumpsig = TRUE;
		break;
	    }
	    if ( cptr_last && cptr->siglw.stbits.size_prev
		 != cptr_last->siglw.stbits.size) {
		printf ("%%E, %s, size != prev size, signal %s time %d\n",
			list_name, sig_ptr->signame, CPTR_TIME(cptr));
		dumpsig = TRUE;
		break;
	    }
	    cptr_last = cptr;
	    if ( CPTR_TIME(cptr) == last_time ) {
		/*printf ("%%W, %s, Double time change at signal %s time %d\n",
			list_name, sig_ptr->signame, CPTR_TIME(cptr));
			dumpsig = TRUE;*/
	    }
	    if ( CPTR_TIME(cptr) < last_time ) {
		printf ("%%E, %s, Reverse time change at signal %s time %d\n",
			list_name, sig_ptr->signame, CPTR_TIME(cptr));
		dumpsig = TRUE;
	    }
	    last_time = CPTR_TIME(cptr);
	}
	if (last_time != sig_ptr->trace->end_time) {
	    printf ("%%E, %s, Doesn't end at right time, signal %s time %d\n",
		    list_name, sig_ptr->signame, CPTR_TIME(cptr));
	    dumpsig = TRUE;
	}
	if (dumpsig) {
	    print_sig_info (sig_ptr);
	}
	cptr_last = cptr;
	last_sig_ptr = sig_ptr;
    }
    if (last_sig_ptr != trace->lastsig) {
	printf ("%%E, %s, Last signal pointer, %p!=%p\n", list_name, last_sig_ptr, trace->lastsig);
    }

    if (!del) {
	if (nsig != trace->numsig) {
	    printf ("%%E, %s, Numsigs is wrong, %d!=%d\n", list_name, nsig, trace->numsig);
	}
	if (!hitstart) {
	    printf ("%%E, %s, Never found display starting point\n", list_name);
	}
	if (nsigstart) nsigstart--;
	if (nsigstart != trace->numsigstart) {
	    printf ("%%E, %s, Numsigstart is wrong, %d!=%d\n", list_name, nsigstart, trace->numsigstart);
        }
    }
}

void    debug_integrity_check_cb (
    Widget	w)
{
    Trace_t *trace;
    for (trace = global->deleted_trace_head; trace; trace = trace->next_trace) {
	debug_signal_integrity (trace, trace->firstsig, trace->dfile.filename, 
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


void    debug_statistics_cb (
    Widget		w)
{
    Trace_t *trace;
    Signal_t	*sig_ptr;	
    Value_t	*cptr;

    int i=0;
    int sigs=0;
    int sigs_ncopy=0;
    int bysize[32];
    int cp_used=0;
    int cp_alloc=0;
    int allhigh=0;
    int states[8];
    
    for (i=0; i<8; i++) states[i]=0;
    for (i=0; i<32; i++) bysize[i]=0;

    for (trace = global->deleted_trace_head; trace; trace = trace->next_trace) {
	for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	    sigs++;
	    if (!sig_ptr->copyof) {
		sigs_ncopy++;
		for (i=0; (1<<i)<sig_ptr->bits; i++); bysize[i]++;
		cp_alloc += sig_ptr->blocks;
		for (cptr = sig_ptr->bptr; CPTR_TIME(cptr) != EOT; cptr = CPTR_NEXT(cptr)) {
		    cp_used++;
		    states[cptr->siglw.stbits.state]++;
		    if (cptr->siglw.stbits.allhigh && cptr->siglw.stbits.state!=STATE_1) allhigh++;
		}
	    }
	}
    }

#define PCT(x,y) ((y)?(int)(((double)(x)*100)/(double)(y)):0)
    printf ("Statistics\n");
    printf ("%10d       Signals\n", sigs);
    printf ("%10d  %3d%% Signals, not a copy\n", sigs_ncopy, PCT(sigs_ncopy,sigs));
    for (i=0; i<32; i++) {
	if (bysize[i]) printf ("  %10d  %3d%% Size <=%d bits\n", bysize[i], PCT(bysize[i],sigs_ncopy), 1<<i);
    }

    printf ("%10d       Cptrs Alloced\n", cp_alloc);
    printf ("%10d  %3d%% Cptrs Used\n", cp_used, PCT(cp_used,cp_alloc));
    for (i=0; i<8; i++) {
	printf ("  %10d  %3d%% State %d\n", states[i], PCT(states[i],cp_used), i);
    }
    printf ("  %10d  %3d%% Allhigh\n", allhigh, PCT(allhigh,cp_used));
#undef PCT
}


void change_title (
    Trace_t	*trace)
{
    char title[300],icontitle[300],*pchar;
    
    /*
     ** Change the name on title bar to filename
     */
    strcpy (title,DTVERSION);
    if (trace->loaded) {
	strcat (title," - ");
	strcat (title,trace->dfile.filename);
    }
    
    /* For icon title drop extension and directory */
    strcpy (icontitle, DTVERSION);
    if (trace->loaded) {
	strcpy (icontitle, trace->dfile.filename);
	if ((pchar=strrchr (icontitle,'/')) != NULL )
	    strcpy_overlap (icontitle, pchar+1);
	if ((pchar=strrchr (icontitle,'\\')) != NULL )
	    strcpy_overlap (icontitle, pchar+1);
#ifdef VMS
	if ((pchar=strrchr (icontitle,']')) != NULL )
	    strcpy_overlap (icontitle, pchar+1);
	if ((pchar=strrchr (trace->dfile.filename,':')) != NULL )
	    strcpy_overlap (icontitle, pchar+1);
	if ((pchar=strchr (icontitle,';')) != NULL )
	    * (pchar) = '\0';
#endif
#ifdef VMS
	/* Tack back on the version number */
	if ((pchar=strrchr (trace->dfile.filename,';')) != NULL )
	    strcat (icontitle, pchar);
#endif
    }
    
    XtSetArg (arglist[0], XmNtitle, title);
    XtSetArg (arglist[1], XmNiconName, icontitle);
    XtSetValues (trace->toplevel,arglist,2);
}

#ifdef VMS
#pragma inline (posx_to_time)
#endif
DTime_t	posx_to_time (
    /* convert x value to a time value, return -1 if invalid click */
    Trace_t 	*trace,
    Position 	x)
{
    /* check if button has been clicked on trace portion of screen */
    if ( !trace->loaded || x < global->xstart || x > trace->width - XMARGIN )
	return (-1);
    
    return (((x) + global->time * global->res - global->xstart) / global->res);
}


Value_t *value_at_time (
    /* Return the value for the given time */
    /* Must be on the screen! */
    Signal_t	*sig_ptr,
    DTime_t	ctime)
{
    Value_t	*cptr;
    for (cptr = sig_ptr->cptr;
	 CPTR_TIME(cptr) != EOT
	     && (CPTR_TIME(CPTR_NEXT(cptr)) <= ctime);
	 cptr = CPTR_NEXT(cptr)) {
    }
    return (cptr);
}


DTime_t	posx_to_time_edge (
    /* convert x value to a time value, return -1 if invalid click */
    /* allow clicking to a edge if it is enabled */
    Trace_t 	*trace,
    Position	x,
    Position	y)
{
    DTime_t	xtime;
    Signal_t 	*sig_ptr;
    Value_t	*cptr;
    DTime_t	left_time, right_time;

    xtime = posx_to_time (trace,x);
    if (xtime<0 || !global->click_to_edge) return (xtime);

    /* If no signal at this position return time directly */
    if (! (sig_ptr = posy_to_signal (trace,y))) return (xtime);

    /* Find time where signal changes */
    for (cptr = sig_ptr->cptr;
	 (CPTR_TIME(cptr) != EOT) && (CPTR_TIME(CPTR_NEXT(cptr)) < xtime);
	 cptr = CPTR_NEXT(cptr)) ;
    left_time = CPTR_TIME(cptr);
    if (left_time == EOT) return (xtime);
    cptr = CPTR_NEXT(cptr);
    right_time = CPTR_TIME(cptr);

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


#ifdef VMS
#pragma inline (posy_to_signal)
#endif
Signal_t	*posy_to_signal (
    /* convert y value to a signal pointer, return NULL if invalid click */
    Trace_t	*trace,
    Position 	y)
{
    Signal_t	*sig_ptr;
    int num,i,max_y;

    /* return if there is no file */
    if ( !trace->loaded )
	return (NULL);
    
    /* make sure button has been clicked in in valid location of screen */
    max_y = MIN (trace->numsig,trace->numsigvis);
    if ( y < trace->ystart || y > trace->ystart + max_y * global->sighgt )
	return (NULL);
    
    /* figure out which signal has been selected */
    num = (int)((y - trace->ystart) / global->sighgt);

    /* set pointer to signal to 'highlight' */
    sig_ptr = trace->dispsig;
    for (i=0; sig_ptr && i<num; i++)
	sig_ptr = sig_ptr->forward;

    return (sig_ptr);
}


#ifdef VMS
#pragma inline (posx_to_cursor)
#endif
DCursor_t *posx_to_cursor (
    /* convert x value to the index of the nearest cursor, return NULL if invalid click */
    Trace_t	*trace,
    Position	x)
{
    DCursor_t 	*csr_ptr;
    DTime_t 	xtime;

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


Grid_t *posx_to_grid (
    /* convert x value to the index of the nearest grid, return NULL if invalid click */
    Trace_t	*trace,
    Position	x,
    DTime_t	*time_ptr)
{
    DTime_t 	xtime;
    Grid_t 	*best_grid_ptr = NULL;
    DTime_t 	best_grid_time = 0;
    DTime_t 	best_grid_error = 0;
    int grid_num;

    xtime = posx_to_time (trace, x);
    if (xtime<0) return (NULL);

    /* find the closest grid */
    for (grid_num=0; grid_num<MAXGRIDS; grid_num++) {
	Grid_t *grid_ptr = &(trace->grid[grid_num]);
	if (grid_ptr->visible && grid_ptr->period) {
	    DTime_t grid_error = ((xtime - (grid_ptr->alignment % grid_ptr->period))
				% grid_ptr->period);
	    DTime_t grid_time = xtime - grid_error;
	    if (grid_error > (grid_ptr->period/2)) {
		grid_error = grid_ptr->period - grid_error;
		grid_time += grid_ptr->period;
	    }
	    if (!best_grid_ptr || grid_error < best_grid_error) {
		best_grid_ptr = grid_ptr;
		best_grid_time = grid_time;
		best_grid_error = grid_error;
	    }
	}
    }
    *time_ptr = best_grid_time;
    return (best_grid_ptr);
}


#ifdef VMS
#pragma inline (time_to_cursor)
#endif
DCursor_t *time_to_cursor (
    /* convert specific time value to the index of the nearest cursor, return NULL if none */
    /* Unlike posx_to_cursor, this will not return a "close" one */
    DTime_t	xtime)
{
    DCursor_t	*csr_ptr;

    /* find the closest cursor */
    csr_ptr = global->cursor_head;
    while ( csr_ptr && (xtime > csr_ptr->time) ) {
	csr_ptr = csr_ptr->next;
    }
    
    if (csr_ptr && (xtime == csr_ptr->time))
	return (csr_ptr);
    else return (NULL);
}

/* Convert time to nearest cycle number. */
double time_to_cyc_num (DTime_t time) {
    Trace_t *trace;
    double f_time;

    trace = global->trace_head;
    if (time == EOT) {
	return (0);	/* Not right, but not important. */
    }

    f_time = (double)time;
    f_time -= (double)(trace->grid[0].alignment % grid_primary_period(trace));	/* Want integer remainder */
    return (f_time / (double)grid_primary_period(trace));
}

/* Convert time to a cycle number. */
DTime_t cyc_num_to_time (double cyc_num) {
  Trace_t *trace;

  trace = global->trace_head;

  return ((DTime_t)cyc_num * grid_primary_period(trace)
	  + trace->grid[0].alignment % grid_primary_period(trace));
}


#ifdef VMS
#pragma inline (string_to_time)
#endif
DTime_t string_to_time (
    /* convert integer to time value */
    Trace_t	*trace,
    char	*strg)
{
    double	f_time;

    f_time = atof (strg);
    if (f_time < 0) return (0);

    if (global->timerep == TIMEREP_CYC) {
	return ((DTime_t)(f_time * grid_primary_period(trace)
			+ trace->grid[0].alignment % grid_primary_period(trace)));
    }

    /* First convert to picoseconds */
    f_time *= global->timerep;

    /* Then convert to internal units and return */
    f_time /= global->time_precision;

    return ((DTime_t)f_time);
}

#ifdef VMS
#pragma inline (time_to_string)
#endif
void time_to_string (
    /* convert specific time value into the string passed in */
    Trace_t	*trace,
    char	*strg,
    DTime_t	ctime,
    Boolean_t	relative)	/* true = time is relative, so don't adjust */
{
    double	f_time, remain;
    int		decimals;

    f_time = (double)ctime;

    if (ctime == EOT) {
	strcpy (strg, "EOT");
	return;
    }

    if (global->timerep == TIMEREP_CYC) {
	if (!relative) {
	    /* Adjust within one cycle so that grids are on .0 boundaries */
	    f_time -= (double)(trace->grid[0].alignment % grid_primary_period(trace));	/* Want integer remainder */
	}

	decimals = 1;
	f_time = f_time / ((double)grid_primary_period(trace));
    }
    else {
	/* Convert time to picoseconds, Preserve fall through order in case statement */
	f_time *= global->time_precision;
	f_time /= global->timerep;

	if (global->time_precision >= TIMEREP_US) decimals = 0;
	else if (global->time_precision >= TIMEREP_NS) decimals = 3;
	else decimals = 6;

	if (global->timerep >= TIMEREP_US) decimals -= 0;
	else if (global->timerep >= TIMEREP_NS) decimals -= 3;
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

char *time_units_to_string (
    /* find units for the given time represetation */
    TimeRep_t	timerep,
    Boolean_t	showvalue)	/* Show value instead of "units" */
{
    static char	units[MAXTIMELEN];

    /* Can't switch as not a integral expression. */
    if (timerep==TIMEREP_CYC)		return ("cycles");
    if (timerep==TIMEREP_FS)		return ("fs");
    if (timerep==TIMEREP_PS)		return ("ps");
    if (timerep==TIMEREP_US)		return ("us");
    if (timerep==TIMEREP_NS)		return ("ns");

    if (showvalue) {
	sprintf (units, "%0.0f", timerep);
	return (units);
    }
    else return ("units");
}

DTime_t time_units_to_multiplier (
    /* find units for the given time represetation */
    TimeRep_t	timerep)
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

