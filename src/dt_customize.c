#ident "$Id$"
/******************************************************************************
 * dt_customize.c --- customization requestor
 *
 * This file is part of Dinotrace.  
 *
 * Author: Wilson Snyder <wsnyder@world.std.com> or <wsnyder@iname.com>
 *
 * Code available from: http://www.veripool.com/dinotrace
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
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <Xm/SelectioB.h>
#include <Xm/Text.h>
#include <Xm/Scale.h>
#include <Xm/RowColumn.h>
#include <Xm/RowColumnP.h>
#include <Xm/Label.h>
#include <Xm/LabelP.h>
#include <Xm/Form.h>
#include <Xm/BulletinB.h>
#include <Xm/Separator.h>

#include "functions.h"

static char *cus_write_text[CUSWRITEM_MAX]
= { "Comment out commands",
    "Personal display preferences",
    "Value searches",
    "Cursors",
    "Grids",
    "File Format",
    "Signal highlights/radix/notes",
    "Signal ordering"};

/***********************************************************************/


void cus_dialog_cb (
    Widget	w,
    Trace_t	*trace,
    XmAnyCallbackStruct *cb)
{
    char		title[MAXFNAMELEN + 15];
    
    if (DTPRINT_ENTRY) printf ("In custom - trace=%p\n",trace);
    
    if (!trace->custom.dialog) {
	if (trace->dfile.filename[0] == '\0')
	    sprintf (title,"Customize Window");
	else
	    sprintf (title,"Customize %s",trace->dfile.filename);
	
	XtSetArg (arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg (arglist[1], XmNdialogTitle, XmStringCreateSimple (title) );
	XtSetArg (arglist[2], XmNverticalSpacing, 7);
	XtSetArg (arglist[3], XmNhorizontalSpacing, 10);
	trace->custom.dialog = XmCreateFormDialog (trace->work,"customize",arglist,4);
	
	/* Create radio box for page increment */
	trace->custom.page_menu = XmCreatePulldownMenu (trace->custom.dialog,"page",arglist,0);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple (" 1  Page"));
	trace->custom.page_item[0] = XmCreatePushButton (trace->custom.page_menu,"rpage",arglist,1);
	DManageChild (trace->custom.page_item[0], trace, MC_NOKEYS);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("1/2 Page"));
	trace->custom.page_item[1] = XmCreatePushButton (trace->custom.page_menu,"rpage",arglist,1);
	DManageChild (trace->custom.page_item[1], trace, MC_NOKEYS);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("1/4 Page"));
	trace->custom.page_item[2] = XmCreatePushButton (trace->custom.page_menu,"rpage",arglist,1);
	DManageChild (trace->custom.page_item[2], trace, MC_NOKEYS);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Page Increment") );
	XtSetArg (arglist[1], XmNsubMenuId, trace->custom.page_menu);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_FORM );
	XtSetArg (arglist[3], XmNleftAttachment, XmATTACH_FORM );
	trace->custom.page_option = XmCreateOptionMenu (trace->custom.dialog,"options",arglist,4);
	DManageChild (trace->custom.page_option, trace, MC_NOKEYS);

	/* Create radio box for time representation */
	trace->custom.time_menu = XmCreatePulldownMenu (trace->custom.dialog,"time",arglist,0);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Femtoseconds"));
	trace->custom.time_item[0] = XmCreatePushButton (trace->custom.time_menu,"rpage",arglist,1);
	DManageChild (trace->custom.time_item[0], trace, MC_NOKEYS);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Picoseconds"));
	trace->custom.time_item[1] = XmCreatePushButton (trace->custom.time_menu,"rpage",arglist,1);
	DManageChild (trace->custom.time_item[1], trace, MC_NOKEYS);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Nanoseconds"));
	trace->custom.time_item[2] = XmCreatePushButton (trace->custom.time_menu,"rpage",arglist,1);
	DManageChild (trace->custom.time_item[2], trace, MC_NOKEYS);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Microseconds"));
	trace->custom.time_item[3] = XmCreatePushButton (trace->custom.time_menu,"rpage",arglist,1);
	DManageChild (trace->custom.time_item[3], trace, MC_NOKEYS);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Grid #0 Cycles") );
	trace->custom.time_item[4] = XmCreatePushButton (trace->custom.time_menu,"rpage",arglist,1);
	DManageChild (trace->custom.time_item[4], trace, MC_NOKEYS);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Time represented in") );
	XtSetArg (arglist[1], XmNsubMenuId, trace->custom.time_menu);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopWidget, trace->custom.page_option );
	XtSetArg (arglist[4], XmNleftAttachment, XmATTACH_FORM );
	trace->custom.time_option = XmCreateOptionMenu (trace->custom.dialog,"options",arglist,4);
	DManageChild (trace->custom.time_option, trace, MC_NOKEYS);

	/* Create signal height slider */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Signal Height"));
	XtSetArg (arglist[1], XmNx, 150);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopWidget, trace->custom.time_option );
	trace->custom.sighgt_label = XmCreateLabel (trace->custom.dialog,"sighgtlabel",arglist,4);
	DManageChild (trace->custom.sighgt_label, trace, MC_NOKEYS);
	
	XtSetArg (arglist[0], XmNshowValue, 1);
	XtSetArg (arglist[1], XmNx, 180);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopWidget, trace->custom.sighgt_label );
	XtSetArg (arglist[4], XmNrightAttachment, XmATTACH_FORM );
	XtSetArg (arglist[5], XmNrightOffset, 10);
	XtSetArg (arglist[6], XmNminimum, 10);
	XtSetArg (arglist[7], XmNmaximum, 50);
	XtSetArg (arglist[8], XmNorientation, XmHORIZONTAL);
	XtSetArg (arglist[9], XmNprocessingDirection, XmMAX_ON_RIGHT);
	trace->custom.s1 = XmCreateScale (trace->custom.dialog,"sighgt",arglist,10);
	DManageChild (trace->custom.s1, trace, MC_NOKEYS);
	
	/* Create click_to_edge button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Click To Edges"));
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNshadowThickness, 1);
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNtopWidget, trace->custom.time_option );
	trace->custom.click_to_edge = XmCreateToggleButton (trace->custom.dialog,
							"click_to_edge",arglist,5);
	DManageChild (trace->custom.click_to_edge, trace, MC_NOKEYS);
	
	/* Create RF button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Rise/Fall Time"));
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNshadowThickness, 1);
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNtopWidget, dmanage_last );
	trace->custom.rfwid = XmCreateToggleButton (trace->custom.dialog,"rfwid",arglist,5);
	DManageChild (trace->custom.rfwid, trace, MC_NOKEYS);
	
	/* Create cursor state on/off button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Cursors On/Off"));
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNshadowThickness, 1);
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNtopWidget, dmanage_last );
	trace->custom.cursor_state = XmCreateToggleButton (trace->custom.dialog,
							   "cursor_state",arglist,5);
	DManageChild (trace->custom.cursor_state, trace, MC_NOKEYS);
	
	/* Create prefix button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Draw common signal prefix"));
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNshadowThickness, 1);
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNtopWidget, dmanage_last );
	trace->custom.prefixes = XmCreateToggleButton (trace->custom.dialog,"refreshing",arglist,5);
	DManageChild (trace->custom.prefixes, trace, MC_NOKEYS);
	
	/* Create refresh button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Manual Refreshing"));
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNshadowThickness, 1);
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNtopWidget, dmanage_last );
	trace->custom.refreshing = XmCreateToggleButton (trace->custom.dialog,"refreshing",arglist,5);
	DManageChild (trace->custom.refreshing, trace, MC_NOKEYS);
	
	/* Ok/apply/cancel */
	ok_apply_cancel (&trace->custom.okapply, trace->custom.dialog,
			 dmanage_last,
			 (XtCallbackProc)cus_ok_cb, trace,
			 (XtCallbackProc)cus_apply_cb, trace,
			 NULL, NULL,
			 (XtCallbackProc)unmanage_cb, (Trace_t*)trace->custom.dialog);
    }

    /* Update with current custom values */
    switch (global->pageinc) {
    case PAGEINC_FULL:
	XtSetArg (arglist[0], XmNmenuHistory, trace->custom.page_item[0]);
	break;
    case PAGEINC_HALF:
	XtSetArg (arglist[0], XmNmenuHistory, trace->custom.page_item[1]);
	break;
    case PAGEINC_QUARTER:
	XtSetArg (arglist[0], XmNmenuHistory, trace->custom.page_item[2]);
	break;
    }
    XtSetValues (trace->custom.page_option, arglist, 1);

    if (global->timerep == TIMEREP_PS)
	XtSetArg (arglist[0], XmNmenuHistory, trace->custom.time_item[0]);
    else if (global->timerep == TIMEREP_FS)
	XtSetArg (arglist[0], XmNmenuHistory, trace->custom.time_item[1]);
    else if (global->timerep == TIMEREP_NS)
	XtSetArg (arglist[0], XmNmenuHistory, trace->custom.time_item[2]);
    else if (global->timerep == TIMEREP_US)
	XtSetArg (arglist[0], XmNmenuHistory, trace->custom.time_item[3]);
    else
	XtSetArg (arglist[0], XmNmenuHistory, trace->custom.time_item[4]);
    XtSetValues (trace->custom.time_option, arglist, 1);

    XtSetArg (arglist[0], XmNvalue, global->sighgt);
    XtSetValues (trace->custom.s1,arglist,1);

    XmToggleButtonSetState (trace->custom.rfwid, global->sigrf, TRUE);
    XmToggleButtonSetState (trace->custom.cursor_state, global->cursor_vis, TRUE);
    XmToggleButtonSetState (trace->custom.click_to_edge, global->click_to_edge, TRUE);
    XmToggleButtonSetState (trace->custom.refreshing, global->redraw_manually, TRUE);
    XmToggleButtonSetState (trace->custom.refreshing, global->prefix_enable, TRUE);
    
    /* Do it */
    DManageChild (trace->custom.dialog, trace, MC_NOKEYS);
}


void cus_reread_cb (
    Widget		w,
    Trace_t		*trace,
    XmAnyCallbackStruct	*cb)
{
    if (DTPRINT_ENTRY) printf ("in cus_reread_cb trace=%p\n",trace);
    
    config_read_defaults (trace, FALSE);
    
    /* Reformat and refresh */
    draw_all_needed ();
}

void	cus_restore_cb (
    Widget		w,
    Trace_t		*trace,
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
    Trace_t		*trace,
    XmAnyCallbackStruct	*cb)
{
    int hgt;
    Widget	clicked;

    if (DTPRINT_ENTRY) printf ("In cus_ok_cb - trace=%p\n",trace);
    
    XmScaleGetValue (trace->custom.s1, &hgt);
    global->sighgt = hgt;
    global->cursor_vis = XmToggleButtonGetState (trace->custom.cursor_state);
    global->click_to_edge = XmToggleButtonGetState (trace->custom.click_to_edge);
    global->redraw_manually = XmToggleButtonGetState (trace->custom.refreshing);
    global->prefix_enable = XmToggleButtonGetState (trace->custom.prefixes);

    if (XmToggleButtonGetState (trace->custom.rfwid)) {
	if (!global->sigrf) global->sigrf = SIG_RF;
    }
    else global->sigrf = 0;

    XtSetArg (arglist[0], XmNmenuHistory, &clicked);
    XtGetValues (trace->custom.time_option, arglist, 1);
    if (clicked == trace->custom.time_item[4])
	global->timerep = TIMEREP_CYC;
    else if (clicked == trace->custom.time_item[3])
	global->timerep = TIMEREP_US;
    else if (clicked == trace->custom.time_item[2])
	global->timerep = TIMEREP_NS;
    else if (clicked == trace->custom.time_item[1])
	global->timerep = TIMEREP_PS;
    else global->timerep = TIMEREP_FS;

    XtSetArg (arglist[0], XmNmenuHistory, &clicked);
    XtGetValues (trace->custom.page_option, arglist, 1);
    if (clicked == trace->custom.page_item[2])
	global->pageinc = PAGEINC_QUARTER;
    else if (clicked == trace->custom.page_item[1])
	global->pageinc = PAGEINC_HALF;
    else global->pageinc = PAGEINC_FULL;
    
    /* hide the customize window */
    XtUnmanageChild (trace->custom.dialog);
    
    /* res units may have changed, fix it & redraw the display */
    new_res (trace, global->res);

    draw_needupd_sig_start ();	/* If prefix dropping changed */
    draw_all_needed ();
}

void	cus_apply_cb (
    Widget		w,
    Trace_t		*trace,
    XmAnyCallbackStruct	*cb)
{
    if (DTPRINT_ENTRY) printf ("In cus_apply_cb - trace=%p\n",trace);
    
    /* Pretend an OK */
    cus_ok_cb (w,trace,cb);
    
    /* manage the customize window */
    DManageChild (trace->custom.dialog, trace, MC_NOKEYS);
}

/****************************** File reading ******************************/

void cus_read_cb (
    Widget	w,
    Trace_t	*trace,
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
	trace->cusread.form = XmCreateForm (trace->cusread.dialog, "wa", arglist, 2);

	/* Create label for this grid */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("When reading a trace, read which .dino files?"));
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNleftOffset, 25);
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_FORM );
	trace->cusread.config_label = XmCreateLabel (trace->cusread.form,"label",arglist,5);
	DManageChild (trace->cusread.config_label, trace, MC_NOKEYS);
	
	for (cfg_num=0; cfg_num<MAXCFGFILES; cfg_num++) {
	    /* enable button */
	    XtSetArg (arglist[0], XmNleftAttachment, XmATTACH_FORM );
	    XtSetArg (arglist[1], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET );
	    XtSetArg (arglist[2], XmNtopOffset, 5);
	    XtSetArg (arglist[3], XmNleftOffset, 15);
	    trace->cusread.config_enable[cfg_num] = XmCreateToggleButton (trace->cusread.form,"",arglist,4);
	    
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
	    trace->cusread.config_filename[cfg_num] = XmCreateText (trace->cusread.form,"textn",arglist,9);

	    /* Set button x*/
	    XtSetArg (arglist[0], XmNtopWidget, trace->cusread.config_filename[cfg_num]);
	    XtSetValues (trace->cusread.config_enable[cfg_num],arglist,1);

	    DManageChild (trace->cusread.config_enable[cfg_num], trace, MC_NOKEYS);
	    DManageChild (trace->cusread.config_filename[cfg_num], trace, MC_NOKEYS);
	}

	DManageChild (trace->cusread.form, trace, MC_NOKEYS);
	
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

    DManageChild (trace->cusread.dialog, trace, MC_NOKEYS);

    XSync (global->display,0);
}

void cus_read_ok_cb (
    Widget	w,
    Trace_t	*trace,
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
    if (DTPRINT_FILE) printf ("In cus_read_ok_cb Filename=%s\n",trace->dfile.filename);

    config_read_file (trace, filename, TRUE, FALSE);

    /* Apply the statenames */
    grid_calc_autos (trace);
    draw_all_needed ();
}


/****************************** File writing ******************************/

void cus_write_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    int		item_num;
    
    if (DTPRINT_ENTRY) printf ("In cus_write_cb trace=%p\n",trace);
    
    if (!trace->cuswr.dialog) {
	XtSetArg (arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg (arglist[1], XmNdialogTitle, XmStringCreateSimple ("Write Dinotrace File") );
	trace->cuswr.dialog = XmCreateFileSelectionDialog ( trace->main, "file", arglist, 2);
	DAddCallback (trace->cuswr.dialog, XmNokCallback, cus_write_ok_cb, trace);
	DAddCallback (trace->cuswr.dialog, XmNcancelCallback, unmanage_cb, trace->cuswr.dialog);
	XtUnmanageChild ( XmFileSelectionBoxGetChild (trace->cuswr.dialog, XmDIALOG_HELP_BUTTON));

	XtSetArg (arglist[0], XmNhorizontalSpacing, 10);
	XtSetArg (arglist[1], XmNverticalSpacing, 0);
	trace->cuswr.form = XmCreateForm (trace->cuswr.dialog, "wa", arglist, 2);

        /* Comment out button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Comment out commands"));
        XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_FORM );
	XtSetArg (arglist[3], XmNtopOffset, 5);
	XtSetArg (arglist[4], XmNleftOffset, 5);
	trace->cuswr.item[CUSWRITEM_COMMENT] = XmCreateToggleButton (trace->cuswr.form,"",arglist,5);
	DManageChild (trace->cuswr.item[CUSWRITEM_COMMENT], trace, MC_NOKEYS);

	/* Write global: */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Write global:"));
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNleftOffset, 8);
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNtopWidget, dmanage_last );
	trace->cuswr.global_label = XmCreateLabel (trace->cuswr.form,"label",arglist,5);
	DManageChild (trace->cuswr.global_label, trace, MC_NOKEYS);

        /* Save personal preferences */
	for (item_num=CUSWRITEM_PERSONAL; item_num<=CUSWRITEM_CURSORS; item_num++) {
	   XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple (cus_write_text[item_num]));
	   XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	   XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	   XtSetArg (arglist[3], XmNtopWidget, dmanage_last );
	   XtSetArg (arglist[4], XmNtopOffset, -5);
	   XtSetArg (arglist[5], XmNleftOffset, 20);
	   trace->cuswr.item[item_num] = XmCreateToggleButton (trace->cuswr.form,"",arglist,6);
	   DManageChild (trace->cuswr.item[item_num], trace, MC_NOKEYS);
	}

	/* Begin pulldown */
	trace->cuswr.trace_pulldown = XmCreatePulldownMenu (trace->cuswr.form,"trace_pulldown",arglist,0);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("All Traces & Deleted") );
	trace->cuswr.trace_button[TRACESEL_ALLDEL] =
	  XmCreatePushButtonGadget (trace->cuswr.trace_pulldown,"pdbutton2",arglist,1);
	DManageChild (trace->cuswr.trace_button[TRACESEL_ALLDEL], trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("All Traces") );
	trace->cuswr.trace_button[TRACESEL_ALL] =
	    XmCreatePushButtonGadget (trace->cuswr.trace_pulldown,"pdbutton1",arglist,1);
	DManageChild (trace->cuswr.trace_button[TRACESEL_ALL], trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("This Trace") );
	trace->cuswr.trace_button[TRACESEL_THIS] =
	    XmCreatePushButtonGadget (trace->cuswr.trace_pulldown,"pdbutton0",arglist,1);
	DManageChild (trace->cuswr.trace_button[TRACESEL_THIS], trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Write for") );
	XtSetArg (arglist[1], XmNsubMenuId, trace->cuswr.trace_pulldown);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopWidget, trace->cuswr.item[CUSWRITEM_CURSORS] );
	XtSetArg (arglist[4], XmNleftAttachment, XmATTACH_FORM );
	trace->cuswr.trace_option = XmCreateOptionMenu (trace->cuswr.form,"options",arglist,5);
	DManageChild (trace->cuswr.trace_option, trace, MC_NOKEYS);

        /* Save trace preferences */
	for (item_num=CUSWRITEM_GRIDS; item_num<=CUSWRITEM_SIGORDER; item_num++) {
	   XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple (cus_write_text[item_num]));
	   XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	   XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	   XtSetArg (arglist[3], XmNtopWidget, dmanage_last );
	   XtSetArg (arglist[4], XmNtopOffset, -5);
	   XtSetArg (arglist[5], XmNleftOffset, 20);
	   trace->cuswr.item[item_num] = XmCreateToggleButton (trace->cuswr.form,"",arglist,6);
	   DManageChild (trace->cuswr.item[item_num], trace, MC_NOKEYS);
	}

	DManageChild (trace->cuswr.form, trace, MC_NOKEYS);
	
	XSync (global->display,0);

	/* Set directory */
	XtSetArg (arglist[0], XmNdirectory, XmStringCreateSimple (global->directory) );
	XtSetValues (trace->cuswr.dialog,arglist,1);
	fil_select_set_pattern (trace, trace->cuswr.dialog, "*.dino");
    }
    
    XtSetArg (arglist[0], XmNmenuHistory, trace->cuswr.trace_button[global->cuswr_traces]);
    XtSetValues (trace->cuswr.trace_option, arglist, 1);

    for (item_num=0; item_num<CUSWRITEM_MAX; item_num++) {
	XmToggleButtonSetState (trace->cuswr.item[item_num],
				(global->cuswr_item[item_num]), TRUE);
    }

    DManageChild (trace->cuswr.dialog, trace, MC_NOKEYS);

    XSync (global->display,0);
}

void cus_write_ok_cb (
    Widget	w,
    Trace_t	*trace,
    XmFileSelectionBoxCallbackStruct *cb)
{
    char	*tmp;
    int item_num, i;
    Widget	clicked;
    
    if (DTPRINT_ENTRY) printf ("In cus_write_ok_cb trace=%p\n",trace);
    
    XtUnmanageChild (trace->cuswr.dialog);
    
    tmp = extract_first_xms_segment (cb->value);
    DFree (global->cuswr_filename);
    global->cuswr_filename = strdup (tmp);
    DFree (tmp);

    for (item_num=0; item_num<CUSWRITEM_MAX; item_num++) {
        global->cuswr_item[item_num] = XmToggleButtonGetState (trace->cuswr.item[item_num]);
    }

    XtSetArg (arglist[0], XmNmenuHistory, &clicked);
    XtGetValues (trace->cuswr.trace_option, arglist, 1);
    for (i=0; i<3; i++) {
        if (clicked == trace->cuswr.trace_button[i]) {
	    global->cuswr_traces = i;
        }
    }

    config_write_file (trace, global->cuswr_filename);
}


