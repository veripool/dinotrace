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
 *     WPS	 8-Jan-93	Added pageinc and unitdec routines
 *     WPS	15-Feb-93	Added zoom, full, new_res routines
 */


#include <X11/DECwDwtApplProg.h>
#include <X11/Xlib.h>

#include "dinotrace.h"
#include "callbacks.h"



void cb_window_expose(w,ptr)
    Widget		w;
    DISPLAY_SB		*ptr;
{
    char	string[20];
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
    if (width != ptr->width)    {
	sprintf(string,"Res=%d ns",(int)((ptr->width-ptr->xstart)/ptr->res) );
        XtSetArg(arglist[0],DwtNlabel,DwtLatin1String(string));
        XtSetValues(ptr->command.reschg_but,arglist,1);
	}
    }

void cb_window_focus(w,ptr)
    Widget		w;
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

void cb_begin(w, ptr )
    Widget		w;
    DISPLAY_SB		*ptr;
{
    if (DTPRINT) printf("In cb_start ptr=%d\n",ptr);
    ptr->time = ptr->start_time;
    new_time(ptr);
    }

void cb_end(w, ptr )
    Widget		w;
    DISPLAY_SB		*ptr;
{
    if (DTPRINT) printf("In cb_end ptr=%d\n",ptr);
    ptr->time = ptr->end_time - (int)((ptr->width-XMARGIN-ptr->xstart)/ptr->res);
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

void vscroll_bot(w,ptr,cb)
    Widget		w;
    DISPLAY_SB		*ptr;
    DwtScrollBarCallbackStruct *cb;
{
    if (DTPRINT) printf("In vscroll_bot ptr=%d\n",ptr);
    }

void vscroll_top(w,ptr,cb)
    Widget		w;
    DISPLAY_SB		*ptr;
    DwtScrollBarCallbackStruct *cb;
{
    if (DTPRINT) printf("In vscroll_top ptr=%d\n",ptr);
    }

void cb_chg_res(w,ptr,cb)
    Widget		w;
    DISPLAY_SB		*ptr;
    DwtAnyCallbackStruct	*cb;
{
    char string[10];

    if (DTPRINT) printf("In cb_chg_res - ptr=%d\n",ptr);
    get_data_popup(ptr,"Resolution",IO_RES);
    }


void new_res(ptr, redisplay)
    DISPLAY_SB		*ptr;
    int		redisplay;	/* TRUE to refresh the screen after change */
{
    char	string[20];

    if (DTPRINT) printf ("In new_res - res = %f\n",ptr->res);

    if (ptr->res==0.0) ptr->res=0.1;	/* prevent div zero error */

    /* change res button's value */
    sprintf(string,"Res=%d ns",(int)((ptr->width-ptr->xstart)/ptr->res) );
    XtSetArg(arglist[0],DwtNlabel,DwtLatin1String(string));
    XtSetValues(ptr->command.reschg_but,arglist,1);

    if (redisplay) {
	/* redraw the screen with new resolution */
	get_geometry(ptr);
	XClearWindow(ptr->disp,ptr->wind);
     	drawsig(ptr);
	draw(ptr);
	}
    }

void cb_inc_res(w,ptr,cb)
    Widget		w;
    DISPLAY_SB		*ptr;
    DwtAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In cb_inc_res - ptr=%d\n",ptr);

    /* increase the resolution by 10% */
    ptr->res = ptr->res*1.1;
    new_res (ptr, TRUE);
    }

void cb_dec_res(w,ptr,cb)
    Widget		w;
    DISPLAY_SB		*ptr;
    DwtAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In cb_dec_res - ptr=%d\n",ptr);

    /* decrease the resolution by 10% */
    ptr->res = ptr->res*0.9;
    new_res (ptr, TRUE);
    }

void cb_full_res(w,ptr,cb)
    Widget		w;
    DISPLAY_SB		*ptr;
    DwtAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In cb_full_res - ptr=%d\n",ptr);

    /*    printf("%d %d %d %d %d\n",
	   ptr->xstart, ptr->width,XMARGIN, ptr->end_time, ptr->start_time);	   */

    /* set resolution  */
    if (ptr->end_time != ptr->start_time) {
	ptr->res = ((float)(ptr->width - ptr->xstart)) /
	    ((float)(ptr->end_time - ptr->start_time));
	ptr->time = ptr->start_time;
	new_res (ptr, FALSE);
	new_time (ptr);
	}
    }

void cb_zoom_res(w,ptr,cb)
    Widget		w;
    DISPLAY_SB		*ptr;
    DwtAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In cb_zoom_res - ptr=%d\n",ptr);

    /* process all subsequent button presses as res_zoom clicks */
    ptr->click_time = -1;
    remove_all_events (ptr);
    XtAddEventHandler(ptr->work,ButtonPressMask,TRUE,res_zoom_click_ev,ptr);
    }

void res_zoom_click_ev(w, ptr, ev)
    Widget		w;
    DISPLAY_SB		*ptr;
    XButtonPressedEvent	*ev;
{
    int		time, tmp;

    if (DTPRINT) printf("In res_zoom_click1_ev - ptr=%d x=%d y=%d\n",ptr,ev->x,ev->y);

    /* check if button has been clicked on trace portion of screen */
    if ( ev->x < ptr->xstart || ev->x > ptr->width - XMARGIN )
	return;

    /* convert x value to a time value */
    time = (ev->x + ptr->time * ptr->res - ptr->xstart) / ptr->res;

    /* If no click time defined, define one and wait for second click */
    if ( ptr->click_time < 0) {
	ptr->click_time = time;
	return;
	}

    if (DTPRINT) printf ("click1 = %d, click2 = %d\n",ptr->click_time, time);

    /* Got 2 clicks, set res */
    if (time != ptr->click_time) {
	/* Swap so time is the max */
	if (time < ptr->click_time) {
	    tmp = time;
	    time = ptr->click_time;
	    ptr->click_time = tmp;
	    }
       
	/* Set new res & time */
	ptr->res = ((float)(ptr->width - ptr->xstart)) /
	    ((float)(time - ptr->click_time));
	
	ptr->time = ptr->click_time;

	new_res (ptr, FALSE);
	new_time(ptr);
	}

    /* remove handlers */
    remove_all_events (ptr);
    }
