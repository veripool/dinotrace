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
 *
 */


#include <stdio.h>
#include <file.h>

#ifdef VMS
#include <math.h> /* removed for Ultrix support... */
#endif VMS

#include <X11/DECwDwtApplProg.h>
#include <X11/Xlib.h>

#include "dinotrace.h"

void
read_DECSIM(ptr)
DISPLAY_SB	*ptr;
{
FILE			*f_ptr;
short int		len,*pshort;
char			inline[MAX_SIG+10],tmp[MAX_SIG+10],*t1,*t2,*t3,sp8[9];  
int			i,j,k,val,state,INIT,DONE,ZBUS;
SIGNAL_SB		*tmp_sig_ptr,*last_sig_ptr;

    /* initialize the string to 8 spaces */
    for (i=0;i<8;i++)
	sp8[i] = ' ';

    /*** Open File For Reading ***/
    if (DTPRINT) printf("Opening File %s\n", ptr->filename);
    f_ptr = fopen(ptr->filename,"r");
    if ( f_ptr == NULL )
    {
	if (DTPRINT) printf("Can't Find File %s\n", ptr->filename);
	dino_message_ack(ptr,"Can't open file");
	ptr->filename[0] = '\0';
	return;
    }

/* Check if header lines are present - if so, ignore */

    fgets(inline,MAX_SIG,f_ptr);
    while ( strncmp(&inline[1],sp8,8) )
      fgets(inline,MAX_SIG+10,f_ptr);

/* INLINE contains 1st signal line - use for signal number */

    ptr->numsig = strlen(&inline[9]) - 1;

/* Allocate memory for signames - assume array[numsig][MAXSIGLEN] */

    if ( !(ptr->signame = (SIGNALNAMES *)malloc(ptr->numsig * MAXSIGLEN)) )
    {
	printf("Can't allocate memory for signal names.\n");
	dino_message_ack(ptr,"Can't allocate memory for signal names.");
	return;
    }

/* allocate memory for bus array - one short per signal */

    if ( !(ptr->bus = (short *)malloc(ptr->numsig * sizeof(short))) )
    {
	printf("Can't allocate memory for bus.\n");
	dino_message_ack(ptr,"Can't allocate memory for bus.");
	return;
    }

/* initialize bus array to zero */

    pshort = ptr->bus;
    for (i=0; i<ptr->numsig; i++)
	*(pshort+i) = 0;

/* initialize cursor array to zero */

    ptr->numcursors = 0;
    for (i=0; i<MAX_CURSORS; i++)
	ptr->cursors[i] = 0;

/* Read Signal Names Into Structure */

    k=0;
    while ( inline[0] == '!' )
    {
	/*** i=char read - j=char written - k=#lines read ***/
        for (i=0,j=0; j<ptr->numsig; i++,j++)
	{
	    if (ptr->separator)
		i++;
            (*(ptr->signame)).array[j][k] = inline[9+i];
	}
        if (++k >= MAXSIGLEN)
	{
	    dino_message_info(ptr,"Signal Lengths > MAXSIGLEN");
            k--;
        }
    fgets(inline,MAX_SIG+10,f_ptr);
    }

/* Add EOS delimiter to each signal name */

    for ( i=0; i < ptr->numsig; i++)
      (*(ptr->signame)).array[i][k] = '\0';

/* Calculate the bus array */

    if ( TRUE )
	{
	pshort = ptr->bus;
	j = 0;
	for (i=0; i<ptr->numsig-1; i++)
	{
	    /*** t1 points to 1st char in signame[i] ***/
	    if ( !(t1 = strrchr((*(ptr->signame)).array[i],' ')) )
		t1 = (*(ptr->signame)).array[i];
	    else
		t1++;
	    /*** t2 points to 1st char in signame[i+1] ***/
	    if ( !(t2 = strrchr((*(ptr->signame)).array[i+1],' ')) )
		t2 = (*(ptr->signame)).array[i+1];
	    else
		t2++;
	    /*** t3 points to '<' in signame[i] ***/
	    t3 = strchr((*(ptr->signame)).array[i],'<');
	    if ( t3 && strncmp(t1,t2,t3-t1+1) == 0 )
		(*(pshort+j))++;
	    else
		j++;
	}
    }

/* adjust numsig accounting for any busses present */

    pshort = ptr->bus;
    for (i=0; i<ptr->numsig; i++)
        ptr->numsig -= *(pshort+i);

/* adjust numsig if a separator is present in the trace file */

    if (ptr->separator)
	ptr->numsig -= (int)(ptr->numsig+1)/2;

/* Get first line of trace data */

    if ( !fgets(inline,MAX_SIG+10,f_ptr) )
    {
	dino_message_ack(ptr,"No data in trace file");
        return;
    }

/* Allocate 1 block of memory to each signal initially */

    ptr->sig.backward = NULL;
    last_sig_ptr = (SIGNAL_SB *)ptr;
    for (i=0; i < ptr->numsig; i++)
    {
	/* allocate the memory */
	if ( !(tmp_sig_ptr = (SIGNAL_SB *)malloc(sizeof(SIGNAL_SB))) )
	{
	    dino_message_ack(ptr,"Can't allocate memory for SIGNAL_SB.");
	    return;
	}

	/* initialize all the pointers */
	last_sig_ptr->forward = (int *)tmp_sig_ptr;
	tmp_sig_ptr->forward = NULL;
	tmp_sig_ptr->backward = (int *)last_sig_ptr;
	if (i==0) ptr->startsig = (int *)tmp_sig_ptr;
	tmp_sig_ptr->signame = (*(ptr->signame)).array[i];
	tmp_sig_ptr->gc = 0;
	if ( !(tmp_sig_ptr->bptr = (int *)malloc(BLK_SIZE)) )
	{
	    printf("Can't allocate memory for signal data.\n");
	    dino_message_ack(ptr,"Can't allocate memory for signal data.");
	    return;
	}
	tmp_sig_ptr->cptr = tmp_sig_ptr->bptr;
	tmp_sig_ptr->blocks = 1;
	last_sig_ptr = tmp_sig_ptr;
    }

/* make SIGNAL_SB signame point to correct signal name */

    pshort = ptr->bus;
    tmp_sig_ptr = (SIGNAL_SB *)ptr->sig.forward;
    for (i=0,j=0; i < ptr->numsig; i++)
    {
	tmp_sig_ptr->signame = (*(ptr->signame)).array[j];
	if (*pshort == 0)
	{
	    tmp_sig_ptr->inc = 1;
	    tmp_sig_ptr->type = 0;
	    tmp_sig_ptr->ind_s = 0;
	    tmp_sig_ptr->ind_e = 0;
	}
	else
	{
	    /*** t1 points to '>' in signame[j] ***/
	    t1 = strchr((*(ptr->signame)).array[j],'>');
	    *t1 = '\0';
	    strcat(t1,":");
	    /*** t2 points to '<' in signame[j] ***/
	    t2 = strchr((*(ptr->signame)).array[j+(*pshort)],'<');
	    strcat(t1,t2+1);
/*	    tmp_sig_ptr->signame += (strlen(t2)-1); */
	    tmp_sig_ptr->inc = (*pshort < 32) ? 2 : (*pshort < 64) ? 3 : 4;
	    tmp_sig_ptr->type = (*pshort < 32) ? STATE_B32 :
				(*pshort < 64) ? STATE_B64 : STATE_B96;
	    tmp_sig_ptr->ind_s = 0;
	    tmp_sig_ptr->ind_e = 0;
	    j += *pshort;
	} 
	j++;
	pshort++;
	tmp_sig_ptr = (SIGNAL_SB *)tmp_sig_ptr->forward;
    }

/* Load initial data into structure unconditionally */

    check_input(inline);
    cvd2d(ptr, inline, ptr->bus, NOCHECK );
    ptr->start_time = atoi(inline);
    ptr->time = atoi(inline);

    DONE = FALSE;

/* Loop to read trace data and reformat */

    while (!DONE)
    {
	/* If next line is EOF then this is the last line */
        if ( fgets(inline,MAX_SIG+10,f_ptr) == NULL )
	{
            DONE = TRUE;
            cvd2d(ptr, tmp, ptr->bus, NOCHECKEND );
        }
        else
	{
	    /* if not a comment or a NULL line then read into data base */
            if ( inline[0] != '!' && inline[0] != '\n' )
	    {
		check_input(inline);
        	strcpy(tmp, inline);
        	cvd2d(ptr, inline, ptr->bus, CHECK );
        	ptr->end_time = atoi(inline);
        	if (DTPRINT) printf("end_time=%d\n",ptr->end_time);
            }
            else
	    {
                if (DTPRINT) printf("Null line or comment: %s\n",inline);
	    }
	}

    }

/* re-initialize the cptr's to the bptr's */

    tmp_sig_ptr = (SIGNAL_SB *)ptr->sig.forward;
    for (i=0; i < ptr->numsig; i++)
    {
	tmp_sig_ptr->cptr = tmp_sig_ptr->bptr;
	tmp_sig_ptr = (SIGNAL_SB *)tmp_sig_ptr->forward;
    }

/* close the file */

    fclose(f_ptr);
}

void
cvb2d(str,len,value)
char		*str;
int		len;
int		*value;
{
    int i,num,cnt=0,exp=0;

    /* compute last value index */
    num = (len > 63) ? 2 : ( (len > 31) ? 1 : 0 );

    for (i=len; i>=0; i--)
    {
	if (str[i] == '1') value[num] += (int)pow(2.0,(double)exp);

	exp++;

	if ( !(exp % 32) )
	{
	    exp = 0;
	    num--;
	}
    }

    return;
}

cvd2d(ptr, line, bus, flag )
DISPLAY_SB	*ptr;
char		line[MAX_SIG];
short		*bus;
int		flag;
{
    int		time,state,value[3],i,j=0,prev=0,inc;
    char	zarr[97];
    char	tmp[100];
    unsigned int *vptr,diff;
    SIGNAL_SB	*sig_ptr;
    SIGNAL_LW	tmp_sig;

    /* initialize the string to 96 Z's */
    for (i=0;i<96;i++)
	zarr[i] = 'Z';

    /* point to the first signal in the display */
    sig_ptr = (SIGNAL_SB *)ptr->sig.forward;

    /* compute the time for this line */
    time = atoi(line);

    if (DTPRINT) printf("time=%d\n",time);

    /* adjust pointer into trace if separator is present in trace file */
    if (ptr->separator) j++;

    /* loop thru each signal */
    for (i=0; i<ptr->numsig; i++)
    {
	/* Check if signal or bus data structure is out of memory */
	diff = sig_ptr->cptr-sig_ptr->bptr;
	if (diff > BLK_SIZE/4*sig_ptr->blocks-4)
	{
	    sig_ptr->blocks++;
	    sig_ptr->bptr = (int *)realloc(sig_ptr->bptr,sig_ptr->blocks*BLK_SIZE);
	    sig_ptr->cptr = sig_ptr->bptr+diff;
	}

	/* decide whether this signal is a bus or a scalar */
	if ( *(bus+i) )
	{
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
	    else
	    {
		/* compute the value - need to fix for 64 and 96 */
		state = sig_ptr->type;
		value[0] = value[1] = value[2] = 0;
		cvb2d(tmp,*(bus+i),value);
	    }
            j += *(bus+i)+1;
	    if (ptr->separator) j++;

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
		switch(sig_ptr->type)
		{
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
		    printf("Error: Bad sig_ptr->type=%d\n",sig_ptr->type);
		}
            }
	}
	else
	{
	    /* scalar signal */  
	    switch( line[j+9] )
	    {
		case '0': state = STATE_0; break;
		case '1': state = STATE_1; break;
		case 'U': state = STATE_U; break;
		case 'Z': state = STATE_Z; break;
		default: printf("Unknown State: %c\n",line[9+j]);
	    }
	    j++;
	    if (ptr->separator) j++;

	    if ( flag != CHECK || 
	    (*(SIGNAL_LW *)((sig_ptr->cptr)-1)).state != state )
	    {
	        (*(SIGNAL_LW *)(sig_ptr->cptr)).time = time;
	        (*(SIGNAL_LW *)(sig_ptr->cptr)).state = state;
	        (sig_ptr->cptr)++;
	    }
	}

	if ( flag == NOCHECKEND )
	    (*(SIGNAL_LW *)(sig_ptr->cptr)).time = EOT;

        sig_ptr = (SIGNAL_SB *)sig_ptr->forward;
    } /* end for */
}


check_input(string)
    char	*string;
{
    char	*pchar;

    while ( (pchar=strchr(string,'#')) != 0 )
    {
	*pchar = '\0';
	strcat(string,pchar+2);
    }

    return;
}



void
read_HLO_TEMPEST(ptr)
DISPLAY_SB	*ptr;
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
    if (DTPRINT) printf("Opening File %s\n", ptr->filename);
    f_ptr = open(ptr->filename,O_RDONLY,0);
    if ( f_ptr == -1 )
    {
	if (DTPRINT) printf("Can't Open File %s\n", ptr->filename);
	dino_message_ack(ptr,"Can't open file");
	ptr->filename[0] = '\0';
	return;
    }

    /*
    ** Initialize cursor array to zero
    */
    ptr->numcursors = 0;
    for (i=0; i<MAX_CURSORS; i++)
	ptr->cursors[i] = 0;

    /*
    ** Read the file identification block
    */
    status = read(f_ptr, chardata, 4);
    if ( !status )
    {
	if (DTPRINT) printf("Bad File Format\n");
	dino_message_ack(ptr,"Bad File Format");
	ptr->filename[0] = '\0';
	return;
    }
    status = read(f_ptr, &numBytes, 4);
    status = read(f_ptr, &ptr->numsig, 4);
    status = read(f_ptr, &numRows, 4);
    status = read(f_ptr, &numBitsRow, 4);
    status = read(f_ptr, &numBitsRowPad, 4);

if (env != NULL)
{
    chardata[4]='\0';
    printf("File Sig=%s Bytes=%d Signals=%d Rows=%d Bits/Row=%d Bits/Row(pad)=%d\n",
	chardata,numBytes,ptr->numsig,numRows,numBitsRow,numBitsRowPad);
}

    /*
    ** Allocate memory for signames - assume array[numsig][MAXSIGLEN]
    */
    if ( !(ptr->signame = (SIGNALNAMES *)malloc(ptr->numsig * MAXSIGLEN)) )
    {
	printf("Can't allocate memory for signal names.\n");
	dino_message_ack(ptr,"Can't allocate memory for signal names.");
	return;
    }

    /*
    ** Read the signal description data - a signal description block is
    ** created for each signal describing the signal and containing ptrs
    ** for the trace data, current trace location, etc.
    */
    ptr->sig.backward = NULL;
    last_sig_ptr = (SIGNAL_SB *)ptr;
    for(i=0;i<ptr->numsig;i++)
    {
	status = read(f_ptr, &sigFlags, 4);
	status = read(f_ptr, &sigOffset, 4);
	status = read(f_ptr, &sigWidth, 4);
	status = read(f_ptr, &sigChars, 4);
	status = read(f_ptr, chardata, sigChars);
	chardata[sigChars] = '\0';

if (env != NULL)
{
    printf("sigFlags=%d sigOffset=%d sigWidth=%d sigChars=%d\n",
	    sigFlags,sigOffset,sigWidth,sigChars);
    printf("sigName=%s\n",chardata);
}

	/*
	** Allocate memory for a signal description block
	*/
	if ( !(sig_ptr = (SIGNAL_SB *)malloc(sizeof(SIGNAL_SB))) )
	{
	    dino_message_ack(ptr,"Can't allocate memory for SIGNAL_SB.");
	    return;
	}

	/*
	** Initialize all pointers and other stuff in the signal
	** description block
	*/
	last_sig_ptr->forward = (int *)sig_ptr;
	sig_ptr->forward = NULL;
	sig_ptr->backward = (int *)last_sig_ptr;
	if (i==0) ptr->startsig = (int *)sig_ptr;
	sig_ptr->gc = 0;
	sig_ptr->inc = 1;
	sig_ptr->type = 0;
	sig_ptr->ind_s = 0;
	sig_ptr->ind_e = sigWidth;

	/*
	** Copy the signal name into the signal name array, add an
	** EOS delimiter and initialize the pointer to it
	*/
	for (j=0;j<sigChars;j++)
	    (*(ptr->signame)).array[i][j] = chardata[j];
	(*(ptr->signame)).array[i][sigChars] = '\0';
	sig_ptr->signame = (*(ptr->signame)).array[i];

	/*
	** Allocate first memory block for signal data and initialize
	** pointers and other stuff
	*/
	if ( !(sig_ptr->bptr = (int *)malloc(BLK_SIZE)) )
	{
	    printf("Can't allocate memory for signal data.\n");
	    dino_message_ack(ptr,"Can't allocate memory for signal data.");
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
    for(i=0;i<numRows;i++)
    {
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
	    ptr->time = ptr->start_time = time;
	else
	    ptr->end_time = time;

if (env != NULL)
{
    printf("time=%d\n",time);
    printf("data=%s\n",(char *)data);
}

	/*
	** For each signal, the trace value is extracted - the width is
	** used to walk bit by bit through the row and the value for each
	** signal is accumulated
	*/
	sig_ptr = ptr->startsig;
	while (sig_ptr != NULL)
	{
	    /*
	    ** Check if signal or bus data structure is out of memory and
	    ** if it is, allocate another contiguous block to existing one
	    */
	    diff = sig_ptr->cptr-sig_ptr->bptr;
	    if (diff > BLK_SIZE/4*sig_ptr->blocks-4)
	    {
		sig_ptr->blocks++;
		sig_ptr->bptr = (int *)realloc(sig_ptr->bptr,sig_ptr->blocks*BLK_SIZE);
		sig_ptr->cptr = sig_ptr->bptr+diff;
	    }

	    width = sig_ptr->ind_e;

	    value[0] = value[1] = value[2] = 0;
	    value_index = value_bit = 0;

	    while (width > 0)
	    {
		/*
		** If the bit is set, accumulate that bits' value
		*/
		if ( data[data_index] & 1<<data_bit )
		    value[value_index] += (int)pow(2.0,(double)value_bit);

		/*
		** Increment the data and value bit pointers and jump into
		** next longword if necessary
		*/
		if ( ++data_bit >= 32)
		{
		    data_index++;
		    data_bit = 0;
		}
		if ( ++value_bit >= 32)
		{
		    value_index++;
		    value_bit = 0;
		}

		/*
		** Decrement the width of the signal by 1 bit
		*/
		width--;
	    }  /* end while width */

	    width = sig_ptr->ind_e;
	    if ( width == 1 )
	    {
		switch(value[0])
		{
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
	    else
	    {
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
		    switch(state)
		    {
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
    sig_ptr = ptr->startsig;
    while (sig_ptr != NULL)
    {
	(*(SIGNAL_LW *)(sig_ptr->cptr)).time = EOT;
	sig_ptr->cptr = sig_ptr->bptr;
	sig_ptr = (SIGNAL_SB *)sig_ptr->forward;
    }

    /*
    ** Close the file
    */
    close(f_ptr);
}

