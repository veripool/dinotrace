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


#include <stdio.h>
#include <time.h>

#include <X11/Xlib.h>
#include <Xm/MessageB.h>
#include <Xm/SelectioB.h>
#include <Xm/FileSB.h>

#include "dinotrace.h"
#include "callbacks.h"

extern void cb_prompt_ok(), cb_prompt_cancel(), cb_fil_ok(), cb_fil_can();


/* get normal string from XmString */
char *extract_first_xms_segment(cs)
    XmString cs;
{
    XmStringContext context;
    XmStringCharSet charset;
    XmStringDirection direction;
    Boolean separator;
    char *primitive_string;
    XmStringInitContext(&context,cs);
    XmStringGetNextSegment(context,&primitive_string,
			   &charset,&direction,&separator);
    XmStringFreeContext(context);
    return(primitive_string);
    }


XmString string_create_with_cr (msg)
    char *msg;
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
    

char *date_string()
{
    static char	date_str[50];
    int time_num;
    
    time_num=time (NULL);
    strcpy (date_str, asctime (localtime (&time_num)));
    if (date_str[strlen(date_str)-1]=='\n')
	date_str[strlen(date_str)-1]='\0';

    return (date_str);
    }


void    unmanage_cb (widget, tag, reason )
    /* Generic unmanage routine, usually for cancel buttons */
    Widget		widget;
    Widget		*tag;
    XmAnyCallbackStruct *reason;
{
    if (DTPRINT) printf("In unmanage_cb\n");
    
    /* unmanage the widget */
    XtUnmanageChild(widget);
    }

void    cancel_all_events(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In cancel_all_events - trace=%d\n",trace);
    
    /* remove all events */
    remove_all_events(trace);
    
    /* unmanage any widgets left around */
    if ( trace->signal.add != NULL )
	XtUnmanageChild(trace->signal.add);
    }

void    update_scrollbar(w,value,inc,min,max,size)
    Widget			w;
    int value, inc, min, max, size;
{
    if (DTPRINT) printf("In update_scrollbar - %d %d %d %d %d\n",
			value, inc, min, max, size);

    if (min==max) {
	XtSetArg(arglist[0], XmNsensitive, FALSE);
	min=0; max=1; inc=1; size=1; value=0;
	}
    else {
	XtSetArg(arglist[0], XmNsensitive, TRUE);
	}
    
    if (value > (max - size)) value = max - size;
    if (value < min) value = min;

    if (size > (max - min)) size = max - min;

    XtSetArg(arglist[1], XmNvalue, value);
    XtSetArg(arglist[2], XmNincrement, inc);
    XtSetArg(arglist[3], XmNsliderSize, size);
    XtSetArg(arglist[4], XmNminimum, min);
    XtSetArg(arglist[5], XmNmaximum, max);
    XtSetValues(w, arglist, 6);
    }


void 	add_event (type, callback)
    int		type;
    void	*callback;
{
    TRACE	*trace;

    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	XtAddEventHandler(trace->work, type, TRUE, callback, trace);
	}
    }

void    remove_all_events(trace)
    TRACE		*trace;
{
    TRACE		*trace_ptr;

    if (DTPRINT) printf("In remove_all_events - trace=%d\n",trace);
    
    for (trace_ptr = global->trace_head; trace_ptr; trace_ptr = trace_ptr->next_trace) {
	/* remove all possible events due to res options */ 
	XtRemoveEventHandler(trace_ptr->work,ButtonPressMask,TRUE,res_zoom_click_ev,trace_ptr);
	
	/* remove all possible events due to cursor options */ 
	XtRemoveEventHandler(trace_ptr->work,ButtonPressMask,TRUE,cur_add_ev,trace_ptr);
	XtRemoveEventHandler(trace_ptr->work,ButtonPressMask,TRUE,cur_move_ev,trace_ptr);
	XtRemoveEventHandler(trace_ptr->work,ButtonPressMask,TRUE,cur_delete_ev,trace_ptr);
	XtRemoveEventHandler(trace_ptr->work,ButtonPressMask,TRUE,cur_highlight_ev,trace_ptr);
	
	/* remove all possible events due to grid options */ 
	XtRemoveEventHandler(trace_ptr->work,ButtonPressMask,TRUE,grid_align_ev,trace_ptr);
	
	/* remove all possible events due to signal options */ 
	XtRemoveEventHandler(trace_ptr->work,ButtonPressMask,TRUE,sig_add_ev,trace_ptr);
	XtRemoveEventHandler(trace_ptr->work,ButtonPressMask,TRUE,sig_move_ev,trace_ptr);
	XtRemoveEventHandler(trace_ptr->work,ButtonPressMask,TRUE,sig_copy_ev,trace_ptr);
	XtRemoveEventHandler(trace_ptr->work,ButtonPressMask,TRUE,sig_delete_ev,trace_ptr);
	XtRemoveEventHandler(trace_ptr->work,ButtonPressMask,TRUE,sig_highlight_ev,trace_ptr);

	/* remove all possible events due to nvalue options */ 
	XtRemoveEventHandler(trace_ptr->work,ButtonPressMask,TRUE,val_examine_ev,trace_ptr);
	}

    global->selected_sig = NULL;

    /* Set the cursor back to normal */
    set_cursor (trace, DC_NORMAL);
    }

void new_time(trace)
    TRACE *trace;
{
    SIGNAL	*sig_ptr;
    
    if ( global->time > trace->end_time - (int)((trace->width-XMARGIN-global->xstart)/global->res) ) {
        if (DTPRINT) printf("At end of trace...\n");
        global->time = trace->end_time - (int)((trace->width-XMARGIN-global->xstart)/global->res);
	}
    
    if ( global->time < trace->start_time ) {
        if (DTPRINT) printf("At beginning of trace...\n");
        global->time = trace->start_time;
	}

    /* Update beginning of all traces */
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	    /* if (DTPRINT)
	       printf("next time=%d\n",(*(SIGNAL_LW *)((sig_ptr->cptr)+sig_ptr->inc)).time);
	       */
	
	    if ( !sig_ptr->cptr ) {
		sig_ptr->cptr = sig_ptr->bptr;
		}

	    if (global->time >= (*(SIGNAL_LW *)(sig_ptr->cptr)).time ) {
		while (((*(SIGNAL_LW *)(sig_ptr->cptr)).time != EOT) &&
		       (global->time > (*(SIGNAL_LW *)((sig_ptr->cptr)+sig_ptr->inc)).time)) {
		    (sig_ptr->cptr) += sig_ptr->inc;
		    }
		}
	    else {
		while ((sig_ptr->cptr > sig_ptr->bptr) &&
		       (global->time < (*(SIGNAL_LW *)(sig_ptr->cptr)).time)) {
		    (sig_ptr->cptr) -= sig_ptr->inc;
		    }
		}
	    }
	}
    
    /* Update windows */
    redraw_all (trace);
    }

/* Get window size, calculate what fits on the screen and update scroll bars */

void get_geometry( trace )
    TRACE	*trace;
{
    int		x,y,width,height,dret,max_y;
    
    XGetGeometry( global->display, XtWindow(trace->work), &dret,
		 &x, &y, &width, &height, &dret, &dret);
    
    trace->width = width;
    trace->height = height;
    
    /* calulate the number of signals possibly visible on the screen */
    /* same calculation in get_geometry */
    max_y = (int)((trace->height-trace->ystart)/trace->sighgt);
    trace->numsigvis = MIN(trace->numsig - trace->numsigstart,max_y);
    
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
    
    if (DTPRINT) printf("In get_geometry: x=%d y=%d width=%d height=%d\n",
			x,y,width,height);
    }

void print_geometry( trace )
    TRACE	*trace;
{
    int		temp,x,y,width,height,dret,max_y;
    
    XGetGeometry( global->display, XtWindow(trace->work), &dret,
		 &x, &y, &width, &height, &dret, &dret);
    
    if (DTPRINT) printf("In print_geometry: x=%d y=%d width=%d height=%d\n",
			x,y,width,height);
    }

void  get_file_name( trace )
    TRACE	*trace;
{
    char mask[200], *pattern;
    
    if (DTPRINT) printf("In get_file_name trace=%d\n",trace);
    
    if (!trace->fileselect) {
	XtSetArg(arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg(arglist[1], XmNdialogTitle, XmStringCreateSimple("Open Trace File") );

	trace->fileselect = XmCreateFileSelectionDialog( trace->main, "file", arglist, 2);
	XtAddCallback(trace->fileselect, XmNokCallback, cb_fil_ok, trace);
	XtAddCallback(trace->fileselect, XmNcancelCallback, cb_fil_can, trace);
	XtUnmanageChild( XmFileSelectionBoxGetChild (trace->fileselect, XmDIALOG_HELP_BUTTON));
	
	XSync(global->display,0);
	}
    
    XtManageChild(trace->fileselect);

    /* Directory is global information */
    strcpy (mask, global->directory);
    
    if ( file_format == FF_DECSIM )
	pattern = "*.tra";
    else if ( file_format == FF_TEMPEST )
	pattern = "*.bt";
    
    strcat (mask, pattern);
    XtSetArg(arglist[0], XmNdirMask, XmStringCreateSimple(mask) );
    XtSetArg(arglist[1], XmNpattern, XmStringCreateSimple(pattern) );
    XtSetArg(arglist[2], XmNdirectory, XmStringCreateSimple(global->directory) );
    XtSetValues(trace->fileselect,arglist,3);
    XSync(global->display,0);
    }

void cb_fil_ok(widget, trace, reason)
    Widget	widget;
    TRACE	*trace;
    XmFileSelectionBoxCallbackStruct *reason;
{
    char *tmp;
    
    if (DTPRINT) printf("In cb_fil_ok trace=%d\n",trace);
    
    /*
     ** Unmanage the file select widget here and wait for sync so
     ** the window goes away before the read process begins in case
     ** the idle is very big.
     */
    XtUnmanageChild(trace->fileselect);
    XSync(global->display,0);
    
    tmp = extract_first_xms_segment(reason->value);
    if (DTPRINT) printf("filename=%s\n",tmp);
    
    strcpy (trace->filename, tmp);
    
    XtFree(tmp);
    
    if (DTPRINT) printf("In cb_fil_ok Filename=%s\n",trace->filename);
    cb_fil_read (trace);
    }


void cb_fil_can( widget, trace, reason )
    Widget	widget;
    TRACE	*trace;
    XmFileSelectionBoxCallbackStruct *reason;
{
    if (DTPRINT) printf("In cb_fil_can trace=%d\n",trace);
    
    /* remove the file select widget */
    XtUnmanageChild( trace->fileselect );
    }


/*
static char io_trans_table[100]; 

static XtActionsRec io_action_table[] = {
    {"hit_return", (XtActionProc)cb_prompt_ok},
    {NULL,        NULL}
    };

XtTranslations io_trans_parsed;
*/

void get_data_popup(trace,string,type)
    TRACE	*trace;
    char	*string;
    int		type;
{
    if (DTPRINT) 
	printf("In get_data trace=%d string=%s type=%d\n",trace,string,type);
    
    if (trace->prompt_popup == NULL) {
	/* only need to create the widget once - now just manage/unmanage */
	/* create the dialog box popup window */
	/* XtSetArg(arglist[0], XmNstyle, DwtModal ); */
	XtSetArg(arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg(arglist[1], XmNwidth, DIALOG_WIDTH);
	XtSetArg(arglist[2], XmNheight, DIALOG_HEIGHT);
	trace->prompt_popup = XmCreatePromptDialog(trace->work,"", arglist, 3);
	XtAddCallback(trace->prompt_popup, XmNokCallback, cb_prompt_ok, trace);
	XtAddCallback(trace->prompt_popup, XmNcancelCallback, cb_prompt_cancel, trace);

	XtUnmanageChild( XmSelectionBoxGetChild (trace->prompt_popup, XmDIALOG_HELP_BUTTON));
	}
    
    XtSetArg(arglist[0], XmNdialogTitle, XmStringCreateSimple(string) );
    XtSetArg(arglist[1], XmNtextString, XmStringCreateSimple("") );
    XtSetValues(trace->prompt_popup,arglist,2);
    
    /* manage the popup window */
    trace->prompt_type = type;
    XtManageChild(trace->prompt_popup);
    }

void    cb_prompt_cancel( widget, trace, reason )
    Widget		widget;
    TRACE	*trace;
    XmAnyCallbackStruct *reason;
{
    if (DTPRINT) printf("In cb_prompt_cancel\n");
    XtUnmanageChild(trace->prompt_popup);
    }

void    cb_prompt_ok(w, trace, reason)
    Widget		w;
    TRACE	*trace;
    XmSelectionBoxCallbackStruct *reason;
{
    char	*valstr;
    int		tempi;
    
    if (DTPRINT) printf("In cb_prompt_ok type=%d\n",trace->prompt_type);
    
    /* Get value */
    valstr = extract_first_xms_segment (reason->value);
    tempi = atoi(valstr);

    /* unmanage the popup window */
    XtUnmanageChild(trace->prompt_popup);
    
    /* do stuff depending on type of data */
    switch(trace->prompt_type)
	{
      case IO_GRIDRES:
	/* get the data and store in display structure */
	if (tempi > 0)
	    trace->grid_res = tempi;
	else {
	    sprintf(message,"Value %d out of range",tempi);
	    dino_error_ack(trace,message);
	    return;
	    }
	
	redraw_all (trace);
	break;
	
      case IO_RES:
	/* get the data and store in display structure */
	if (tempi <= 0) {
	    sprintf(message,"Value %d out of range",tempi);
	    dino_error_ack(trace,message);
	    return;
	    }
	else {
	    global->res = RES_SCALE / (float)tempi;
	    
	    /* change the resolution string on display */
	    new_res (trace, TRUE);
	    }
	break;
	
      case IO_TIME:
	if (tempi < 0) {
	    sprintf(message,"Value %d out of range",tempi);
	    dino_error_ack(trace,message);
	    return;
	    }
	else {
	    /* Center it on the screen */
	    global->time = int_to_time (trace, tempi)
		- (int)((trace->width-global->xstart)/global->res/2);

	    new_time(trace);
	    }
	break;
	
      default:
	printf("Error - bad type %d\n",trace->prompt_type);
	}
    }


void dino_message_ack(trace, type, msg)
    TRACE	*trace;
    int		type;	/* See dino_warning_ack macros, 1=warning */
    char		*msg;
{
    static	MAPPED=FALSE;
    static Widget message;
    Arg		arglist[10];
    XmString	xsout;
    
    if (DTPRINT) printf("In dino_message_ack msg=%s\n",msg);
    
    /* create the widget if it hasn't already been */
    if (!MAPPED)
	{
	XtSetArg(arglist[0], XmNdefaultPosition, TRUE);
	switch (type) {
	  case 2:
	    message = XmCreateInformationDialog(trace->work, "info", arglist, 1);
	    break;
	  case 1:
	    message = XmCreateWarningDialog(trace->work, "warning", arglist, 1);
	    break;
	  default:
	    message = XmCreateErrorDialog(trace->work, "error", arglist, 1);
	    break;
	    }
	XtAddCallback(message, XmNokCallback, unmanage_cb, trace);
	XtUnmanageChild( XmMessageBoxGetChild (message, XmDIALOG_CANCEL_BUTTON));
	XtUnmanageChild( XmMessageBoxGetChild (message, XmDIALOG_HELP_BUTTON));
	MAPPED=TRUE;
	}

    /* create string w/ seperators */
    xsout = string_create_with_cr (msg);
    
    /* change the label value and location */
    XtSetArg(arglist[0], XmNmessageString, xsout);
    XtSetArg(arglist[1], XmNdialogTitle, XmStringCreateSimple("Dinotrace Message") );
    XtSetValues(message,arglist,2);
    
    /* manage the widget */
    XtManageChild(message);
    }

/******************************************************************************
 *
 *			Dinotrace Debugging Routines
 *
 *****************************************************************************/

DINO_NUMBER_TO_VALUE(num)
    int	num;
{
    switch(num)
	{
      case STATE_1:
	printf("1");
	return;
	
      case STATE_0:
	printf("0");
	return;
	
      case STATE_U:
	printf("U");
	return;
	
      case STATE_Z:
	printf("Z");
	return;
	
      case STATE_B32:
	printf("B32");
	return;
	
      default:
	printf("UNKNOWN VALUE");
	return;
	}
    }

void    print_sig_names(w,trace)
    Widget		w;
    TRACE	*trace;
{
    SIGNAL	*sig_ptr;

    if (DTPRINT) printf ("In print_sig_names\n");

    printf ("  Number of signals = %d\n", trace->numsig);

    /* loop thru each signal */
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	printf (" Sig '%s'  ty=%d inc=%d index=%d btyp=%d bpos=%d bits=%d\n",
		sig_ptr->signame, sig_ptr->type, sig_ptr->inc,
		sig_ptr->index,
		sig_ptr->file_type, sig_ptr->file_pos, sig_ptr->bits
		);
	}
    
    /* print_signal_states (trace); */
    }

void    print_all_traces(w,trace)
    Widget		w;
    TRACE	*trace;
{
    printf("In print_all_traces.\n");
    }

void    print_screen_traces(w,trace)
    Widget		w;
    TRACE	*trace;
{
    int		i,adj,num;
    SIGNAL	*sig_ptr;
    SIGNAL_LW	*cptr;
    
    printf("There are %d signals currently visible.\n",trace->numsigvis);
    printf("Which signal do you wish to view (0-%d): ",trace->numsigvis-1);
    scanf("%d",&num);
    if ( num < 0 || num > trace->numsigvis-1 )
	{
	printf("Illegal Value.\n");
	return;
	}
    
    adj = global->time * global->res - global->xstart;
    printf("Adjustment value is %d\n",adj);
    
    sig_ptr = (SIGNAL *)trace->dispsig;
    for (i=0; i<num; i++) {
	sig_ptr = (SIGNAL *)sig_ptr->forward;	
	}
    
    cptr = (SIGNAL_LW *)sig_ptr->cptr;
    
    printf("Signal %s starts at %d with a value of ",sig_ptr->signame,cptr->time);
    DINO_NUMBER_TO_VALUE(cptr->state);
    printf("\n");
    
    while( cptr->time != 0x1FFFFFFF &&
	  (cptr->time*global->res-adj) < trace->width - XMARGIN)
	{
	DINO_NUMBER_TO_VALUE(cptr->state);
	printf(" at time %d ns",cptr->time);
	
	if ( cptr->state >= STATE_B32 )
	    {
	    switch(sig_ptr->type)
		{
	      case STATE_B32:
		cptr++;
		printf(" with a value of %x\n",*((unsigned int *)cptr));
		break;
		
	      case STATE_B64:
		cptr++;
		printf(" with a value of %x ",*((unsigned int *)cptr));
		cptr++;
		printf("%x\n",*((unsigned int *)cptr));
		break;
		
	      case STATE_B96:
		cptr++;
		printf(" with a value of %x ",*((unsigned int *)cptr));
		cptr++;
		printf("%x ",*((unsigned int *)cptr));
		cptr++;
		printf("%x\n",*((unsigned int *)cptr));
		break;
		
	      default:
		printf("Error: Bad sig_ptr->type=%d\n",sig_ptr->type);
		break;
		}
	    cptr++;
	    }
	else
	    {
	    printf("\n");
	    cptr += sig_ptr->inc;
	    }
	}
    }


/* Keep only the directory portion of a file spec */
void file_directory (strg)
    char *strg;
{
    char *pchar;
    
#ifdef VMS
    if ((pchar=strrchr(strg,']')) != NULL )
	*(pchar+1) = '\0';
    else
	if ((pchar=strrchr(strg,':')) != NULL )
	    *(pchar+1) = '\0';
	else
	    strg[0] = '\0';
#else
    if ((pchar=strrchr(strg,'/')) != NULL )
	*(pchar+1) = '\0';
    else
	strg[0] = '\0';
#endif
    }


void change_title(trace)
    TRACE	*trace;
{
    char title[300],icontitle[300],*pchar;
    
    /*
     ** Change the name on title bar to filename
     */
    strcpy(title,DTVERSION);
    if (trace->loaded) {
	strcat(title," - ");
	strcat(title,trace->filename);
	}
    
    /* For icon title drop extension and directory */
    if (trace->loaded) {
	strcpy (icontitle, trace->filename);
#ifdef VMS
	if ((pchar=strrchr(trace->filename,']')) != NULL )
	    strcpy (icontitle, pchar+1);
	else
	    if ((pchar=strrchr(trace->filename,':')) != NULL )
		strcpy (icontitle, pchar+1);
	if ((pchar=strchr(icontitle,'.')) != NULL )
	    *(pchar) = '\0';
	if ((pchar=strchr(icontitle,';')) != NULL )
	    *(pchar) = '\0';
	/* Tack on the version number */
	if ((pchar=strrchr(trace->filename,';')) != NULL )
	    strcat (icontitle, pchar);
#else
	if ((pchar=strrchr(trace->filename,'/')) != NULL )
	    strcpy (icontitle, pchar+1);
#endif
	}
    else {
	strcpy(icontitle, DTVERSION);
	}
    
    XtSetArg(arglist[0], XmNtitle, title);
    XtSetArg(arglist[1], XmNiconName, icontitle);
    XtSetValues(trace->toplevel,arglist,2);
    }

#pragma inline (posx_to_time)
int	posx_to_time(trace,x)
    /* convert x value to a time value, return -1 if invalid click */
    TRACE *trace;
    int x;
{
    /* check if button has been clicked on trace portion of screen */
    if ( !trace->loaded || x < global->xstart || x > trace->width - XMARGIN )
	return (-1);
    
    return (((x) + global->time * global->res - global->xstart) / global->res);
    }


#pragma inline (posy_to_signal)
SIGNAL	*posy_to_signal(trace,y)
    /* convert y value to a signal pointer, return NULL if invalid click */
    TRACE *trace;
    int y;
{
    SIGNAL	*sig_ptr;
    int num,i,max_y;

    /* return if there is no file */
    if ( !trace->loaded )
	return (NULL);
    
    /* make sure button has been clicked in in valid location of screen */
    max_y = MIN(trace->numsig,trace->numsigvis);
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
CURSOR *posx_to_cursor(trace, x)
    /* convert x value to the index of the nearest cursor, return NULL if invalid click */
    TRACE *trace;
    int x;
{
    CURSOR *csr_ptr;
    int time;

    /* check if there are any cursors */
    if (!global->cursor_head) {
	return (NULL);
	}
    
    time = posx_to_time (trace, x);
    if (time<0) return (NULL);
    
    /* find the closest cursor */
    csr_ptr = global->cursor_head;
    while ( (time > csr_ptr->time) && csr_ptr->next ) {
	csr_ptr = csr_ptr->next;
	}
    
    /* i is between cursors[i-1] and cursors[i] - determine which is closest */
    if ( csr_ptr->prev && ( (csr_ptr->time - time) > (time - csr_ptr->prev->time) ) ) {
	csr_ptr = csr_ptr->prev;
	}

    return (csr_ptr);
    }


#pragma inline (time_to_cursor)
CURSOR *time_to_cursor (time)
    /* convert specific time value to the index of the nearest cursor, return NULL if none */
    /* Unlike posx_to_cursor, this will not return a "close" one */
    int time;
{
    CURSOR *csr_ptr;

    /* find the closest cursor */
    csr_ptr = global->cursor_head;
    while ( csr_ptr && (time > csr_ptr->time) ) {
	csr_ptr = csr_ptr->next;
	}
    
    if (csr_ptr && (time == csr_ptr->time))
	return (csr_ptr);
    else return (NULL);
    }

#pragma inline (int_to_time)
int int_to_time (trace, value)
    /* convert integer to time value */
    TRACE *trace;
    int value;
{
    if (trace->timerep == TIMEREP_CYC) {
	return (value * trace->grid_res);
	}
    else return (value);
    }

#pragma inline (time_to_strg)
void time_to_string (trace, strg, time, relative)
    /* convert specific time value into the string passed in */
    int time;
    char *strg;
    TRACE *trace;
    int relative;	/* true = time is relative, so don't adjust */
{
    int remain;

    if (trace->timerep == TIMEREP_CYC) {
	if (!relative) {

	    /* Adjust within one cycle so that grids are on .0 boundaries */
	    time -= trace->grid_align % trace->grid_res;
	    }

	remain = ((time * 10)/ trace->grid_res) % 10;
	
	if (!remain) {
	    sprintf (strg, "%d", 
		     (int)(time / trace->grid_res));
	    }
	else {
	    sprintf (strg, "%d.%d", 
		     (int)(time / trace->grid_res), remain);
	    }
	}
    else {
	sprintf (strg, "%d", time);
	}
    }

