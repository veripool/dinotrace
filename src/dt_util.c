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

#include <X11/Xlib.h>
#include <Xm/Xm.h>

#include "dinotrace.h"
#include "callbacks.h"

extern void cb_prompt_ok(), cb_prompt_cancel(),
    cb_fil_ok(), cb_fil_can();


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


void    cancel_all_events(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In cancel_all_events - trace=%d\n",trace);
    
    /* remove all events */
    remove_all_events(trace);
    
    /* unmanage any widgets left around */
    if ( trace->signal.customize != NULL )
	XtUnmanageChild(trace->signal.customize);
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

    XtSetArg(arglist[1], XmNvalue, value);
    XtSetArg(arglist[2], XmNincrement, inc);
    XtSetArg(arglist[3], XmNsliderSize, size);
    XtSetArg(arglist[4], XmNminimum, min);
    XtSetArg(arglist[5], XmNmaximum, max);
    XtSetValues(w, arglist, 6);
    }


void    remove_all_events(trace)
    TRACE		*trace;
{
    if (DTPRINT) printf("In remove_all_events - trace=%d\n",trace);
    
    /* remove all possible events due to res options */ 
    XtRemoveEventHandler(trace->work,ButtonPressMask,TRUE,res_zoom_click_ev,trace);
    
    /* remove all possible events due to cursor options */ 
    XtRemoveEventHandler(trace->work,ButtonPressMask,TRUE,add_cursor,trace);
    XtRemoveEventHandler(trace->work,ButtonPressMask,TRUE,move_cursor,trace);
    XtRemoveEventHandler(trace->work,ButtonPressMask,TRUE,delete_cursor,trace);
    
    /* remove all possible events due to grid options */ 
    XtRemoveEventHandler(trace->work,ButtonPressMask,TRUE,align_grid,trace);
    
    /* remove all possible events due to signal options */ 
    XtRemoveEventHandler(trace->work,ButtonPressMask,TRUE,add_signal,trace);
    XtRemoveEventHandler(trace->work,ButtonPressMask,TRUE,move_signal,trace);
    XtRemoveEventHandler(trace->work,ButtonPressMask,TRUE,delete_signal,trace);

    /* Set the cursor back to normal */
    set_cursor (trace, DC_NORMAL);
    }

new_time(trace)
    TRACE *trace;
{
    int		i,inc;
    short		*pshort;
    SIGNAL_SB	*sig_ptr;
    
    if ( trace->time > trace->end_time - (int)((trace->width-XMARGIN-trace->xstart)/trace->res) ) {
        if (DTPRINT) printf("At end of trace...\n");
        trace->time = trace->end_time - (int)((trace->width-XMARGIN-trace->xstart)/trace->res);
	}
    
    if ( trace->time < trace->start_time ) {
        if (DTPRINT) printf("At beginning of trace...\n");
        trace->time = trace->start_time;
	}
    
    pshort = trace->bus;
    
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	/*
	  if (DTPRINT)
	  printf("next time=%d\n",(*(SIGNAL_LW *)((sig_ptr->cptr)+sig_ptr->inc)).time);
	  */
	
        if ( trace->time >= (*(SIGNAL_LW *)(sig_ptr->cptr)).time ) {
            while (trace->time>(*(SIGNAL_LW *)((sig_ptr->cptr)+sig_ptr->inc)).time) {
		(sig_ptr->cptr)+=sig_ptr->inc;
		}
	    }
	else {
            while (trace->time < (*(SIGNAL_LW *)(sig_ptr->cptr)).time) {
                (sig_ptr->cptr)-=sig_ptr->inc;
		}
	    }
	}
    
    /* Update window */
    XClearWindow(trace->display, trace->wind);
    get_geometry(trace);
    draw(trace);
    }

/* Get window size, calculate what fits on the screen and update scroll bars */

get_geometry( trace )
    TRACE	*trace;
{
    int		temp,x,y,width,height,dret,max_y;
    
    XGetGeometry( XtDisplay(toplevel), XtWindow(trace->work), &dret,
		 &x, &y, &width, &height, &dret, &dret);
    
    trace->width = width;
    trace->height = height;
    
    /* calulate the number of signals possibly visible on the screen */
    /* same calculation in get_geometry */
    max_y = (int)((trace->height-trace->ystart)/trace->sighgt);
    trace->numsigvis = MIN(trace->numsig - trace->numsigdel - trace->numsigstart,max_y);
    
    /* if there are cursors showing, subtract one to make room for cursor */
    if ( trace->numcursors > 0 &&
	trace->cursor_vis &&
	trace->numsigvis > 1 &&
	trace->numsigvis >= max_y ) {
	trace->numsigvis--;
	}
    
    update_scrollbar (trace->hscroll, trace->time,
		      (int)((trace->width-trace->xstart)/trace->res/trace->pageinc),
		      trace->start_time, trace->end_time, 
		      (int)((trace->width-trace->xstart)/trace->res) );

    update_scrollbar (trace->vscroll, trace->numsigstart, 1,
		      0, trace->numsig - trace->numsigdel, trace->numsigvis); 
    
    if (DTPRINT) printf("In get_geometry: x=%d y=%d width=%d height=%d\n",
			x,y,width,height);
    }

void  get_file_name( trace )
    TRACE	*trace;
{
    char mask[200], *pchar;
    
    if (DTPRINT) printf("In get_file_name trace=%d\n",trace);
    
    if (!trace->fileselect) {
	XtSetArg(arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg(arglist[1], XmNdialogTitle, XmStringCreateSimple("Open Trace File") );

	trace->fileselect = XmCreateFileSelectionDialog( trace->main, "file", arglist, 2);
	XtAddCallback(trace->fileselect, XmNokCallback, cb_fil_ok, trace);
	XtAddCallback(trace->fileselect, XmNcancelCallback, cb_fil_can, trace);
	XtUnmanageChild( XmFileSelectionBoxGetChild (trace->fileselect, XmDIALOG_HELP_BUTTON));
	
	XSync(trace->display,0);
	}
    
    XtManageChild(trace->fileselect);

    strcpy (mask, trace->filename);
    file_directory (mask);
    
    if ( trace_format == DECSIM )
	strcat (mask, "*.tra");
    else if ( trace_format == HLO_TEMPEST )
	strcat (mask, "*.bt");
    
    XtSetArg(arglist[0], XmNdirMask, XmStringCreateSimple(mask) );
    XtSetValues(trace->fileselect,arglist,1);
    XSync(trace->display,0);
    }

void cb_fil_ok(widget, trace, reason)
    Widget	widget;
    TRACE	*trace;
    XmFileSelectionBoxCallbackStruct *reason;
{
    int d,status,charset,direction,language,rendition;
    char *tmp;
    
    if (DTPRINT) printf("In cb_fil_ok trace=%d\n",trace);
    
    /*
     ** Unmanage the file select widget here and wait for sync so
     ** the window goes away before the read process begins in case
     ** the idle is very big.
     */
    XtUnmanageChild(trace->fileselect);
    XSync(trace->display,0);
    
    tmp = extract_first_xms_segment(reason->value);
    if (DTPRINT) printf("filename=%s\n",tmp);
    
    strcpy (trace->filename,tmp);
    
    XtFree(tmp);
    
    if (DTPRINT) printf("In cb_fil_ok Filename=%s\n",trace->filename);
    cb_fil_read (trace);
    }


void cb_fil_read(trace)
    TRACE	*trace;
{
    if (DTPRINT) printf("In cb_fil_read trace=%d filename=%s\n",trace,trace->filename);
    
    /* Clear the data structures & the screen */
    clear_display (0, trace);
    set_cursor (trace, DC_BUSY);
    XSync(trace->display,0);
    
    /*
     ** Read in the trace file using the format selected by the user
     */
    if (trace_format == DECSIM)
	read_DECSIM(trace);
    else if (trace_format == HLO_TEMPEST)
	read_HLO_TEMPEST(trace);
    
    /* Change the name on title bar to filename */
    change_title (trace);
    
    /*
     ** Clear the number of deleted signals and the starting signal
     */
    trace->numsigdel = 0;
    trace->numsigstart = 0;
    
    /* get applicable config files */
    config_read_defaults (trace);
    
    /*
     ** Clear the window and draw the screen with the new file
     */
    set_cursor (trace, DC_NORMAL);
    get_geometry(trace);
    XClearWindow(trace->display,trace->wind);
    draw(trace);
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
    
    /* parse the translation table - do this each time for diff types */
/*
    sprintf(io_trans_table,"<KeyPress>0xff0d: cb_prompt_ok(%d,%d)",trace,type); 
    XtAddActions(io_action_table, 1);
    io_trans_parsed = XtParseTranslationTable(io_trans_table);
*/    
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
    
    /*
    XtSetArg(arglist[0], DwtNtextMergeTranslations, io_trans_parsed );
    XtSetValues(popup_wid,arglist,1);
    */
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
    char	string[20]="";
    int		tempi;
    float	tempf;
    
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
	
	/* validate the geometry */
	get_geometry( trace );
	break;
	
      case IO_RES:
	/* get the data and store in display structure */
	if (tempi <= 0) {
	    sprintf(message,"Value %d out of range",tempi);
	    dino_error_ack(trace,message);
	    return;
	    }
	else {
	    trace->res = ((float)(trace->width-trace->xstart))/(float)tempi;
	    
	    /* change the resolution string on display */
	    sprintf(string,"Res=%d ns",(int)((trace->width-trace->xstart)/trace->res) );
	    XtSetArg(arglist[0],XmNlabelString, XmStringCreateSimple(string));
	    XtSetValues(trace->command.reschg_but,arglist,1);
	    }
	/* validate the geometry */
	get_geometry( trace );
	break;
	
      default:
	printf("Error - bad type %d\n",trace->prompt_type);
	}
    
    /* redraw the screen */
    XClearWindow(trace->display, trace->wind);
    draw(trace);
    }


void dino_message_info(trace,msg)
    TRACE	*trace;
    char		*msg;
{
    short	wx,wy,wh,ww;
    static	MAPPED=FALSE;
    static Widget message;
    
    if (DTPRINT) printf("In dino_message_info msg=%s\n",msg);
    
    /* display message at terminal */
    printf("DINO_MESSAGE_INFO: %s\n",msg);
    
    /* display message in main_wid window */
    }

void dino_message_ack(trace, type, msg)
    TRACE	*trace;
    int		type;	/* See dino_warning_ack macros, 1=warning */
    char		*msg;
{
    short	wx,wy,wh,ww;
    static	MAPPED=FALSE;
    static Widget message;
    Arg		arglist[10];
    
    if (DTPRINT) printf("In dino_message_ack msg=%s\n",msg);
    
    /* create the widget if it hasn't already been */
    if (!MAPPED)
	{
	XtSetArg(arglist[0], XmNdefaultPosition, TRUE);
	switch (type) {
	  case 1:
	    message = XmCreateWarningDialog(trace->work, "warning", arglist, 1);
	    break;
	  default:
	    message = XmCreateErrorDialog(trace->work, "error", arglist, 1);
	    break;
	    }
	XtAddCallback(message, XmNokCallback, message_ack, trace);
	XtUnmanageChild( XmMessageBoxGetChild (message, XmDIALOG_CANCEL_BUTTON));
	XtUnmanageChild( XmMessageBoxGetChild (message, XmDIALOG_HELP_BUTTON));
	MAPPED=TRUE;
	}
    
    /* change the label value and location */
    XtSetArg(arglist[0], XmNmessageString, XmStringCreateSimple(msg));
    XtSetArg(arglist[1], XmNdialogTitle, XmStringCreateSimple("Dinotrace Message") );
    XtSetValues(message,arglist,2);
    
    /* manage the widget */
    XtManageChild(message);
    }

void    message_ack(widget, tag, reason )
    Widget		widget;
    Widget		*tag;
    XmAnyCallbackStruct *reason;
{
    if (DTPRINT) printf("In message_ack\n");
    
    /* unmanage the widget */
    XtUnmanageChild(widget);
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
    int		i;
    short	*pbus;
    
    printf("There are %d signals in this trace.\n",trace->numsig);
    printf("Num:\tType:\tName:\n");
    
    for (i=0; i< trace->numsig; i++)
	{
	printf("%d)\t\t%s\n",i,(*(trace->signame)).array[i]);
	}
    
    printf("Bus array: trace->bus[]=");
    pbus = trace->bus;
    for (i=0; i< trace->numsig; i++) {
	printf("%d|",*(pbus+i));
	}
    
    print_signal_states (trace);
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
    SIGNAL_SB	*sig_ptr;
    SIGNAL_LW	*cptr;
    
    printf("There are %d signals currently visible.\n",trace->numsigvis);
    printf("Which signal do you wish to view (0-%d): ",trace->numsigvis-1);
    scanf("%d",&num);
    if ( num < 0 || num > trace->numsigvis-1 )
	{
	printf("Illegal Value.\n");
	return;
	}
    
    adj = trace->time * trace->res - trace->xstart;
    printf("Adjustment value is %d\n",adj);
    
    sig_ptr = (SIGNAL_SB *)trace->dispsig;
    for (i=0; i<num; i++) {
	sig_ptr = (SIGNAL_SB *)sig_ptr->forward;	
	}
    
    cptr = (SIGNAL_LW *)sig_ptr->cptr;
    
    printf("Signal %s starts at %d with a value of ",sig_ptr->signame,cptr->time);
    DINO_NUMBER_TO_VALUE(cptr->state);
    printf("\n");
    
    while( cptr->time != 0x1FFFFFFF &&
	  (cptr->time*trace->res-adj) < trace->width - XMARGIN)
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
    
    if ((pchar=strrchr(strg,']')) != NULL )
	*(pchar+1) = '\0';
    else
	if ((pchar=strrchr(strg,':')) != NULL )
	    *(pchar+1) = '\0';
	else
	    strg[0] = '\0';
    }


void change_title(trace)
    TRACE	*trace;
{
    char title[300],icontitle[300],*pchar;
    
    /*
     ** Change the name on title bar to filename
     */
    strcpy(title,DTVERSION);
    if (trace->filename[0]!='\0') {
	strcat(title," - ");
	strcat(title,trace->filename);
	}
    
    /* For icon title drop extension and directory */
    if (trace->filename[0]!='\0') {
	strcpy (icontitle, trace->filename);
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
	}
    else {
	strcpy(icontitle, DTVERSION);
	}
    
    XtSetArg(arglist[0], XmNtitle, title);
    XtSetArg(arglist[1], XmNiconName, icontitle);
    XtSetValues(toplevel,arglist,2);
    }

