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

#ifdef VMS
#include <file.h>
#include <unixio.h>
#endif VMS

#include <X11/Xlib.h>
#include <Xm/Xm.h>

#include "dinotrace.h"
#include "callbacks.h"

int	separator=0;		/* File temporary, true if | in trace file */

void free_signals (trace, sig_pptr, select)
    TRACE	*trace;
    SIGNAL	**sig_pptr;	/* Pointer to head of the linked list */
    int		select;		/* True = selectively pick trace's signals from the list */
{
    SIGNAL	*sig_ptr,*tmp_sig_ptr, *back_ptr=NULL;

    /* loop and free signal data and each signal structure */
    sig_ptr = *sig_pptr;
    while (sig_ptr) {
	if (!select || sig_ptr->trace == trace) {
	    /* Check head pointers */
	    if ( sig_ptr == sig_ptr->trace->dispsig )
		trace->dispsig = sig_ptr->forward;
	    if ( sig_ptr == sig_ptr->trace->firstsig )
		trace->firstsig = sig_ptr->forward;
	    if ( sig_ptr == global->delsig )
		global->delsig = sig_ptr->forward;

	    /* free the signal name */
	    if (sig_ptr->signame) XtFree (sig_ptr->signame);

	    /* free the signal data */
	    tmp_sig_ptr = sig_ptr;
	    sig_ptr = sig_ptr->forward;
	    *sig_pptr = sig_ptr;	/* Relink */
	    if (sig_ptr) sig_ptr->backward = back_ptr;
	
	    /* free the signal structure */
	    if (tmp_sig_ptr->copyof == NULL) XtFree (tmp_sig_ptr->bptr);
	    XtFree (tmp_sig_ptr);
	    }
	else {
	    sig_pptr = &(sig_ptr->forward);
	    back_ptr = sig_ptr;
	    sig_ptr = sig_ptr->forward;
	    }
	}
    }

void free_data (trace)
    TRACE	*trace;
{
    TRACE	*trace_ptr;

    if (DTPRINT) printf("In free_data - trace=%d\n",trace);

    if (!trace->loaded) return;
    trace->loaded = 0;

    /* free any deleted signals */
    free_signals (trace, &(global->delsig), TRUE);

    /* free any added signals in other traces from this trace */
    for (trace_ptr = global->trace_head; trace_ptr; trace_ptr = trace_ptr->next_trace) {
	free_signals (trace, &(trace_ptr->firstsig), TRUE);
	}

    /* free signal data and each signal structure */
    free_signals (trace, &(trace->firstsig), FALSE);
    trace->firstsig = NULL;
    trace->dispsig = NULL;
    trace->numsig = 0;
    }

void trace_read_cb(w,trace)
    Widget		w;
    TRACE		*trace;
{
    if (DTPRINT) printf("In trace_read_cb - trace=%d\n",trace);

    /* get the filename */
    get_file_name(trace);
    }

void trace_reread_cb(w,trace)
    Widget		w;
    TRACE		*trace;
{
    char *semi;

    if (!trace->loaded)
	trace_read_cb(w,trace);
    else {
	/* Drop ;xxx */
	if (semi = strchr(trace->filename,';'))
	    *semi = '\0';
	
	if (DTPRINT) printf("In trace_reread_cb - rereading file=%s\n",trace->filename);
	
	/* read the file */
	cb_fil_read(trace);
	}
    }

void cb_fil_read(trace)
    TRACE	*trace;
{
    if (DTPRINT) printf("In cb_fil_read trace=%d filename=%s\n",trace,trace->filename);
    
    /* Update directory name */
    strcpy (global->directory, trace->filename);
    file_directory (global->directory);

    /* Clear the data structures & the screen */
    XClearWindow(global->display, trace->wind);
    /* free memory associated with the data */
    free_data(trace);

    set_cursor (trace, DC_BUSY);
    XSync(global->display,0);
    
    /*
     ** Read in the trace file using the format selected by the user
     */
    if (file_format == FF_DECSIM)
	read_decsim(trace);
    else if (file_format == FF_TEMPEST)
	read_hlo_tempest(trace);
    
    /* Change the name on title bar to filename */
    change_title (trace);
    
    /*
     ** Clear the number of deleted signals and the starting signal
     */
    trace->numsigstart = 0;
    
    /* get applicable config files */
    config_read_defaults (trace);
    
    /*
     ** Clear the window and draw the screen with the new file
     */
    set_cursor (trace, DC_NORMAL);
    new_time (trace);	/* Realignes start and displays */
    }

void quit(w,trace,cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("Quitting\n");

    /* destroy all widgets in the hierarchy */
    XtDestroyWidget(trace->toplevel);

    /* all done */
    exit(1);
    }

void help_cb (w,trace,cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("in help_cb\n");

    dino_information_ack(trace, help_message());
    }

void read_decsim_ascii(trace)
    TRACE	*trace;
{
    FILE		*f_ptr;
    short int		*pshort;
    char		inline[MAX_SIG+20],tmp[MAX_SIG+20],*t1,*t2,*t3;  
    char		*sp8 = "        ";
    int			i,j,k,DONE;
    SIGNAL		*sig_ptr,*last_sig_ptr;

    separator = 0;

    /*** Open File For Reading ***/
    if (DTPRINT) printf("Opening File %s\n", trace->filename);
    f_ptr = fopen(trace->filename,"r");
    if ( f_ptr == NULL ) {
	if (DTPRINT) printf("Can't Find File %s\n", trace->filename);
	sprintf(message,"Can't open file %s",trace->filename);
	dino_error_ack(trace, message);
	return;
	}

    /* Check if header lines are present - if so, ignore */
    fgets(inline,MAX_SIG,f_ptr);
    while ( strncmp(&inline[1],sp8,8) ) {
	fgets(inline,MAX_SIG+10,f_ptr);
        }

    /* INLINE contains 1st signal line - use for signal number */
    trace->numsig = strlen(&inline[9]) - 1;

    /* Make a signal structure for each signal */
    trace->firstsig = NULL;
    for ( i=0; i < trace->numsig; i++) {
	sig_ptr = (SIGNAL *)XtMalloc(sizeof(SIGNAL));
	memset (sig_ptr, 0, sizeof (SIGNAL));

	/* initialize all the pointers that aren't NULL */
	sig_ptr->backward = last_sig_ptr;
	if (trace->firstsig==NULL)
	    trace->firstsig = sig_ptr;
	else 
	    last_sig_ptr->forward = sig_ptr;

	/* allow extra space in case becomes vector - don't know size yet */
	sig_ptr->signame = (char *)XtMalloc(10+MAXSIGLEN);

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
	    dino_error_ack(trace,"Signal Lengths > MAXSIGLEN");
	    k--;
	    }
	fgets(inline,MAX_SIG+10,f_ptr);
	}

    /* Add EOS delimiter to each signal name */
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	sig_ptr->signame[k] = '\0';
	}

    /* Assign position and bit information for each signal */
    for (i=0, sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward, i++) {
	sig_ptr->file_type = 0;
	sig_ptr->file_pos = i + 9;	/* 9 characters are for the time, then signal one */
	sig_ptr->bits = 0;	/* = buf->TRA$W_BITLEN; */
	sig_ptr->index = 0;
	if (separator) i++;
	}

    /* Get first line of trace data */
    if ( !fgets(inline,MAX_SIG+10,f_ptr) ) {
	dino_error_ack(trace,"No data in trace file");
	return;
	}

    /* Make the busses */
    if (DTPRINT) print_sig_names (NULL, trace);
    read_make_busses(trace);
    if (DTPRINT) print_sig_names (NULL, trace);

    /* Load initial data into structure unconditionally */
    fil_remove_hash(inline);
    fil_read_decsim_line(trace, inline, NOCHECK );
    trace->start_time = atoi(inline);

    /* Loop to read trace data and reformat */
    DONE = FALSE;
    while (!DONE) {
	/* If next line is EOF then this is the last line */
	if ( fgets(inline,MAX_SIG+10,f_ptr) == NULL ) {
	    DONE = TRUE;
	    fil_read_decsim_line(trace, tmp, NOCHECKEND );
	    }
	else {
	    /* if not a comment or a NULL line then read into data base */
	    if ( inline[0] != '!' && inline[0] != '\n' ) {
		fil_remove_hash (inline);
		strcpy (tmp, inline);
		fil_read_decsim_line (trace, inline, CHECK );
		trace->end_time = atoi(inline);
		/* if (DTPRINT) printf("end_time=%d\n",trace->end_time); */
		}
	    else {
		if (DTPRINT) printf("Null line or comment: %s\n",inline);
		}
	    }
	}

    read_trace_end (trace);

    fclose(f_ptr);
    }


/* Perform stuff at end of trace - common across all reading routines */

void read_trace_end (trace)
    TRACE	*trace;
{
    SIGNAL	*sig_ptr;

    if (DTPRINT) printf ("In read_trace_end\n");

    /* loop thru each signal */
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	/* Mark end of time */
	(*(SIGNAL_LW *)(sig_ptr->cptr)).time = EOT;

	/* re-initialize the cptr's to the bptr's */
	sig_ptr->cptr = sig_ptr->bptr;

	/* Init trace ptrs */
	sig_ptr->trace = trace;
	sig_ptr->color = 0;

	/* Create xstring of the name (to avoid calling again and again) */
	sig_ptr->xsigname = XmStringCreateSimple (sig_ptr->signame);
	sig_ptr->srch_ena = 0;
	}

    /* Misc */
    trace->dispsig = trace->firstsig;

    /* Make sure time is within bounds */
    if ( (global->time < trace->start_time) || (global->time > trace->end_time)) {
	global->time = trace->start_time;
	}

    /* Mark as loaded */
    trace->loaded = TRUE;
    }


fil_read_decsim_line (trace, line, flag )
    TRACE	*trace;
    char	line[MAX_SIG];
    int		flag;
{
    int		time,state,len;
    unsigned int value[3];
    int		bitcnt;
    unsigned int diff;
    register	char *cp;
    SIGNAL	*sig_ptr;

    /* compute the time for this line */
    time = atoi(line);
    /* if (DTPRINT) printf("time=%d\n",time); */

    /* loop thru each signal */
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	/* Check if signal or bus data structure is out of memory */
	diff = sig_ptr->cptr-sig_ptr->bptr;
	if (diff > BLK_SIZE/4*sig_ptr->blocks-4) {
	    sig_ptr->blocks++;
	    sig_ptr->bptr = (int *)XtRealloc(sig_ptr->bptr,sig_ptr->blocks*BLK_SIZE);
	    sig_ptr->cptr = sig_ptr->bptr+diff;
	    }
	
	/* decide whether this signal is a bus or a scalar */
	if (sig_ptr->bits == 0) {
	    /* scalar signal */  
	    switch( line[sig_ptr->file_pos] ) {
	      case '0': state = STATE_0; break;
	      case '1': state = STATE_1; break;
	      case 'U':
	      case 'u': state = STATE_U; break;
	      case 'Z': 
	      case 'z': state = STATE_Z; break;
	      default: 
		if (DTDEBUG) printf("Unknown State: %c\n",line[sig_ptr->file_pos]);
		}
	    
	    if ( flag != CHECK || 
		(*(SIGNAL_LW *)((sig_ptr->cptr)-1)).state != state )
		{
		(*(SIGNAL_LW *)(sig_ptr->cptr)).time = time;
		(*(SIGNAL_LW *)(sig_ptr->cptr)).state = state;
		(sig_ptr->cptr)++;
		}
	    }

	else {
	    /* Multi bit signal */
	    
	    /* determine the state of the bus */
	    state = STATE_0;
	    cp = line + sig_ptr->file_pos;
	    for (bitcnt=0; bitcnt <= sig_ptr->bits; bitcnt++, cp++) {
		switch (*cp) {
		  case 'u':
		  case 'U':
		    state = STATE_U; break;
		  case 'z': 
		  case 'Z': 
		    if (state != STATE_0 && state != STATE_Z) state = STATE_U;
		    else state = STATE_Z;
		    break;
		  default:
		    state = STATE_1;
		    }
		}
	    if (state != STATE_U && state != STATE_Z) {
		/* compute the value - need to fix for 64 and 96 */
		state = sig_ptr->type;
		value[0] = value[1] = value[2] = 0;
		cp = line + sig_ptr->file_pos;
		len = sig_ptr->bits;
		for (bitcnt=64; bitcnt <= (MIN(95,len)); bitcnt++)
		    value[2] = (value[2]<<1) + ((*cp++ == '1')?1:0);

		for (bitcnt=32; bitcnt <= (MIN(63,len)); bitcnt++)
		    value[1] = (value[1]<<1) + ((*cp++ == '1')?1:0);

		for (bitcnt=0; bitcnt <= (MIN(31,len)); bitcnt++)
		    value[0] = (value[0]<<1) + ((*cp++ == '1')?1:0);

		if (sig_ptr->type == STATE_B96) {
		    bitcnt = value[0];		/* Swap 2 & 0 */
		    value[0] = value[2];
		    value[2] = bitcnt;
		    }
		else if (sig_ptr->type == STATE_B64) {
		    bitcnt = value[0];		/* Swap 1 & 0 */
		    value[0] = value[1];
		    value[1] = bitcnt;
		    }
		}
	    
	    /* now see if data has changed since previous entry */
	    /* add if states !=, or if both are a bus and values != */
	    
	    /*************************DEBUG CONDITIONAL STATEMENT****************
	      if ( flag != CHECK )
	      printf("1\n");
	      if ( (*(SIGNAL_LW *)((sig_ptr->cptr)-sig_ptr->inc)).state != state )
	      printf("2\n");
	      if ( state == STATE_B32 && (*((sig_ptr->cptr)-1)) != value[0] ) 
	      printf("3\n");
	      if ( state == STATE_B64 && ( (*((sig_ptr->cptr)-2)) != value[0]
	      ||
	      (*((sig_ptr->cptr)-1)) != value[1] ) )
	      printf("4\n");
	      if ( state == STATE_B96 && ( (*((sig_ptr->cptr)-3)) != value[0]
	      ||
	      (*((sig_ptr->cptr)-2)) != value[1]
	      ||
	      (*((sig_ptr->cptr)-1)) != value[2] ) )
	      printf("5\n");
	      ********************************************************************/
	    
 	    if ( ( flag != CHECK )
		||
		( (*(SIGNAL_LW *)((sig_ptr->cptr)-sig_ptr->inc)).state != state )
		||
		( state == STATE_B32 && (*((sig_ptr->cptr)-1)) != value[0] ) 
		||
		( state == STATE_B64 && ( (*((sig_ptr->cptr)-2)) != value[0]
					 ||
					 (*((sig_ptr->cptr)-1)) != value[1] ) )
		||
		( state == STATE_B96 && ( (*((sig_ptr->cptr)-3)) != value[0]
					 ||
					 (*((sig_ptr->cptr)-2)) != value[1]
					 ||
					 (*((sig_ptr->cptr)-1)) != value[2] ) ))
		{
		/* add new bus entry to data structure */
		(*(SIGNAL_LW *)(sig_ptr->cptr)).time = time;
		(*(SIGNAL_LW *)(sig_ptr->cptr)).state = state;
		(sig_ptr->cptr)++;
		
		/* now add the value */
		switch(sig_ptr->type) {
		  case STATE_B32:
		    *((unsigned int *)(sig_ptr->cptr)) = value[0];
		    (sig_ptr->cptr)++;
		    break;
		    
		  case STATE_B64:
		    *((unsigned int *)(sig_ptr->cptr)) = value[0];
		    (sig_ptr->cptr)++;
		    *((unsigned int *)(sig_ptr->cptr)) = value[1];
		    (sig_ptr->cptr)++;
		    break;
		    
		  case STATE_B96:
		    *((unsigned int *)(sig_ptr->cptr)) = value[0];
		    (sig_ptr->cptr)++;
		    *((unsigned int *)(sig_ptr->cptr)) = value[1];
		    (sig_ptr->cptr)++;
		    *((unsigned int *)(sig_ptr->cptr)) = value[2];
		    (sig_ptr->cptr)++;
		    break;
		    
		  default:
		    if (DTPRINT) printf("Error: Bad sig_ptr->type=%d\n",sig_ptr->type);
		    }
		}
	    }
	} /* end for sig_ptr */
    }


fil_remove_hash(string)
    char	*string;
{
    char	*pchar;

    while ( (pchar=strchr(string,'#')) != 0 ) {
	*pchar = '\0';
	strcat(string,pchar+2);
	}
    }


void read_hlo_tempest(trace)
    TRACE	*trace;
{
    int		f_ptr,status;
    int		numBytes,numRows,numBitsRow,numBitsRowPad;
    int		sigChars,sigFlags,sigOffset,sigWidth;
    char		chardata[128];
    unsigned int	data[128];
    int		first_row;
    int		i,j,state;
    int		pad_len;
    unsigned int	time, last_time=EOT;
    int		odd_time = FALSE;
    SIGNAL	*sig_ptr,*last_sig_ptr;

    /*
     ** Open the binary file file for reading
     */
    if (DTPRINT) printf("Opening File %s\n", trace->filename);
    f_ptr = open(trace->filename,O_RDONLY,0);
    if ( f_ptr == -1 ) {
	if (DTPRINT) printf("Can't Open File %s\n", trace->filename);
	sprintf(message,"Can't open file %s",trace->filename);
	dino_error_ack(trace, message);
	return;
	}

    /*
     ** Read the file identification block
     */
    status = read(f_ptr, chardata, 4);
    chardata[4]='\0';
    if ( !status || strncmp(chardata,"BT0",3) ) {
	if (DTPRINT) printf("Bad File Format (=%s)\n", chardata);
	dino_error_ack(trace,"Bad File Format");
	return;
	}
    status = read(f_ptr, &numBytes, 4);
    status = read(f_ptr, &trace->numsig, 4);
    status = read(f_ptr, &numRows, 4);
    status = read(f_ptr, &numBitsRow, 4);
    status = read(f_ptr, &numBitsRowPad, 4);

    if (DTPRINT) {
	printf("File Sig=%s Bytes=%d Signals=%d Rows=%d Bits/Row=%d Bits/Row(pad)=%d\n",
	       chardata,numBytes,trace->numsig,numRows,numBitsRow,numBitsRowPad);
	}

    /*
     ** Read the signal description data - a signal description block is
     ** created for each signal describing the signal and containing ptrs
     ** for the trace data, current trace location, etc.
     */
    trace->firstsig=NULL;
    for(i=0;i<trace->numsig;i++) {
	status = read(f_ptr, &sigFlags, 4);
	status = read(f_ptr, &sigOffset, 4);
	status = read(f_ptr, &sigWidth, 4);
	status = read(f_ptr, &sigChars, 4);
	status = read(f_ptr, chardata, sigChars);
	chardata[sigChars] = '\0';
	
	if (DTPRINT) {
	    printf("sigFlags=%x sigOffset=%d sigWidth=%d sigChars=%d sigName=%s\n",
		   sigFlags,sigOffset,sigWidth,sigChars,chardata);
	    }
	
	/*
	 ** Initialize all pointers and other stuff in the signal
	 ** description block
	 */
	sig_ptr = (SIGNAL *)XtMalloc(sizeof(SIGNAL));
	sig_ptr->forward = NULL;
	if (trace->firstsig==NULL) {
	    trace->firstsig = sig_ptr;
	    sig_ptr->backward = NULL;
	    }
	else {
	    last_sig_ptr->forward = sig_ptr;
	    sig_ptr->backward = last_sig_ptr;
	    }
	sig_ptr->index = 0;
	sig_ptr->bits = sigWidth - 1;
	sig_ptr->file_pos = sigOffset;
	sig_ptr->file_type = sigFlags;
	
	/*
	 ** Copy the signal name, add EOS delimiter and initialize the pointer to it
	 */
	sig_ptr->signame = (char *)XtMalloc(10+sigChars); /* allow extra space in case becomes vector */
	for (j=0;j<sigChars;j++)
	    sig_ptr->signame[j] = chardata[j];
	sig_ptr->signame[sigChars] = '\0';
	
	/*
	 ** Read the pad bits
	 */
	pad_len = ( sigChars%8 ) ? 8 - (sigChars%8) : 0;
	status = read(f_ptr, chardata, pad_len);

	last_sig_ptr = sig_ptr;
	}

    /* Make the busses */
    if (DTPRINT) print_sig_names (NULL, trace);
    read_make_busses(trace);
    if (DTPRINT) print_sig_names (NULL, trace);

    /* Read the signal trace data
     * Pass 0-(numRows-1) reads the data, pass numRows processes last line */
    first_row = TRUE;
    for(i=0;i<numRows;i++) {
	int data_index,data_bit,value_index,value_bit,width,diff;
	unsigned int value[3];
	
	/* Read a row of data */
	status = read(f_ptr, data, numBitsRowPad/8);
	if (DTPRINT) {
	    printf("read: time=%d  data=%08x %08x\n", data[0], 
		   data[0], data[1]);
	    }
	
	/** Extract the phase - this will be used as a 'time' value and
	 ** is multiplied by 100 to make the trace easier to read
	 */
	time = data[0] * 2;
	if (time == last_time) time++;
	last_time = time;
	data_index = 0;
	data_bit = 0;
	
	/*
	 ** If this is the first row, save the starting and initial
	 ** time, else eventually the end time will be saved.
	 */
	if (first_row)
	    trace->start_time = time;
	else
	    trace->end_time = time;
	    
	/*
	 ** For each signal, the trace value is extracted - the width is
	 ** used to walk bit by bit through the row and the value for each
	 ** signal is accumulated
	 */
	for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	    /*
	     ** Check if signal or bus data structure is out of memory and
	     ** if it is, allocate another contiguous block to existing one
	     */
	    diff = sig_ptr->cptr-sig_ptr->bptr;
	    if (diff > BLK_SIZE/4*sig_ptr->blocks-4) {
		sig_ptr->blocks++;
		sig_ptr->bptr = (int *)XtRealloc(sig_ptr->bptr,sig_ptr->blocks*BLK_SIZE);
		sig_ptr->cptr = sig_ptr->bptr+diff;
		}
	    
	    width = sig_ptr->bits + 1;
	    
	    value[0] = value[1] = value[2] = 0;
	    value_index = value_bit = 0;
	    
	    while (width > 0) {
		/*
		 ** If the bit is set, accumulate that bits' value
		 */
		if ( data[data_index] & 1<<data_bit )
		    value[value_index] += 1<<value_bit;
		/* value[value_index] += (int)pow(2.0,(double)value_bit);*/
		
		/*
		 ** Increment the data and value bit pointers and jump into
		 ** next longword if necessary
		 */
		if ( ++data_bit >= 32) {
		    data_index++;
		    data_bit = 0;
		    }
		if ( ++value_bit >= 32) {
		    value_index++;
		    value_bit = 0;
		    }
		
		/*
		 ** Decrement the width of the signal by 1 bit
		 */
		width--;
		}  /* end while width */
	    
	    width = sig_ptr->bits + 1;
	    if ( width == 1 ) {
		switch(value[0]) {
		  case 0:
		    state = STATE_0;
		    break;
		  case 1:
		    state = STATE_1;
		    break;
		  default:
		    printf("Illegal state value: %d\n",value[0]);
		    break;
		    }
		
		/*
		 ** Determine if this scalar signal is either the first signal
		 ** or the last signal or has a different
		 ** state than the previous signal and if so, record it
		 */
		if ( (first_row) || (i==(numRows-1)) ||
		    ((*(SIGNAL_LW *)((sig_ptr->cptr)-sig_ptr->inc)).state != state) )
		    {
		    (*(SIGNAL_LW *)(sig_ptr->cptr)).time = time;
		    (*(SIGNAL_LW *)(sig_ptr->cptr)).state = state;
		    (sig_ptr->cptr)++;
		    }
		}
	    else {
		if ( width <= 32 )
		    state = STATE_B32;
		else if ( width <= 64 )
		    state = STATE_B64;
		else if ( width <= 96 )
		    state = STATE_B96;
		else
		    printf("Illegal bus width: %d\n",width);
		
		/*
		 ** Determine if this bus signal is either the first signal
		 ** row or the last signal or has a different
		 ** state than the previous signal or has the same state and
		 ** has different values and if so, record it
		 */
		if ( (first_row) || (i==(numRows-1))
		    ||
		    ( (*(SIGNAL_LW *)((sig_ptr->cptr)-sig_ptr->inc)).state != state )
		    ||
		    ( state == STATE_B32 && (*((sig_ptr->cptr)-1)) != value[0] ) 
		    ||
		    ( state == STATE_B64 && ( (*((sig_ptr->cptr)-2)) != value[0]
					     ||
					     (*((sig_ptr->cptr)-1)) != value[1] ) )
		    ||
		    ( state == STATE_B96 && ( (*((sig_ptr->cptr)-3)) != value[0]
					     ||
					     (*((sig_ptr->cptr)-2)) != value[1]
					     ||
					     (*((sig_ptr->cptr)-1)) != value[2] ) ))
		    {
		    
		    
		    sig_ptr->type = state;
		    
		    (*(SIGNAL_LW *)(sig_ptr->cptr)).time = time;
		    (*(SIGNAL_LW *)(sig_ptr->cptr)).state = state;
		    (sig_ptr->cptr)++;
		    
		    /* now add the value */
		    switch(state) {
		      case STATE_B32:
			*((unsigned int *)(sig_ptr->cptr)) = value[0];
			(sig_ptr->cptr)++;
			sig_ptr->inc = 2;
			break;
			
		      case STATE_B64:
			*((unsigned int *)(sig_ptr->cptr)) = value[0];
			(sig_ptr->cptr)++;
			*((unsigned int *)(sig_ptr->cptr)) = value[1];
			(sig_ptr->cptr)++;
			sig_ptr->inc = 3;
			break;
			
		      case STATE_B96:
			*((unsigned int *)(sig_ptr->cptr)) = value[0];
			(sig_ptr->cptr)++;
			*((unsigned int *)(sig_ptr->cptr)) = value[1];
			(sig_ptr->cptr)++;
			*((unsigned int *)(sig_ptr->cptr)) = value[2];
			(sig_ptr->cptr)++;
			sig_ptr->inc = 4;
			break;
			} /* end switch */
		    } /* end if check */
		
		} /* end if scalar */
	    
	    }/* end for sig_ptr */
	
	first_row = FALSE;
	}/* end for */

    /* Close the file */
    close(f_ptr);

    /* Now add EOT to each signal and reset the cptr */
    read_trace_end (trace);
    }

/**********************************************************************
 *	update_signal_states
 **********************************************************************/

void	update_signal_states (trace)
    TRACE	*trace;
{
    SIGNAL *sig_ptr;
    SIGNAL_LW	*cptr;
    int rise1=0, fall1=0, rise2=0, fall2=0;

    if (DTPRINT) printf("In update_signal_states\n");

    /* don't do anything if no file is loaded */
    if (!trace->loaded) return;

    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	if (NULL != (sig_ptr->decode = find_signal_state (trace, sig_ptr->signame))) {
	    /* if (DTPRINT) printf("Signal %s is patterned\n",sig_ptr->signame); */
	    }
	/* else if (DTPRINT) printf("Signal %s  no pattern\n",sig_ptr->signame); */
	}

    /* Determine period, rise point and fall point of first signal */
    sig_ptr = (SIGNAL *)trace->firstsig;
    cptr = (SIGNAL_LW *)sig_ptr->cptr;
    /* Skip first one, as is not representative of period */
    if ( cptr->time != EOT) cptr += sig_ptr->inc;
    while( cptr->time != EOT) {
	switch (cptr->state) {
	  case STATE_1:
	    if (!rise1) rise1 = cptr->time;
	    else if (!rise2) rise2 = cptr->time;
	    break;
	  case STATE_0:
	    if (!fall1) fall1 = cptr->time;
	    else if (!fall2) fall2 = cptr->time;
	    break;
	  case STATE_B32:
	  case STATE_B64:
	  case STATE_B96:
	    if (!rise1) rise1 = cptr->time;
	    else if (!rise2) rise2 = cptr->time;
	    if (!fall1) fall1 = cptr->time;
	    else if (!fall2) fall2 = cptr->time;
	    break;
	    }
	cptr += sig_ptr->inc;
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
    update_globals();
    }

