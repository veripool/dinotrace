/******************************************************************************
 *
 * Filename:
 *     dt_customize.c
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
 *     This module contains the callback routines that are the interface
 *     that allows the user to customize his trace display.
 *
 * Modification History:
 *     AAG	28-Jul-89	Original Version
 *     AAG	22-Aug-90	Base Level V4.1
 *     AAG	29-Apr-91	Use X11,fixed casts for Ultrix support
 *     WPS	17-Mar-93	Conversion to Motif
 *
 */


#include <X11/Xlib.h>
#include <Xm/Xm.h>
#include <Xm/RowColumn.h>

#include "dinotrace.h"
#include "callbacks.h"



void cus_dialog_cb(w,trace,cb)
    Widget		w;
    TRACE	*trace;
    XmAnyCallbackStruct *cb;
{
    char		title[100];
    
    if (DTPRINT) printf("In customize - trace=%d\n",trace);
    
    if (!trace->custom.customize)
	{
	if (trace->filename[0] == '\0')
	    sprintf(title,"Customize #%d",trace);
	else
	    sprintf(title,"Customize %s",trace->filename);
	
	XtSetArg(arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg(arglist[1], XmNdialogTitle, XmStringCreateSimple(title) );
	XtSetArg(arglist[2], XmNwidth, 500);
	XtSetArg(arglist[3], XmNheight, 400);
	trace->custom.customize = XmCreateBulletinBoardDialog(trace->work,"customize",arglist,4);
	
	/* Create radio box for page increment */
	XtSetArg(arglist[0], XmNx, 10);
	XtSetArg(arglist[1], XmNy, 25);
	XtSetArg(arglist[2], XmNspacing, 2);
	trace->custom.rpage = XmCreateRadioBox(trace->custom.customize,"rpage",arglist,3);
	
	/* Create label for page increment */
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("Page Inc/Dec"));
	XtSetArg(arglist[1], XmNx, 10);
	XtSetArg(arglist[2], XmNy, 5);
	trace->custom.page_label = XmCreateLabel(trace->custom.customize,"page_label",arglist,3);
	XtManageChild(trace->custom.page_label);
	
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("1/4 Page"));
	trace->custom.tpage1 = XmCreateToggleButton(trace->custom.rpage,"rpage",arglist,1);
	XtAddCallback(trace->custom.tpage1, XmNarmCallback, cus_page_cb, QPAGE);
	XtManageChild(trace->custom.tpage1);
	
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("1/2 Page"));
	trace->custom.tpage2 = XmCreateToggleButton(trace->custom.rpage,"tpage2",arglist,1);
	XtAddCallback(trace->custom.tpage2, XmNarmCallback, cus_page_cb, HPAGE);
	XtManageChild(trace->custom.tpage2);
	
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple(" 1  Page"));
	trace->custom.tpage3 = XmCreateToggleButton(trace->custom.rpage,"tpage3",arglist,1);
	XtAddCallback(trace->custom.tpage3, XmNarmCallback, cus_page_cb, FPAGE);
	XtManageChild(trace->custom.tpage3);
	
	/* Create radio box for bus representation */
	XtSetArg(arglist[0], XmNx, 170);
	XtSetArg(arglist[1], XmNy, 25);
	XtSetArg(arglist[2], XmNspacing, 2);
	trace->custom.rbus = XmCreateRadioBox(trace->custom.customize,"rbus",arglist,3);
	
	/* Create label for bus value */
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("Bus Repres."));
	XtSetArg(arglist[1], XmNx, 170);
	XtSetArg(arglist[2], XmNy, 5);
	trace->custom.bus_label = XmCreateLabel(trace->custom.customize,"buslabel",arglist,3);
	XtManageChild(trace->custom.bus_label);
	
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("INDIVIDUAL"));
	XtSetArg(arglist[1], XmNsensitive, FALSE);
	trace->custom.tbus1 = XmCreateToggleButton(trace->custom.rbus,"tbus1",arglist,2);
	XtAddCallback(trace->custom.tbus1, XmNarmCallback, cus_bus_cb, IBUS);
	XtManageChild(trace->custom.tbus1);
	
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("BINARY"));
	XtSetArg(arglist[1], XmNsensitive, FALSE);
	trace->custom.tbus2 = XmCreateToggleButton(trace->custom.rbus,"tbus2",arglist,2);
	XtAddCallback(trace->custom.tbus2, XmNarmCallback, cus_bus_cb, BBUS);
	XtManageChild(trace->custom.tbus2);
	
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("OCTAL"));
	trace->custom.tbus3 = XmCreateToggleButton(trace->custom.rbus,"tbus3",arglist,1);
	XtAddCallback(trace->custom.tbus3, XmNarmCallback, cus_bus_cb, OBUS);
	XtManageChild(trace->custom.tbus3);
	
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("HEXADECIMAL"));
	trace->custom.tbus4 = XmCreateToggleButton(trace->custom.rbus,"tbus4",arglist,1);
	XtAddCallback(trace->custom.tbus4, XmNarmCallback, cus_bus_cb, HBUS);
	XtManageChild(trace->custom.tbus4);
	
	/* Create signal height slider */
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("Signal Height"));
	XtSetArg(arglist[1], XmNx, 180);
	XtSetArg(arglist[2], XmNy, 200);
	trace->custom.sighgt_label = XmCreateLabel(trace->custom.customize,"sighgtlabel",arglist,3);
	XtManageChild(trace->custom.sighgt_label);
	
	XtSetArg(arglist[0], XmNshowValue, 1);
	XtSetArg(arglist[1], XmNx, 180);
	XtSetArg(arglist[2], XmNy, 230);
	XtSetArg(arglist[3], XmNwidth, 100);
	XtSetArg(arglist[4], XmNminimum, 15);
	XtSetArg(arglist[5], XmNmaximum, 50);
	XtSetArg(arglist[6], XmNorientation, XmHORIZONTAL);
	XtSetArg(arglist[7], XmNprocessingDirection, XmMAX_ON_RIGHT);
	trace->custom.s1 = XmCreateScale(trace->custom.customize,"sighgt",arglist,8);
	XtAddCallback(trace->custom.s1, XmNvalueChangedCallback, cus_sighgt_cb, trace);
	XtManageChild(trace->custom.s1);
	
	/* Create RF button */
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("Rise/Fall Time"));
	XtSetArg(arglist[1], XmNx, 10);
	XtSetArg(arglist[2], XmNy, 185);
	XtSetArg(arglist[3], XmNshadowThickness, 1);
	trace->custom.rfwid = XmCreateToggleButton(trace->custom.customize,"rfwid",arglist,4);
	XtAddCallback(trace->custom.rfwid, XmNvalueChangedCallback, cus_rf_cb, trace);
	XtManageChild(trace->custom.rfwid);
	
	/* Create grid state on/off button */
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("Grid On/Off"));
	XtSetArg(arglist[1], XmNx, 10);
	XtSetArg(arglist[2], XmNy, 220);
	XtSetArg(arglist[3], XmNshadowThickness, 1);
	trace->custom.grid_state = XmCreateToggleButton(trace->custom.customize,
							"grid_state",arglist,4);
	XtAddCallback(trace->custom.grid_state, XmNvalueChangedCallback, cus_grid_cb, trace);
	XtManageChild(trace->custom.grid_state);
	
	/* Create cursor state on/off button */
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("Cursors On/Off"));
	XtSetArg(arglist[1], XmNx, 10);
	XtSetArg(arglist[2], XmNy, 255);
	XtSetArg(arglist[3], XmNshadowThickness, 1);
	trace->custom.cursor_state = XmCreateToggleButton(trace->custom.customize,
							   "cursor_state",arglist,4);
	XtAddCallback(trace->custom.cursor_state, XmNvalueChangedCallback, cus_cur_cb, trace);
	XtManageChild(trace->custom.cursor_state);
	
	/* Create OK button */
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple(" OK ") );
	XtSetArg(arglist[1], XmNx, 10);
	XtSetArg(arglist[2], XmNy, 300);
	trace->custom.b1 = XmCreatePushButton(trace->custom.customize,"ok",arglist,3);
	XtAddCallback(trace->custom.b1, XmNactivateCallback, cus_ok_cb, trace);
	XtManageChild(trace->custom.b1);
	
	/* create apply button */
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("Apply") );
	XtSetArg(arglist[1], XmNx, 70);
	XtSetArg(arglist[2], XmNy, 300);
	trace->custom.b2 = XmCreatePushButton(trace->custom.customize,"apply",arglist,3);
	XtAddCallback(trace->custom.b2, XmNactivateCallback, cus_apply_cb, trace);
	XtManageChild(trace->custom.b2);
	
	/* create cancel button */
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("Cancel") );
	XtSetArg(arglist[1], XmNx, 140);
	XtSetArg(arglist[2], XmNy, 300);
	trace->custom.b3 = XmCreatePushButton(trace->custom.customize,"cancel",arglist,3);
	XtAddCallback(trace->custom.b3, XmNactivateCallback, cus_cancel_cb, trace);
	XtManageChild(trace->custom.b3);
	
	XtManageChild(trace->custom.rpage);
	XtManageChild(trace->custom.rbus);
	}
    
    /* Copy settings to local area to allow cancel to work */
    trace->custom.pageinc = trace->pageinc;
    trace->custom.busrep = trace->busrep;
    trace->custom.sighgt = trace->sighgt;
    trace->custom.sigrf = trace->sigrf;
    trace->custom.cursor_vis = trace->cursor_vis;
    trace->custom.grid_vis = trace->grid_vis;

    /* Update with current custom values */
    XtSetArg(arglist[0], XmNset, (trace->custom.pageinc==4));
    XtSetValues(trace->custom.tpage1,arglist,1);
    XtSetArg(arglist[0], XmNset, (trace->custom.pageinc==2));
    XtSetValues(trace->custom.tpage2,arglist,1);
    XtSetArg(arglist[0], XmNset, (trace->custom.pageinc==1));
    XtSetValues(trace->custom.tpage3,arglist,1);
    
    XtSetArg(arglist[0], XmNset, (trace->custom.busrep==OBUS));
    XtSetValues(trace->custom.tbus3,arglist,1);
    XtSetArg(arglist[0], XmNset, (trace->custom.busrep==HBUS));
    XtSetValues(trace->custom.tbus4,arglist,1);
    
    XtSetArg(arglist[0], XmNvalue, trace->custom.sighgt);
    XtSetValues(trace->custom.s1,arglist,1);
    
    XtSetArg(arglist[0], XmNset, trace->custom.sigrf ? 1:0);
    XtSetValues(trace->custom.rfwid,arglist,1);
    
    XtSetArg(arglist[0], XmNset, trace->custom.grid_vis ? 1:0);
    XtSetValues(trace->custom.grid_state,arglist,1);
    
    XtSetArg(arglist[0], XmNset, trace->custom.cursor_vis ? 1:0);
    XtSetValues(trace->custom.cursor_state,arglist,1);
    
    /* Do it */
    XtManageChild(trace->custom.customize);
    }


void cus_read_cb(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("in cus_read_cb trace=%d\n",trace);
    
    /* create popup to get filename */
    
    config_read_file (trace, "DINO$DISK:DINOTRACE.DINO", 0);
    
    /* Reformat and refresh */
    get_geometry(trace);
    XClearWindow(trace->display, trace->wind);
    draw(trace);
    }

void cus_reread_cb(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("in cus_reread_cb trace=%d\n",trace);
    
    config_read_defaults (trace);
    
    /* Reformat and refresh */
    get_geometry(trace);
    XClearWindow(trace->display, trace->wind);
    draw(trace);
    }

void	cus_restore_cb(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("in cus_restore_cb trace=%d\n",trace);
    
    config_restore_defaults (trace);
    
    /* do the default thing */
    
    /* redraw the display */
    get_geometry(trace);
    XClearWindow(trace->display,trace->wind);
    draw(trace);
    }

void	cus_save_cb(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("in cus_save_cb trace=%d\n",trace);
    }

void	cus_page_cb(w,tag,cb)
    Widget		w;
    int		tag;
    XmAnyCallbackStruct *cb;
{
    if (DTPRINT) printf("In cus_page_cb - tag=%d\n",tag);
    
    /* change the value of the page increment */
    curptr->custom.pageinc = tag;
    }

void	cus_bus_cb(w,tag,cb)
    Widget		w;
    int		tag;
    XmAnyCallbackStruct *cb;
{
    if (DTPRINT) printf("In cus_bus_cb - tag=%d\n",tag);
    
    /* change the value of the bus representation */
    curptr->custom.busrep = tag;
    }

void	cus_rf_cb(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmToggleButtonCallbackStruct *cb;
{
    if (DTPRINT) printf("In cus_rf_cb - trace=%d\n",trace);
    
    /* determine value of rise/fall parameter */
    if ( cb->set )
	trace->custom.sigrf = SIG_RF;
    else
	trace->custom.sigrf = 0;
    }

void	cus_grid_cb(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmToggleButtonCallbackStruct *cb;
{
    if (DTPRINT) printf("In cus_grid_cb - trace=%d\n",trace);
    
    /* copy value of toggle button to display structure */
    trace->custom.grid_vis = cb->set;
    }

void	cus_cur_cb(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmToggleButtonCallbackStruct *cb;
{
    if (DTPRINT) printf("In cus_cur_cb - trace=%d\n",trace);
    
    /* copy value of toggle button to display structure */
    trace->custom.cursor_vis = cb->set;
    }

void	cus_sighgt_cb(w,trace,cb)
    Widget		w;
    TRACE	*trace;
    XmScaleCallbackStruct *cb;
{
    if (DTPRINT) printf("In cus_sighgt_cb - trace=%d\n",trace);
    
    /* update sighgt value in the custom_ptr */
    trace->custom.sighgt =  (int)cb->value;
    }

void	cus_ok_cb(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In cus_ok_cb - trace=%d\n",trace);
    
    /* Copy settings from local area */
    trace->pageinc = trace->custom.pageinc;
    trace->busrep = trace->custom.busrep;
    trace->sighgt = trace->custom.sighgt;
    trace->sigrf = trace->custom.sigrf;
    trace->cursor_vis = trace->custom.cursor_vis;
    trace->grid_vis = trace->custom.grid_vis;

    /* hide the customize window */
    XtUnmanageChild(trace->custom.customize);
    
    /* redraw the display */
    get_geometry(trace);
    XClearWindow(trace->display,trace->wind);
    draw(trace);
    }

void	cus_apply_cb(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In cus_apply_cb - trace=%d\n",trace);
    
    /* Pretend an OK */
    cus_ok_cb (w,trace,cb);
    
    /* manage the customize window */
    XtManageChild(trace->custom.customize);
    }

void	cus_cancel_cb(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In cus_cancel_cb - trace=%d\n",trace);
    
    /* hide the window, don't copy the custom_ptr to the display */
    XtUnmanageChild(trace->custom.customize);
    }
