/******************************************************************************
 *
 * Filename:
 *     dt_window.c
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
 *     AAG	 5-Jul-89	Original Version
 *     AAG	22-Aug-90	Base Level V4.1
 *     AAG	29-Apr-91	Use X11, removed aggregate initializer, fixed
 *				 casts for Ultrix support
 *     AAG	 9-Jul-91	Fixed sigstart on vscroll inc/dec/drag and
 *				 added get_geom call
 *     WPS	 8-Jan-92	Added pageinc and unitdec routines
 */


#include <X11/DECwDwtApplProg.h>
#include <X11/Xlib.h>

#include "dinotrace.h"
#include "callbacks.h"



void
free_data(ptr)
DISPLAY_SB		*ptr;
{
    int		i;
    SIGNAL_SB	*sig_ptr,*tmp_sig_ptr;

    if (DTPRINT) printf("In free_data - ptr=%d\n",ptr);

    /* free bus array */
    if (ptr->bus != NULL)
	free(ptr->bus);
    ptr->bus = NULL;

    /* free signal array */
    if (ptr->signame != NULL)
	free(ptr->signame);
    ptr->signame = NULL;

    /* loop and free signal data and each signal structure */
    sig_ptr = (SIGNAL_SB *)ptr->sig.forward;
    for (i=0;i<ptr->numsig-ptr->numsigdel;i++) {
	/* free the signal data */
	if (sig_ptr->bptr != NULL)
	    free(sig_ptr->bptr);

	tmp_sig_ptr = sig_ptr;
	sig_ptr = (SIGNAL_SB *)sig_ptr->forward;

	/* free the signal structure */
	if (tmp_sig_ptr != NULL)
	    free(tmp_sig_ptr);
	}
    ptr->sig.forward = NULL;
    ptr->sig.backward = NULL;
    ptr->numsig = 0;
    ptr->numsigdel = 0;
    }

void
clear_display(w,ptr)
Widget			w;
DISPLAY_SB		*ptr;
{
    char	title[100];

    if (DTPRINT) printf("In clear_display - ptr=%d\n",ptr);

    /* clear the screen */
    XClearWindow(ptr->disp, ptr->wind);

    /* free memory associated with the data */
    free_data(ptr);

    /* clear the name from DISPLAY_SB */
    ptr->filename[0] = '\0';

    /* change the name on title bar back to the ptr */
    change_title (ptr);
}

void
delete_display(w,ptr)
Widget			w;
DISPLAY_SB		*ptr;
{
    if (DTPRINT) printf("In delete_display - ptr=%d\n",ptr);

    /* remove the display */
    XtUnmanageChild(ptr->main);

    /* free memory associated with the data */
    free_data(ptr);

    /* destroy all the widgets created for the screen */
    XtDestroyWidget(ptr->main);

    /* free the display structure */
    if (ptr != NULL)
	free(ptr);

    /* all done */
    exit(1);
}

void
cb_window_expose(w,ptr)
Widget			w;
DISPLAY_SB		*ptr;
{
    char	string[20],data[10];
    int		width;

    /* initialize to NULL string */
    string[0] = '\0';

    if (DTPRINT) printf("In Window Expose - ptr=%d\n",ptr);

    /* change the current pointer to point to the new window */
    curptr = ptr;

    if (DTPRINT) printf("New curptr=%d\n",curptr);

    /* save the old with in case of a resize */
    width = ptr->width;

    /* redraw the entire screen */
    get_geometry( ptr );
    draw( ptr );
    drawsig( ptr );

    /* if the width has changed, update the resolution button */
    if (width != ptr->width)
    {
	sprintf(data,"%d",(int)((ptr->width-ptr->xstart)/ptr->res) );
	strcat(string,"Res=");
	strcat(string,data);
	strcat(string," ns");
        XtSetArg(arglist[0],DwtNlabel,DwtLatin1String(string));
        XtSetValues(ptr->command.reschg_but,arglist,1);
    }

}

void
cb_window_focus(w,ptr)
Widget			w;
DISPLAY_SB		*ptr;
{
    if (DTPRINT) printf("In Window Focus - ptr=%d\n",ptr);

    /* change the current pointer to point to the new window */
    curptr = ptr;

    /* redraw the entire screen */
    get_geometry( ptr );
    draw( ptr );
    drawsig( ptr );
}

void cb_read_trace(w,ptr)
    Widget			w;
    DISPLAY_SB		*ptr;
{
    char	*filename;

    if (DTPRINT) printf("In cb_read_trace - ptr=%d\n",ptr);

    /* free all previous memory */
    free_data(ptr);

    /* get the filename */
    get_file_name(ptr);
    }

void cb_reread_trace(w,ptr)
    Widget			w;
    DISPLAY_SB		*ptr;
{
    char *semi;

    if (ptr->filename[0]=='\0')
	cb_read_trace(w,ptr);
    else {
	if (DTPRINT) printf("In cb_reread_trace - ptr=%d file=%s\n",ptr,ptr->filename);

	/* Drop ;xxx */
	if (semi = strchr(ptr->filename,';'))
	    *semi = '\0';

	if (DTPRINT) printf("In cb_reread_trace - rereading file=%s\n",ptr->filename);

	/* free all previous memory */
	free_data(ptr);

	/* read the filename */
	cb_fil_read(ptr);
	}
    }

void
quit(w,ptr,cb)
Widget			w;
DISPLAY_SB		*ptr;
DwtAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("Quitting\n");

    /* destroy all widgets in the hierarchy */
    XtDestroyWidget(toplevel);

    /* all done */
    exit(1);
}

void hscroll_unitinc(w,ptr,cb)
    Widget			w;
    DISPLAY_SB		*ptr;
    DwtScrollBarCallbackStruct *cb;
{
    if (DTPRINT) printf("In hscroll_unitinc - ptr=%d  old_time=%d",ptr,ptr->time);
    ptr->time += ptr->grid_res;
    if (DTPRINT) printf(" new time=%d\n",ptr->time);

    new_time(ptr);
    }

void hscroll_unitdec(w,ptr,cb)
    Widget			w;
    DISPLAY_SB		*ptr;
    DwtScrollBarCallbackStruct *cb;
{
    if (DTPRINT) printf("In hscroll_unitdec - ptr=%d  old_time=%d",ptr,ptr->time);
    ptr->time -= ptr->grid_res;
    new_time(ptr);
    }

void hscroll_pageinc(w,ptr,cb)
    Widget			w;
    DISPLAY_SB		*ptr;
    DwtScrollBarCallbackStruct *cb;
{
    if (DTPRINT) printf("In hscroll_pageinc - ptr=%d  old_time=%d",ptr,ptr->time);

    if ( ptr->pageinc == QPAGE )
	ptr->time += (int)(((ptr->width-ptr->xstart)/ptr->res)/4);
    else if ( ptr->pageinc == HPAGE )
	ptr->time += (int)(((ptr->width-ptr->xstart)/ptr->res)/2);
    else if ( ptr->pageinc == FPAGE )
	ptr->time += (int)((ptr->width-ptr->xstart)/ptr->res);

    if (DTPRINT) printf(" new time=%d\n",ptr->time);

    new_time(ptr);
    }

void hscroll_pagedec(w,ptr,cb)
    Widget			w;
    DISPLAY_SB		*ptr;
    DwtScrollBarCallbackStruct *cb;
{
    if (DTPRINT) printf("In hscroll_pagedec - ptr=%d  old_time=%d",ptr,ptr->time);

    if ( ptr->pageinc == QPAGE )
	ptr->time -= (int)(((ptr->width-ptr->xstart)/ptr->res)/4);
    else if ( ptr->pageinc == HPAGE )
	ptr->time -= (int)(((ptr->width-ptr->xstart)/ptr->res)/2);
    else if ( ptr->pageinc == FPAGE )
	ptr->time -= (int)((ptr->width-ptr->xstart)/ptr->res);

    if (DTPRINT) printf(" new time=%d\n",ptr->time);

    new_time(ptr);
    }

void hscroll_drag(w,ptr,cb)
    Widget			w;
    DISPLAY_SB		*ptr;
    DwtScrollBarCallbackStruct *cb;
{
    int inc;

    arglist[0].name = DwtNvalue;
    arglist[0].value = (int)&inc;
    XtGetValues(ptr->hscroll,arglist,1);
    if (DTPRINT) printf("inc=%d\n",inc);

    ptr->time = inc;

    new_time(ptr);
    }

void hscroll_bot(w,ptr,cb)
    Widget		w;
    DISPLAY_SB	*ptr;
    DwtScrollBarCallbackStruct *cb;
{
    if (DTPRINT) printf("In hscroll_bot ptr=%d\n",ptr);
    }

void hscroll_top(w,ptr,cb)
    Widget			w;
    DISPLAY_SB		*ptr;
    DwtScrollBarCallbackStruct *cb;
{
    if (DTPRINT) printf("In hscroll_top ptr=%d\n",ptr);
    }

/* Vertically scroll + or - inc lines */

void vscroll_new(ptr,inc)
    DISPLAY_SB		*ptr;
    int inc;
{
    SIGNAL_SB	*tptr,*ttptr;
    
    tptr = (SIGNAL_SB *)ptr->startsig;
    ttptr = (SIGNAL_SB *)tptr->backward;

    while ((inc > 0) &&  (tptr->forward != NULL)) {
	ptr->sigstart++;
	ptr->startsig = tptr->forward;
	tptr = (SIGNAL_SB *)ptr->startsig;
	inc--;
	}
    
    while ((inc < 0) &&  (ttptr->backward != NULL)) {
	ptr->sigstart--;
	ptr->startsig = tptr->backward;
	tptr = (SIGNAL_SB *)ptr->startsig;
	ttptr = (SIGNAL_SB *)tptr->backward;
	inc++;
	}
    
    get_geometry(ptr);
    XClearWindow(ptr->disp, ptr->wind);
    draw(ptr);
    drawsig(ptr);
    }

void vscroll_unitinc(w,ptr,cb)
    Widget			w;
    DISPLAY_SB		*ptr;
    DwtScrollBarCallbackStruct *cb;
{
    vscroll_new (ptr, 1);
    }

void vscroll_unitdec(w,ptr,cb)
    Widget			w;
    DISPLAY_SB		*ptr;
    DwtScrollBarCallbackStruct *cb;
{
    vscroll_new (ptr, -1);
    }

void vscroll_pageinc(w,ptr,cb)
    Widget			w;
    DISPLAY_SB		*ptr;
    DwtScrollBarCallbackStruct *cb;
{
    vscroll_new (ptr, ptr->numsigvis);
    }

void vscroll_pagedec(w,ptr,cb)
    Widget			w;
    DISPLAY_SB		*ptr;
    DwtScrollBarCallbackStruct *cb;
{
    int sigs;

    /* Not numsigvis because may not be limited by screen size */
    sigs = (int)((ptr->height-ptr->ystart)/ptr->sighgt);

    if ( ptr->numcursors > 0 &&
	 ptr->cursor_vis &&
	 ptr->numsigvis > 1 &&
	 ptr->numsigvis >= sigs ) {
	sigs--;
	}

    vscroll_new (ptr, -sigs);
    }

void vscroll_drag(w,ptr,cb)
    Widget			w;
    DISPLAY_SB		*ptr;
    DwtScrollBarCallbackStruct *cb;
{
    int		inc,diff,p;
    SIGNAL_SB	*tptr;

    if (DTPRINT) printf("In vscroll_drag ptr=%d\n",ptr);

    arglist[0].name = DwtNvalue;
    arglist[0].value = (int)&inc;
    XtGetValues(ptr->vscroll,arglist,1);
    if (DTPRINT) printf("inc=%d\n",inc);

    /*
    ** The sig pointer is reset to the start and the loop will set
    ** it to the signal that inc represents
    */
    ptr->sigstart = 0;
    ptr->startsig = ptr->sig.forward;
    vscroll_new (ptr, inc);
    }

void
vscroll_bot(w,ptr,cb)
Widget			w;
DISPLAY_SB		*ptr;
DwtScrollBarCallbackStruct *cb;
{
    if (DTPRINT) printf("In vscroll_bot ptr=%d\n",ptr);
}

void
vscroll_top(w,ptr,cb)
Widget			w;
DISPLAY_SB		*ptr;
DwtScrollBarCallbackStruct *cb;
{
    if (DTPRINT) printf("In vscroll_top ptr=%d\n",ptr);
}

void
cb_chg_res(w,ptr,cb)
Widget			w;
DISPLAY_SB		*ptr;
DwtAnyCallbackStruct	*cb;
{
    char string[10];

    if (DTPRINT) printf("In cb_chg_res - ptr=%d\n",ptr);
    get_data_popup(ptr,"Resolution",IO_RES);
}

void
cb_inc_res(w,ptr,cb)
Widget			w;
DISPLAY_SB		*ptr;
DwtAnyCallbackStruct	*cb;
{
    char	string[20],data[10];

    /* initialize to NULL string */
    string[0] = '\0';

    if (DTPRINT) printf("In cb_inc_res - ptr=%d\n",ptr);

    /* increase the resolution by 10% */
    ptr->res = ptr->res*1.1;
    sprintf(data,"%d",(int)((ptr->width-ptr->xstart)/ptr->res) );
    strcat(string,"Res=");
    strcat(string,data);
    strcat(string," ns");
    XtSetArg(arglist[0],DwtNlabel,DwtLatin1String(string));
    XtSetValues(ptr->command.reschg_but,arglist,1);

    /* redraw the screen with new resolution */
    get_geometry(ptr);
    XClearWindow(ptr->disp,ptr->wind);
    drawsig(ptr);
    draw(ptr);
}

void
cb_dec_res(w,ptr,cb)
Widget			w;
DISPLAY_SB		*ptr;
DwtAnyCallbackStruct	*cb;
{
    char	string[20],data[10];

    /* initialize to NULL string */
    string[0] = '\0';

    if (DTPRINT) printf("In cb_dec_res - ptr=%d\n",ptr);

    /* decrease the resolution by 10% */
    ptr->res = ptr->res*0.9;
    sprintf(data,"%d",(int)((ptr->width-ptr->xstart)/ptr->res) );
    strcat(string,"Res=");
    strcat(string,data);
    strcat(string," ns");
    XtSetArg(arglist[0],DwtNlabel,DwtLatin1String(string));
    XtSetValues(ptr->command.reschg_but,arglist,1);

    /* redraw the screen with new resolution */
    get_geometry(ptr);
    XClearWindow(ptr->disp,ptr->wind);
    drawsig(ptr);
    draw(ptr);
}

void
cb_begin(w, ptr )
Widget		w;
DISPLAY_SB	*ptr;
{
    if (DTPRINT) printf("In cb_start ptr=%d\n",ptr);
    ptr->time = ptr->start_time;
    new_time(ptr);
    }

void
cb_end(w, ptr )
Widget		w;
DISPLAY_SB	*ptr;
{
    if (DTPRINT) printf("In cb_end ptr=%d\n",ptr);
    ptr->time = ptr->end_time - (int)((ptr->width-XMARGIN-ptr->xstart)/ptr->res);
    new_time(ptr);
    }
