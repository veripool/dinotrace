#ident "$Id$"
/******************************************************************************
 * DESCRIPTION: Dinotrace source: trace file reading
 *
 * This file is part of Dinotrace.  
 *
 * Author: Wilson Snyder <wsnyder@wsnyder.org> or <wsnyder@iname.com>
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

/****************************** UTILITIES ******************************/

void free_data (
    /* Free trace information, also used when deleting preserved structure */
    Trace_t	*trace)
{
    Trace_t	*trace_ptr;

    if (DTPRINT_ENTRY) printf ("In free_data - trace=%p\n",trace);

    if (!trace || !trace->loaded) return;
    trace->loaded = 0;
    trace->numsigstart = 0;

    /* free any added signals in other traces from this trace */
    for (trace_ptr = global->deleted_trace_head; trace_ptr; trace_ptr = trace_ptr->next_trace) {
	sig_free (trace, trace_ptr->firstsig, TRUE, TRUE);
    }
    /* free signal data and each signal structure */
    sig_free (trace, trace->firstsig, FALSE, TRUE);
    trace->firstsig = NULL;
    trace->lastsig = NULL;
    trace->dispsig = NULL;
    trace->numsig = 0;
}

void trace_read_cb (
    Widget		w,
    Trace_t		*trace)
{
    int		i;

    if (DTPRINT_ENTRY) printf ("In trace_read_cb - trace=%p\n",trace);

    /* Clear the file format */
    trace->dfile.fileformat = FF_AUTO;

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

    DManageChild (trace->fileselect.dialog, trace, MC_NOKEYS);

    XSync (global->display,0);
}

void trace_reread_all_cb (
    Widget		w,
    Trace_t		*trace)
{
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	if (trace->loaded) {
	    trace_reread_cb (w, trace);
	}
    }
}

void trace_reread_cb (
    Widget		w,
    Trace_t		*trace)
{
    char *semi;
    int		read_fd;
    struct stat		newstat;	/* New status on the reread file*/

    if (!trace->loaded)
	trace_read_cb (w,trace);
    else {
	/* Drop ;xxx */
	if ((semi = strchr (trace->dfile.filename,';')))
	    *semi = '\0';
	
	if (DTPRINT_ENTRY) printf ("In trace_reread_cb - rereading file=%s\n",trace->dfile.filename);
	
	/* check the date first */
	read_fd = open (trace->dfile.filename, O_RDONLY, 0);
	if (read_fd>=1) {
	    /* Opened ok */
	    fstat (read_fd, &newstat);
	    close (read_fd);
	    if ((newstat.st_mtime == trace->dfile.filestat.st_mtime)
		&& (newstat.st_ctime == trace->dfile.filestat.st_ctime)) {
		if (DTPRINT_FILE) printf ("  file has not changed.\n");
		return;
	    }
	}

	/* read the file */
	fil_read (trace);
    }
}

void fil_read (
    Trace_t	*trace)
{
    int		read_fd;
    FILE	*read_fp;	/* Routines are responsible for assigning this! */
    char	*pchar;
    char 	pipecmd[MAXFNAMELEN+20];
    pipecmd[0]='\0';	/* MIPS: no automatic aggregate initialization */
    read_fp = NULL;	/* MIPS: no automatic aggregate initialization */

    if (DTPRINT_ENTRY) printf ("In fil_read trace=%p filename=%s\n",trace,trace->dfile.filename);
    
    /* Update directory name */
    strcpy (global->directory, trace->dfile.filename);
    file_directory (global->directory);

    /* Clear the data structures & the screen */
    XClearWindow (global->display, trace->wind);
    set_cursor (DC_BUSY);
    XSync (global->display,0);

    /* free memory associated with the data */
    sig_cross_preserve(trace);
    free_data (trace);

    /* get applicable config files */
    config_read_defaults (trace, TRUE);
    
    /* Compute the file format */
    if (trace->dfile.fileformat == FF_AUTO) {
	trace->dfile.fileformat =	file_format;
    }

    /* Normalize format */
    switch (trace->dfile.fileformat) {
    case	FF_AUTO:
    case	FF_DECSIM:
    case	FF_DECSIM_BIN:
#ifdef VMS
	trace->dfile.fileformat =	FF_DECSIM_BIN;
#else
	/* Binary relys on varible RMS records - Ultrix has no such thing */
	trace->dfile.fileformat =	FF_DECSIM_ASCII;
#endif
	break;
	/* No default */
    }

    /* Open file and copy descriptor information */
    read_fd = open (trace->dfile.filename, O_RDONLY, 0);
    if (read_fd>0) {
	fstat (read_fd, &(trace->dfile.filestat));
#ifndef VMS
	if (! S_ISREG(trace->dfile.filestat.st_mode)) {
	    /* Not regular file */
	    close (read_fd);
	    read_fd = -1;
	}
#endif
    }
    if (read_fd<1) {
	/* Similar code below! */
	sprintf (message,"Can't open file %s", trace->dfile.filename);
	dino_error_ack (trace, message);

	/* Clear cursor and return*/
	sig_cross_restore (trace);
	change_title (trace);
	set_cursor (DC_NORMAL);
	return;
    }

#ifndef VMS
    /* If compressed, close the file and open as uncompressed */
    pipecmd[0]='\0';
    if ((pchar=strrchr(trace->dfile.filename,'.')) != NULL ) {
	if (!strcmp (pchar, ".Z")) sprintf (pipecmd, "uncompress -c %s", trace->dfile.filename);
	if (!strcmp (pchar, ".gz")) sprintf (pipecmd, "gunzip -c %s", trace->dfile.filename);
    }
	
    if (trace->dfile.fileformat == FF_VERILOG_VPD) {
	if (pipecmd[0]) {
	    /* Because vpd2vcd fseeks, it won't take a pipe as input, and we... */
	    sprintf (message,"Can't unzip/uncompress VPD traces.");
	    dino_error_ack (trace, message);
	    return;
	}
	sprintf (pipecmd, "vpd2vcd %s 2>/dev/null", trace->dfile.filename);
    }

    if (pipecmd[0]) {

	if (DTPRINT_FILE) printf ("Piping: %s\n", pipecmd);

	/* Decsim must be ASCII because of record format */
	if (trace->dfile.fileformat == FF_DECSIM_BIN) trace->dfile.fileformat = FF_DECSIM_ASCII;
	
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
	    set_cursor (DC_NORMAL);
	    return;
	}
    }
#endif

    /*
     ** Read in the trace file using the format selected by the user
     */
    switch (trace->dfile.fileformat) {
      case	FF_DECSIM_BIN:
#ifdef VMS
	decsim_read_binary (trace, read_fd);
#endif /* VMS */
	break;
      case	FF_TEMPEST:
	tempest_read (trace, read_fd);
	break;
      case	FF_VERILOG:
      case	FF_VERILOG_VPD:
	verilog_read (trace, read_fd);
	break;
      case	FF_DECSIM_ASCII:
	ascii_read (trace, read_fd, read_fp);
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
    set_cursor (DC_NORMAL);
    if (global->res_default) win_full_res (trace);
    new_time (trace);		/* Realignes start and displays */
    vscroll_new (trace,0);	/* Realign time */
    if (DTPRINT_ENTRY) printf ("fil_read done!\n");
}

void  fil_select_set_pattern (
    Trace_t	*trace,
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

void    fil_format_option_cb (
    Widget	w,
    Trace_t	*trace,
    XmSelectionBoxCallbackStruct *cb)
{
    int 	i;
    if (DTPRINT_ENTRY) printf ("In fil_format_option_cb trace=%p\n",trace);

    for (i=0; i<FF_NUMFORMATS; i++) {
	if (w == trace->fileselect.format_item[i]) {
	    file_format = i;	/* Change global verion */
	    trace->dfile.fileformat = i;	/* Specifically make this file use this format */
	    fil_select_set_pattern (trace, trace->fileselect.dialog, filetypes[file_format].mask);
	}
    }
}

void fil_ok_cb (
    Widget	w,
    Trace_t	*trace,
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
    
    strcpy (trace->dfile.filename, tmp);
    
    DFree (tmp);
    
    if (DTPRINT_FILE) printf ("In fil_ok_cb Filename=%s\n",trace->dfile.filename);
    fil_read (trace);
}


void help_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("in help_cb\n");
    dino_information_ack (trace, help_message ());
}

void help_trace_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    static char msg[2000];
    static char msg2[1000];

    if (DTPRINT_ENTRY) printf ("in help_trace_cb\n");
    
    if (!trace->loaded) {
	sprintf (msg, "No trace is loaded.\n");
    }
    else {
	sprintf (msg, "%s\n\n", trace->dfile.filename);

	sprintf (msg2, "File Format: %s\n", filetypes[trace->dfile.fileformat].name);
	strcat (msg, msg2);

	sprintf (msg2, "File Modified Date: %s\n", date_string (trace->dfile.filestat.st_ctime));
	strcat (msg, msg2);

	sprintf (msg2, "File Creation Date: %s\n", date_string (trace->dfile.filestat.st_mtime));
	strcat (msg, msg2);

	sprintf (msg2, "\nTimes stored to nearest: %s\n", time_units_to_string (global->time_precision, TRUE));
	strcat (msg, msg2);
    }

    dino_information_ack (trace, msg);
}


void help_doc_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);

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

	/* create scrolled text widget */
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

#if !defined (fil_add_cptr)
#pragma inline (fil_add_cptr)
/* WARNING, INLINED CODE IN CALLBACKS.H */
void	fil_add_cptr (
    Signal_t	*sig_ptr,
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
    Trace_t	*trace,
    Boolean_t	not_tempest)	/* Use the name of the bus to find the bit vectors */
{
    Signal_t	*sig_ptr;	/* ptr to current signal (lower bit number) */
    int		pos;
    char	postbusstuff[MAXSIGLEN];

    if (DTPRINT_ENTRY) printf ("In fil_make_busses\n");
    if (DTPRINT_BUSSES) sig_print_names (trace);

    /* Calculate numsig */
    /* and allocate Signal's cptr, bptr, blocks, inc, type */
    trace->numsig = 0;
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	/* Stash the characters after the bus name */
	if (sig_ptr->signame_buspos) strcpy (postbusstuff, sig_ptr->signame_buspos);

	/* Change the name to include the vector subscripts */
	if (sig_ptr->msb_index >= 0) {
	    /* Add new vector info */
	    if (sig_ptr->bits >= 2) {
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
	if (sig_ptr->bits < 2) {
	    sig_ptr->type = 0;
	}
	else if (sig_ptr->bits < 33) {
	    sig_ptr->type = STATE_B32;
	}
	else if (sig_ptr->bits < 129) {
	    sig_ptr->type = STATE_B128;
	}

	/* Compute value_mask.  This mask ANDed with the value will clear any bits that
	   are not represented in this vector.  For example a 12 bit vector will only have the lower
	   twelve bits set, all others will be clear */

	sig_ptr->value_mask[3] = sig_ptr->value_mask[2] = sig_ptr->value_mask[1] = sig_ptr->value_mask[0] = 0;

	if (sig_ptr->bits > 127) sig_ptr->value_mask[3] = 0xFFFFFFFF;
	else if (sig_ptr->bits > 96 ) sig_ptr->value_mask[3] = (0x7FFFFFFF >> (127-sig_ptr->bits));
	else if (sig_ptr->bits > 95 ) sig_ptr->value_mask[2] = 0xFFFFFFFF;
	else if (sig_ptr->bits > 64 ) sig_ptr->value_mask[2] = (0x7FFFFFFF >> (95-sig_ptr->bits));
	else if (sig_ptr->bits > 63 ) sig_ptr->value_mask[1] = 0xFFFFFFFF;
	else if (sig_ptr->bits > 32 ) sig_ptr->value_mask[1] = (0x7FFFFFFF >> (63-sig_ptr->bits));
	else if (sig_ptr->bits > 31 ) sig_ptr->value_mask[0] = 0xFFFFFFFF;
	else sig_ptr->value_mask[0] = (0x7FFFFFFF >> (31-sig_ptr->bits));

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
	    + ((sig_ptr->file_type.flag.four_state) ? 2:1) * (sig_ptr->bits-1);
	
	/* allocate the data storage memory */
	if (!sig_ptr->copyof) {
	    sig_ptr->blocks = BLK_SIZE;
	    sig_ptr->bptr = (Value_t *)XtMalloc ((sig_ptr->blocks*sizeof(uint_t))
						 + (sizeof(Value_t)*2 + 2));
	    val_zero (sig_ptr->bptr);	/* So we know is empty */
	} else {
	    sig_ptr->bptr = NULL;
	}
	sig_ptr->cptr = sig_ptr->bptr;
    }

    if (DTPRINT_BUSSES) sig_print_names (trace);
}


static void fil_mark_cptr_end (
    Trace_t	*trace)
{
    Signal_t	*sig_ptr, *sig_next_ptr;
    Value_t	value;
    Boolean_t	msg=FALSE;

    /* loop thru each signal */
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_next_ptr) {
	sig_next_ptr = sig_ptr->forward;

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
	    sig_free (trace, sig_ptr, FALSE, FALSE);
	    continue;
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
    Trace_t	*trace)
{
    Signal_t	*sig_ptr;

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

    switch (trace->dfile.fileformat) {
    case	FF_TEMPEST:
    case	FF_DECSIM_ASCII:
    case	FF_DECSIM_BIN:
	sig_modify_enables (trace);
	break;
    case	FF_VERILOG:
    case	FF_VERILOG_VPD:
    default:
	break;
    }

    /* Create xstring of the name (to avoid calling again and again) */
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	sig_ptr->xsigname = XmStringCreateSimple (sig_ptr->signame);
    }

    /* Preserve file information */
    sig_cross_restore (trace);

    /* Read .dino file stuff yet again to get signal_highlights */
    /* Don't report errors, as they would pop up for a second time. */
    config_read_defaults (trace, FALSE);

    /* Modify deleted trace to have latest options */
    global->deleted_trace_head->dfile.hierarchy_separator = trace->dfile.hierarchy_separator;
    global->deleted_trace_head->dfile.vector_separator = trace->dfile.hierarchy_separator;
    global->deleted_trace_head->dfile.vectorend_separator = trace->dfile.vectorend_separator;

    /* Apply the statenames */
    draw_needupd_val_states ();
    draw_needupd_val_search ();
    draw_needupd_sig_search ();
    draw_needupd_sig_start ();
    grid_calc_autos (trace);

    if (DTPRINT_FILE) printf ("fil_trace_end: Done\n");
}

