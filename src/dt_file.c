/******************************************************************************
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


#include <stdio.h>
#include <math.h> /* removed for Ultrix support... */
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef VMS
#include <file.h>
#include <unixio.h>
#endif VMS

#include <X11/Xlib.h>
#include <Xm/Xm.h>

#include "dinotrace.h"
#include "callbacks.h"

Boolean	separator=FALSE;		/* File temporary, true if | in trace file */

/****************************** UTILITIES ******************************/

void free_data (trace)
    TRACE	*trace;
{
    TRACE	*trace_ptr;

    if (DTPRINT) printf ("In free_data - trace=%d\n",trace);

    if (!trace->loaded) return;
    trace->loaded = 0;

    /* free any deleted signals */
    sig_free (trace, global->delsig, TRUE, TRUE);

    /* free any added signals in other traces from this trace */
    for (trace_ptr = global->trace_head; trace_ptr; trace_ptr = trace_ptr->next_trace) {
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
    if (DTPRINT) printf ("In trace_read_cb - trace=%d\n",trace);

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
	
	if (DTPRINT) printf ("In trace_reread_cb - rereading file=%s\n",trace->filename);
	
	/* check the date first */
	read_fd = open (trace->filename, O_RDONLY, 0);
	if (read_fd>=1) {
	    /* Opened ok */
	    fstat (read_fd, &newstat);
	    close (read_fd);
	    if ((newstat.st_mtime == trace->filestat.st_mtime)
		&& (newstat.st_ctime == trace->filestat.st_ctime)) {
		if (DTPRINT) printf ("  file has not changed.\n");
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

    if (DTPRINT) printf ("In fil_read_cb trace=%d filename=%s\n",trace,trace->filename);
    
    /* Update directory name */
    strcpy (global->directory, trace->filename);
    file_directory (global->directory);

    /* Clear the data structures & the screen */
    XClearWindow (global->display, trace->wind);
    /* free memory associated with the data */
    free_data (trace);

    set_cursor (trace, DC_BUSY);
    XSync (global->display,0);
    
    /* get applicable config files */
    config_read_defaults (trace);
    
    /* Compute the file format */
    if (trace->fileformat == FF_AUTO) {
	switch (file_format) {
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
	  case	FF_TEMPEST:
	  case	FF_VERILOG:
	  case	FF_DECSIM_ASCII:
	  case	FF_DECSIM_Z:
	    trace->fileformat =	file_format;
	    break;
	  default:
	    fprintf (stderr, "Unknown file format (AUTO)!!\n");
	    }
	}

    /* Open file and copy descriptor information */
    read_fd = open (trace->filename, O_RDONLY, 0);
    if (read_fd<1) {
	/* Similar code below! */
	if (DTPRINT) printf ("Can't Open File %s\n", trace->filename);
	sprintf (message,"Can't open file %s", trace->filename);
	dino_error_ack (trace, message);

	/* Clear cursor and return */
	change_title (trace);
	set_cursor (trace, DC_NORMAL);
	return;
	}
    fstat (read_fd, &(trace->filestat));

#ifndef VMS
    /* If compressed, close the file and open as uncompressed */
    if (trace->fileformat == FF_DECSIM_Z) {
	char scmd[MAXFNAMELEN];

	/* Close compressed file and open uncompressed file */
	close (read_fd);

	sprintf (scmd, "uncompress -c %s", trace->filename);
	read_fp = popen (scmd, "r");
	if (!read_fp) {
	    /* Similar above! */
	    if (DTPRINT) printf ("Can't create pipe, Command %s\n", scmd);
	    sprintf (message,"Can't create pipe with command '%s'", scmd);
	    dino_error_ack (trace, message);

	    /* Clear cursor and return */
	    change_title (trace);
	    set_cursor (trace, DC_NORMAL);
	    return;
	    }
	}
#endif

    /*
     ** Read in the trace file using the format selected by the user
     */
    switch (trace->fileformat) {
      case	FF_DECSIM_BIN:
	decsim_read_binary (trace, read_fd);
	break;
      case	FF_TEMPEST:
	tempest_read (trace, read_fd);
	break;
      case	FF_VERILOG:
	verilog_read (trace, read_fd);
	break;
      case	FF_DECSIM_ASCII:
      case	FF_DECSIM_Z:
	decsim_read_ascii (trace, read_fd, read_fp);
	break;
      default:
	fprintf (stderr, "Unknown file format!!\n");
	}

    /* Close the file */
    if (trace->fileformat != FF_DECSIM_Z) {
	close (read_fd);
	}
#ifndef VMS
    else {
	pclose (read_fp);
	}
#endif

    /* Change the name on title bar to filename */
    change_title (trace);
    
    /*
     ** Clear the window and draw the screen with the new file
     */
    set_cursor (trace, DC_NORMAL);
    if (global->res_default) win_full_res_cb (NULL, trace, NULL);
    new_time (trace);	/* Realignes start and displays */
    }

void help_cb (w,trace,cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf ("in help_cb\n");

    dino_information_ack (trace, help_message ());
    }

void help_trace_cb (w,trace,cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    static char msg[2000];
    static char msg2[100];
    static char	date_str[50];

    if (DTPRINT) printf ("in help_trace_cb\n");
    
    if (!trace->loaded) {
	sprintf (msg, "No trace is loaded.\n");
	}
    else {
	sprintf (msg,
	     "%s\n\
\n", 	 trace->filename);

	sprintf (msg2, "File Format: %s\n", filetypes[trace->fileformat].name);
	strcat (msg, msg2);

	strcpy (date_str, asctime (localtime (&(trace->filestat.st_ctime))));
	if (date_str[strlen (date_str)-1]=='\n')
	    date_str[strlen (date_str)-1]='\0';
	sprintf (msg2, "File Modified Date: %s\n", date_str);
	strcat (msg, msg2);

	strcpy (date_str, asctime (localtime (&(trace->filestat.st_mtime))));
	if (date_str[strlen (date_str)-1]=='\n')
	    date_str[strlen (date_str)-1]='\0';
	sprintf (msg2, "File Creation Date: %s\n", date_str);
	strcat (msg, msg2);

	sprintf (msg2, "\nTimes stored to nearest: %s\n", time_units_to_string (global->time_precision));
	strcat (msg, msg2);
	}

    dino_information_ack (trace, msg);
    }

#pragma inline (fil_string_to_value)
void	fil_string_to_value (sig_ptr, value_strg, value_ptr)
    /* Returns state and value corresponding to the text at value_strg */
    SIGNAL	*sig_ptr;
    char	*value_strg;
    VALUE	*value_ptr;
{
    register unsigned int state;
    char	*cp;
    int		len, bitcnt;

    /* zero the value */
    value_ptr->siglw.number = value_ptr->number[0] = value_ptr->number[1] = value_ptr->number[2] = 0;

    if (sig_ptr->bits == 0) {
	/* scalar signal */  
	switch ( *value_strg ) {
	  case '0': state = STATE_0; break;
	  case '1': state = STATE_1; break;
	  case 'X':
	  case 'x':
	  case 'U':
	  case 'u': state = STATE_U; break;
	  case 'Z': 
	  case 'z': state = STATE_Z; break;
	  default: 
	    printf ("%%E, Unknown state character '%c' in fil_string_to_value\n", *value_strg);
	    }
	}

    else {
	/* Multi bit signal */

	/* determine the state of the bus */
	state = STATE_0;
	cp = value_strg;
	for (bitcnt=0; bitcnt <= sig_ptr->bits; bitcnt++, cp++) {
	    switch (*cp) {
	      case 'X':
	      case 'x':
	      case 'u':
	      case 'U':
		state = STATE_U; break;
	      case 'z': 
	      case 'Z': 
		if (state != STATE_0 && state != STATE_Z) state = STATE_U;
		else state = STATE_Z;
		break;
	      default:
		if (state != STATE_0 && state != STATE_1) state = STATE_U;
		else state = STATE_1;
		}
	    }
	if (state != STATE_U && state != STATE_Z) {
	    /* compute the value - need to fix for 64 and 96 */
	    state = sig_ptr->type;
	    cp = value_strg;
	    len = sig_ptr->bits;
	    for (bitcnt=64; bitcnt <= (MIN (95,len)); bitcnt++)
		value_ptr->number[2] = (value_ptr->number[2]<<1) + ((*cp++ == '1')?1:0);
	    
	    for (bitcnt=32; bitcnt <= (MIN (63,len)); bitcnt++)
		value_ptr->number[1] = (value_ptr->number[1]<<1) + ((*cp++ == '1')?1:0);

	    for (bitcnt=0; bitcnt <= (MIN (31,len)); bitcnt++)
		value_ptr->number[0] = (value_ptr->number[0]<<1) + ((*cp++ == '1')?1:0);
	    }
	}

    value_ptr->siglw.sttime.state = state;
    }

#pragma inline (cptr_to_value)
void	cptr_to_value (cptr, value_ptr)
    SIGNAL_LW	*cptr;
    VALUE	*value_ptr;
{
    value_ptr->siglw.number = cptr[0].number;
    value_ptr->number[0] = cptr[1].number;
    value_ptr->number[1] = cptr[2].number;
    value_ptr->number[2] = cptr[3].number;
    }

#pragma inline (fil_add_cptr)
void	fil_add_cptr (sig_ptr, value_ptr, check)
    SIGNAL	*sig_ptr;
    VALUE	*value_ptr;
    int		check;		/* compare against previous data */
{
    unsigned int diff;
    SIGNAL_LW	*cptr;

    /*
    printf ("Checking st %d tm %d with st %d time %d\n",
	    value_ptr->siglw.sttime.state,
	    value_ptr->siglw.sttime.time,
	    ((sig_ptr->cptr) - sig_ptr->lws)->sttime.state,      
	    ((sig_ptr->cptr) - sig_ptr->lws)->sttime.time
	    );
	    */

    diff = sig_ptr->cptr - sig_ptr->bptr;
    if (diff > BLK_SIZE / 4 * sig_ptr->blocks - 4) {
	sig_ptr->blocks++;
	sig_ptr->bptr = (SIGNAL_LW *)XtRealloc (sig_ptr->bptr, sig_ptr->blocks*BLK_SIZE);
	sig_ptr->cptr = sig_ptr->bptr+diff;
	}
       
    /* Comparing all 4 LW's works because we keep the unused LWs zeroed */
    cptr = sig_ptr->cptr - sig_ptr->lws;
    if ( !check
	|| ( cptr->sttime.state != value_ptr->siglw.sttime.state )
	|| ( (cptr+1)->number != value_ptr->number[0] )
	|| ( (cptr+2)->number != value_ptr->number[1] )
	|| ( (cptr+3)->number != value_ptr->number[2] ) )
	{
	(sig_ptr->cptr)[0].number = value_ptr->siglw.number;
	(sig_ptr->cptr)[1].number = value_ptr->number[0];
	(sig_ptr->cptr)[2].number = value_ptr->number[1];
	(sig_ptr->cptr)[3].number = value_ptr->number[2];
	(sig_ptr->cptr) += sig_ptr->lws;
	}
    }


void read_make_busses (trace)
    /* Take the list of signals and make it into a list of busses */
    /* Also do the common stuff required for each signal. */
    TRACE	*trace;
{
    SIGNAL	*sig_ptr;	/* ptr to current signal (lower bit number) */
    SIGNAL	*bus_sig_ptr;	/* ptr to signal which is being bussed (upper bit number) */
    char	*bbeg;
    int		pos;
    char	sepchar;

    if (DTPRINT) printf ("In read_make_busses\n");
    if (DTPRINT) print_sig_names (NULL, trace);

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
	if (sig_ptr->bits) {
	    sig_ptr->msb_index = sig_ptr->bits;
	    sig_ptr->lsb_index = 0;
	    }
	else {
	    sig_ptr->msb_index = -1;
	    sig_ptr->lsb_index = -1;
	    }

	sepchar = trace->vector_seperator;
	if (sepchar == '\0') sepchar='<';	/* Allow both '\0' and '<' for DANGER::DORMITZER */
	bbeg = strrchr (sig_ptr->signame, sepchar);
	if (bbeg && isdigit (*(bbeg+1))) {
	    sig_ptr->msb_index = atoi (bbeg+1);
	    sig_ptr->lsb_index = sig_ptr->msb_index - sig_ptr->bits;
	    /* Don't need to search for :'s, as the bits should already be computed */
	    *bbeg = '\0';
	    }
	else if (trace->vector_seperator == '\0') {
	    for (bbeg = sig_ptr->signame + strlen (sig_ptr->signame) - 1;
		 (bbeg > sig_ptr->signame) && isdigit (*bbeg);
		 bbeg --) ;
	    bbeg++;
	    if (isdigit (*bbeg)) {
		sig_ptr->msb_index = atoi (bbeg);
		sig_ptr->lsb_index = sig_ptr->msb_index - sig_ptr->bits;
		*bbeg = '\0';
		}
	    }
	}

    /* Vectorize signals */
    bus_sig_ptr = NULL;
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	/*
	if (DTPRINT && bus_sig_ptr) printf ("  '%s'%d/%d/%d\t'%s'%d/%d/%d\n",
					    bus_sig_ptr->signame, bus_sig_ptr->msb_index,
					    bus_sig_ptr->bits, bus_sig_ptr->file_pos, 
					    sig_ptr->signame, sig_ptr->msb_index, 
					    sig_ptr->bits, sig_ptr->file_pos);
					    */

	/* Start on 2nd signal, see if can merge with previous signal */
	if (bus_sig_ptr) {
	    /* Combine signals with same base name, if */
	    /*  are a vector */
	    /* 	& have subscripts that are different by 1  (<20:10> and <9:5> are seperated by 10-9=1) */
	    /*  & the result would have < 96 bits */
	    /*	& are placed next to each other in the source */
	    /*	& not a tempest trace (because the bit ordering is backwards, <31:0> would look line 0:31 */
	    /*	& not (verilog trace which had a signal already as a vector) */
	    if (sig_ptr->msb_index >= 0
		&& !strcmp (sig_ptr->signame, bus_sig_ptr->signame)
		&& ((bus_sig_ptr->msb_index - sig_ptr->lsb_index) < 96 )
		&& ( ((bus_sig_ptr->msb_index >= sig_ptr->lsb_index)
		      && ((bus_sig_ptr->lsb_index - 1) == sig_ptr->msb_index))
		    || ((bus_sig_ptr->msb_index <= sig_ptr->lsb_index)
			&& ((bus_sig_ptr->lsb_index + 1) == sig_ptr->msb_index)))
		&& trace->fileformat != FF_TEMPEST
		&& (trace->fileformat != FF_VERILOG
		    || ((bus_sig_ptr->file_pos + bus_sig_ptr->bits + 1) == sig_ptr->file_pos))
		&& ! (sig_ptr->file_type.flag.perm_vector || bus_sig_ptr->file_type.flag.perm_vector)) {

		/* Can be bussed with previous signal */
		bus_sig_ptr->bits += sig_ptr->bits + 1;
		bus_sig_ptr->lsb_index = sig_ptr->lsb_index;

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
	/* Change the name to include the vector subscripts */
	if (sig_ptr->msb_index >= 0) {
	    if (sig_ptr->bits >= 1) {
		sprintf (sig_ptr->signame + strlen (sig_ptr->signame), "<%d:%d>",
			 sig_ptr->msb_index, sig_ptr->lsb_index);
		}
	    else {
		sprintf (sig_ptr->signame + strlen (sig_ptr->signame), "<%d>",
			 sig_ptr->msb_index);
		}
	    }

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
	else if (sig_ptr->bits < 64) {
	    sig_ptr->lws = 3;
	    sig_ptr->type = STATE_B64;
	    }
	else if (sig_ptr->bits < 96) {
	    sig_ptr->lws = 4;
	    sig_ptr->type = STATE_B96;
	    }

	/* Compute value_mask.  This mask ANDed with the value will clear any bits that
	   are not represented in this vector.  For example a 12 bit vector will only have the lower
	   twelve bits set, all others will be clear */

	sig_ptr->value_mask[2] = sig_ptr->value_mask[1] = sig_ptr->value_mask[0] = 0;

	if (sig_ptr->bits >= 95) sig_ptr->value_mask[2] = 0xFFFFFFFF;
	else if (sig_ptr->bits >= 64 ) sig_ptr->value_mask[2] = (0x7FFFFFFF >> (94-sig_ptr->bits));
	else if (sig_ptr->bits >= 63 ) sig_ptr->value_mask[1] = 0xFFFFFFFF;
	else if (sig_ptr->bits >= 32 ) sig_ptr->value_mask[1] = (0x7FFFFFFF >> (62-sig_ptr->bits));
	else if (sig_ptr->bits >= 31 ) sig_ptr->value_mask[0] = 0xFFFFFFFF;
	else sig_ptr->value_mask[0] = (0x7FFFFFFF >> (30-sig_ptr->bits));

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
	sig_ptr->bptr = (SIGNAL_LW *)XtMalloc (BLK_SIZE);
	sig_ptr->cptr = sig_ptr->bptr;
	sig_ptr->blocks = 1;
	}

    if (DTPRINT) print_sig_names (NULL, trace);
    }


void read_trace_end (trace)
    /* Perform stuff at end of trace - common across all reading routines */
    TRACE	*trace;
{
    SIGNAL	*sig_ptr;
    SIGNAL_LW	*cptr;
    VALUE	value;

    if (DTPRINT) printf ("In read_trace_end\n");

    /* loop thru each signal */
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {

	/* Must be a cptr which has the same time as the last time on the screen */
	/* If none exists, create it */
	if (sig_ptr->bptr != sig_ptr->cptr) {
	    cptr = sig_ptr->cptr - sig_ptr->lws;
	    if (cptr->sttime.time != trace->end_time) {
		cptr_to_value (cptr, &value);
		value.siglw.sttime.time = trace->end_time;
		fil_add_cptr (sig_ptr, &value, FALSE);
		}
	    }
	else {
	    if (DTDEBUG) printf ("%%W, No data for signal %s\n", sig_ptr->signame);
	    }

	/* Mark end of time */
	sig_ptr->cptr->sttime.time = EOT;

	/* re-initialize the cptr's to the bptr's */
	sig_ptr->cptr = sig_ptr->bptr;

	/* Create xstring of the name (to avoid calling again and again) */
	sig_ptr->xsigname = XmStringCreateSimple (sig_ptr->signame);
	}

    /* Misc */
    trace->dispsig = trace->firstsig;

    /* Make sure time is within bounds */
    if ( (global->time < trace->start_time) || (global->time > trace->end_time)) {
	global->time = trace->start_time;
	}

    /* Clear the starting signal */
    trace->numsigstart = 0;

    /* Mark as loaded */
    trace->loaded = TRUE;

    /* Apply the statenames */
    update_signal_states (trace);
    val_update_search ();
    sig_update_search ();
    }


void	update_signal_states (trace)
    TRACE	*trace;
{
    SIGNAL	*sig_ptr;
    SIGNAL_LW	*cptr;
    int		rise1=0, fall1=0, rise2=0, fall2=0;

    if (DTPRINT) printf ("In update_signal_states\n");

    /* don't do anything if no file is loaded */
    if (!trace->loaded) return;

    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	if (NULL != (sig_ptr->decode = find_signal_state (trace, sig_ptr->signame))) {
	    /* if (DTPRINT) printf ("Signal %s is patterned\n",sig_ptr->signame); */
	    }
	/* else if (DTPRINT) printf ("Signal %s  no pattern\n",sig_ptr->signame); */
	}

    /* Determine period, rise point and fall point of first signal */
    sig_ptr = (SIGNAL *)trace->firstsig;
    cptr = sig_ptr->cptr;
    /* Skip first one, as is not representative of period */
    if ( cptr->sttime.time != EOT) cptr += sig_ptr->lws;
    while ( cptr->sttime.time != EOT) {
	switch (cptr->sttime.state) {
	  case STATE_1:
	    if (!rise1) rise1 = cptr->sttime.time;
	    else if (!rise2) rise2 = cptr->sttime.time;
	    break;
	  case STATE_0:
	    if (!fall1) fall1 = cptr->sttime.time;
	    else if (!fall2) fall2 = cptr->sttime.time;
	    break;
	  case STATE_B32:
	  case STATE_B64:
	  case STATE_B96:
	    if (!rise1) rise1 = cptr->sttime.time;
	    else if (!rise2) rise2 = cptr->sttime.time;
	    if (!fall1) fall1 = cptr->sttime.time;
	    else if (!fall2) fall2 = cptr->sttime.time;
	    break;
	    }
	cptr += sig_ptr->lws;
	}

    /* Set defaults based on changes */
    if (trace->grid_res_auto==GRID_AUTO_ASS) {
	if (rise1 < rise2)	trace->grid_res = rise2 - rise1;
	else if (fall1 < fall2) trace->grid_res = fall2 - fall1;
	}
    if (trace->grid_align_auto==GRID_AUTO_ASS && rise1)
	trace->grid_align = rise1 % trace->grid_res;
    if (trace->grid_align_auto==GRID_AUTO_DEASS && fall1)
	trace->grid_align = fall1 % trace->grid_res;
    if (DTPRINT) printf ("align=%d %d %d\n", trace->grid_align_auto
			 ,GRID_AUTO_DEASS, fall1);
    if (DTPRINT) printf ("rise1=%d, fall1=%d, rise2=%d, fall2=%d, res=%d, align=%d\n",
			 rise1, fall1, rise2, fall2, trace->grid_res, trace->grid_align);

    /* Update global information */
    update_globals ();
    }


/****************************** DECSIM ASCII ******************************/

void decsim_read_ascii (trace, read_fd, decsim_z_readfp)
    TRACE	*trace;
    int		read_fd;
    FILE	*decsim_z_readfp;	/* Only pre-set if FF_DECSIM_Z */
{
    FILE	*readfp;
    int		first_data;
    char	inline[MAX_SIG+20];
    char	*sp8 = "        ";
    int		i,k;
    SIGNAL	*sig_ptr,*last_sig_ptr=NULL;
    int		time;
    VALUE	value;
    char	*pchar;
    DTime	time_divisor;

    separator = FALSE;
    time_divisor = time_units_to_multiplier (global->time_precision);

    if (trace->fileformat != FF_DECSIM_Z) {
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

    /* Check if header lines are present - if so, ignore */
    fgets (inline,MAX_SIG,readfp);
    while ( strncmp (&inline[1],sp8,8) ) {
	fgets (inline,MAX_SIG+10,readfp);
        }

    /* INLINE contains 1st signal line - use for signal number */
    trace->numsig = strlen (&inline[9]) - 1;

    /* Make a signal structure for each signal */
    trace->firstsig = NULL;
    for ( i=0; i < trace->numsig; i++) {
	sig_ptr = (SIGNAL *)XtMalloc (sizeof (SIGNAL));
	memset (sig_ptr, 0, sizeof (SIGNAL));

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
	sig_ptr->signame = (char *)XtMalloc (10+MAXSIGLEN);
	sig_ptr->trace = trace;

	last_sig_ptr = sig_ptr;
	}

    /* Read Signal Names Into Structure */
    k=0;
    while ( inline[0] == '!' ) {
	/*** i=char read - j=char written - k=#lines read ***/
	for (i=0, sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward, i++) {
	    if (separator) i++;
	    sig_ptr->signame[k] = inline[9+i];
	    }
	if (++k >= MAXSIGLEN) {
	    dino_error_ack (trace,"Signal Lengths > MAXSIGLEN");
	    k--;
	    }
	fgets (inline,MAX_SIG+10,readfp);
	}

    /* Add EOS delimiter to each signal name */
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	sig_ptr->signame[k] = '\0';
	}

    /* Assign position and bit information for each signal */
    for (i=0, sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward, i++) {
	sig_ptr->file_type.flags = 0;
	sig_ptr->file_pos = i + 9;	/* 9 characters are for the time, then signal one */
	sig_ptr->bits = 0;	/* = buf->TRA$W_BITLEN; */
	if (separator) i++;
	}

    /* Make the busses */
    read_make_busses (trace);

    /* Loop to read trace data and reformat */
    first_data = TRUE;
    while (!feof (readfp)) {
	fgets (inline, MAX_SIG+10, readfp);

	/* Remove leading hash mark (old fil_remove_hash) */
	while ( (pchar=strchr (inline,'#')) != 0 ) {
	    *pchar = '\0';
	    strcat (inline, pchar+2);
	    }

	if ( inline[0] && (inline[0] != '!') && (inline[0] != '\n') ) {
	    time = (atoi (inline) * 1000) / time_divisor;	/* ASCII traces are always in NS */
	    
	    if (first_data) {
		trace->start_time = time;
		}
	    trace->end_time = time;

	    /* save information on each signal */
	    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
		fil_string_to_value (sig_ptr, inline + sig_ptr->file_pos, &value);
		value.siglw.sttime.time = time;
		/*
		if (DTPRINT) printf ("time %d sig %s state %d\n", time, sig_ptr->signame, 
				     value.siglw.sttime.state);
				     */
		fil_add_cptr (sig_ptr, &value, !first_data);
		}

	    first_data = FALSE;
	    }
	}

    if (first_data) {
	dino_error_ack (trace,"No data in trace file");
	}

    read_trace_end (trace);

    /* Mark format as may have presumed binary */
    if (trace->fileformat != FF_DECSIM_Z) {
	trace->fileformat = FF_DECSIM_ASCII;
	}
    }

