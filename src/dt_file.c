#ident "$Id$"
/******************************************************************************
 * dt_file.c --- trace file reading
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

#include "assert.h"
#include <Xm/Text.h>
#include <Xm/MessageB.h>
#include <Xm/SelectioB.h>
#include <Xm/FileSB.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <Xm/RowColumn.h>

#include "functions.h"

#if HAVE_DINODOC_H
#include "dinodoc.h"
#else
char dinodoc[] = "Unavailable";
#endif

/**********************************************************************/

int fil_line_num=0;

#define FIL_SIZE_INC	1024	/* Characters to increase length by */

/****************************** UTILITIES ******************************/

void free_data (
    /* Free trace information, also used when deleting preserved structure */
    Trace	*trace)
{
    Trace	*trace_ptr;

    if (DTPRINT_ENTRY) printf ("In free_data - trace=%p\n",trace);

    if (!trace || !trace->loaded) return;
    trace->loaded = 0;

    /* free any added signals in other traces from this trace */
    for (trace_ptr = global->deleted_trace_head; trace_ptr; trace_ptr = trace_ptr->next_trace) {
	sig_free (trace, trace_ptr->firstsig, TRUE, TRUE);
    }
    /* free signal data and each signal structure */
    sig_free (trace, trace->firstsig, FALSE, TRUE);
    trace->firstsig = NULL;
    trace->dispsig = NULL;
    trace->numsig = 0;
}

void trace_read_cb (
    Widget		w,
    Trace		*trace)
{
    if (DTPRINT_ENTRY) printf ("In trace_read_cb - trace=%p\n",trace);

    /* Clear the file format */
    trace->fileformat = FF_AUTO;

    /* get the filename */
    get_file_name (trace);
}

void trace_reread_all_cb (
    Widget		w,
    Trace		*trace)
{
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	if (trace->loaded) {
	    trace_reread_cb (w, trace);
	}
    }
}

void trace_reread_cb (
    Widget		w,
    Trace		*trace)
{
    char *semi;
    int		read_fd;
    struct stat		newstat;	/* New status on the reread file*/

    if (!trace->loaded)
	trace_read_cb (w,trace);
    else {
	/* Drop ;xxx */
	if ((semi = strchr (trace->filename,';')))
	    *semi = '\0';
	
	if (DTPRINT_ENTRY) printf ("In trace_reread_cb - rereading file=%s\n",trace->filename);
	
	/* check the date first */
	read_fd = open (trace->filename, O_RDONLY, 0);
	if (read_fd>=1) {
	    /* Opened ok */
	    fstat (read_fd, &newstat);
	    close (read_fd);
	    if ((newstat.st_mtime == trace->filestat.st_mtime)
		&& (newstat.st_ctime == trace->filestat.st_ctime)) {
		if (DTPRINT_FILE) printf ("  file has not changed.\n");
		return;
	    }
	}

	/* read the file */
	fil_read (trace);
    }
}

void fil_read (
    Trace	*trace)
{
    int		read_fd;
    FILE	*read_fp;	/* Routines are responsible for assigning this! */
    char	*pchar;
    char 	pipecmd[MAXFNAMELEN+20];
    pipecmd[0]='\0';	/* MIPS: no automatic aggregate initialization */
    read_fp = NULL;	/* MIPS: no automatic aggregate initialization */

    if (DTPRINT_ENTRY) printf ("In fil_read trace=%p filename=%s\n",trace,trace->filename);
    
    /* Update directory name */
    strcpy (global->directory, trace->filename);
    file_directory (global->directory);

    /* Clear the data structures & the screen */
    XClearWindow (global->display, trace->wind);
    set_cursor (trace, DC_BUSY);
    XSync (global->display,0);

    /* free memory associated with the data */
    sig_cross_preserve(trace);
    free_data (trace);

    /* get applicable config files */
    config_read_defaults (trace, TRUE);
    
    /* Compute the file format */
    if (trace->fileformat == FF_AUTO) {
	trace->fileformat =	file_format;
    }

    /* Normalize format */
    switch (trace->fileformat) {
    case	FF_AUTO:
    case	FF_DECSIM:
    case	FF_DECSIM_BIN:
#ifdef VMS
	trace->fileformat =	FF_DECSIM_BIN;
#else
	/* Binary relys on varible RMS records - Ultrix has no such thing */
	trace->fileformat =	FF_DECSIM_ASCII;
#endif
	break;
	/* No default */
    }

    /* Open file and copy descriptor information */
    read_fd = open (trace->filename, O_RDONLY, 0);
    if (read_fd>0) {
	fstat (read_fd, &(trace->filestat));
#ifndef VMS
	if (! S_ISREG(trace->filestat.st_mode)) {
	    /* Not regular file */
	    close (read_fd);
	    read_fd = -1;
	}
#endif
    }
    if (read_fd<1) {
	/* Similar code below! */
	sprintf (message,"Can't open file %s", trace->filename);
	dino_error_ack (trace, message);

	/* Clear cursor and return*/
	sig_cross_restore (trace);
	change_title (trace);
	set_cursor (trace, DC_NORMAL);
	return;
    }

#ifndef VMS
    /* If compressed, close the file and open as uncompressed */
    pipecmd[0]='\0';
    if ((pchar=strrchr(trace->filename,'.')) != NULL ) {

	if (!strcmp (pchar, ".Z")) sprintf (pipecmd, "uncompress -c %s", trace->filename);
	if (!strcmp (pchar, ".gz")) sprintf (pipecmd, "gunzip -c %s", trace->filename);
	
	/* Decsim must be ASCII because of record format */
	if (trace->fileformat == FF_DECSIM_BIN) trace->fileformat = FF_DECSIM_ASCII;

	if (pipecmd[0]) {

	    /* Close compressed file and open uncompressed file */
	    close (read_fd);

	    read_fp = popen (pipecmd, "r");
	    read_fd = fileno (read_fp);
	    if (!read_fp) {
		/* Similar above! */
		sprintf (message,"Can't create pipe with command '%s'", pipecmd);
		dino_error_ack (trace, message);

		/* Clear cursor and return */
		sig_cross_restore (trace);
		change_title (trace);
		set_cursor (trace, DC_NORMAL);
		return;
	    }
	}
    }
#endif

    /*
     ** Read in the trace file using the format selected by the user
     */
    switch (trace->fileformat) {
      case	FF_DECSIM_BIN:
#ifdef VMS
	decsim_read_binary (trace, read_fd);
#endif /* VMS */
	break;
      case	FF_TEMPEST:
	tempest_read (trace, read_fd);
	break;
      case	FF_VERILOG:
	verilog_read (trace, read_fd);
	break;
      case	FF_DECSIM_ASCII:
	decsim_read_ascii (trace, read_fd, read_fp);
	break;
      default:
	fprintf (stderr, "Unknown file format!!\n");
    }

    /* Close the file */
    if (pipecmd[0] == '\0') {
	close (read_fd);
    }
#ifndef VMS
    else {
	fflush (read_fp);
	pclose (read_fp);
    }
#endif

    /* Now add EOT to each signal and reset the cptr */
    fil_trace_end (trace);

    /* Change the name on title bar to filename */
    change_title (trace);
    
    /*
     ** Clear the window and draw the screen with the new file
     */
    set_cursor (trace, DC_NORMAL);
    if (global->res_default) win_full_res (trace);
    new_time (trace);		/* Realignes start and displays */
    vscroll_new (trace,0);	/* Realign time */
    if (DTPRINT_ENTRY) printf ("fil_read done!\n");
}

void  fil_select_set_pattern (
    Trace	*trace,
    Widget	dialog,
    char	*pattern)
    /* Set the file requester pattern information (called in 2 places) */
{
    char mask[MAXFNAMELEN], *dirname;
    XmString	xs_dirname;

    /* Grab directory information */
    XtSetArg (arglist[0], XmNdirectory, &xs_dirname);
    XtGetValues (dialog, arglist, 1);
    dirname = extract_first_xms_segment (xs_dirname);

    /* Pattern information */
    strcpy (mask, dirname);
    strcat (mask, pattern);
    XtSetArg (arglist[0], XmNdirMask, XmStringCreateSimple (mask) );
    XtSetArg (arglist[1], XmNpattern, XmStringCreateSimple (pattern) );
    XtSetValues (dialog,arglist,2);
}


void  get_file_name (
    Trace	*trace)
{
    int		i;
    
    if (DTPRINT_ENTRY) printf ("In get_file_name trace=%p\n",trace);
    
    if (!trace->fileselect.dialog) {
	XtSetArg (arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg (arglist[1], XmNdialogTitle, XmStringCreateSimple ("Open Trace File") );

	trace->fileselect.dialog = XmCreateFileSelectionDialog ( trace->main, "file", arglist, 2);
	DAddCallback (trace->fileselect.dialog, XmNokCallback, fil_ok_cb, trace);
	DAddCallback (trace->fileselect.dialog, XmNcancelCallback, (XtCallbackProc)unmanage_cb, trace->fileselect.dialog);
	XtUnmanageChild ( XmFileSelectionBoxGetChild (trace->fileselect.dialog, XmDIALOG_HELP_BUTTON));
	
	trace->fileselect.work_area = XmCreateWorkArea (trace->fileselect.dialog, "wa", arglist, 0);

	/* Create FILE FORMAT */
	trace->fileselect.format_menu = XmCreatePulldownMenu (trace->fileselect.work_area,"fmtrad",arglist,0);

	for (i=0; i<FF_NUMFORMATS; i++) {
	    if (filetypes[i].selection) {
		XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple (filetypes[i].name) );
		trace->fileselect.format_item[i] =
		    XmCreatePushButtonGadget (trace->fileselect.format_menu,"pdbutton",arglist,1);
		DManageChild (trace->fileselect.format_item[i], trace, MC_NOKEYS);
		DAddCallback (trace->fileselect.format_item[i], XmNactivateCallback, fil_format_option_cb, trace);
	    }
	}

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("File Format"));
	XtSetArg (arglist[1], XmNsubMenuId, trace->fileselect.format_menu);
	trace->fileselect.format_option = XmCreateOptionMenu (trace->fileselect.work_area,"format",arglist,2);
	DManageChild (trace->fileselect.format_option, trace, MC_NOKEYS);

	/* Create save_ordering button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Preserve Signal Ordering"));
	XtSetArg (arglist[1], XmNx, 0);
	XtSetArg (arglist[2], XmNy, 100);
	XtSetArg (arglist[3], XmNshadowThickness, 1);
	trace->fileselect.save_ordering = XmCreateToggleButton (trace->fileselect.work_area,"save_ordering",arglist,4);
	DManageChild (trace->fileselect.save_ordering, trace, MC_NOKEYS);
	
	DManageChild (trace->fileselect.work_area, trace, MC_NOKEYS);
	
	XSync (global->display,0);
    }
    
    DManageChild (trace->fileselect.dialog, trace, MC_NOKEYS);

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

    fil_select_set_pattern (trace, trace->fileselect.dialog, filetypes[file_format].mask);

    XSync (global->display,0);
}

void    fil_format_option_cb (
    Widget	w,
    Trace	*trace,
    XmSelectionBoxCallbackStruct *cb)
{
    int 	i;
    if (DTPRINT_ENTRY) printf ("In fil_format_option_cb trace=%p\n",trace);

    for (i=0; i<FF_NUMFORMATS; i++) {
	if (w == trace->fileselect.format_item[i]) {
	    file_format = i;	/* Change global verion */
	    trace->fileformat = i;	/* Specifically make this file use this format */
	    fil_select_set_pattern (trace, trace->fileselect.dialog, filetypes[file_format].mask);
	}
    }
}

void fil_ok_cb (
    Widget	w,
    Trace	*trace,
    XmFileSelectionBoxCallbackStruct *cb)
{
    char	*tmp;
    
    if (DTPRINT_ENTRY) printf ("In fil_ok_cb trace=%p\n",trace);
    
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
    fil_read (trace);
}


void help_cb (
    Widget		w,
    Trace		*trace,
    XmAnyCallbackStruct	*cb)
{
    if (DTPRINT_ENTRY) printf ("in help_cb\n");

    dino_information_ack (trace, help_message ());
}

void help_trace_cb (
    Widget		w,
    Trace		*trace,
    XmAnyCallbackStruct	*cb)
{
    static char msg[2000];
    static char msg2[1000];

    if (DTPRINT_ENTRY) printf ("in help_trace_cb\n");
    
    if (!trace->loaded) {
	sprintf (msg, "No trace is loaded.\n");
    }
    else {
	sprintf (msg,
	     "%s\n\
\n", 	 trace->filename);

	sprintf (msg2, "File Format: %s\n", filetypes[trace->fileformat].name);
	strcat (msg, msg2);

	sprintf (msg2, "File Modified Date: %s\n", date_string (trace->filestat.st_ctime));
	strcat (msg, msg2);

	sprintf (msg2, "File Creation Date: %s\n", date_string (trace->filestat.st_mtime));
	strcat (msg, msg2);

	sprintf (msg2, "\nTimes stored to nearest: %s\n", time_units_to_string (global->time_precision, TRUE));
	strcat (msg, msg2);
    }

    dino_information_ack (trace, msg);
}


void help_doc_cb (
    Widget		w,
    Trace		*trace,
    XmAnyCallbackStruct	*cb)
{
    if (DTPRINT_ENTRY) printf ("in help_doc_cb\n");

    /* May be called before the window was opened, if so ignore the help_doc */
    if (!trace->work) return;

    /* create the widget if it hasn't already been */
    if (!trace->help_doc) {
	XtSetArg (arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg (arglist[1], XmNdialogTitle, XmStringCreateSimple ("Dinotrace Documentation") );
	trace->help_doc = XmCreateInformationDialog (trace->work, "info", arglist, 2);
	DAddCallback (trace->help_doc, XmNokCallback, unmanage_cb, trace->help_doc);
	XtUnmanageChild ( XmMessageBoxGetChild (trace->help_doc, XmDIALOG_CANCEL_BUTTON));
	XtUnmanageChild ( XmMessageBoxGetChild (trace->help_doc, XmDIALOG_HELP_BUTTON));

	/* create the file name text widget */
	XtSetArg (arglist[0], XmNrows, 40);
	XtSetArg (arglist[1], XmNcolumns, 80);
	XtSetArg (arglist[2], XmNeditable, FALSE);
	XtSetArg (arglist[3], XmNvalue, dinodoc);
	XtSetArg (arglist[4], XmNscrollVertical, TRUE);
	XtSetArg (arglist[5], XmNeditMode, XmMULTI_LINE_EDIT);
	XtSetArg (arglist[6], XmNscrollHorizontal, FALSE);
	trace->help_doc_text = XmCreateScrolledText (trace->help_doc,"textn",arglist,7);

	DManageChild (trace->help_doc_text, trace, MC_NOKEYS);
    }

    /* manage the widget */
    DManageChild (trace->help_doc, trace, MC_NOKEYS);
}

#pragma inline (fil_string_add_cptr)
void	fil_string_add_cptr (
    /* Add a cptr corresponding to the text at value_strg */
    /* WARNING: Similar verilog_string_to_value in dt_verilog for speed reasons */
    Signal	*sig_ptr,
    const char	*value_strg,
    DTime	time,
    Boolean_t	nocheck)		/* don't compare against previous data */
{
    register uint_t state;
    register const char	*cp;
    register char 	chr_or, chr_and;
    register int	len, bitcnt;
    Value_t	value;

    /* zero the value */
    /* We do NOT need to set value.siglw.number, as is set later */
    /* I set it anyways to get it into the cache */
    val_zero (&value);

    if (sig_ptr->bits == 0) {
	/* This branch not used for verilog, only Decsim */
	/* scalar signal */  
	switch ( *value_strg ) {
	case '0': state = STATE_0; break;
	case '1': state = STATE_1; break;
	case '?':
	case 'X':
	case 'x':
	case 'U':
	case 'u': state = STATE_U; break;
	case 'Z': 
	case 'z': state = STATE_Z; break;
	default: 
	    state = STATE_U;
	    printf ("%%E, Unknown state character '%c' at line %d\n", *value_strg, fil_line_num);
	}
    }

    else {
	/* Multi bit signal */
	/* determine the state of the bus */
	/* Ok, here comes a fast algorithm (5% faster file reads)
	 * Take each letter in the trace as a binary vector
	 *	?   = 0011 1111	state_u
	 *	x/X = 01x1 1000	state_u
	 *	u/U = 01x1 0101	state_u
	 *	z/Z = 01x1 1010	state_z
	 *	0/1 = 0011 000x	state_0
	 *
	 * If we OR together every character, we will get:
	 *	    01x1 1010	if everything is z
	 *	    0011 000x	if everything is 0 or 1
	 *	    else	it's a U
	 */
	chr_or = chr_and = *value_strg;
	len = sig_ptr->bits;
	for (cp = value_strg; cp <= (value_strg + len); cp++) {
	    chr_or |= *cp; chr_and &= *cp;
	}
	switch (chr_or) {
	case '0':
	    /* Got all zeros, else LSB would have gotten set */
	    state = STATE_0;
	    break;
	case '1':
	    if (chr_and == '1') {
		/* All ones */
		value.siglw.stbits.allhigh = 1;
	    }
	    /* Mix of 0's and 1's, or all 1's */
	    /* compute the value */
	    /* Note '0'->0 and '1'->1 by just ANDing with 1 */
	    state = sig_ptr->type;
	    cp = value_strg;
	    for (bitcnt=96; bitcnt <= (MIN (127,len)); bitcnt++, cp++)
		value.number[3] = (value.number[3]<<1) | (*cp=='1');
	    for (bitcnt=64; bitcnt <= (MIN (95,len)); bitcnt++, cp++)
		value.number[2] = (value.number[2]<<1) | (*cp=='1');
	    for (bitcnt=32; bitcnt <= (MIN (63,len)); bitcnt++, cp++)
		value.number[1] = (value.number[1]<<1) | (*cp=='1');
	    for (bitcnt=0; bitcnt <= (MIN (31,len)); bitcnt++, cp++)
		value.number[0] = (value.number[0]<<1) | (*cp=='1');
	    break;
	case 'z':
	case 'Z':
	    state = STATE_Z;
	    break;

	case 'u':
	case 'U':
	    state = STATE_U;	/* Pure U */
	    break;

	default: /* 4-state mix */
	    cp = value_strg;
	    if (sig_ptr->bits <= 31 ) {
		state = STATE_F32;
		for (bitcnt=0; bitcnt <= (MIN (31,len)); bitcnt++, cp++) {
		    value.number[0] = (value.number[0]<<1) | (*cp=='1') | (*cp=='z') | (*cp=='Z');
		    value.number[1] = (value.number[1]<<1) | (((*cp)|1)!='1');
		    cp++;
		}
	    } else {
		state = STATE_F128;
		for (bitcnt=96; bitcnt <= (MIN (127,len)); bitcnt++, cp++) {
		    value.number[3] = (value.number[3]<<1) | (*cp=='1') | (*cp=='z') | (*cp=='Z');
		    value.number[7] = (value.number[7]<<1) | (((*cp)|1)!='1');
		}
		for (bitcnt=64; bitcnt <= (MIN (95,len)); bitcnt++, cp++) {
		    value.number[2] = (value.number[2]<<1) | (*cp=='1') | (*cp=='z') | (*cp=='Z');
		    value.number[6] = (value.number[6]<<1) | (((*cp)|1)!='1');
		}
		for (bitcnt=32; bitcnt <= (MIN (63,len)); bitcnt++, cp++) {
		    value.number[1] = (value.number[1]<<1) | (*cp=='1') | (*cp=='z') | (*cp=='Z');
		    value.number[5] = (value.number[5]<<1) | (((*cp)|1)!='1');
		}
		for (bitcnt=0; bitcnt <= (MIN (31,len)); bitcnt++, cp++) {
		    value.number[0] = (value.number[0]<<1) | (*cp=='1') | (*cp=='z') | (*cp=='Z');
		    value.number[4] = (value.number[4]<<1) | (((*cp)|1)!='1');
		}
	    }
	    break;
	}
    }

    /*if (DTPRINT_FILE) printf ("time %d sig %s str %s cor %c cand %c state %d n0 %d n1 %d\n",				time, sig_ptr->signame, value_strg, chr_or, chr_and, state, value.number[0], value.number[1]); */
    value.siglw.stbits.state = state;
    value.time = time;
    fil_add_cptr (sig_ptr, &value, nocheck);
}

#if !defined (fil_add_cptr)
#pragma inline (fil_add_cptr)
/* WARNING, INLINED CODE IN CALLBACKS.H */
void	fil_add_cptr (
    Signal	*sig_ptr,
    Value_t	*value_ptr,
    Boolean_t	nocheck)		/* compare against previous data */
{
    ulong_t	diff;
    Value_t	*cptr = sig_ptr->cptr;

    /* Important! During the adding cptr points to the __last__ value added, not the next! */

    /*
    if (DTPRINT_FILE) printf ("Checking st %d v %d tm %d with st %d v %d time %d\n",
	    value_ptr->siglw.stbits.state, value_ptr->number[0],
	    CPTR_TIME(value_ptr),
	    cptr->siglw.stbits.state,      cptr->number[0],
	    CPTR_TIME(cptr)
	    );
    if (DTPRINT_FILE) printf ("val: %08x %08x %08x %08x %08x %08x    ",
	    value_ptr->siglw.number, value_ptr->time,
	    value_ptr->number[0],value_ptr->number[1],
	    value_ptr->number[2],value_ptr->number[3]);
    */

    /* Comparing all 4 LW's works because we keep the unused LWs zeroed */
    if ( nocheck
	|| ( cptr->siglw.stbits.state != value_ptr->siglw.stbits.state )
	|| ( cptr->number[0] != value_ptr->number[0] )
	|| ( cptr->number[1] != value_ptr->number[1] )
	|| ( cptr->number[2] != value_ptr->number[2] )
	|| ( cptr->number[3] != value_ptr->number[3] )
	|| ( cptr->number[4] != value_ptr->number[4] )
	|| ( cptr->number[5] != value_ptr->number[5] )
	|| ( cptr->number[6] != value_ptr->number[6] )
	|| ( cptr->number[7] != value_ptr->number[7] ) ) {

	diff = (uint_t*)sig_ptr->cptr - (uint_t*)sig_ptr->bptr;
	if (diff > sig_ptr->blocks ) {
	    /* if (DTPRINT_FILE) printf ("Realloc\n"); */
	    sig_ptr->blocks += BLK_SIZE;
	    sig_ptr->bptr = (Value_t *)XtRealloc
		((char*)sig_ptr->bptr,
		 (sig_ptr->blocks*sizeof(uint_t)) + (sizeof(Value_t)*2 + 2));
	    sig_ptr->cptr = (Value_t *)((uint_t*)sig_ptr->bptr + diff);
	}
       
	/* Update size of previous */
	/* Note that if at the beginning of a trace, the loaded value may */
	/* be from uninitialized data, in that case we don't care about the size */
	value_ptr->siglw.stbits.size_prev = sig_ptr->cptr->siglw.stbits.size;
	value_ptr->siglw.stbits.size = STATE_SIZE(value_ptr->siglw.stbits.state);
	if (sig_ptr->cptr->siglw.number || sig_ptr->cptr!=sig_ptr->bptr) {
	    /* Not empty cptr list... have loaded something */
	    sig_ptr->cptr = CPTR_NEXT(sig_ptr->cptr);
	}

	/* Load new datum */
	val_copy (sig_ptr->cptr, value_ptr);
	/*if (DTPRINT_FILE) print_sig_info(sig_ptr);*/
    }

}
#endif

void fil_make_busses (
    /* Take the list of signals and make it into a list of busses */
    /* Also do the common stuff required for each signal. */
    Trace	*trace,
    Boolean_t	not_tempest)	/* Use the name of the bus to find the bit vectors */
{
    Signal	*sig_ptr;	/* ptr to current signal (lower bit number) */
    Signal	*bus_sig_ptr;	/* ptr to signal which is being bussed (upper bit number) */
    char	*bbeg;		/* bus beginning */
    char	*sep;		/* separator position */
    int		pos;
    char	sepchar;
    char	postbusstuff[MAXSIGLEN];

    if (DTPRINT_ENTRY) printf ("In fil_make_busses\n");
    if (DTPRINT_BUSSES) sig_print_names (trace);

    /* Convert the signal names to the internal format */
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	/* Drop leading spaces */
	for (bbeg=sig_ptr->signame; isspace (*bbeg); bbeg++);
	strcpy (sig_ptr->signame, bbeg);	/* side effects */

	/* Drop trailing spaces */
	for (bbeg = sig_ptr->signame + strlen (sig_ptr->signame) - 1;
	     (bbeg >= sig_ptr->signame) && isspace (*bbeg);
	     bbeg--)
	    *bbeg = '\0';

	/* Use the separator character to split signals into vector and base */
	/* IE "signal_1" becomes "signal" with index=1 if the separator is _ */
	sepchar = trace->vector_separator;
	if (sepchar == '\0') sepchar='<';	/* Allow both '\0' and '<' for DANGER::DORMITZER */
	sep = strrchr (sig_ptr->signame, sepchar);
	bbeg = sep+1;
	if (!sep || !isdigit (*bbeg)) {
	    if (trace->vector_separator == '\0') {
		/* Allow numbers at end to be stripped off as the vector bit */
		for (sep = sig_ptr->signame + strlen (sig_ptr->signame) - 1;
		     (sep > sig_ptr->signame) && isdigit (*sep);
		     sep --) ;
		if (sep) sep++;
		bbeg = sep;
	    }
	}
	
	/* Extract the bit subscripts from the name of the signal */
	sig_ptr->signame_buspos = 0;	/* Presume nothing after the vector */
	if (sep && isdigit (*(bbeg))) {
	    /* Is a named bus, with <subscript> */
	    if (not_tempest) {
		sig_ptr->msb_index = atoi (bbeg);
		sig_ptr->lsb_index = sig_ptr->msb_index - sig_ptr->bits;
	    }
	    else {
		sig_ptr->msb_index = atoi (bbeg) + sig_ptr->bit_index - sig_ptr->bits + 1;
		sig_ptr->lsb_index = sig_ptr->msb_index;
		sig_ptr->bits = 0;
	    }
	    /* Don't need to search for :'s, as the bits should already be computed */
	    /* Mark this first digit, _, whatever as null (truncate the name) */
	    *sep++ = '\0';
	    /* Hunt for the end of the vector */
	    while (*sep && *sep!=trace->vector_endseparator) sep++;
	    /* Remember if there is stuff after the vector */
	    if (*sep && *(sep+1)) sig_ptr->signame_buspos = sep+1;
	}
	else {
	    if (sig_ptr->bits) {
		/* Is a unnamed bus */
		if (not_tempest) {
		    sig_ptr->msb_index = sig_ptr->bits;
		    sig_ptr->lsb_index = 0;
		}
		else {
		    sig_ptr->msb_index = sig_ptr->bit_index;
		    sig_ptr->lsb_index = sig_ptr->bit_index;
		    sig_ptr->bits = 0;
		}
	    }
	    else {
		/* Is a unnamed single signals */
		sig_ptr->msb_index = -1;
		sig_ptr->lsb_index = -1;
	    }
	}
    }

    /* Remove signals with NULL names */
    /* Remove huge memories */
    for (sig_ptr = trace->firstsig; sig_ptr;) {
	bus_sig_ptr = sig_ptr;
	if ((sig_ptr->signame[0]=='\0')
	    || (sig_ptr->bits > 2048)) {
	    /* Null, remove this signal */
	    printf ("Remove %s %d\n", sig_ptr->signame, sig_ptr->bits);
	    sig_free (trace, sig_ptr, FALSE, FALSE);
	}
	sig_ptr = bus_sig_ptr->forward;
    }
    
    
    if (trace->fileformat == FF_VERILOG) {
	/* Verilog may have busses > 128 bits, other formats should have one record per
	   bit, so it shouldn't matter.  Make consistent sometime in the future */
	verilog_womp_128s (trace);
    }

    /* Vectorize signals */
    bus_sig_ptr = NULL;
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	/*
	if (DTPRINT_BUSSES && bus_sig_ptr) printf ("  '%s'%d/%d/%d\t'%s'%d/%d/%d\n",
					    bus_sig_ptr->signame, bus_sig_ptr->msb_index,
					    bus_sig_ptr->bits, bus_sig_ptr->file_pos, 
					    sig_ptr->signame, sig_ptr->msb_index, 
					    sig_ptr->bits, sig_ptr->file_pos);
					    */
	/* Start on 2nd signal, see if can merge with previous signal */
	if (bus_sig_ptr) {
	    /* Combine signals with same base name, if */
	    /*  are a vector */
	    if (sig_ptr->msb_index >= 0
		/* Signal name */
		&& !strcmp (sig_ptr->signame, bus_sig_ptr->signame)
		/* Signal name following bus separator */
		&& (sig_ptr->signame_buspos==NULL || !strcmp (sig_ptr->signame_buspos, bus_sig_ptr->signame_buspos))
		/*  & the result would have < 128 bits */
		&& (ABS(bus_sig_ptr->msb_index - sig_ptr->lsb_index) < 128 )
		/* 	& have subscripts that are different by 1  (<20:10> and <9:5> are seperated by 10-9=1) */
		&& ( ((bus_sig_ptr->msb_index >= sig_ptr->lsb_index)
		      && ((bus_sig_ptr->lsb_index - 1) == sig_ptr->msb_index))
		    || ((bus_sig_ptr->msb_index <= sig_ptr->lsb_index)
			&& ((bus_sig_ptr->lsb_index + 1) == sig_ptr->msb_index)))
		/*	& are placed next to each other in the source */
		/*	& not a tempest trace (because the bit ordering is backwards, <31:0> would look line 0:31 */
		&& ((trace->fileformat != FF_TEMPEST)
		    || (((bus_sig_ptr->file_pos) == (sig_ptr->file_pos + sig_ptr->bits + 1))
			&& trace->vector_separator=='<'))
		&& (trace->fileformat != FF_VERILOG
		    || ((bus_sig_ptr->file_pos + bus_sig_ptr->bits + 1) == sig_ptr->file_pos))
		&& ! (sig_ptr->file_type.flag.vector_msb)
		/*	& not (verilog trace which had a signal already as a vector) */
		&& ! (sig_ptr->file_type.flag.perm_vector || bus_sig_ptr->file_type.flag.perm_vector)) {

		/* Can be bussed with previous signal */
		bus_sig_ptr->bits += sig_ptr->bits + 1;
		bus_sig_ptr->lsb_index = sig_ptr->lsb_index;
		if (trace->fileformat == FF_TEMPEST) bus_sig_ptr->file_pos = sig_ptr->file_pos;

		/* Delete this signal */
		sig_free (trace, sig_ptr, FALSE, FALSE);
		sig_ptr = bus_sig_ptr;
	    }
	}
	bus_sig_ptr = sig_ptr;
    }
    
    /* Calculate numsig */
    /* and allocate Signal's cptr, bptr, blocks, inc, type */
    trace->numsig = 0;
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	/* Stash the characters after the bus name */
	if (sig_ptr->signame_buspos) strcpy (postbusstuff, sig_ptr->signame_buspos);

	/* Change the name to include the vector subscripts */
	if (sig_ptr->msb_index >= 0) {
	    /* Add new vector info */
	    if (sig_ptr->bits >= 1) {
		sprintf (sig_ptr->signame + strlen (sig_ptr->signame), "[%d:%d]",
			 sig_ptr->msb_index, sig_ptr->lsb_index);
	    }
	    else {
		sprintf (sig_ptr->signame + strlen (sig_ptr->signame), "[%d]",
			 sig_ptr->msb_index);
	    }
	}

	/* Restore the characters after the bus name */
	if (sig_ptr->signame_buspos) strcat (sig_ptr->signame, postbusstuff);

	/* Calc numsig, last_sig_ptr */
	(trace->numsig) ++;

	/* Create the bussed name & type */
	if (sig_ptr->bits < 1) {
	    sig_ptr->type = 0;
	}
	else if (sig_ptr->bits < 32) {
	    sig_ptr->type = STATE_B32;
	}
	else if (sig_ptr->bits < 128) {
	    sig_ptr->type = STATE_B128;
	}

	/* Compute value_mask.  This mask ANDed with the value will clear any bits that
	   are not represented in this vector.  For example a 12 bit vector will only have the lower
	   twelve bits set, all others will be clear */

	sig_ptr->value_mask[3] = sig_ptr->value_mask[2] = sig_ptr->value_mask[1] = sig_ptr->value_mask[0] = 0;

	if (sig_ptr->bits >= 127) sig_ptr->value_mask[3] = 0xFFFFFFFF;
	else if (sig_ptr->bits >= 96 ) sig_ptr->value_mask[3] = (0x7FFFFFFF >> (126-sig_ptr->bits));
	else if (sig_ptr->bits >= 95 ) sig_ptr->value_mask[2] = 0xFFFFFFFF;
	else if (sig_ptr->bits >= 64 ) sig_ptr->value_mask[2] = (0x7FFFFFFF >> (94-sig_ptr->bits));
	else if (sig_ptr->bits >= 63 ) sig_ptr->value_mask[1] = 0xFFFFFFFF;
	else if (sig_ptr->bits >= 32 ) sig_ptr->value_mask[1] = (0x7FFFFFFF >> (62-sig_ptr->bits));
	else if (sig_ptr->bits >= 31 ) sig_ptr->value_mask[0] = 0xFFFFFFFF;
	else sig_ptr->value_mask[0] = (0x7FFFFFFF >> (30-sig_ptr->bits));

	if (sig_ptr->value_mask[3]) sig_ptr->value_mask[2] = 0xFFFFFFFF;
	if (sig_ptr->value_mask[2]) sig_ptr->value_mask[1] = 0xFFFFFFFF;
	if (sig_ptr->value_mask[1]) sig_ptr->value_mask[0] = 0xFFFFFFFF;

	/* Compute the position mask.  This is used to convert a LW from the file into a LW for the
	   value.  A condition is to prevent sign extension.  */
	pos = sig_ptr->file_pos & 0x1F;		/* Must be position of LSB */
	if (pos!=0) sig_ptr->pos_mask = (0x7FFFFFFF >> (pos-1));
	else	     sig_ptr->pos_mask = 0xFFFFFFFF;

	/* Compute the ending position of this data (DECSIM_Binary) */
	sig_ptr->file_end_pos = sig_ptr->file_pos
	    + ((sig_ptr->file_type.flag.four_state) ? 2:1) * sig_ptr->bits;
	
	/* allocate the data storage memory */
	sig_ptr->blocks = BLK_SIZE;
	sig_ptr->bptr = (Value_t *)XtMalloc ((sig_ptr->blocks*sizeof(uint_t))
						+ (sizeof(Value_t)*2 + 2));
	val_zero (sig_ptr->bptr);	/* So we know is empty */
	sig_ptr->cptr = sig_ptr->bptr;
    }

    if (DTPRINT_BUSSES) sig_print_names (trace);
}


static void fil_mark_cptr_end (
    Trace	*trace)
{
    Signal	*sig_ptr;
    Value_t	value;
    Boolean_t	msg=FALSE;

    /* loop thru each signal */
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {

	/* Must be a cptr which has the same time as the last time on the screen */
	/* If none exists, create it */
	if (sig_ptr->cptr->siglw.number || sig_ptr->cptr!=sig_ptr->bptr) {
	    Value_t	*cptr = sig_ptr->cptr;
	    if (CPTR_TIME(cptr) != trace->end_time) {
		val_copy (&value, sig_ptr->cptr);
		value.time = trace->end_time;
		fil_add_cptr (sig_ptr, &value, TRUE);
	    }
	}
	else {
	    if (DTDEBUG && !msg) {
		printf ("%%W, No data for signal %s\n\tAdditional messages suppressed\n", sig_ptr->signame);
		msg = TRUE;
	    }
	}

	/* Mark end of time */
	value.time = EOT;
	fil_add_cptr (sig_ptr, &value, TRUE);

	/* re-initialize the cptr's to the bptr's */
	sig_ptr->cptr = sig_ptr->bptr;

	assert (sig_ptr->bptr->siglw.stbits.size!=0);
    }
}

void fil_trace_end (
    /* Perform stuff at end of trace - common across all reading routines */
    Trace	*trace)
{
    Signal	*sig_ptr;

    if (DTPRINT_FILE) printf ("In fil_trace_end\n");

    /* Modify ending cptrs to be correct */
    fil_mark_cptr_end (trace);

    /* Misc */
    trace->dispsig = trace->firstsig;

    /* Make sure time is within bounds */
    if ( (global->time < trace->start_time) || (global->time > trace->end_time)) {
	global->time = trace->start_time;
    }

    /* Mark as loaded */
    trace->loaded = TRUE;

    switch (trace->fileformat) {
    case	FF_TEMPEST:
    case	FF_DECSIM_ASCII:
    case	FF_DECSIM_BIN:
	sig_modify_enables (trace);
	break;
    case	FF_VERILOG:
    default:
	break;
    }

    /* Create xstring of the name (to avoid calling again and again) */
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	sig_ptr->xsigname = XmStringCreateSimple (sig_ptr->signame);
    }

    /* Preserve file information */
    sig_cross_restore (trace);

    /* Read .dino file stuff yet again to get signal_heighlights */
    /* Don't report errors, as they would pop up for a second time. */
    config_read_defaults (trace, FALSE);

    /* Apply the statenames */
    draw_needupd_val_states ();
    draw_needupd_val_search ();
    draw_needupd_sig_search ();
    draw_needupd_sig_start ();
    grid_calc_autos (trace);

    if (DTPRINT_FILE) printf ("fil_trace_end: Done\n");
}

/****************************** DECSIM ASCII ******************************/

void decsim_read_ascii_header (
    Trace	*trace,
    char	*header_start,
    char	*data_begin_ptr,
    int		sig_start_pos,
    int		sig_end_pos,
    int		header_lines)
{
    int		line,col;
    Signal	*sig_ptr,*last_sig_ptr=NULL;
    char	*line_ptr, *tmp_ptr;
    char	*signame_array;
    Boolean_t	hit_name_block, past_name_block, no_names;

#define	SIGLINE(_l_) (signame_array+(_l_)*(sig_end_pos+5))

    /* make array:  [header_lines+5][sig_end_pos+5]; */
    signame_array = XtMalloc ((header_lines+5)*(sig_end_pos+5));

    /* Make a signal structure for each signal */
    trace->firstsig = NULL;
    for ( col=sig_start_pos; col < sig_end_pos; col++) {
	sig_ptr = DNewCalloc (Signal);

	/* initialize all the pointers that aren't NULL */
	if (trace->firstsig==NULL) {
	    trace->firstsig = sig_ptr;
	    sig_ptr->backward = NULL;
	}
	else {
	    last_sig_ptr->forward = sig_ptr;
	    sig_ptr->backward = last_sig_ptr;
	}

	/* allow extra space in case becomes vector - don't know size yet */
	sig_ptr->signame = (char *)XtMalloc (20+header_lines);
	sig_ptr->trace = trace;
	sig_ptr->base = global->bases[0];
	sig_ptr->file_type.flags = 0;
	sig_ptr->file_pos = col;
	sig_ptr->bits = 0;	/* = buf->TRA$W_BITLEN; */

	last_sig_ptr = sig_ptr;
    }

    /* Save where lines begin */
    line=0;
    for (line_ptr = header_start; line_ptr<data_begin_ptr; line_ptr++) {
	strncpy (SIGLINE(line), line_ptr, sig_end_pos);
	SIGLINE(line)[sig_end_pos]='\0';
	if ((tmp_ptr = strchr (SIGLINE(line), '\n')) != 0 ) {
	    *tmp_ptr = '\0';
	}
	while ( line_ptr<data_begin_ptr && *line_ptr!='\n') line_ptr++;
	line++;
    }
    header_lines = line;

    /* Chop out lines that are beginning comments by decsim */
    hit_name_block = past_name_block = FALSE;
    for (line=header_lines-1; line>=0; line--) {
	if (past_name_block) {
	    SIGLINE(line)[0] = '\0';
	}
	else {
	    if (SIGLINE(line)[0] == '!') {
		hit_name_block = TRUE;
	    }
	    else {
		past_name_block = hit_name_block;
	    }
	}
    }

    /* Special exemption, "! MakeDinoHeader\n!\n!....header stuff" */
    if (!strncmp (SIGLINE(0), "! MakeDinoHeader", 16)) {
	SIGLINE(0)[0]='\0';
    }

    /* Read Signal Names Into Structure */
    for (line=0; line<header_lines; line++) {
	/*printf ("%d):	'%s'\n", line, SIGLINE(line));*/
	/* Extend short lines with spaces */
	col=strlen(SIGLINE(line));
	if (SIGLINE(line)[0]!='!') col=0;
	for ( ; col<sig_end_pos; col++) SIGLINE(line)[col]=' '; 
	/* Load signal into name array */
	for (col=sig_start_pos, sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward, col++) {
	    sig_ptr->signame[line] = SIGLINE(line)[col];
	}
    }

    /* Add EOS delimiter to each signal name */
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	sig_ptr->signame[header_lines] = '\0';
	/*printf ("Sig '%s'\n", sig_ptr->signame);*/
    }

    /* See if no header at all */
    no_names=TRUE;
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	for (tmp_ptr=sig_ptr->signame; *tmp_ptr; tmp_ptr++)
	    if (!isspace (*tmp_ptr)) {
		no_names=FALSE;
		break;
	    }
	if (*tmp_ptr) break;
    }
    if (no_names) {
	for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	    sprintf (sig_ptr->signame, "SIG %d", sig_ptr->file_pos);
	}
    }

    XtFree (signame_array);
}

void decsim_read_ascii (
    Trace	*trace,
    int		read_fd,
    FILE	*decsim_z_readfp)	/* Only pre-set if FF_DECSIM_Z */
{
    FILE	*readfp;
    Boolean_t	first_data;
    char	*line_in;
    Signal	*sig_ptr;
    long	time_stamp;
    char	*pchar;
    DTime	time_divisor;
    int		ch;

    char	*header_start, *header_ptr;
    long	header_length, header_lines=0;
    Boolean_t	got_start=FALSE, hit_start=FALSE, in_comment=FALSE;
    long	base_chars_left=0;

    char	*t;
    Boolean_t	chango_format=FALSE;
    long	sig_start_pos, sig_end_pos;
    char	*data_begin_ptr;

    time_divisor = time_units_to_multiplier (global->time_precision);

    if (!decsim_z_readfp) {
	/* Make file into descriptor */
	readfp = fdopen (read_fd, "r");
	if (!readfp) {
	    return;
	}
	rewind (readfp);	/* as binary may have used some of the chars */
    }
    else {
	readfp = decsim_z_readfp;
    }

    /* Make the buffer */
    header_length = FIL_SIZE_INC - 4; 	/* -4 for saftey room */
    header_start = (char *)XtMalloc (FIL_SIZE_INC);
    header_ptr = header_start;

    /* Read characters */
    while (! got_start && (EOF != (ch = getc (readfp)))) {
	switch (ch) {
	case '\n':
	case '\r':
	    in_comment = FALSE;
	    got_start = hit_start;
	    header_lines ++;
	    break;
	case '!':
	    in_comment = TRUE;
	    break;
	case ' ':
	case '\t':
	    break;
	case '#':
	    if (!in_comment) base_chars_left = 2;
	    break;
	default:	/* starting digit */
	    if (!in_comment) {
		hit_start = TRUE;
	    }
	    break;
	}
	if (!base_chars_left) {
	    *header_ptr++ = ch;
	}
	else {
	    base_chars_left--;
	}
	/* Get more space if needed */
	if (header_ptr - header_start > header_length) {
	    long header_size;

	    header_length += FIL_SIZE_INC;
	    header_size = header_ptr - header_start;
	    header_start = (char *)XtRealloc (header_start, header_length);
	    header_ptr = header_size + header_start;
	}
    }

    if ((header_ptr > header_start) && (*(header_ptr-1)=='\n')) header_ptr--;
    *header_ptr = '\0';
    if (DTPRINT_FILE) printf ("Header = '%s'\n", header_start);

    /***** Find number of signals and where they begin and end */
    /* Find beginning of this line (start before null and newline) */
    t=header_ptr;
    if (t >= (header_start+1)) t -= 1;
    if (t >= (header_start+1)) t -= 1;
    for (t=header_ptr; t>header_start; t--) {
	if (*t=='\n') {
	    break;
	}
    }
    if (*t && t!=header_start) t++;
    data_begin_ptr = t;

    /* Skip beginning spaces */
    while (*t && isspace(*t)) t++;
    sig_start_pos = t - data_begin_ptr;

    /* Skip timestamp or data (don't know which yet) */
    /* Note that a '?' isn't alnum, but need it for WEDOIT::MDBELKIN's traces */
    while (*t && (isalnum(*t) || *t=='?')) t++;
    sig_end_pos = t - data_begin_ptr;

    /* Skip more spaces */
    while (*t && isspace(*t)) t++;
    if (isalnum (*t) || *t=='?') {
	/* Found second digit, as in the 1 in: " 0000 1" */
	/* So, this must not be chango format, as it has a timestamp */
	chango_format = FALSE;
	sig_start_pos = t - data_begin_ptr;

	/* Find real ending of the signals */
	while (*t && (isalnum(*t) || *t=='?')) t++;
	sig_end_pos = t - data_begin_ptr;
    }
    else {
	chango_format = TRUE;
    }

    if (DTPRINT_FILE) printf ("Line:%s\nHeader has %ld lines, Signals run from char %ld to %ld. %s format\n", 
			      data_begin_ptr, header_lines, sig_start_pos, sig_end_pos,
			      chango_format?"Chango":"DECSIM");

    if (sig_start_pos == sig_end_pos) {
	dino_error_ack (trace,"No data in trace file");
	XtFree (header_start);
	return;
    }

    /***** READ SIGNALS DATA ****/
    decsim_read_ascii_header (trace,
			      header_start, data_begin_ptr, sig_start_pos, sig_end_pos,
			      header_lines);

    /***** SET UP FOR READING DATA ****/

    /* Make the busses */
    /* Any lines that were all spaces will be pulled off.  This will strip off the time section of the lines */
    fil_make_busses (trace, TRUE);

    /* Loop to read trace data and reformat */
    first_data = TRUE;
    time_stamp = -100;	/* so increment makes it 0 */
    while (1) {
	if (data_begin_ptr) {
	    /* We already got one line, use it. */
	    line_in = data_begin_ptr;
	    fil_line_num = header_lines;
	    data_begin_ptr = NULL;
	}
	else {
	    if (feof (readfp)) break;
	    line_in = header_start;
	    fgets (line_in, header_length, readfp);
	    fil_line_num++;
	}

	/* Remove leading hash mark (old fil_remove_hash) */
	while ( (pchar=strchr (line_in,'#')) != 0 ) {
	    *pchar = '\0';
	    strcat (line_in, pchar+2);
	}

	if ( line_in[0] && (line_in[0] != '!') && (line_in[0] != '\n') ) {
	    if (chango_format) {
		time_stamp += 100;
	    }
	    else {
		/* Be careful of round off error making us overflow the time */
		time_stamp = (atof (line_in) * 1000.0) / ((float)time_divisor);	/* ASCII traces are always in NS */
		if (time_stamp<0) {
		    printf ("%%E, Time underflow!\n");
		}
	    }
	    
	    if (first_data) {
		trace->start_time = time_stamp;
	    }
	    trace->end_time = time_stamp;

	    /* save information on each signal */
	    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
		fil_string_add_cptr (sig_ptr, line_in + sig_ptr->file_pos, time_stamp, first_data);
	    }

	    first_data = FALSE;
	}
    }

    XtFree (header_start);

    /* Mark format as may have presumed binary */
    trace->fileformat = FF_DECSIM_ASCII;
    if (DTPRINT_FILE) printf ("decsim_read_ascii done\n");
}

