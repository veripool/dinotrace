#include <stdio.h>

#ifdef VMS
#include <file.h>
#include <unixio.h>
#include <math.h> /* removed for Ultrix support... */
#endif VMS

#include <X11/Xlib.h>
#include <Xm/Xm.h>

#include "dinotrace.h"
#include "bintradef.h"


#define MAXSIG 1000

#define TRA$W_NODNAMLEN tra$r_data.tra$$r_nnr_data.tra$r_fill_11.tra$r_fill_12.tra$w_nodnamlen
#define TRA$T_NODNAMSTR tra$r_data.tra$$r_nnr_data.tra$r_fill_11.tra$r_fill_12.tra$t_nodnamstr
#define TRA$B_DATTYP tra$r_data.tra$$r_nfd_data.tra$b_dattyp
#define TRA$W_BITLEN tra$r_data.tra$$r_nfd_data.tra$w_bitlen
#define TRA$L_BITPOS tra$r_data.tra$$r_nfd_data.tra$l_bitpos
#define TRA$L_TIME_LO tra$r_data.tra$$r_nsr_data.tra$r_fill_13.tra$r_fill_14.tra$l_time_lo

/* Extract 1 bit or 2 bits from bit position POS in the buffer */
/* Type casting to long is important to prevent bit 7 & 8 from seperating */
/* Note that pos is used twice, no ++'s! */
#define EXTRACT_2STATE(buf,pos)	(((*((unsigned long *)(((unsigned long)(buf)) + ((pos)>>3)))) >> ((pos) & 7)) & 1)
#define EXTRACT_4STATE(buf,pos)	(((*((unsigned long *)(((unsigned long)(buf)) + ((pos)>>3)))) >> ((pos) & 7)) & 3)

void read_decsim(trace)
    TRACE	*trace;
{
    int		in_fd;
    static struct bintrarec *buf;
    static struct bintrarec bufa;
    static struct bintrarec bufb;
    int		buf_stat=1;		/* 0=EOF, 1=use buf a, 2= use buf b */
    int 	first_data=TRUE;	/* True till first data is loaded */
    int		len;
    int		time;
    char	*t1;
    SIGNAL		*sig_ptr,*last_sig_ptr;
    int		eof_next=FALSE;

    int	pass=0;

#ifndef VMS
    /* The reads below rely on varible RMS records - Ultrix has no such thing */
    if (DTPRINT) printf ("Binary traces are disabled under Ultrix.\n");
    read_decsim_ascii (trace);
    return;
#endif

    in_fd = open (trace->filename, O_RDONLY, 0);
    if (in_fd<1) {
	read_decsim_ascii (trace);
	return;
	}

    /* Signal description data */
    last_sig_ptr = NULL;
    trace->firstsig = NULL;

    /* preload 2 entry buffer */
    buf_stat=1;
    if (read( in_fd, &bufa, 40000) == 0) {
	close (in_fd);
	return;
	}

    for(;;) {
	if (!buf_stat) break;
	if (buf_stat==1) {
	    buf_stat=2;
	    buf = &bufa;
	    if (read( in_fd, &bufb, 40000) == 0)
		buf_stat=0;
	    eof_next = (buf_stat==0) || (bufb.tra$b_class==4);
	    }
	else {
	    buf_stat=1;
	    buf = &bufb;
	    if (read( in_fd, &bufa, 40000) == 0)
		buf_stat=0;
	    eof_next = (buf_stat==0) || (bufa.tra$b_class==4);
	    }

	switch( buf->tra$b_class) {
	    /***** CLASS: Header *****/
	  case tra$k_mhr:
	    switch( buf->tra$b_type) {
	      case tra$k_mmh:
	      case tra$k_mdr:
		break;
	      default:
		if (DTPRINT) printf( "Unknown header type %d\n", buf->tra$b_type);
		}
	    break;

	    /***** CLASS: Signal *****/
	  case tra$k_sir:
	    switch( buf->tra$b_type) {
		/**** TYPE: Begin Of Signal Section ****/
	      case tra$k_nns:
		break;

		/**** TYPE: End Of Signal Section ****/
	      case tra$k_nss:
		read_binary_make_busses(trace);
		if (DTPRINT) read_trace_dump(trace);
		break;

		/**** TYPE: Unknown ****/
	      default:
		if (DTPRINT) printf( "Unknown section identifier type %d\n", buf->tra$b_type);
		}
	    break;

	    /***** CLASS: Data records *****/
	  case tra$k_dr:
	    switch( buf->tra$b_type) {

		/**** TYPE: Node format data ****/
	      case tra$k_nfd:
		sig_ptr = (SIGNAL *)XtMalloc(sizeof(SIGNAL));
		memset (sig_ptr, 0, sizeof (SIGNAL));
		sig_ptr->binary_type = buf->TRA$B_DATTYP;
		sig_ptr->binary_pos = buf->TRA$L_BITPOS;
		sig_ptr->bits = 0;	/* = buf->TRA$W_BITLEN; */
		/* if (DTPRINT) printf ("Reading signal format data, ptr=%d\n", sig_ptr); */

		/* initialize all the pointers that aren't NULL */
		if (last_sig_ptr) last_sig_ptr->forward = sig_ptr;
		sig_ptr->backward = last_sig_ptr;
		if (trace->firstsig==NULL) trace->firstsig = sig_ptr;
		break;

		/**** TYPE: Node signal name data ****/
	      case tra$k_nnr:
		/* if (DTPRINT) printf ("Reading signal name data, ptr=%d\n", sig_ptr); */
		len = MIN (buf->TRA$W_NODNAMLEN, MAXSIGLEN);
		
		/* Must drop leading spaces! */
		for (t1=buf->TRA$T_NODNAMSTR; *t1==' ' && len>0; t1++)
		    len--;

		sig_ptr->signame = (char *)XtMalloc(10+len);	/* allow extra space incase becomes vector */
		strncpy(sig_ptr->signame, t1, len);
		sig_ptr->signame[len] = '\0';
		
		last_sig_ptr = sig_ptr;
		break;

		/**** TYPE: Node state data ****/
	      case tra$k_nsr:
		time = ((buf->TRA$L_TIME_LO>>1)&0x1FFFFFFF)/1000;

		/* save start/ end time */
		if (first_data) {
		    trace->start_time = time;
		    }
		trace->end_time = time;

		/* if (DTPRINT && eof_next) printf ("Detected EOF.\n"); */
	    
		/* if (pass++ < 20) */
		    read_binary_time (trace, buf, time, (first_data || eof_next)?NOCHECK:CHECK);
		/*
		if (pass==21)
		    read_binary_time (trace, buf, time, NOCHECK);
		    */

		first_data = FALSE;
		break;

		/**** TYPE: Unknown ****/
	      default:
		if (DTDEBUG) printf( "Unknown data record type %d\n", buf->tra$b_type);
		}
	    break;

	    /**** CLASS: EOF ****/
	  case tra$k_mtr:
	    break;

	    /**** CLASS: Unknown ****/
	  default:
	    if (DTPRINT) printf( "Unknown record class %d, assuming ASCII\n", buf->tra$b_class);
	    close (in_fd);
	    read_decsim_ascii (trace);
	    return;
	    }
	}

    read_trace_end (trace);

    close(in_fd);
    }


/* Take the list of signals and make it into a list of busses */
read_binary_make_busses (trace)
    TRACE	*trace;
{
    SIGNAL		*sig_ptr,*bus_sig_ptr;	/* ptr to signal which is being bussed */
    char *bbeg, *bcol, *bnew, *sbeg;

    /* Vectorize signals */
    bus_sig_ptr = NULL;
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	if (bus_sig_ptr) {	/* Start on 2nd signal */
	    bbeg = strchr(bus_sig_ptr->signame,'<');
	    bcol = strchr(bus_sig_ptr->signame,':');
	    sbeg = bbeg-bus_sig_ptr->signame + sig_ptr->signame;	/* < in sig_ptr->signame */
	    if (!bcol) bcol=bbeg;
	    if ( bbeg && 
		( strncmp(sig_ptr->signame, bus_sig_ptr->signame, bbeg-bus_sig_ptr->signame+1) == 0 ) &&
		(abs(atoi(bcol+1) - atoi(sbeg+1)) == 1)) {
		/* Only allow signals that are different by 1 in the subscript to combine */

		/* Can be bussed with previous signal */
		(bus_sig_ptr->bits) ++;

		/* Change bussed name */
		if (bcol == bbeg)
		    bnew = strchr(bus_sig_ptr->signame,'>');
		else bnew = bcol;
		
		*bnew = '\0';
		strcat (bnew,":");
		strcat (bnew, sbeg+1);

		/* Delete this signal */
		bus_sig_ptr->forward = sig_ptr->forward;
		if (sig_ptr->forward)
		    ((SIGNAL *)(sig_ptr->forward))->backward = bus_sig_ptr;
		XtFree (sig_ptr->signame);
		XtFree (sig_ptr);
		sig_ptr = bus_sig_ptr;
		}
	    }
	bus_sig_ptr = sig_ptr;
	}
    
    /* Calculate numsig, last_sig_ptr, and allocate cptrs */
    trace->numsig = 0;
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	/* Calc numsig, last_sig_ptr */
	(trace->numsig) ++;

	/* Create the bussed name & type */
	if (sig_ptr->bits < 1) {
	    sig_ptr->inc = 1;
	    sig_ptr->type = 0;
	    }
	else if (sig_ptr->bits < 32) {
	    sig_ptr->inc = 2;
	    sig_ptr->type = STATE_B32;
	    }
	else if (sig_ptr->bits < 64) {
	    sig_ptr->inc = 3;
	    sig_ptr->type = STATE_B64;
	    }
	else if (sig_ptr->bits < 96) {
	    sig_ptr->inc = 4;
	    sig_ptr->type = STATE_B96;
	    }

	/* allocate the data storage memory */
	sig_ptr->bptr = (int *)XtMalloc(BLK_SIZE);
	sig_ptr->cptr = sig_ptr->bptr;
	sig_ptr->blocks = 1;
	}
    }

#pragma inline (read_2state_to_value, read_4state_to_value)
int	read_4state_to_value (sig_ptr, buf, value)
    SIGNAL	*sig_ptr;
    char *buf;
    unsigned int *value;
{
    register int state = STATE_0;	/* Unknown */
    register int bitcnt, bit_pos;

    value[0] = value[1] = value[2] = 0;

    /* Preset the state to be based upon first bit (to speed things up) */
    switch (EXTRACT_4STATE(buf, sig_ptr->binary_pos)) {
      case 0:
      case 1:	state = STATE_0;	break;	/* Value */
      case 2:	state = STATE_Z;	break;
      case 3:	state = STATE_U;	break;
	}

    /* Extract the values, HIGH 32 BITS */
    bit_pos = sig_ptr->binary_pos;
    for (bitcnt=64; bitcnt <= (MIN(95, sig_ptr->bits)); bitcnt++, bit_pos+=2) {
	switch (EXTRACT_4STATE(buf, bit_pos)) {
	  case 0:
	    if (state!=STATE_0) 
		state = STATE_U;
	    value[2] = (value[2]<<1);	break;
	  case 1:
	    if (state!=STATE_0) 
		state = STATE_U;
	    value[2] = (value[2]<<1) + 1;      break;
	  case 2:
	    if (state!=STATE_Z)
	      state = STATE_U;
	    break;
	  case 3:	state = STATE_U;      	break;
	    }
	}

    /* Extract the values MID 32 BITS */
    for (bitcnt=32; bitcnt <= (MIN(63, sig_ptr->bits)); bitcnt++, bit_pos+=2) {
	switch (EXTRACT_4STATE(buf, bit_pos)) {
	  case 0:
	    if (state!=STATE_0) 
		state = STATE_U;
	    value[1] = (value[1]<<1);	break;
	  case 1:
	    if (state!=STATE_0) 
		state = STATE_U;
	    value[1] = (value[1]<<1) + 1;      break;
	  case 2:
	    if (state!=STATE_Z)
	      state = STATE_U;
	    break;
	  case 3:	state = STATE_U;      	break;
	    }
	}

    /* Extract the values LOW 32 BITS */
    for (bitcnt=0; bitcnt <= (MIN(31, sig_ptr->bits)); bitcnt++, bit_pos+=2) {
	switch (EXTRACT_4STATE(buf, bit_pos)) {
	  case 0:
	    if (state!=STATE_0) 
		state = STATE_U;
	    value[0] = (value[0]<<1);	break;
	  case 1:
	    if (state!=STATE_0) 
		state = STATE_U;
	    value[0] = (value[0]<<1) + 1;      break;
	  case 2:
	    if (state!=STATE_Z)
	      state = STATE_U;
	    break;
	  case 3:	state = STATE_U;      	break;
	    }
	}

    /* DEBUGGING 
    if (state == STATE_0 && (0==strcmp (sig_ptr->signame, "%NET.MEMRASL<8:0>"))) {
	unsigned long *byte, p, b;

	p = sig_ptr->binary_pos + 14;
	byte = buf + (p>>3);	
	p = p & 7;		
	b = (*byte >> p) & 3;	

	printf ("Pos %d Final Val = %d, real %c mine %c%c%c%c%c%c%c%c\n", sig_ptr->binary_pos +14,
		value[0],
		("01ZU")[b],
		("01ZU")[(EXTRACT_4STATE (buf, sig_ptr->binary_pos + 0))],
		("01ZU")[(EXTRACT_4STATE (buf, sig_ptr->binary_pos + 2))],
		("01ZU")[(EXTRACT_4STATE (buf, sig_ptr->binary_pos + 4))],
		("01ZU")[(EXTRACT_4STATE (buf, sig_ptr->binary_pos + 8))],
		("01ZU")[(EXTRACT_4STATE (buf, sig_ptr->binary_pos + 10))],
		("01ZU")[(EXTRACT_4STATE (buf, sig_ptr->binary_pos + 12))],
		("01ZU")[(EXTRACT_4STATE (buf, sig_ptr->binary_pos + 14))],
		("01ZU")[(EXTRACT_4STATE (buf, sig_ptr->binary_pos + 16))]
		);
	}
	*/

    switch (state) {
      case STATE_0:	/* Value */
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
	return (sig_ptr->type);
      case STATE_Z:
	return (STATE_Z);
      case STATE_U:
	return (STATE_U);
	}
    }

int	read_2state_to_value (sig_ptr, buf, value)
    SIGNAL	*sig_ptr;
    char *buf;
    unsigned int *value;
{
    register int bitcnt, bit_pos;

    value[0] = value[1] = value[2] = 0;

    /* Extract the values, HIGH 32 BITS */
    bit_pos = sig_ptr->binary_pos;
    for (bitcnt=64; bitcnt <= (MIN(95, sig_ptr->bits)); bitcnt++, bit_pos++) {
	value[2] = (value[2]<<1) + (EXTRACT_2STATE(buf, bit_pos));
	}

    /* Extract the values MID 32 BITS */
    for (bitcnt=32; bitcnt <= (MIN(63, sig_ptr->bits)); bitcnt++, bit_pos++) {
	value[1] = (value[1]<<1) + (EXTRACT_2STATE(buf, bit_pos));
	}

    /* Extract the values LOW 32 BITS */
    for (bitcnt=0; bitcnt <= (MIN(31, sig_ptr->bits)); bitcnt++, bit_pos++) {
	value[0] = (value[0]<<1) + (EXTRACT_2STATE(buf, bit_pos));
	}

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

    return (sig_ptr->type);
    }



read_binary_time (trace, buf, time, flag )
    TRACE	*trace;
    char	*buf;
    int		time;
    int		flag;
{
/*
    int		state,i,j=0,prev=0,inc,len;
    char	tmp[100];
    char	*line,*zarr;
    register	char *cp;
    short	*bus = trace->bus;
    SIGNAL_LW	tmp_sig;
*/
    unsigned int value[3];
    int		state;
    unsigned int *vptr,diff;
    SIGNAL	*sig_ptr;

    /*
    if (DTPRINT) printf ("read time %d\n", time);
    */

    /* loop thru each signal */
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {

	/* Check if signal or bus data structure is out of memory */
	diff = sig_ptr->cptr - sig_ptr->bptr;
	if (diff > BLK_SIZE / 4 * sig_ptr->blocks - 4) {
	    sig_ptr->blocks++;
	    sig_ptr->bptr = (int *)XtRealloc(sig_ptr->bptr,sig_ptr->blocks*BLK_SIZE);
	    sig_ptr->cptr = sig_ptr->bptr+diff;
	    }
       
	if (sig_ptr->bits == 0) {
	    /* Single bit signal */
	    if ( sig_ptr->binary_type == tra$k_twosta )
		state = EXTRACT_2STATE(buf, sig_ptr->binary_pos)?STATE_1:STATE_0;
	    else {
		switch (EXTRACT_4STATE(buf, sig_ptr->binary_pos)) {
		  case 0: state = STATE_0; break;
		  case 1: state = STATE_1; break;
		  case 2: state = STATE_Z; break;
		  case 3: state = STATE_U; break;
		    }
		}
	    
	    if ( flag != CHECK || 
		(*(SIGNAL_LW *)((sig_ptr->cptr)-1)).state != state )
		{
		/*
		if (DTPRINT) printf ("signal %s changed to %d\n", sig_ptr->signame, state);
		*/
		(*(SIGNAL_LW *)(sig_ptr->cptr)).time = time;
		(*(SIGNAL_LW *)(sig_ptr->cptr)).state = state;
		(sig_ptr->cptr)++;
		}
	    }
	else {
	    /* Multibit signal */
	    if ( sig_ptr->binary_type == tra$k_twosta )
		state = read_2state_to_value (sig_ptr, buf, value);
	    else state = read_4state_to_value (sig_ptr, buf, value);
	    
	    /* now see if data has changed since previous entry */
	    /* add if states !=, or if both are a bus and values != */
	    
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
	}
    }
	
