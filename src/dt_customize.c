#ident "$Id$"
/******************************************************************************
 * dt_customize.c --- customization requestor
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

#include <Xm/FileSB.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/SelectioB.h>
#include <Xm/Text.h>
#include <Xm/Scale.h>
#include <Xm/Label.h>
#include <Xm/BulletinB.h>

#include "functions.h"

/***********************************************************************/


void cus_dialog_cb (
    Widget	w,
    Trace	*trace,
    XmAnyCallbackStruct *cb)
{
    char		title[MAXFNAMELEN + 15];
    
    if (DTPRINT_ENTRY) printf ("In customize - trace=%p\n",trace);
    
    if (!trace->custom.customize) {
	if (trace->filename[0] == '\0')
	    sprintf (title,"Customize Window");
	else
	    sprintf (title,"Customize %s",trace->filename);
	
	XtSetArg (arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg (arglist[1], XmNdialogTitle, XmStringCreateSimple (title) );
	/* XtSetArg (arglist[2], XmNwidth, 600);
	   XtSetArg (arglist[3], XmNheight, 400); */
	trace->custom.customize = XmCreateBulletinBoardDialog (trace->work,"customize",arglist,2);
	
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
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Decimal"));
	trace->custom.tbus5 = XmCreateToggleButton (trace->custom.rbus,"tbus5",arglist,1);
	XtManageChild (trace->custom.tbus5);
	
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
	
	/* Create signal height slider */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Signal Height"));
	XtSetArg (arglist[1], XmNx, 180);
	XtSetArg (arglist[2], XmNy, 170);
	trace->custom.sighgt_label = XmCreateLabel (trace->custom.customize,"sighgtlabel",arglist,3);
	XtManageChild (trace->custom.sighgt_label);
	
	XtSetArg (arglist[0], XmNshowValue, 1);
	XtSetArg (arglist[1], XmNx, 180);
	XtSetArg (arglist[2], XmNy, 200);
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
	
	/* Create refresh button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Manual Refreshing"));
	XtSetArg (arglist[1], XmNx, 180);
	XtSetArg (arglist[2], XmNy, 255);
	XtSetArg (arglist[3], XmNshadowThickness, 1);
	trace->custom.refreshing = XmCreateToggleButton (trace->custom.customize,"refreshing",arglist,4);
	XtManageChild (trace->custom.refreshing);
	
	/* Create RF button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Rise/Fall Time"));
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNy, 185);
	XtSetArg (arglist[3], XmNshadowThickness, 1);
	trace->custom.rfwid = XmCreateToggleButton (trace->custom.customize,"rfwid",arglist,4);
	XtManageChild (trace->custom.rfwid);
	
	/* Create cursor state on/off button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Cursors On/Off"));
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNy, 220);
	XtSetArg (arglist[3], XmNshadowThickness, 1);
	trace->custom.cursor_state = XmCreateToggleButton (trace->custom.customize,
							   "cursor_state",arglist,4);
	XtManageChild (trace->custom.cursor_state);
	
	/* Create OK button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple (" OK ") );
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNy, 300);
	trace->custom.b1 = XmCreatePushButton (trace->custom.customize,"ok",arglist,3);
	DAddCallback (trace->custom.b1, XmNactivateCallback, cus_ok_cb, trace);
	XtManageChild (trace->custom.b1);
	
	/* create apply button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Apply") );
	XtSetArg (arglist[1], XmNx, 70);
	XtSetArg (arglist[2], XmNy, 300);
	trace->custom.b2 = XmCreatePushButton (trace->custom.customize,"apply",arglist,3);
	DAddCallback (trace->custom.b2, XmNactivateCallback, cus_apply_cb, trace);
	XtManageChild (trace->custom.b2);
	
	/* create cancel button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Cancel") );
	XtSetArg (arglist[1], XmNx, 140);
	XtSetArg (arglist[2], XmNy, 300);
	trace->custom.b3 = XmCreatePushButton (trace->custom.customize,"cancel",arglist,3);
	DAddCallback (trace->custom.b3, XmNactivateCallback, unmanage_cb, trace->custom.customize);
	XtManageChild (trace->custom.b3);
	
	XtManageChild (trace->custom.rpage);
	XtManageChild (trace->custom.rbus);
    }

    /* Update with current custom values */
    XtSetArg (arglist[0], XmNset, (global->pageinc==QPAGE));
    XtSetValues (trace->custom.tpage1,arglist,1);
    XtSetArg (arglist[0], XmNset, (global->pageinc==HPAGE));
    XtSetValues (trace->custom.tpage2,arglist,1);
    XtSetArg (arglist[0], XmNset, (global->pageinc==FPAGE));
    XtSetValues (trace->custom.tpage3,arglist,1);
    
    XtSetArg (arglist[0], XmNset, (trace->busrep==OBUS));
    XtSetValues (trace->custom.tbus3,arglist,1);
    XtSetArg (arglist[0], XmNset, (trace->busrep==HBUS));
    XtSetValues (trace->custom.tbus4,arglist,1);
    XtSetArg (arglist[0], XmNset, (trace->busrep==DBUS));
    XtSetValues (trace->custom.tbus5,arglist,1);
    
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
    
    XtSetArg (arglist[0], XmNset, trace->cursor_vis ? 1:0);
    XtSetValues (trace->custom.cursor_state,arglist,1);
    
    XtSetArg (arglist[0], XmNset, global->click_to_edge ? 1:0);
    XtSetValues (trace->custom.click_to_edge,arglist,1);
    
    XtSetArg (arglist[0], XmNset, global->redraw_manually ? 1:0);
    XtSetValues (trace->custom.refreshing,arglist,1);
    
    /* Do it */
    XtManageChild (trace->custom.customize);
}


void cus_reread_cb (
    Widget		w,
    Trace		*trace,
    XmAnyCallbackStruct	*cb)
{
    if (DTPRINT_ENTRY) printf ("in cus_reread_cb trace=%p\n",trace);
    
    config_read_defaults (trace, TRUE);
    
    /* Reformat and refresh */
    draw_all_needed ();
}

void	cus_restore_cb (
    Widget		w,
    Trace		*trace,
    XmAnyCallbackStruct	*cb)
{
    if (DTPRINT_ENTRY) printf ("in cus_restore_cb trace=%p\n",trace);
    
    /* do the default thing */
    config_trace_defaults (trace);
    config_global_defaults ();
    
    /* redraw the display */
    draw_all_needed ();
}

void	cus_ok_cb (
    Widget		w,
    Trace		*trace,
    XmAnyCallbackStruct	*cb)
{
    int hgt;

    if (DTPRINT_ENTRY) printf ("In cus_ok_cb - trace=%p\n",trace);
    
    XmScaleGetValue (trace->custom.s1, &hgt);
    trace->sighgt = hgt;
    trace->cursor_vis = XmToggleButtonGetState (trace->custom.cursor_state);
    global->click_to_edge = XmToggleButtonGetState (trace->custom.click_to_edge);
    global->redraw_manually = XmToggleButtonGetState (trace->custom.refreshing);

    if (XmToggleButtonGetState (trace->custom.rfwid))
	trace->sigrf = SIG_RF;
    else trace->sigrf = 0;

    if (XmToggleButtonGetState (trace->custom.tbus3))
	trace->busrep = OBUS;
    else if (XmToggleButtonGetState (trace->custom.tbus4))
	trace->busrep = HBUS;
    else trace->busrep = DBUS;

    if (XmToggleButtonGetState (trace->custom.ttimecyc))
	trace->timerep = TIMEREP_CYC;
    else if (XmToggleButtonGetState (trace->custom.ttimeus))
	trace->timerep = TIMEREP_US;
    else if (XmToggleButtonGetState (trace->custom.ttimeps))
	trace->timerep = TIMEREP_PS;
    else trace->timerep = TIMEREP_NS;

    if (XmToggleButtonGetState (trace->custom.tpage1))
	global->pageinc = QPAGE;
    else if (XmToggleButtonGetState (trace->custom.tpage2))
	global->pageinc = HPAGE;
    else global->pageinc = FPAGE;
    
    /* hide the customize window */
    XtUnmanageChild (trace->custom.customize);
    
    /* res units may have changed, fix it & redraw the display */
    new_res (trace, global->res);
}

void	cus_apply_cb (
    Widget		w,
    Trace		*trace,
    XmAnyCallbackStruct	*cb)
{
    if (DTPRINT_ENTRY) printf ("In cus_apply_cb - trace=%p\n",trace);
    
    /* Pretend an OK */
    cus_ok_cb (w,trace,cb);
    
    /* manage the customize window */
    XtManageChild (trace->custom.customize);
}

/****************************** File reading ******************************/

void cus_read_cb (
    Widget	w,
    Trace	*trace,
    XmFileSelectionBoxCallbackStruct *cb)
{
    int		cfg_num;
    
    if (DTPRINT_ENTRY) printf ("In cus_read_cb trace=%p\n",trace);
    
    if (!trace->cusread.dialog) {
	XtSetArg (arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg (arglist[1], XmNdialogTitle, XmStringCreateSimple ("Read Dinotrace File") );
	trace->cusread.dialog = XmCreateFileSelectionDialog ( trace->main, "file", arglist, 2);
	DAddCallback (trace->cusread.dialog, XmNokCallback, cus_read_ok_cb, trace);
	DAddCallback (trace->cusread.dialog, XmNcancelCallback, unmanage_cb, trace->cusread.dialog);
	XtUnmanageChild ( XmFileSelectionBoxGetChild (trace->cusread.dialog, XmDIALOG_HELP_BUTTON));
	
	XtSetArg (arglist[0], XmNhorizontalSpacing, 10);
	XtSetArg (arglist[1], XmNverticalSpacing, 7);
	trace->cusread.work_area = XmCreateForm (trace->cusread.dialog, "wa", arglist, 2);
	/*trace->cusread.form = XmCreateWorkArea (trace->cusread.work_, "wa", arglist, 0);*/

	/* Create label for this grid */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("When reading a trace, read which .dino files?"));
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNleftOffset, 25);
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_FORM );
	trace->cusread.config_label = XmCreateLabel (trace->cusread.work_area,"label",arglist,5);
	XtManageChild (trace->cusread.config_label);
	
	for (cfg_num=0; cfg_num<MAXCFGFILES; cfg_num++) {
	    /* enable button */
	    XtSetArg (arglist[0], XmNleftAttachment, XmATTACH_FORM );
	    XtSetArg (arglist[1], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET );
	    XtSetArg (arglist[2], XmNtopOffset, 5);
	    XtSetArg (arglist[3], XmNleftOffset, 15);
	    trace->cusread.config_enable[cfg_num] = XmCreateToggleButton (trace->cusread.work_area,"",arglist,4);
	    
	    /* file name */
	    XtSetArg (arglist[0], XmNrows, 1);
	    XtSetArg (arglist[1], XmNcolumns, 30);
	    XtSetArg (arglist[2], XmNleftAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[3], XmNleftWidget, trace->cusread.config_enable[cfg_num]);
	    XtSetArg (arglist[4], XmNtopAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[5], XmNtopWidget, 
		      (cfg_num>0)? trace->cusread.config_filename[cfg_num-1] : trace->cusread.config_label);
	    XtSetArg (arglist[6], XmNresizeHeight, FALSE);
	    XtSetArg (arglist[7], XmNeditMode, XmSINGLE_LINE_EDIT);
	    XtSetArg (arglist[8], XmNsensitive, (cfg_num<3));
	    trace->cusread.config_filename[cfg_num] = XmCreateText (trace->cusread.work_area,"textn",arglist,9);

	    /* Set button x*/
	    XtSetArg (arglist[0], XmNtopWidget, trace->cusread.config_filename[cfg_num]);
	    XtSetValues (trace->cusread.config_enable[cfg_num],arglist,1);

	    XtManageChild (trace->cusread.config_enable[cfg_num]);
	    XtManageChild (trace->cusread.config_filename[cfg_num]);
	}

	XtManageChild (trace->cusread.work_area);
	
	XSync (global->display,0);

	/* Set directory */
	XtSetArg (arglist[0], XmNdirectory, XmStringCreateSimple (global->directory) );
	XtSetValues (trace->cusread.dialog,arglist,1);
	fil_select_set_pattern (trace, trace->cusread.dialog, "*.dino");
    }
    
    config_update_filenames (trace);
    for (cfg_num=0; cfg_num<MAXCFGFILES; cfg_num++) {
	XmToggleButtonSetState (trace->cusread.config_enable[cfg_num], (global->config_enable[cfg_num]), TRUE);
	XmTextSetString (trace->cusread.config_filename[cfg_num], global->config_filename[cfg_num]);
    }

    XtManageChild (trace->cusread.dialog);

    XSync (global->display,0);
}

void cus_read_ok_cb (
    Widget	w,
    Trace	*trace,
    XmFileSelectionBoxCallbackStruct *cb)
{
    char	*tmp;
    char	filename[MAXFNAMELEN];
    int		cfg_num;
    
    if (DTPRINT_ENTRY) printf ("In cus_read_ok_cb trace=%p\n",trace);
    
    XtUnmanageChild (trace->cusread.dialog);
    XSync (global->display,0);
    
    for (cfg_num=0; cfg_num<MAXCFGFILES; cfg_num++) {
	global->config_enable[cfg_num] = XmToggleButtonGetState (trace->cusread.config_enable[cfg_num]);
	strcpy (global->config_filename[cfg_num], XmTextGetString (trace->cusread.config_filename[cfg_num]));
    }

    tmp = extract_first_xms_segment (cb->value);
    strcpy (filename, tmp);
    DFree (tmp);
    if (DTPRINT_FILE) printf ("In fil_ok_cb Filename=%s\n",trace->filename);

    config_read_file (trace, filename, TRUE, TRUE);

    /* Apply the statenames */
    grid_calc_autos (trace);
    draw_all_needed ();
}


