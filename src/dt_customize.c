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


#include <sys/types.h>
#include <sys/stat.h>

#include <X11/Xlib.h>
#include <Xm/Xm.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/SelectioB.h>
#include <Xm/Text.h>
#include <Xm/Scale.h>
#include <Xm/Label.h>
#include <Xm/BulletinB.h>

#include "dinotrace.h"
#include "callbacks.h"



void cus_dialog_cb (w,trace,cb)
    Widget		w;
    TRACE	*trace;
    XmAnyCallbackStruct *cb;
{
    char		title[MAXFNAMELEN + 15];
    
    if (DTPRINT) printf ("In customize - trace=%d\n",trace);
    
    if (!trace->custom.customize)
	{
	if (trace->filename[0] == '\0')
	    sprintf (title,"Customize Window");
	else
	    sprintf (title,"Customize %s",trace->filename);
	
	XtSetArg (arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg (arglist[1], XmNdialogTitle, XmStringCreateSimple (title) );
	XtSetArg (arglist[2], XmNwidth, 600);
	XtSetArg (arglist[3], XmNheight, 400);
	trace->custom.customize = XmCreateBulletinBoardDialog (trace->work,"customize",arglist,4);
	
	/* Create label for page increment */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Page Inc/Dec"));
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNy, 5);
	trace->custom.page_label = XmCreateLabel (trace->custom.customize,"page_label",arglist,3);
	XtManageChild (trace->custom.page_label);
	
	/* Create radio box for page increment */
	XtSetArg (arglist[0], XmNx, 10);
	XtSetArg (arglist[1], XmNy, 35);
	XtSetArg (arglist[2], XmNspacing, 2);
	trace->custom.rpage = XmCreateRadioBox (trace->custom.customize,"rpage",arglist,3);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("1/4 Page"));
	trace->custom.tpage1 = XmCreateToggleButton (trace->custom.rpage,"rpage",arglist,1);
	XtManageChild (trace->custom.tpage1);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("1/2 Page"));
	trace->custom.tpage2 = XmCreateToggleButton (trace->custom.rpage,"tpage2",arglist,1);
	XtManageChild (trace->custom.tpage2);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple (" 1  Page"));
	trace->custom.tpage3 = XmCreateToggleButton (trace->custom.rpage,"tpage3",arglist,1);
	XtManageChild (trace->custom.tpage3);
	
	/* Create label for bus value */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Bus Repres."));
	XtSetArg (arglist[1], XmNx, 160);
	XtSetArg (arglist[2], XmNy, 5);
	trace->custom.bus_label = XmCreateLabel (trace->custom.customize,"buslabel",arglist,3);
	XtManageChild (trace->custom.bus_label);
	
	/* Create radio box for bus representation */
	XtSetArg (arglist[0], XmNx, 160);
	XtSetArg (arglist[1], XmNy, 35);
	XtSetArg (arglist[2], XmNspacing, 2);
	trace->custom.rbus = XmCreateRadioBox (trace->custom.customize,"rbus",arglist,3);
	
	/*
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Individual"));
	XtSetArg (arglist[1], XmNsensitive, FALSE);
	trace->custom.tbus1 = XmCreateToggleButton (trace->custom.rbus,"tbus1",arglist,2);
	XtManageChild (trace->custom.tbus1);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Binary"));
	XtSetArg (arglist[1], XmNsensitive, FALSE);
	trace->custom.tbus2 = XmCreateToggleButton (trace->custom.rbus,"tbus2",arglist,2);
	XtManageChild (trace->custom.tbus2);
	*/
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Octal"));
	trace->custom.tbus3 = XmCreateToggleButton (trace->custom.rbus,"tbus3",arglist,1);
	XtManageChild (trace->custom.tbus3);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Hexadecimal"));
	trace->custom.tbus4 = XmCreateToggleButton (trace->custom.rbus,"tbus4",arglist,1);
	XtManageChild (trace->custom.tbus4);
	
	/* Create label for time value */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Time Repres."));
	XtSetArg (arglist[1], XmNx, 300);
	XtSetArg (arglist[2], XmNy, 5);
	trace->custom.time_label = XmCreateLabel (trace->custom.customize,"timelabel",arglist,3);
	XtManageChild (trace->custom.time_label);
	
	/* Create radio box for time representation */
	XtSetArg (arglist[0], XmNx, 300);
	XtSetArg (arglist[1], XmNy, 35);
	XtSetArg (arglist[2], XmNspacing, 2);
	trace->custom.rtime = XmCreateRadioBox (trace->custom.customize,"rtime",arglist,3);
	XtManageChild (trace->custom.rtime);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Picoseconds"));
	trace->custom.ttimeps = XmCreateToggleButton (trace->custom.rtime,"ttimeps",arglist,1);
	XtManageChild (trace->custom.ttimeps);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Nanoseconds"));
	trace->custom.ttimens = XmCreateToggleButton (trace->custom.rtime,"ttimens",arglist,1);
	XtManageChild (trace->custom.ttimens);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Microseconds"));
	trace->custom.ttimeus = XmCreateToggleButton (trace->custom.rtime,"ttimeus",arglist,1);
	XtManageChild (trace->custom.ttimeus);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Grid Cycles"));
	trace->custom.ttimecyc = XmCreateToggleButton (trace->custom.rtime,"ttimecyc",arglist,1);
	XtManageChild (trace->custom.ttimecyc);
	
/*	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Decsim"));
	trace->custom.format_decsim = XmCreateToggleButton (trace->custom.format_radio,"fmtdec",arglist,1);
	XtManageChild (trace->custom.format_decsim);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Tempest CCLI"));
	trace->custom.format_tempest = XmCreateToggleButton (trace->custom.format_radio,"fmttmp",arglist,1);
	XtManageChild (trace->custom.format_tempest);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Verilog DMP"));
	trace->custom.format_verilog = XmCreateToggleButton (trace->custom.format_radio,"fmtvlg",arglist,1);
	XtManageChild (trace->custom.format_verilog);
*/	
	/* Create signal height slider */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Signal Height"));
	XtSetArg (arglist[1], XmNx, 180);
	XtSetArg (arglist[2], XmNy, 190);
	trace->custom.sighgt_label = XmCreateLabel (trace->custom.customize,"sighgtlabel",arglist,3);
	XtManageChild (trace->custom.sighgt_label);
	
	XtSetArg (arglist[0], XmNshowValue, 1);
	XtSetArg (arglist[1], XmNx, 180);
	XtSetArg (arglist[2], XmNy, 220);
	XtSetArg (arglist[3], XmNwidth, 100);
	XtSetArg (arglist[4], XmNminimum, 15);
	XtSetArg (arglist[5], XmNmaximum, 50);
	XtSetArg (arglist[6], XmNorientation, XmHORIZONTAL);
	XtSetArg (arglist[7], XmNprocessingDirection, XmMAX_ON_RIGHT);
	trace->custom.s1 = XmCreateScale (trace->custom.customize,"sighgt",arglist,8);
	XtManageChild (trace->custom.s1);
	
	/* Create click_to_edge button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Click To Edges"));
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNy, 150);
	XtSetArg (arglist[3], XmNshadowThickness, 1);
	trace->custom.click_to_edge = XmCreateToggleButton (trace->custom.customize,
							"click_to_edge",arglist,4);
	XtManageChild (trace->custom.click_to_edge);
	
	/* Create RF button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Rise/Fall Time"));
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNy, 185);
	XtSetArg (arglist[3], XmNshadowThickness, 1);
	trace->custom.rfwid = XmCreateToggleButton (trace->custom.customize,"rfwid",arglist,4);
	XtManageChild (trace->custom.rfwid);
	
	/* Create grid state on/off button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Grid On/Off"));
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNy, 220);
	XtSetArg (arglist[3], XmNshadowThickness, 1);
	trace->custom.grid_state = XmCreateToggleButton (trace->custom.customize,
							"grid_state",arglist,4);
	XtManageChild (trace->custom.grid_state);
	
	/* Create cursor state on/off button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Cursors On/Off"));
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNy, 255);
	XtSetArg (arglist[3], XmNshadowThickness, 1);
	trace->custom.cursor_state = XmCreateToggleButton (trace->custom.customize,
							   "cursor_state",arglist,4);
	XtManageChild (trace->custom.cursor_state);
	
	/* Create OK button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple (" OK ") );
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNy, 300);
	trace->custom.b1 = XmCreatePushButton (trace->custom.customize,"ok",arglist,3);
	XtAddCallback (trace->custom.b1, XmNactivateCallback, cus_ok_cb, trace);
	XtManageChild (trace->custom.b1);
	
	/* create apply button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Apply") );
	XtSetArg (arglist[1], XmNx, 70);
	XtSetArg (arglist[2], XmNy, 300);
	trace->custom.b2 = XmCreatePushButton (trace->custom.customize,"apply",arglist,3);
	XtAddCallback (trace->custom.b2, XmNactivateCallback, cus_apply_cb, trace);
	XtManageChild (trace->custom.b2);
	
	/* create cancel button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Cancel") );
	XtSetArg (arglist[1], XmNx, 140);
	XtSetArg (arglist[2], XmNy, 300);
	trace->custom.b3 = XmCreatePushButton (trace->custom.customize,"cancel",arglist,3);
	XtAddCallback (trace->custom.b3, XmNactivateCallback, unmanage_cb, trace->custom.customize);
	XtManageChild (trace->custom.b3);
	
	XtManageChild (trace->custom.rpage);
	XtManageChild (trace->custom.rbus);
	}

    /* Update with current custom values */
    XtSetArg (arglist[0], XmNset, (trace->pageinc==4));
    XtSetValues (trace->custom.tpage1,arglist,1);
    XtSetArg (arglist[0], XmNset, (trace->pageinc==2));
    XtSetValues (trace->custom.tpage2,arglist,1);
    XtSetArg (arglist[0], XmNset, (trace->pageinc==1));
    XtSetValues (trace->custom.tpage3,arglist,1);
    
    XtSetArg (arglist[0], XmNset, (trace->busrep==OBUS));
    XtSetValues (trace->custom.tbus3,arglist,1);
    XtSetArg (arglist[0], XmNset, (trace->busrep==HBUS));
    XtSetValues (trace->custom.tbus4,arglist,1);
    
    XtSetArg (arglist[0], XmNset, (trace->timerep==TIMEREP_NS));
    XtSetValues (trace->custom.ttimens,arglist,1);
    XtSetArg (arglist[0], XmNset, (trace->timerep==TIMEREP_PS));
    XtSetValues (trace->custom.ttimeps,arglist,1);
    XtSetArg (arglist[0], XmNset, (trace->timerep==TIMEREP_US));
    XtSetValues (trace->custom.ttimeus,arglist,1);
    XtSetArg (arglist[0], XmNset, (trace->timerep==TIMEREP_CYC));
    XtSetValues (trace->custom.ttimecyc,arglist,1);
    
    XtSetArg (arglist[0], XmNvalue, trace->sighgt);
    XtSetValues (trace->custom.s1,arglist,1);
    
    XtSetArg (arglist[0], XmNset, trace->sigrf ? 1:0);
    XtSetValues (trace->custom.rfwid,arglist,1);
    
    XtSetArg (arglist[0], XmNset, trace->grid_vis ? 1:0);
    XtSetValues (trace->custom.grid_state,arglist,1);
    
    XtSetArg (arglist[0], XmNset, trace->cursor_vis ? 1:0);
    XtSetValues (trace->custom.cursor_state,arglist,1);
    
    XtSetArg (arglist[0], XmNset, global->click_to_edge ? 1:0);
    XtSetValues (trace->custom.click_to_edge,arglist,1);
    
    /* Do it */
    XtManageChild (trace->custom.customize);
    }


void cus_read_cb (w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf ("in cus_read_cb trace=%d\n",trace);
    
    /* create popup to get filename */
    
    config_read_file (trace, "DINODISK:DINOTRACE.DINO", 0);
    
    /* Reformat and refresh */
    redraw_all (trace);
    }

void cus_reread_cb (w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf ("in cus_reread_cb trace=%d\n",trace);
    
    config_read_defaults (trace);
    
    /* Reformat and refresh */
    redraw_all (trace);
    }

void	cus_restore_cb (w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf ("in cus_restore_cb trace=%d\n",trace);
    
    /* do the default thing */
    config_restore_defaults (trace);
    
    /* redraw the display */
    redraw_all (trace);
    }

void	cus_ok_cb (w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf ("In cus_ok_cb - trace=%d\n",trace);
    
    XmScaleGetValue (trace->custom.s1, (int*) &(trace->sighgt));
    trace->grid_vis = XmToggleButtonGetState (trace->custom.grid_state);
    trace->cursor_vis = XmToggleButtonGetState (trace->custom.cursor_state);
    global->click_to_edge = XmToggleButtonGetState (trace->custom.click_to_edge);

    if (XmToggleButtonGetState (trace->custom.rfwid))
	trace->sigrf = SIG_RF;
    else trace->sigrf = 0;

    if (XmToggleButtonGetState (trace->custom.tbus3))
	trace->busrep = OBUS;
    else trace->busrep = HBUS;

    if (XmToggleButtonGetState (trace->custom.ttimecyc))
	trace->timerep = TIMEREP_CYC;
    else if (XmToggleButtonGetState (trace->custom.ttimeus))
	trace->timerep = TIMEREP_US;
    else if (XmToggleButtonGetState (trace->custom.ttimeps))
	trace->timerep = TIMEREP_PS;
    else trace->timerep = TIMEREP_NS;

    if (XmToggleButtonGetState (trace->custom.tpage1))
	trace->pageinc = QPAGE;
    if (XmToggleButtonGetState (trace->custom.tpage2))
	trace->pageinc = FPAGE;
    else trace->pageinc = HPAGE;
    
    /* hide the customize window */
    XtUnmanageChild (trace->custom.customize);
    
    /* res units may have changed, fix it & redraw the display */
    new_res (trace, TRUE);
    }

void	cus_apply_cb (w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf ("In cus_apply_cb - trace=%d\n",trace);
    
    /* Pretend an OK */
    cus_ok_cb (w,trace,cb);
    
    /* manage the customize window */
    XtManageChild (trace->custom.customize);
    }

