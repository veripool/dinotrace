/******************************************************************************
 *
 * Filename:
 *     dt_util.c
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
 *     AAG	 6-Nov-90	Changed get_geometry to calculate numsigvis
 *				correctly when cursors are added
 *     AAG	29-Apr-91	Use X11, removed '#include descrip', removed
 *				 '&' in hit_return() param list for Ultrix
 *				 support
 *     AAG	 8-Jul-91	Adding call to read hlo binary trace files,
 *				 added XSync after unmanaging file select
 *				 window, fixed vscroll 'Value' parameter
 *     WPS	 8-Jan-92	Added new_time gadget controls
 */
static char rcsid[] = "$Id$";


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <X11/Xlib.h>
#include <Xm/MessageB.h>
#include <Xm/SelectioB.h>
#include <Xm/FileSB.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <Xm/RowColumn.h>

#include "dinotrace.h"
#include "callbacks.h"

extern void prompt_ok_cb(), fil_ok_cb();


/* get normal string from XmString */
char *extract_first_xms_segment (cs)
    XmString	cs;
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


XmString string_create_with_cr (msg)
    char	*msg;
{
    XmString	xsout,xsnew,xsfree;
    char	*ptr,*nptr;
    char	aline[200];

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
    

char *date_string (time_num)
    time_t	time_num;	/* Time to parse, or 0 for current time */
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


void    unmanage_cb (w, tag, cb )
    /* Generic unmanage routine, usually for cancel buttons */
    /* NOTE that you pass the widget in the tag area, not the trace!! */
    Widget		w;
    Widget		tag;
    XmAnyCallbackStruct *cb;
{
    if (DTPRINT_ENTRY) printf ("In unmanage_cb\n");
    
    /* unmanage the widget */
    XtUnmanageChild (tag);
    }

void    cancel_all_events (w,trace,cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT_ENTRY) printf ("In cancel_all_events - trace=%d\n",trace);
    
    /* remove all events */
    remove_all_events (trace);
    
    /* unmanage any widgets left around */
    if ( trace->signal.add != NULL )
	XtUnmanageChild (trace->signal.add);
    }

void    update_scrollbar (w,value,inc,min,max,size)
    Widget		w;
    int 	value, inc, min, max, size;
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


void 	add_event (type, callback)
    int		type;
    void	(*callback)();
{
    TRACE	*trace;

    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	XtAddEventHandler (trace->work, type, TRUE, callback, trace);
	}
    }

void    remove_all_events (trace)
    TRACE	*trace;
{
    TRACE	*trace_ptr;

    if (DTPRINT_ENTRY) printf ("In remove_all_events - trace=%d\n",trace);
    
    for (trace_ptr = global->trace_head; trace_ptr; trace_ptr = trace_ptr->next_trace) {
	/* remove all possible events due to res options */ 
	XtRemoveEventHandler (trace_ptr->work,ButtonPressMask,TRUE,res_zoom_click_ev,trace_ptr);
	
	/* remove all possible events due to cursor options */ 
	XtRemoveEventHandler (trace_ptr->work,ButtonPressMask,TRUE,cur_add_ev,trace_ptr);
	XtRemoveEventHandler (trace_ptr->work,ButtonPressMask,TRUE,cur_move_ev,trace_ptr);
	XtRemoveEventHandler (trace_ptr->work,ButtonPressMask,TRUE,cur_delete_ev,trace_ptr);
	XtRemoveEventHandler (trace_ptr->work,ButtonPressMask,TRUE,cur_highlight_ev,trace_ptr);
	
	/* remove all possible events due to grid options */ 
	XtRemoveEventHandler (trace_ptr->work,ButtonPressMask,TRUE,grid_align_ev,trace_ptr);
	
	/* remove all possible events due to signal options */ 
	XtRemoveEventHandler (trace_ptr->work,ButtonPressMask,TRUE,sig_add_ev,trace_ptr);
	XtRemoveEventHandler (trace_ptr->work,ButtonPressMask,TRUE,sig_move_ev,trace_ptr);
	XtRemoveEventHandler (trace_ptr->work,ButtonPressMask,TRUE,sig_copy_ev,trace_ptr);
	XtRemoveEventHandler (trace_ptr->work,ButtonPressMask,TRUE,sig_delete_ev,trace_ptr);
	XtRemoveEventHandler (trace_ptr->work,ButtonPressMask,TRUE,sig_highlight_ev,trace_ptr);

	/* remove all possible events due to nvalue options */ 
	XtRemoveEventHandler (trace_ptr->work,ButtonPressMask,TRUE,val_examine_ev,trace_ptr);
	XtRemoveEventHandler (trace_ptr->work,ButtonPressMask,TRUE,val_highlight_ev,trace_ptr);
	}

    global->selected_sig = NULL;

    /* Set the cursor back to normal */
    set_cursor (trace, DC_NORMAL);
    }

ColorNum submenu_to_color (trace, w, base)
    TRACE	*trace;		/* Display information */
    Widget	w;
    int		base;		/* Loaded when the menu was loaded */
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

TRACE	*widget_to_trace (w)
    Widget		w;
{
    TRACE	*trace;		/* Display information */
    
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	if (trace->work == w) break;
	}
    if (!trace && DTDEBUG) printf ("widget_to_trace failed lookup.\n");
    return (trace);
    }

void new_time (trace)
    TRACE 	*trace;
{
    SIGNAL	*sig_ptr;
    
    if ( global->time > trace->end_time - (int)((trace->width-XMARGIN-global->xstart)/global->res) ) {
        if (DTPRINT_ENTRY) printf ("At end of trace...\n");
        global->time = trace->end_time - (int)((trace->width-XMARGIN-global->xstart)/global->res);
	}
    
    if ( global->time < trace->start_time ) {
        if (DTPRINT_ENTRY) printf ("At beginning of trace...\n");
        global->time = trace->start_time;
	}

    /* Update beginning of all traces */
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	    /* if (DTPRINT)
	       printf ("next time=%d\n", (* (SIGNAL_LW *)((sig_ptr->cptr)+sig_ptr->lws)).sttime.time);
	       */
	
	    if ( !sig_ptr->cptr ) {
		sig_ptr->cptr = sig_ptr->bptr;
		}

	    if (global->time >= (* (SIGNAL_LW *)(sig_ptr->cptr)).sttime.time ) {
		while (((* (SIGNAL_LW *)(sig_ptr->cptr)).sttime.time != EOT) &&
		       (global->time > (* (SIGNAL_LW *)((sig_ptr->cptr)+sig_ptr->lws)).sttime.time)) {
		    (sig_ptr->cptr) += sig_ptr->lws;
		    }
		}
	    else {
		while ((sig_ptr->cptr > sig_ptr->bptr) &&
		       (global->time < (* (SIGNAL_LW *)(sig_ptr->cptr)).sttime.time)) {
		    (sig_ptr->cptr) -= sig_ptr->lws;
		    }
		}
	    }
	}
    
    /* Update windows */
    draw_all_needed ();
    }

/* Get window size, calculate what fits on the screen and update scroll bars */
void get_geometry ( trace )
    TRACE	*trace;
{
    int		x,y,max_y;
    unsigned int	width,height,dret;
    
    XGetGeometry ( global->display, XtWindow (trace->work), (Window *)&dret,
		 &x, &y, &width, &height, &dret, &dret);
    
    trace->width = width;
    trace->height = height;
    
    /* calulate the number of signals possibly visible on the screen */
    max_y = max_sigs_on_screen (trace);
    trace->numsigvis = MIN (trace->numsig - trace->numsigstart,max_y);
    
    /* if there are cursors showing, subtract one to make room for cursor */
    if ( (global->cursor_head != NULL) &&
	trace->cursor_vis &&
	trace->numsigvis > 1 &&
	trace->numsigvis >= max_y ) {
	trace->numsigvis--;
	}
    
    update_scrollbar (trace->hscroll, global->time,
		      trace->grid_res,
		      trace->start_time, trace->end_time, 
		      (int)((trace->width-global->xstart)/global->res) );

    update_scrollbar (trace->vscroll, trace->numsigstart, 1,
		      0, trace->numsig, trace->numsigvis); 
    
    if (DTPRINT_ENTRY) printf ("In get_geometry: x=%d y=%d width=%d height=%d\n",
			x,y,width,height);
    }

void  fil_select_set_pattern (trace)
    TRACE	*trace;
    /* Set the file requester pattern information (called in 2 places) */
{
    char mask[MAXFNAMELEN], *dirname, *pattern;
    XmString	xs_dirname;

    /* Grab directory information */
    XtSetArg (arglist[0], XmNdirectory, &xs_dirname);
    XtGetValues (trace->fileselect.dialog, arglist, 1);
    dirname = extract_first_xms_segment (xs_dirname);

    /* Pattern information */
    strcpy (mask, dirname);
    pattern = filetypes[file_format].mask;
    strcat (mask, pattern);
    XtSetArg (arglist[0], XmNdirMask, XmStringCreateSimple (mask) );
    XtSetArg (arglist[1], XmNpattern, XmStringCreateSimple (pattern) );
    XtSetValues (trace->fileselect.dialog,arglist,2);
    }


void  get_file_name ( trace )
    TRACE	*trace;
{
    int		i;
    
    if (DTPRINT_ENTRY) printf ("In get_file_name trace=%d\n",trace);
    
    if (!trace->fileselect.dialog) {
	XtSetArg (arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg (arglist[1], XmNdialogTitle, XmStringCreateSimple ("Open Trace File") );

	trace->fileselect.dialog = XmCreateFileSelectionDialog ( trace->main, "file", arglist, 2);
	XtAddCallback (trace->fileselect.dialog, XmNokCallback, fil_ok_cb, trace);
	XtAddCallback (trace->fileselect.dialog, XmNcancelCallback, unmanage_cb, trace->fileselect.dialog);
	XtUnmanageChild ( XmFileSelectionBoxGetChild (trace->fileselect.dialog, XmDIALOG_HELP_BUTTON));
	
	trace->fileselect.work_area = XmCreateWorkArea (trace->fileselect.dialog, "wa", arglist, 0);

	/* Create FILE FORMAT */
	trace->fileselect.format_menu = XmCreatePulldownMenu (trace->fileselect.work_area,"fmtrad",arglist,0);

	for (i=0; i<FF_NUMFORMATS; i++) {
	    if (filetypes[i].selection) {
		XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple (filetypes[i].name) );
		trace->fileselect.format_item[i] =
		    XmCreatePushButtonGadget (trace->fileselect.format_menu,"pdbutton",arglist,1);
		XtManageChild (trace->fileselect.format_item[i]);
		XtAddCallback (trace->fileselect.format_item[i], XmNactivateCallback, fil_format_option_cb, trace);
		}
	    }

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("File Format"));
	XtSetArg (arglist[1], XmNsubMenuId, trace->fileselect.format_menu);
	trace->fileselect.format_option = XmCreateOptionMenu (trace->fileselect.work_area,"format",arglist,2);
	XtManageChild (trace->fileselect.format_option);

	/* Create save_ordering button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Preserve Signal Ordering"));
	XtSetArg (arglist[1], XmNx, 0);
	XtSetArg (arglist[2], XmNy, 100);
	XtSetArg (arglist[3], XmNshadowThickness, 1);
	trace->fileselect.save_ordering = XmCreateToggleButton (trace->fileselect.work_area,"save_ordering",arglist,4);
	XtManageChild (trace->fileselect.save_ordering);
	
	XtManageChild (trace->fileselect.work_area);
	
	XSync (global->display,0);
	}
    
    XtManageChild (trace->fileselect.dialog);

    /* Ordering */
    XtSetArg (arglist[0], XmNset, global->save_ordering ? 1:0);
    XtSetValues (trace->fileselect.save_ordering,arglist,1);
    
    /* File format */
    if (filetypes[file_format].selection) {
	XtSetArg (arglist[0], XmNmenuHistory, trace->fileselect.format_item[file_format]);
	XtSetValues (trace->fileselect.format_option, arglist, 1);
	}
    
    /* Set directory */
    XtSetArg (arglist[0], XmNdirectory, XmStringCreateSimple (global->directory) );
    XtSetValues (trace->fileselect.dialog,arglist,1);

    fil_select_set_pattern (trace);

    XSync (global->display,0);
    }

void    fil_format_option_cb (w,trace,cb)
    Widget	w;
    TRACE	*trace;
    XmSelectionBoxCallbackStruct *cb;
{
    int 	i;
    if (DTPRINT_ENTRY) printf ("In fil_format_option_cb trace=%d\n",trace);

    for (i=0; i<FF_NUMFORMATS; i++) {
	if (w == trace->fileselect.format_item[i]) {
	    file_format = i;	/* Change global verion */
	    trace->fileformat = i;	/* Specifically make this file use this format */
	    fil_select_set_pattern (trace);
	    }
	}
    }

void fil_ok_cb (w, trace, cb)
    Widget	w;
    TRACE	*trace;
    XmFileSelectionBoxCallbackStruct *cb;
{
    char	*tmp;
    
    if (DTPRINT_ENTRY) printf ("In fil_ok_cb trace=%d\n",trace);
    
    /*
     ** Unmanage the file select widget here and wait for sync so
     ** the window goes away before the read process begins in case
     ** the idle is very big.
     */
    XtUnmanageChild (trace->fileselect.dialog);
    XSync (global->display,0);
    
    tmp = extract_first_xms_segment (cb->value);
    if (DTPRINT_FILE) printf ("filename=%s\n",tmp);
    
    global->save_ordering = XmToggleButtonGetState (trace->fileselect.save_ordering);
    
    strcpy (trace->filename, tmp);
    
    DFree (tmp);
    
    if (DTPRINT_FILE) printf ("In fil_ok_cb Filename=%s\n",trace->filename);
    fil_read_cb (trace);
    }


void get_data_popup (trace,string,type)
    TRACE	*trace;
    char	*string;
    int		type;
{
    if (DTPRINT_ENTRY) 
	printf ("In get_data trace=%d string=%s type=%d\n",trace,string,type);
    
    if (trace->prompt_popup == NULL) {
	/* only need to create the widget once - now just manage/unmanage */
	/* create the dialog box popup window */
	/* XtSetArg (arglist[0], XmNstyle, DwtModal ); */
	XtSetArg (arglist[0], XmNdefaultPosition, TRUE);
	/* XtSetArg (arglist[1], XmNheight, DIALOG_HEIGHT); */
	/* no width so autochosen XtSetArg (arglist[2], XmNwidth, DIALOG_WIDTH); */
	trace->prompt_popup = XmCreatePromptDialog (trace->work,"", arglist, 1);
	XtAddCallback (trace->prompt_popup, XmNokCallback, prompt_ok_cb, trace);
	XtAddCallback (trace->prompt_popup, XmNcancelCallback, unmanage_cb, trace->prompt_popup);

	XtUnmanageChild ( XmSelectionBoxGetChild (trace->prompt_popup, XmDIALOG_HELP_BUTTON));
	}
    
    XtSetArg (arglist[0], XmNdialogTitle, XmStringCreateSimple (string) );
    XtSetArg (arglist[1], XmNtextString, XmStringCreateSimple ("") );
    XtSetValues (trace->prompt_popup,arglist,2);
    
    /* manage the popup window */
    trace->prompt_type = type;
    XtManageChild (trace->prompt_popup);
    }

void    prompt_ok_cb (w, trace, cb)
    Widget		w;
    TRACE		*trace;
    XmSelectionBoxCallbackStruct *cb;
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
      case IO_GRIDRES:
	/* get the data and store in display structure */
	trace->grid_res = (int)restime;
	draw_all_needed ();
	break;
	
      case IO_RES:
	/* get the data and store in display structure */
	global->res = RES_SCALE / (float)restime;
	    
	/* change the resolution string on display */
	new_res (trace);
	break;
	
      default:
	printf ("Error - bad type %d\n",trace->prompt_type);
	}
    }


void dino_message_ack (trace, type, msg)
    TRACE	*trace;
    int		type;	/* See dino_warning_ack macros, 1=warning */
    char	*msg;
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
	XtAddCallback (trace->message, XmNokCallback, unmanage_cb, trace->message);
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
    XtManageChild (trace->message);
    }

SIGNAL_LW *cptr_at_time (sig_ptr, ctime)
    /* Return the CPTR for the given time */
    SIGNAL	*sig_ptr;
    DTime	ctime;
{
    SIGNAL_LW	*cptr;
    for (cptr = (SIGNAL_LW *)(sig_ptr->cptr);
	 (cptr->sttime.time != EOT) && (((cptr + sig_ptr->lws)->sttime.time) <= ctime);
	     cptr += sig_ptr->lws) ;
    return (cptr);
    }


/******************************************************************************
 *
 *			Dinotrace Debugging Routines
 *
 *****************************************************************************/

void    cptr_to_string (cptr, strg)
    SIGNAL_LW	*cptr;
    char	*strg;
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
	sprintf (strg, "%X", (cptr+1)->number);
	return;
	
      case STATE_B64:
	sprintf (strg, "%X_%08X", (cptr+2)->number,
		 (cptr+1)->number);
	return;
	
      case STATE_B96:
	sprintf (strg, "%X_%08X_%08X", (cptr+3)->number,
		 (cptr+2)->number,
		 (cptr+1)->number);
	return;
	
      default:
	strg[0] = '?';
	return;
	}
    }

void    print_sig_names (w,trace)
    Widget	w;
    TRACE	*trace;
{
    SIGNAL	*sig_ptr, *back_sig_ptr;

    if (DTPRINT_ENTRY) printf ("In print_sig_names\n");

    printf ("  Number of signals = %d\n", trace->numsig);

    /* loop thru each signal */
    back_sig_ptr = NULL;
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	printf (" Sig '%s'  ty=%d inc=%d index=%d-%d tempbit %d btyp=%d bpos=%d bits=%d\n",
		sig_ptr->signame, sig_ptr->type, sig_ptr->lws,
		sig_ptr->msb_index ,sig_ptr->lsb_index, sig_ptr->bit_index,
		sig_ptr->file_type.flags, sig_ptr->file_pos, sig_ptr->bits
		);
	if (sig_ptr->backward != back_sig_ptr) {
	    printf (" %%E, Backward link is to '%d' not '%d'\n", sig_ptr->backward, back_sig_ptr);
	    }
	back_sig_ptr = sig_ptr;
	}
    
    /* print_signal_states (trace); */
    }

void    print_cptr (cptr)
    SIGNAL_LW	*cptr;
{
    char strg[100];

    cptr_to_string (cptr, strg);
    printf ("%s at time %d\n",strg,cptr->sttime.time);
    }

void    print_screen_traces (w,trace)
    Widget	w;
    TRACE	*trace;
{
    int		i,adj,num;
    SIGNAL	*sig_ptr;
    
    printf ("There are %d signals currently visible.\n",trace->numsigvis);
    printf ("Which signal do you wish to view [0-%d]: ",trace->numsigvis-1);
    scanf ("%d",&num);
    if ( num < 0 || num > trace->numsigvis-1 )
	{
	printf ("Illegal Value.\n");
	return;
	}
    
    adj = global->time * global->res - global->xstart;
    printf ("Adjustment value is %d\n",adj);
    
    sig_ptr = (SIGNAL *)trace->dispsig;
    for (i=0; i<num; i++) {
	sig_ptr = (SIGNAL *)sig_ptr->forward;	
	}
    
    print_sig_info (sig_ptr);
    }

void    print_sig_info (sig_ptr)
    SIGNAL	*sig_ptr;
{
    SIGNAL_LW	*cptr;
    
    cptr = (SIGNAL_LW *)sig_ptr->bptr;
    
    printf ("Signal %s starts ",sig_ptr->signame);
    print_cptr (cptr);
    
    for ( ; cptr->sttime.time != EOT; cptr += sig_ptr->lws ) {
	/*printf ("%x %x %x %x %x    ", (cptr)->number, (cptr+1)->number, (cptr+2)->number,
		(cptr+3)->number, (cptr+4)->number);*/
	print_cptr (cptr);
	}
    }

void    debug_signal_integrity (trace, sig_ptr, list_name, del)
    TRACE	*trace;
    SIGNAL	*sig_ptr;
    char	*list_name;
    Boolean	del;
{
    SIGNAL_LW	*cptr;
    DTime	last_time;
    int		nsigstart, nsig;
    Boolean	hitstart;

    if (!sig_ptr) return;

    if (sig_ptr->backward != NULL) {
	printf ("%s, Backward pointer should be NULL on signal %s\n", list_name, sig_ptr->signame);
	}

    nsig = 0; nsigstart = -1;
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
	for (cptr = (SIGNAL_LW *)sig_ptr->bptr; cptr->sttime.time != EOT; cptr += sig_ptr->lws) {
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
	if (nsigstart != trace->numsigstart) {
	    printf ("%s, Numsigstart is wrong, %d!=%d\n", list_name, nsigstart, trace->numsigstart);
        }
    }
}

void    debug_integrity_check_cb (w,trace,cb)
    Widget		w;
    TRACE	       	*trace;
    XmAnyCallbackStruct	*cb;
{
    SIGNAL	*sig_ptr;

    debug_signal_integrity (NULL, global->delsig, "Deleted Signals", TRUE);

    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	debug_signal_integrity (trace, trace->firstsig, trace->filename, FALSE);
	}
    }


void    debug_increase_debugtemp_cb (w,trace,cb)
    Widget		w;
    TRACE	       	*trace;
    XmAnyCallbackStruct	*cb;
{
    DebugTemp++;
    printf ("New DebugTemp = %d\n", DebugTemp);
    draw_all_needed ();
    }

void    debug_decrease_debugtemp_cb (w,trace,cb)
    Widget		w;
    TRACE	       	*trace;
    XmAnyCallbackStruct	*cb;
{
    DebugTemp--;
    printf ("New DebugTemp = %d\n", DebugTemp);
    draw_all_needed ();
    }


/* Keep only the directory portion of a file spec */
void file_directory (strg)
    char	*strg;
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


void change_title (trace)
    TRACE	*trace;
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
DTime	posx_to_time (trace,x)
    /* convert x value to a time value, return -1 if invalid click */
    TRACE 	*trace;
    Position 	x;
{
    /* check if button has been clicked on trace portion of screen */
    if ( !trace->loaded || x < global->xstart || x > trace->width - XMARGIN )
	return (-1);
    
    return (((x) + global->time * global->res - global->xstart) / global->res);
    }


DTime	posx_to_time_edge (trace,x,y)
    /* convert x value to a time value, return -1 if invalid click */
    /* allow clicking to a edge if it is enabled */
    TRACE 	*trace;
    Position	x,y;
{
    DTime	xtime;
    SIGNAL 	*sig_ptr;
    SIGNAL_LW	*cptr;
    DTime	left_time, right_time;

    xtime = posx_to_time (trace,x);
    if (xtime<0 || !global->click_to_edge) return (xtime);

    /* If no signal at this position return time directly */
    if (! (sig_ptr = posy_to_signal (trace,y))) return (xtime);

    /* Find time where signal changes */
    for (cptr = (SIGNAL_LW *)(sig_ptr->cptr);
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
SIGNAL	*posy_to_signal (trace,y)
    /* convert y value to a signal pointer, return NULL if invalid click */
    TRACE	*trace;
    Position 	y;
{
    SIGNAL	*sig_ptr;
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
CURSOR *posx_to_cursor (trace, x)
    /* convert x value to the index of the nearest cursor, return NULL if invalid click */
    TRACE	*trace;
    Position	x;
{
    CURSOR 	*csr_ptr;
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
CURSOR *time_to_cursor (xtime)
    /* convert specific time value to the index of the nearest cursor, return NULL if none */
    /* Unlike posx_to_cursor, this will not return a "close" one */
    DTime	xtime;
{
    CURSOR	*csr_ptr;

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
DTime string_to_time (trace, strg)
    /* convert integer to time value */
    TRACE	*trace;
    char	*strg;
{
    double	f_time;

    f_time = atof (strg);
    if (f_time < 0) return (0);

    if (trace->timerep == TIMEREP_CYC) {
	return ((DTime)(f_time * trace->grid_res
			+ trace->grid_align % trace->grid_res));
	}

    /* First convert to picoseconds */
    f_time *= trace->timerep;

    /* Then convert to internal units and return */
    f_time /= global->time_precision;

    return ((DTime)f_time);
    }

#pragma inline (time_to_string)
void time_to_string (trace, strg, ctime, relative)
    /* convert specific time value into the string passed in */
    DTime	ctime;
    char	*strg;
    TRACE	*trace;
    Boolean	relative;	/* true = time is relative, so don't adjust */
{
    double	f_time, remain;
    int		decimals;

    f_time = (double)ctime;

    if (trace->timerep == TIMEREP_CYC) {
	if (!relative) {
	    /* Adjust within one cycle so that grids are on .0 boundaries */
	    f_time -= (double)(trace->grid_align % trace->grid_res);	/* Want integer remainder */
	    }

	decimals = 1;
	f_time = f_time / ((double)trace->grid_res);
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
	    if (decimals >= 6) sprintf (strg, "%0.06lf", f_time);
	    else if (decimals >= 3) sprintf (strg, "%0.03lf", f_time);
	    else if (decimals >= 1) sprintf (strg, "%0.01lf", f_time);
	    else sprintf (strg, "%lf", f_time);
	    }
	}
    }

#pragma inline (time_units_to_string)
char *time_units_to_string (timerep, showvalue)
    /* find units for the given time represetation */
    TimeRep	timerep;
    Boolean	showvalue;	/* Show value instead of "units" */
{
    static char	units[20];

    /* Can't switch as not a integral expression. */
    if (timerep==TIMEREP_CYC)		return ("cycles");
    if (timerep==TIMEREP_PS)		return ("ps");
    if (timerep==TIMEREP_US)		return ("us");
    if (timerep==TIMEREP_NS)		return ("ns");

    if (showvalue) {
	sprintf (units, "%0.0lf", timerep);
	return (units);
	}
    else return ("units");
    }

#pragma inline (time_units_to_multiplier)
DTime time_units_to_multiplier (timerep)
    /* find units for the given time represetation */
    TimeRep	timerep;
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

