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
#include <file.h>
#include <unixio.h>

#ifdef VMS
#include <math.h> /* removed for Ultrix support... */
#endif VMS

#include <X11/Xlib.h>
#include <X11/Xm.h>

#include "dinotrace.h"

int	separator=0;		/* File temporary, true if | in trace file */

void free_data(trace)
    TRACE		*trace;
{
    int		i;
    SIGNAL_SB	*sig_ptr,*tmp_sig_ptr;

    if (DTPRINT) printf("In free_data - trace=%d\n",trace);

    /* Initialize cursor array to zero */
    trace->numcursors = 0;
    for (i=0; i<MAX_CURSORS; i++)
	trace->cursors[i] = 0;

    /* free bus array */
    XtFree(trace->bus);
    trace->bus = NULL;

    /* free signal array */
    XtFree(trace->signame);
    trace->signame = NULL;

    /* loop and free signal data and each signal structure */
    sig_ptr = trace->firstsig;
    while (sig_ptr) {
	/* free the signal data */
	XtFree(sig_ptr->bptr);
	
	tmp_sig_ptr = sig_ptr;
	sig_ptr = sig_ptr->forward;
	
	/* free the signal structure */
	XtFree(tmp_sig_ptr);
	}
    trace->firstsig = NULL;
    trace->dispsig = NULL;
    trace->loaded = 0;
    trace->numsig = 0;
    trace->numsigdel = 0;
    }

void clear_display(w,trace)
    Widget		w;
    TRACE		*trace;
{
    char	title[100];

    if (DTPRINT) printf("In clear_display - trace=%d\n",trace);

    /* clear the screen */
    XClearWindow(trace->display, trace->wind);

    /* free memory associated with the data */
    free_data(trace);

    /* change the name on title bar back to the trace */
    change_title (trace);
    }

void delete_display(w,trace)
    Widget		w;
    TRACE		*trace;
{
    if (DTPRINT) printf("In delete_display - trace=%d\n",trace);

    /* remove the display */
    XtUnmanageChild(trace->main);

    /* free memory associated with the data */
    free_data(trace);

    /* destroy all the widgets created for the screen */
    XtDestroyWidget(trace->main);

    /* free the display structure */
    XtFree(trace);

    /* all done */
    exit(1);
    }

void cb_read_trace(w,trace)
    Widget		w;
    TRACE		*trace;
{
    char	*filename;

    if (DTPRINT) printf("In cb_read_trace - trace=%d\n",trace);

    /* get the filename */
    get_file_name(trace);
    }

void cb_reread_trace(w,trace)
    Widget		w;
    TRACE		*trace;
{
    char *semi;

    if (!trace->loaded)
	cb_read_trace(w,trace);
    else {
	/* Drop ;xxx */
	if (semi = strchr(trace->filename,';'))
	    *semi = '\0';
	
	if (DTPRINT) printf("In cb_reread_trace - rereading file=%s\n",trace->filename);
	
	/* read the file */
	cb_fil_read(trace);
	}
    }

void quit(w,trace,cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("Quitting\n");

    /* destroy all widgets in the hierarchy */
    XtDestroyWidget(toplevel);

    /* all done */
    exit(1);
    }

void read_DECSIM_ascii(trace)
    TRACE	*trace;
{
    FILE		*f_ptr;
    short int		len,*pshort;
    char		inline[MAX_SIG+10],tmp[MAX_SIG+10],*t1,*t2,*t3,sp8[9];  
    int			i,j,k,val,state,INIT,DONE,ZBUS;
    SIGNAL_SB		*sig_ptr,*last_sig_ptr;

    separator = 0;

    /* initialize the string to 8 spaces */
    for (i=0;i<8;i++)
	sp8[i] = ' ';

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
    while ( strncmp(&inline[1],sp8,8) )
	fgets(inline,MAX_SIG+10,f_ptr);

    /* INLINE contains 1st signal line - use for signal number */
    trace->numsig = strlen(&inline[9]) - 1;

    /* Allocate memory for signames - assume array[numsig][MAXSIGLEN] */
    if ( !(trace->signame = (SIGNALNAMES *)malloc(trace->numsig * MAXSIGLEN)) ) {
	if (DTPRINT) printf("Can't allocate memory for signal names.\n");
	dino_error_ack(trace,"Can't allocate memory for signal names.");
	return;
	}

    /* allocate memory for bus array - one short per signal */
    if ( !(trace->bus = (short *)malloc(trace->numsig * sizeof(short))) ) {
	if (DTPRINT) printf("Can't allocate memory for bus.\n");
	dino_error_ack(trace,"Can't allocate memory for bus.");
	return;
	}

    /* initialize bus array to zero */
    pshort = trace->bus;
    for (i=0; i<trace->numsig; i++)
	*(pshort+i) = 0;

    /* Read Signal Names Into Structure */

    k=0;
    while ( inline[0] == '!' ) {
	/*** i=char read - j=char written - k=#lines read ***/
	for (i=0,j=0; j<trace->numsig; i++,j++) {
	    if (separator)
		i++;
	    (*(trace->signame)).array[j][k] = inline[9+i];
	    }
	if (++k >= MAXSIGLEN) {
	    dino_error_ack(trace,"Signal Lengths > MAXSIGLEN");
	    k--;
	    }
	fgets(inline,MAX_SIG+10,f_ptr);
	}

    /* Add EOS delimiter to each signal name */
    for ( i=0; i < trace->numsig; i++) {
	(*(trace->signame)).array[i][k] = '\0';
	}

    /* Calculate the bus array */
    if (TRUE) {
	pshort = trace->bus;
	j = 0;
	for (i=0; i<trace->numsig-1; i++) {
	    /*** t1 points to 1st char in signame[i] ***/
	    if ( !(t1 = strrchr((*(trace->signame)).array[i],' ')) )
		t1 = (*(trace->signame)).array[i];
	    else
		t1++;
	    /*** t2 points to 1st char in signame[i+1] ***/
	    if ( !(t2 = strrchr((*(trace->signame)).array[i+1],' ')) )
		t2 = (*(trace->signame)).array[i+1];
	    else
		t2++;
	    /*** t3 points to '<' in signame[i] ***/
	    t3 = strchr((*(trace->signame)).array[i],'<');
	    if ( t3 && strncmp(t1,t2,t3-t1+1) == 0 
		&& (abs(atoi(t3+1) - atoi(t3-t1+t2+1)) == 1)) /* added by WPS */
		(*(pshort+j))++;
	    else
		j++;
	    }
	}

    /* adjust numsig accounting for any busses present */
    pshort = trace->bus;
    for (i=0; i<trace->numsig; i++)
	trace->numsig -= *(pshort+i);

    /* adjust numsig if a separator is present in the trace file */
    if (separator)
	trace->numsig -= (int)(trace->numsig+1)/2;

    /* Get first line of trace data */
    if ( !fgets(inline,MAX_SIG+10,f_ptr) ) {
	dino_error_ack(trace,"No data in trace file");
	return;
	}

    /* Allocate 1 block of memory to each signal initially */
    for (i=0; i < trace->numsig; i++) {
	/* allocate the memory */
	if ( !(sig_ptr = (SIGNAL_SB *)malloc(sizeof(SIGNAL_SB))) ) {
	    dino_error_ack(trace,"Can't allocate memory for SIGNAL_SB.");
	    return;
	    }
	
	/* initialize all the pointers */
	sig_ptr->forward = NULL;
	if (trace->firstsig==NULL) {
	    trace->firstsig = sig_ptr;
	    sig_ptr->backward = NULL;
	    }
	else {
	    last_sig_ptr->forward = sig_ptr;
	    sig_ptr->backward = last_sig_ptr;
	    }

	sig_ptr->signame = (*(trace->signame)).array[i];
	if ( !(sig_ptr->bptr = (int *)malloc(BLK_SIZE)) ) {
	    if (DTPRINT) printf("Can't allocate memory for signal data.\n");
	    dino_error_ack(trace,"Can't allocate memory for signal data.");
	    return;
	    }
	sig_ptr->cptr = sig_ptr->bptr;
	sig_ptr->blocks = 1;
	last_sig_ptr = sig_ptr;
	}

    /* make SIGNAL_SB signame point to correct signal name */
    pshort = trace->bus;
    sig_ptr = trace->firstsig;
    for (i=0,j=0; i < trace->numsig; i++) {
	sig_ptr->signame = (*(trace->signame)).array[j];
	if (*pshort == 0) {
	    sig_ptr->inc = 1;
	    sig_ptr->type = 0;
	    sig_ptr->ind_e = 0;
	    }
	else {
	    /*** t1 points to '>' in signame[j] ***/
	    t1 = strchr((*(trace->signame)).array[j],'>');
	    *t1 = '\0';
	    strcat(t1,":");
	    /*** t2 points to '<' in signame[j] ***/
	    t2 = strchr((*(trace->signame)).array[j+(*pshort)],'<');
	    strcat(t1,t2+1);
	    /*	    sig_ptr->signame += (strlen(t2)-1); */
	    sig_ptr->inc = (*pshort < 32) ? 2 : (*pshort < 64) ? 3 : 4;
	    sig_ptr->type = (*pshort < 32) ? STATE_B32 :
		(*pshort < 64) ? STATE_B64 : STATE_B96;
	    for (t1=sig_ptr->signame; *t1==' '; t1++);
	    sig_ptr->ind_e = 0;
	    j += *pshort;
	    } 
	j++;
	pshort++;
	sig_ptr = sig_ptr->forward;
	}

    /* Load initial data into structure unconditionally */
    check_input(inline);
    cvd2d(trace, inline, trace->bus, NOCHECK );
    trace->start_time = atoi(inline);
    trace->time = atoi(inline);

    /* Drop leading spaces */
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	for (t1=sig_ptr->signame; *t1==' '; t1++);
	strcpy (sig_ptr->signame, t1);	/* side effects */
	}

    if (DTPRINT) read_trace_dump (trace);

    /* Loop to read trace data and reformat */
    DONE = FALSE;
    while (!DONE) {
	/* If next line is EOF then this is the last line */
	if ( fgets(inline,MAX_SIG+10,f_ptr) == NULL ) {
	    DONE = TRUE;
	    cvd2d(trace, tmp, trace->bus, NOCHECKEND );
	    }
	else {
	    /* if not a comment or a NULL line then read into data base */
	    if ( inline[0] != '!' && inline[0] != '\n' ) {
		check_input(inline);
		strcpy(tmp, inline);
		cvd2d(trace, inline, trace->bus, CHECK );
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

read_trace_end (trace)
    TRACE	*trace;
{
    int		sigcnt;
    SIGNAL_SB	*sig_ptr;

    if (DTPRINT) printf ("In read_trace_end\n");

    /* loop thru each signal */
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	/* Mark end of time */
	(*(SIGNAL_LW *)(sig_ptr->cptr)).time = EOT;

	/* re-initialize the cptr's to the bptr's */
	sig_ptr->cptr = sig_ptr->bptr;

	/* Create xstring of the name (to avoid calling again and again) */
	sig_ptr->xsigname = XmStringCreateSimple (sig_ptr->signame);
	sig_ptr->color = 0;
	}

    /* Misc */
    trace->dispsig = trace->firstsig;

    /* Mark as loaded */
    trace->loaded = TRUE;
    }


read_trace_dump (trace)
    TRACE	*trace;
{
    SIGNAL_SB	*sig_ptr;

    if (DTPRINT) printf ("In read_trace_dump\n");

    printf ("  Number of signals = %d\n", trace->numsig);

    /* loop thru each signal */
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	printf (" Sig '%s'  ty=%d inc=%d btyp=%d bpos=%d bits=%d\n",
		sig_ptr->signame, sig_ptr->type, sig_ptr->inc,
		sig_ptr->binary_type, sig_ptr->binary_pos, sig_ptr->bits
		);
	}
    }


cvd2d(trace, line, bus, flag )
    TRACE	*trace;
    char		line[MAX_SIG];
    short		*bus;
    int		flag;
{
    int		time,state,i,j=0,prev=0,inc,len;
    unsigned int value[3];
    int		bitcnt;
    char	zarr[97];
    char	tmp[100];
    unsigned int *vptr,diff;
    register	char *cp;
    SIGNAL_SB	*sig_ptr;
    SIGNAL_LW	tmp_sig;

    /* initialize the string to 96 Z's */
    for (i=0;i<96;i++)
	zarr[i] = 'Z';

    /* point to the first signal in the display */
    sig_ptr = trace->firstsig;

    /* compute the time for this line */
    time = atoi(line);

    /* if (DTPRINT) printf("time=%d\n",time); */

    /* adjust pointer into trace if separator is present in trace file */
    if (separator) j++;

    /* loop thru each signal */
    for (i=0; i<trace->numsig; i++) {
	/* Check if signal or bus data structure is out of memory */
	diff = sig_ptr->cptr-sig_ptr->bptr;
	if (diff > BLK_SIZE/4*sig_ptr->blocks-4) {
	    sig_ptr->blocks++;
	    sig_ptr->bptr = (int *)realloc(sig_ptr->bptr,sig_ptr->blocks*BLK_SIZE);
	    sig_ptr->cptr = sig_ptr->bptr+diff;
	    }
	
	/* decide whether this signal is a bus or a scalar */
	if ( *(bus+i) ) {
	    /* copy bus string to a temp location */
	    strncpy(tmp,&line[j+9],*(bus+i)+1);
	    tmp[*(bus+i)+1] = '\0';
	    
	    /* determine the state of the bus */
	    if ( strncmp(tmp,zarr,*(bus+i)+1) == 0 )
		state = STATE_Z;
	    else if (strchr(tmp,'U') != 0 )
		state = STATE_U;
	    else if (strchr(tmp,'Z') != 0 )
		state = STATE_U;
	    else {
		/* compute the value - need to fix for 64 and 96 */
		state = sig_ptr->type;
		value[0] = value[1] = value[2] = 0;
		len = *(bus+i);
		cp = tmp+0;
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
	    j += *(bus+i)+1;
	    if (separator) j++;
	    
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
	else {
	    /* scalar signal */  
	    switch( line[j+9] ) {
	      case '0': state = STATE_0; break;
	      case '1': state = STATE_1; break;
	      case 'U': state = STATE_U; break;
	      case 'Z': state = STATE_Z; break;
	      default: 
		if (DTDEBUG) printf("Unknown State: %c\n",line[9+j]);
		}
	    j++;
	    if (separator) j++;
	    
	    if ( flag != CHECK || 
		(*(SIGNAL_LW *)((sig_ptr->cptr)-1)).state != state )
		{
		(*(SIGNAL_LW *)(sig_ptr->cptr)).time = time;
		(*(SIGNAL_LW *)(sig_ptr->cptr)).state = state;
		(sig_ptr->cptr)++;
		}
	    }
	
	sig_ptr = sig_ptr->forward;
	} /* end for */
    }


check_input(string)
    char	*string;
{
    char	*pchar;

    while ( (pchar=strchr(string,'#')) != 0 ) {
	*pchar = '\0';
	strcat(string,pchar+2);
	}
    }


void read_HLO_TEMPEST(trace)
    TRACE	*trace;
{
    int		f_ptr,status;
    int		numBytes,numRows,numBitsRow,numBitsRowPad;
    int		sigChars,sigFlags,sigOffset,sigWidth;
    short int	len,*pshort;
    char		chardata[128],*env;
    unsigned int	data[128];
    int		i,j,k,val,state,INIT,DONE,ZBUS;
    int		pad_len;
    SIGNAL_SB	*sig_ptr,*last_sig_ptr;

    /*
     ** Set the debugging variables
     */
    env = getenv("DT_FILE_INPUT");

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
    if ( !status ) {
	if (DTPRINT) printf("Bad File Format\n");
	dino_error_ack(trace,"Bad File Format");
	return;
	}
    status = read(f_ptr, &numBytes, 4);
    status = read(f_ptr, &trace->numsig, 4);
    status = read(f_ptr, &numRows, 4);
    status = read(f_ptr, &numBitsRow, 4);
    status = read(f_ptr, &numBitsRowPad, 4);

    if (env != NULL) {
	chardata[4]='\0';
	printf("File Sig=%s Bytes=%d Signals=%d Rows=%d Bits/Row=%d Bits/Row(pad)=%d\n",
	       chardata,numBytes,trace->numsig,numRows,numBitsRow,numBitsRowPad);
	}

    /*
     ** Allocate memory for signames - assume array[numsig][MAXSIGLEN]
     */
    if ( !(trace->signame = (SIGNALNAMES *)malloc(trace->numsig * MAXSIGLEN)) ) {
	printf("Can't allocate memory for signal names.\n");
	dino_error_ack(trace,"Can't allocate memory for signal names.");
	return;
	}

    /*
     ** Read the signal description data - a signal description block is
     ** created for each signal describing the signal and containing ptrs
     ** for the trace data, current trace location, etc.
     */
    for(i=0;i<trace->numsig;i++) {
	status = read(f_ptr, &sigFlags, 4);
	status = read(f_ptr, &sigOffset, 4);
	status = read(f_ptr, &sigWidth, 4);
	status = read(f_ptr, &sigChars, 4);
	status = read(f_ptr, chardata, sigChars);
	chardata[sigChars] = '\0';
	
	if (env != NULL) {
	    printf("sigFlags=%d sigOffset=%d sigWidth=%d sigChars=%d\n",
		   sigFlags,sigOffset,sigWidth,sigChars);
	    printf("sigName=%s\n",chardata);
	    }
	
	/*
	 ** Allocate memory for a signal description block
	 */
	if ( !(sig_ptr = (SIGNAL_SB *)malloc(sizeof(SIGNAL_SB))) ) {
	    dino_error_ack(trace,"Can't allocate memory for SIGNAL_SB.");
	    return;
	    }

	/*
	 ** Initialize all pointers and other stuff in the signal
	 ** description block
	 */
	sig_ptr->forward = NULL;
	if (trace->firstsig==NULL) {
	    trace->firstsig = sig_ptr;
	    sig_ptr->backward = NULL;
	    }
	else {
	    last_sig_ptr->forward = sig_ptr;
	    sig_ptr->backward = last_sig_ptr;
	    }
	sig_ptr->inc = 1;
	sig_ptr->type = 0;
	sig_ptr->ind_e = sigWidth;
	
	/*
	 ** Copy the signal name into the signal name array, add an
	 ** EOS delimiter and initialize the pointer to it
	 */
	for (j=0;j<sigChars;j++)
	    (*(trace->signame)).array[i][j] = chardata[j];
	(*(trace->signame)).array[i][sigChars] = '\0';
	sig_ptr->signame = (*(trace->signame)).array[i];
	
	/*
	 ** Allocate first memory block for signal data and initialize
	 ** pointers and other stuff
	 */
	if ( !(sig_ptr->bptr = (int *)malloc(BLK_SIZE)) ) {
	    printf("Can't allocate memory for signal data.\n");
	    dino_error_ack(trace,"Can't allocate memory for signal data.");
	    return;
	    }
	sig_ptr->cptr = sig_ptr->bptr;
	sig_ptr->blocks = 1;
	last_sig_ptr = sig_ptr;
	
	/*
	 ** Read the pad bits
	 */
	pad_len = ( sigChars%8 ) ? 8 - (sigChars%8) : 0;
	status = read(f_ptr, chardata, pad_len);
	}

    /*
     ** Read the signal trace data
     */
    for(i=0;i<numRows;i++) {
	int data_index,data_bit,value_index,value_bit,width,diff;
	unsigned int time,value[3];
	
	/*
	 ** Read a row of data
	 */
	status = read(f_ptr, data, numBitsRowPad/8);
	
	/*
	 ** Extract the phase - this will be used as a 'time' value and
	 ** is multiplied by 100 to make the trace easier to read
	 */
	time = data[0]*100;
	data_index = 0;
	data_bit = 0;
	
	/*
	 ** If this is the first row, save the starting and initial
	 ** time, else eventually the end time will be saved.
	 */
	if (i==0)
	    trace->time = trace->start_time = time;
	else
	    trace->end_time = time;
	
	if (env != NULL) {
	    printf("time=%d\n",time);
	    printf("data=%s\n",(char *)data);
	    }
	
	/*
	 ** For each signal, the trace value is extracted - the width is
	 ** used to walk bit by bit through the row and the value for each
	 ** signal is accumulated
	 */
	sig_ptr = trace->firstsig;
	while (sig_ptr != NULL) {
	    /*
	     ** Check if signal or bus data structure is out of memory and
	     ** if it is, allocate another contiguous block to existing one
	     */
	    diff = sig_ptr->cptr-sig_ptr->bptr;
	    if (diff > BLK_SIZE/4*sig_ptr->blocks-4) {
		sig_ptr->blocks++;
		sig_ptr->bptr = (int *)realloc(sig_ptr->bptr,sig_ptr->blocks*BLK_SIZE);
		sig_ptr->cptr = sig_ptr->bptr+diff;
		}
	    
	    width = sig_ptr->ind_e;
	    
	    value[0] = value[1] = value[2] = 0;
	    value_index = value_bit = 0;
	    
	    while (width > 0) {
		/*
		 ** If the bit is set, accumulate that bits' value
		 */
		if ( data[data_index] & 1<<data_bit )
		    value[value_index] += (int)pow(2.0,(double)value_bit);
		
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
	    
	    width = sig_ptr->ind_e;
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
		 ** (i==0) or the last signal (i==numRows-1) or has a different
		 ** state than the previous signal and if so, record it
		 */
 		if ( (i==0) || (i==numRows-1) ||
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
		 ** (i==0) or the last signal (i==numRows-1) or has a different
		 ** state than the previous signal or has the same state and
		 ** has different values and if so, record it
		 */
 		if ( (i==0) || (i==numRows-1)
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
	    
	    /*
	     ** Get next signal block
	     */
	    sig_ptr = (SIGNAL_SB *)sig_ptr->forward;
	    
	    }/* end while */
	}/* end for */

    /*
     ** Now add EOT to each signal and reset the cptr
     */
    read_trace_end (trace);

    /*
     ** Close the file
     */
    close(f_ptr);
    }

/**********************************************************************
 *	update_signal_states
 **********************************************************************/

void	update_signal_states (trace)
    TRACE	*trace;
{
    SIGNAL_SB *sig_ptr;
    SIGNAL_LW	*cptr;
    int rise1=0, fall1=0, rise2=0, fall2=0, period;
    int			xstarttemp;
    char *t1;

    if (DTPRINT) printf("In update_signal_states\n");

    /* don't do anything if no file is loaded */
    if (!trace->loaded) return;

    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	if (NULL != (sig_ptr->decode = find_signal_state (trace, sig_ptr->signame))) {
	    if (DTPRINT) printf("Signal %s is patterned\n",sig_ptr->signame);
	    }
	/* else if (DTPRINT) printf("Signal %s  no pattern\n",sig_ptr->signame); */
	}

    /* Determine period, rise point and fall point of first signal */
    cptr = (SIGNAL_LW *)((SIGNAL_SB *)trace->firstsig)->cptr;
    /* Skip first one, as is not representative of period */
    if ( cptr->time != 0x1FFFFFFF) cptr++;
    while( cptr->time != 0x1FFFFFFF) {
	if (cptr->state == STATE_1) {
	    if (!rise1) rise1 = cptr->time;
	    else if (!rise2) rise2 = cptr->time;
	    }
	if (cptr->state == STATE_0) {
	    if (!fall1) fall1 = cptr->time;
	    else if (!fall2) fall2 = cptr->time;
	    }
	cptr++;
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

    /* Calculate xstart from longest signal name */
    xstarttemp=XSTART_MIN;
    for (sig_ptr = (SIGNAL_SB *)trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	for (t1=sig_ptr->signame; *t1==' '; t1++);
	if (strncmp (t1, "%NET.",5)==0) t1+=5;
	/* if (DTPRINT) printf("Signal = '%s'  xstart=%d\n",t1,xstarttemp); */
	if (xstarttemp < XTextWidth(trace->text_font,t1,strlen(t1)))
	    xstarttemp = XTextWidth(trace->text_font,t1,strlen(t1));
	}
    trace->xstart = xstarttemp + XSTART_MARGIN;
    }

