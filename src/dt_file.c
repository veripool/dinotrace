/******************************************************************************a
 *
 * Filename:
 *     dt_file.c
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
 *     AAG	 6-Nov-90	Added check for #2's
 *     AAG	29-Apr-91	Use X11, removed math include, fixed many casts
 *				 and initialized strings using for loops for
 *				 Ultrix support
 *     AAG	 8-Jul-91	Added read_HLO_TEMPEST
 *     AAG	 7-Feb-92	Corrected calculation of pad bits when reading
 *				 tempest file format
 *     WPS	 5-Jan-93	Made xstart be calculated from signal widths
 *     WPS	15-Jan-93	Added binary trace support
 *
 */
static char rcsid[] = "$Id$";


#include <stdio.h>
#include <math.h> /* removed for Ultrix support... */
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef VMS
# include <file.h>
# include <unixio.h>
#endif

#ifdef __osf__
# include <sys/fcntl.h>
#endif

#include <X11/Xlib.h>
#include <Xm/Xm.h>
#include <Xm/Text.h>
#include <Xm/MessageB.h>
#include <Xm/SelectioB.h>

#include "dinotrace.h"
#include "callbacks.h"
#include "dinodoc.h"

int fil_line_num=0;

/****************************** UTILITIES ******************************/

void free_data (trace)
    /* Free trace information, also used when deleting preserved structure */
    TRACE	*trace;
{
    TRACE	*trace_ptr;

    if (DTPRINT_ENTRY) printf ("In free_data - trace=%d\n",trace);

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

void trace_read_cb (w,trace)
    Widget		w;
    TRACE		*trace;
{
    if (DTPRINT_ENTRY) printf ("In trace_read_cb - trace=%d\n",trace);

    /* Clear the file format */
    trace->fileformat = FF_AUTO;

    /* get the filename */
    get_file_name (trace);
    }

void trace_reread_all_cb (w,trace)
    Widget		w;
    TRACE		*trace;
{
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	if (trace->loaded) {
	    trace_reread_cb (w, trace);
	    }
	}
    }

void trace_reread_cb (w,trace)
    Widget		w;
    TRACE		*trace;
{
    char *semi;
    int		read_fd;
    struct stat		newstat;	/* New status on the reread file*/

    if (!trace->loaded)
	trace_read_cb (w,trace);
    else {
	/* Drop ;xxx */
	if (semi = strchr (trace->filename,';'))
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
	fil_read_cb (trace);
	}
    }

void fil_read_cb (trace)
    TRACE	*trace;
{
    int		read_fd;
    FILE	*read_fp;	/* Routines are responsible for assigning this! */
    char	*pchar;
    char 	pipecmd[MAXFNAMELEN+20];
    pipecmd[0]='\0';	/* MIPS: no automatic aggregate initialization */
    read_fp = NULL;	/* MIPS: no automatic aggregate initialization */

    if (DTPRINT_ENTRY) printf ("In fil_read_cb trace=%d filename=%s\n",trace,trace->filename);
    
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
    read_trace_end (trace);

    /* Change the name on title bar to filename */
    change_title (trace);
    
    /*
     ** Clear the window and draw the screen with the new file
     */
    set_cursor (trace, DC_NORMAL);
    if (global->res_default) win_full_res_cb (NULL, trace, NULL);
    new_time (trace);		/* Realignes start and displays */
    vscroll_new (trace,0);	/* Realign time */
    if (DTPRINT_ENTRY) printf ("fil_read_cb done!\n");
    }

void help_cb (w,trace,cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT_ENTRY) printf ("in help_cb\n");

    dino_information_ack (trace, help_message ());
    }

void help_trace_cb (w,trace,cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
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


void help_doc_cb (w,trace,cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    XmString	xsout;

    if (DTPRINT_ENTRY) printf ("in help_doc_cb\n");

    /* May be called before the window was opened, if so ignore the help_doc */
    if (!trace->work) return;

    /* create the widget if it hasn't already been */
    if (!trace->help_doc) {
	XtSetArg (arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg (arglist[1], XmNdialogTitle, XmStringCreateSimple ("Dinotrace Documentation") );
	trace->help_doc = XmCreateInformationDialog (trace->work, "info", arglist, 2);
	XtAddCallback (trace->help_doc, XmNokCallback, unmanage_cb, trace->help_doc);
	XtUnmanageChild ( XmMessageBoxGetChild (trace->help_doc, XmDIALOG_CANCEL_BUTTON));
	XtUnmanageChild ( XmMessageBoxGetChild (trace->help_doc, XmDIALOG_HELP_BUTTON));

	/* create string w/ seperators */
	/*xsout = string_create_with_cr (dinodoc);*/

	/* create the file name text widget */
	XtSetArg (arglist[0], XmNrows, 40);
	XtSetArg (arglist[1], XmNcolumns, 80);
	XtSetArg (arglist[2], XmNeditable, FALSE);
	XtSetArg (arglist[3], XmNvalue, dinodoc);
	XtSetArg (arglist[4], XmNscrollVertical, TRUE);
	XtSetArg (arglist[5], XmNeditMode, XmMULTI_LINE_EDIT);
	XtSetArg (arglist[6], XmNscrollHorizontal, FALSE);
	trace->help_doc_text = XmCreateScrolledText (trace->help_doc,"textn",arglist,7);

	XtManageChild (trace->help_doc_text);
    }

    /* manage the widget */
    XtManageChild (trace->help_doc);
}

#pragma inline (fil_string_add_cptr)
void	fil_string_add_cptr (sig_ptr, value_strg, time, nocheck)
    /* Add a cptr corresponding to the text at value_strg */
    /* WARNING: Similar verilog_string_to_value in dt_verilog for speed reasons */
    SIGNAL	*sig_ptr;
    char	*value_strg;
    DTime	time;
    Boolean	nocheck;		/* don't compare against previous data */
{
    register unsigned int state;
    register char	*cp, *cep;
    register char 	state_chr;
    register int	len, bitcnt;
    VALUE	value;

    /* zero the value */
    /* We do NOT need to set value.siglw.number, as is set later */
    /* I set it anyways to get it into the cache */
    value.siglw.number = value.number[0] = value.number[1] = value.number[2] = value.number[3] = 0;

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
	state_chr = 0;
	len = sig_ptr->bits;
	cp = value_strg;
	cep = cp + len;
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
	while (cp <= cep) {
	    state_chr |= *cp++;
	}
	switch (state_chr) {
	  case '0':
	  case '1':
	    /* compute the value */
	    /* Note '0'->0 and '1'->1 by just ANDing with 1 */
	    state = sig_ptr->type;
	    cp = value_strg;
	    for (bitcnt=96; bitcnt <= (MIN (127,len)); bitcnt++)
		value.number[3] = (value.number[3]<<1) | (*cp++=='1');
	    
	    for (bitcnt=64; bitcnt <= (MIN (95,len)); bitcnt++)
		value.number[2] = (value.number[2]<<1) | (*cp++=='1');
	    
	    for (bitcnt=32; bitcnt <= (MIN (63,len)); bitcnt++)
		value.number[1] = (value.number[1]<<1) | (*cp++=='1');

	    for (bitcnt=0; bitcnt <= (MIN (31,len)); bitcnt++)
		value.number[0] = (value.number[0]<<1) | (*cp++=='1');
	    break;

	  case 'z':
	  case 'Z':
	    state = STATE_Z;
	    break;

	  default:
	    state = STATE_U;
	    break;
	}
    }

    /* if (DTPRINT_FILE) printf ("time %d sig %s state %d\n", time, sig_ptr->signame,  state); */
    value.siglw.sttime.state = state;
    value.siglw.sttime.time = time;
    fil_add_cptr (sig_ptr, &value, nocheck);
}

#if !defined (fil_add_cptr)
#pragma inline (fil_add_cptr)
/* WARNING, INLINED CODE IN CALLBACKS.H */
void	fil_add_cptr (sig_ptr, value_ptr, nocheck)
    SIGNAL	*sig_ptr;
    VALUE	*value_ptr;
    Boolean	nocheck;		/* compare against previous data */
{
    long	diff;
    SIGNAL_LW	*cptr;

    printf ("Checking st %d tm %d with st %d time %d\n",
	    value_ptr->siglw.sttime.state,
	    value_ptr->siglw.sttime.time,
	    ((sig_ptr->cptr) - sig_ptr->lws)->sttime.state,      
	    ((sig_ptr->cptr) - sig_ptr->lws)->sttime.time
	    );

    /* Comparing all 4 LW's works because we keep the unused LWs zeroed */
    cptr = sig_ptr->cptr - sig_ptr->lws;
    if ( nocheck
	|| ( cptr->sttime.state != value_ptr->siglw.sttime.state )
	|| ( cptr[1].number != value_ptr->number[0] )
	|| ( cptr[2].number != value_ptr->number[1] )
	|| ( cptr[3].number != value_ptr->number[2] )
	|| ( cptr[4].number != value_ptr->number[3] ) ) {

	diff = sig_ptr->cptr - sig_ptr->bptr;
	if (diff > sig_ptr->blocks ) {
	    sig_ptr->blocks += BLK_SIZE;
	    sig_ptr->bptr = (SIGNAL_LW *)XtRealloc ((char*)sig_ptr->bptr,
						    (sig_ptr->blocks*sizeof(unsigned int)) + (sizeof(VALUE)*2 + 2));
	    sig_ptr->cptr = sig_ptr->bptr + diff;
	}
       
	(sig_ptr->cptr)[0].number = value_ptr->siglw.number;
	(sig_ptr->cptr)[1].number = value_ptr->number[0];
	(sig_ptr->cptr)[2].number = value_ptr->number[1];
	(sig_ptr->cptr)[3].number = value_ptr->number[2];
	(sig_ptr->cptr)[4].number = value_ptr->number[3];
	(sig_ptr->cptr) += sig_ptr->lws;
    }
}
#endif

void read_make_busses (trace, not_tempest)
    /* Take the list of signals and make it into a list of busses */
    /* Also do the common stuff required for each signal. */
    TRACE	*trace;
    Boolean	not_tempest;	/* Use the name of the bus to find the bit vectors */
{
    SIGNAL	*sig_ptr;	/* ptr to current signal (lower bit number) */
    SIGNAL	*bus_sig_ptr;	/* ptr to signal which is being bussed (upper bit number) */
    char	*bbeg;		/* bus beginning */
    char	*sep;		/* seperator position */
    int		pos;
    char	sepchar;
    char	postbusstuff[MAXSIGLEN];

    if (DTPRINT_ENTRY) printf ("In read_make_busses\n");
    if (DTPRINT_BUSSES) print_sig_names (NULL, trace);

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

	/* Use the seperator character to split signals into vector and base */
	/* IE "signal_1" becomes "signal" with index=1 if the seperator is _ */
	sepchar = trace->vector_seperator;
	if (sepchar == '\0') sepchar='<';	/* Allow both '\0' and '<' for DANGER::DORMITZER */
	sep = strrchr (sig_ptr->signame, sepchar);
	bbeg = sep+1;
	if (!sep || !isdigit (*bbeg)) {
	    if (trace->vector_seperator == '\0') {
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
	    while (*sep && *sep!=trace->vector_endseperator) sep++;
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
    for (sig_ptr = trace->firstsig; sig_ptr;) {
	bus_sig_ptr = sig_ptr;
	if (sig_ptr->signame[0]=='\0') {
	    /* Null, remove this signal */
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
		/* Signal name following bus seperator */
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
			&& trace->vector_seperator=='<'))
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
    /* and allocate SIGNAL's cptr, bptr, blocks, inc, type */
    trace->numsig = 0;
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	/* Stash the characters after the bus name */
	if (sig_ptr->signame_buspos) strcpy (postbusstuff, sig_ptr->signame_buspos);

	/* Change the name to include the vector subscripts */
	if (sig_ptr->msb_index >= 0) {
	    /* Add new vector info */
	    if (sig_ptr->bits >= 1) {
		sprintf (sig_ptr->signame + strlen (sig_ptr->signame), "<%d:%d>",
			 sig_ptr->msb_index, sig_ptr->lsb_index);
		}
	    else {
		sprintf (sig_ptr->signame + strlen (sig_ptr->signame), "<%d>",
			 sig_ptr->msb_index);
		}
	    }

	/* Restore the characters after the bus name */
	if (sig_ptr->signame_buspos) strcat (sig_ptr->signame, postbusstuff);

	/* Calc numsig, last_sig_ptr */
	(trace->numsig) ++;

	/* Create the bussed name & type */
	if (sig_ptr->bits < 1) {
	    sig_ptr->lws = 1;
	    sig_ptr->type = 0;
	    }
	else if (sig_ptr->bits < 32) {
	    sig_ptr->lws = 2;
	    sig_ptr->type = STATE_B32;
	    }
	else if (sig_ptr->bits < 128) {
	    sig_ptr->lws = 5;
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
	sig_ptr->bptr = (SIGNAL_LW *)XtMalloc ((sig_ptr->blocks*sizeof(unsigned int)) + (sizeof(VALUE)*2 + 2));
	sig_ptr->cptr = sig_ptr->bptr;
	}

    if (DTPRINT_BUSSES) print_sig_names (NULL, trace);
    }


void read_mark_cptr_end (trace)
    TRACE	*trace;
{
    SIGNAL	*sig_ptr;
    SIGNAL_LW	*cptr;
    VALUE	value;

    /* loop thru each signal */
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {

	/* Must be a cptr which has the same time as the last time on the screen */
	/* If none exists, create it */
	if (sig_ptr->bptr != sig_ptr->cptr) {
	    cptr = sig_ptr->cptr - sig_ptr->lws;
	    if (cptr->sttime.time != trace->end_time) {
		cptr_to_value (cptr, &value);
		value.siglw.sttime.time = trace->end_time;
		fil_add_cptr (sig_ptr, &value, TRUE);
		}
	    }
	else {
	    if (DTDEBUG) printf ("%%W, No data for signal %s\n", sig_ptr->signame);
	    }

	/* Mark end of time */
	sig_ptr->cptr->sttime.time = EOT;

	/* re-initialize the cptr's to the bptr's */
	sig_ptr->cptr = sig_ptr->bptr;
	}
    }

void read_trace_end (trace)
    /* Perform stuff at end of trace - common across all reading routines */
    TRACE	*trace;
{
    SIGNAL	*sig_ptr;

    if (DTPRINT_FILE) printf ("In read_trace_end\n");

    /* Modify ending cptrs to be correct */
    read_mark_cptr_end (trace);

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

    if (DTPRINT_FILE) printf ("read_trace_end: Done\n");
    }

/****************************** DECSIM ASCII ******************************/

#define FIL_SIZE_INC	1024	/* Characters to increase length by */

void fgets_dynamic (line_pptr, length_ptr, readfp)
    /* fgets a line, with dynamically allocated line storage */
    char **line_pptr;	/* & Line pointer */
    int	*length_ptr;	/* & Length of the buffer */
    FILE *readfp;
{
    if (*length_ptr == 0) {
	*length_ptr = FIL_SIZE_INC;
	*line_pptr = XtMalloc (*length_ptr);
	}

    fgets ((*line_pptr), (*length_ptr), readfp);
    
    /* Did fgets overflow the string? */
    while (*((*line_pptr) + *length_ptr - 2)!='\n'
	   && strlen(*line_pptr)==(*length_ptr - 1)) {
	/* Alloc more */
	*length_ptr += FIL_SIZE_INC;
	*line_pptr = XtRealloc (*line_pptr, *length_ptr);
	/* Read remainder */
	fgets (((*line_pptr) + *length_ptr - 2) + 1 - FIL_SIZE_INC,
	       FIL_SIZE_INC+1, readfp);
	}
    }

void decsim_read_ascii_header (trace, header_start, data_begin_ptr, sig_start_pos, sig_end_pos,
			       header_lines)
    TRACE	*trace;
    int		sig_start_pos, sig_end_pos, header_lines;
    char	*data_begin_ptr;
    char	*header_start;
{
    int		line,col;
    SIGNAL	*sig_ptr,*last_sig_ptr=NULL;
    char	*line_ptr, *tmp_ptr;
    char	*signame_array;
    Boolean	hit_name_block, past_name_block, no_names;

#define	SIGLINE(_l_) (signame_array+(_l_)*(sig_end_pos+5))

    /* make array:  [header_lines+5][sig_end_pos+5]; */
    signame_array = XtMalloc ((header_lines+5)*(sig_end_pos+5));

    /* Make a signal structure for each signal */
    trace->firstsig = NULL;
    for ( col=sig_start_pos; col < sig_end_pos; col++) {
	sig_ptr = DNewCalloc (SIGNAL);

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

void decsim_read_ascii (trace, read_fd, decsim_z_readfp)
    TRACE	*trace;
    int		read_fd;
    FILE	*decsim_z_readfp;	/* Only pre-set if FF_DECSIM_Z */
{
    FILE	*readfp;
    Boolean	first_data;
    char	*line_in;
    SIGNAL	*sig_ptr;
    long	time_stamp;
    VALUE	value;
    char	*pchar;
    DTime	time_divisor;
    int		ch;

    char	*header_start, *header_ptr;
    long	header_length, header_lines=0;
    Boolean	got_start=FALSE, hit_start=FALSE, in_comment=FALSE;
    long	base_chars_left=0;

    char	*t;
    Boolean	chango_format=FALSE;
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
    while (! got_start && (EOF != (ch = fgetc (readfp)))) {
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

    if (DTPRINT_FILE) printf ("Line:%s\nHeader has %d lines, Signals run from char %d to %d. %s format\n", 
			      data_begin_ptr, header_lines, sig_start_pos, sig_end_pos,
			      chango_format?"Chango":"DECSIM");

    if (sig_start_pos == sig_end_pos) {
	dino_error_ack (trace,"No data in trace file");
	XtFree (header_start);
	return;
	}

    /***** READ SIGNALS DATA ****/
    decsim_read_ascii_header (trace, header_start, data_begin_ptr, sig_start_pos, sig_end_pos,
			      header_lines);

    /***** SET UP FOR READING DATA ****/

    /* Make the busses */
    /* Any lines that were all spaces will be pulled off.  This will strip off the time section of the lines */
    read_make_busses (trace, TRUE);

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

