static char rcsid[] = "$Id$";
/******************************************************************************
 * dt_binary.c --- CCLI tempest trace reading
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

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef VMS
# include <file.h>
# include <unixio.h>
# include <math.h> /* removed for Ultrix support... */
#endif

#include <X11/Xlib.h>
#include <Xm/Xm.h>

#include "dinotrace.h"
#include "functions.h"
#include "bintradef.h"

#ifdef VMS
#define TRA$W_NODNAMLEN tra$r_data.tra$$r_nnr_data.tra$r_fill_11.tra$r_fill_12.tra$w_nodnamlen
#define TRA$T_NODNAMSTR tra$r_data.tra$$r_nnr_data.tra$r_fill_11.tra$r_fill_12.tra$t_nodnamstr
#define TRA$B_DATTYP tra$r_data.tra$$r_nfd_data.tra$b_dattyp
#define TRA$W_BITLEN tra$r_data.tra$$r_nfd_data.tra$w_bitlen
#define TRA$L_BITPOS tra$r_data.tra$$r_nfd_data.tra$l_bitpos
#define TRA$L_TIME_LO tra$r_data.tra$$r_nsr_data.tra$r_fill_13.tra$r_fill_14.tra$l_time_lo
#define TRA$L_TIME_HI tra$r_data.tra$$r_nsr_data.tra$r_fill_13.tra$r_fill_14.tra$l_time_hi
#endif /* VMS */

/* Extract 1 bit or 2 bits from bit position POS in the buffer */
/* Type casting to long is important to prevent bit 7 & 8 from seperating */
/* Note that pos is used twice, no ++'s! */
#define EXTRACT_2STATE(buf,pos)	((int)(((*((unsigned long *)(((unsigned long)(buf)) + ((pos)>>3)))) >> ((pos) & 7)) & 1))
#define EXTRACT_4STATE(buf,pos)	((int)(((*((unsigned long *)(((unsigned long)(buf)) + ((pos)>>3)))) >> ((pos) & 7)) & 3))

#if 0
int	EXTRACT_2STATE (
     unsigned int pos,
     unsigned long *buf)
{
    register unsigned int bitcnt, bit_pos;
    register unsigned int bit_mask;
    register unsigned int bit_pos_in_lw;
    register unsigned int *lw_ptr;
    register unsigned int data;
    unsigned int bit_data;

    return (((*((unsigned long *)(((unsigned long)(buf)) + ((pos)>>3)))) >> ((pos) & 7)) & 1);

    if (DTPRINT_FILE) printf ("extr2st buf %x pos %x temp %x ", buf, pos);

    /*
      (( ( *((((unsigned long *)_buf_) + ((_pos_)>>5))) ) >> ((_pos_) & 0x1F) )&1)
      */

    lw_ptr = buf + ( pos >> 5 );
    data = *lw_ptr;
    /* bit_mask = 0x1F - ( pos & 0x1F); */
    bit_mask = (pos) & 0x1F;
    bit_data = ( data >> bit_mask ) & 1;

    if (DTPRINT_FILE) printf (" Data %x >> Mask %x = %d Bit %d\n", data, bit_mask, data>>bit_mask,bit_data);
    /*
    for (bit_mask=0; bit_mask<32; bit_mask++) {
      printf ("%d=%x  ", bit_mask, data>>bit_mask);
      }
      printf ("\n");
      */
    return (bit_data);
}
#endif

#pragma inline (read_4state_to_value)
int	read_4state_to_value (
    SIGNAL	*sig_ptr,
    char	*buf,
    VALUE	*value_ptr)
{
    register int state = STATE_0;	/* Unknown */
    register int bitcnt, bit_pos;

    /* Preset the state to be based upon first bit (to speed things up) */
    switch (EXTRACT_4STATE (buf, sig_ptr->file_pos)) {
      case 0:
      case 1:	state = STATE_0;	break;	/* Value */
      case 2:	state = STATE_Z;	break;
      case 3:	state = STATE_U;	break;
    }

    /* Extract the values, HIGH 32 BITS */
    bit_pos = sig_ptr->file_pos;
    for (bitcnt=96; bitcnt <= (MIN (127, sig_ptr->bits)); bitcnt++, bit_pos+=2) {
	switch (EXTRACT_4STATE (buf, bit_pos)) {
	  case 0:
	    if (state!=STATE_0) 
		state = STATE_U;
	    value_ptr->number[3] = (value_ptr->number[3]<<1);	break;
	  case 1:
	    if (state!=STATE_0) 
		state = STATE_U;
	    value_ptr->number[3] = (value_ptr->number[3]<<1) + 1;      break;
	  case 2:
	    if (state!=STATE_Z)
	      state = STATE_U;
	    break;
	  case 3:	state = STATE_U;      	break;
	}
    }

    /* Extract the values, UPPER MID 32 BITS */
    bit_pos = sig_ptr->file_pos;
    for (bitcnt=64; bitcnt <= (MIN (95, sig_ptr->bits)); bitcnt++, bit_pos+=2) {
	switch (EXTRACT_4STATE (buf, bit_pos)) {
	  case 0:
	    if (state!=STATE_0) 
		state = STATE_U;
	    value_ptr->number[2] = (value_ptr->number[2]<<1);	break;
	  case 1:
	    if (state!=STATE_0) 
		state = STATE_U;
	    value_ptr->number[2] = (value_ptr->number[2]<<1) + 1;      break;
	  case 2:
	    if (state!=STATE_Z)
	      state = STATE_U;
	    break;
	  case 3:	state = STATE_U;      	break;
	}
    }

    /* Extract the values LOWER MID 32 BITS */
    for (bitcnt=32; bitcnt <= (MIN (63, sig_ptr->bits)); bitcnt++, bit_pos+=2) {
	switch (EXTRACT_4STATE (buf, bit_pos)) {
	  case 0:
	    if (state!=STATE_0) 
		state = STATE_U;
	    value_ptr->number[1] = (value_ptr->number[1]<<1);	break;
	  case 1:
	    if (state!=STATE_0) 
		state = STATE_U;
	    value_ptr->number[1] = (value_ptr->number[1]<<1) + 1;      break;
	  case 2:
	    if (state!=STATE_Z)
	      state = STATE_U;
	    break;
	  case 3:	state = STATE_U;      	break;
	}
    }

    /* Extract the values LOW 32 BITS */
    for (bitcnt=0; bitcnt <= (MIN (31, sig_ptr->bits)); bitcnt++, bit_pos+=2) {
	switch (EXTRACT_4STATE (buf, bit_pos)) {
	  case 0:
	    if (state!=STATE_0) 
		state = STATE_U;
	    value_ptr->number[0] = (value_ptr->number[0]<<1);	break;
	  case 1:
	    if (state!=STATE_0) 
		state = STATE_U;
	    value_ptr->number[0] = (value_ptr->number[0]<<1) + 1;      break;
	  case 2:
	    if (state!=STATE_Z)
	      state = STATE_U;
	    break;
	  case 3:	state = STATE_U;      	break;
	}
    }

    switch (state) {
      case STATE_0:	/* Value */
	return (sig_ptr->type);
      case STATE_Z:
	return (STATE_Z);
      case STATE_U:
	return (STATE_U);
    }

    return (state);
}

#pragma inline (read_2state_to_value)
int	read_2state_to_value (
    SIGNAL	*sig_ptr,
    char	*buf,
    VALUE	*value_ptr)
{
    register int bitcnt, bit_pos;

    /*
      could use this routine for tempest, if so, bit_pos += inc;
    int inc = tempest_1f ? -1 : 1;
    if (tempest_1f) bit_pos += sig_ptr->bits;
    */

    /* Extract the values, HIGH 32 BITS */
    bit_pos = sig_ptr->file_pos;
    for (bitcnt=96; bitcnt <= (MIN (127, sig_ptr->bits)); bitcnt++, bit_pos++) {
	value_ptr->number[3] = (value_ptr->number[3]<<1) + (EXTRACT_2STATE (buf, bit_pos));
    }

    /* Extract the values, UPPER MID 32 BITS */
    bit_pos = sig_ptr->file_pos;
    for (bitcnt=64; bitcnt <= (MIN (95, sig_ptr->bits)); bitcnt++, bit_pos++) {
	value_ptr->number[2] = (value_ptr->number[2]<<1) + (EXTRACT_2STATE (buf, bit_pos));
    }

    /* Extract the values LOWER MID 32 BITS */
    for (bitcnt=32; bitcnt <= (MIN (63, sig_ptr->bits)); bitcnt++, bit_pos++) {
	value_ptr->number[1] = (value_ptr->number[1]<<1) + (EXTRACT_2STATE (buf, bit_pos));
    }

    /* Extract the values LOW 32 BITS */
    for (bitcnt=0; bitcnt <= (MIN (31, sig_ptr->bits)); bitcnt++, bit_pos++) {
	value_ptr->number[0] = (value_ptr->number[0]<<1) + (EXTRACT_2STATE (buf, bit_pos));
    }

    return (sig_ptr->type);
}


#ifdef VMS
#pragma inline (fil_decsim_binary_add_cptr)
void	fil_decsim_binary_add_cptr (
    /* Add a cptr corresponding to the decsim value for this signal */
    SIGNAL	*sig_ptr,
    char	*buf,
    DTime	time,
    Boolean	nocheck)		/* don't compare against previous data */
{
    register	int		state;
    register	VALUE	value;

    /* zero the value */
    value.siglw.number = value.number[0] = value.number[1] = value.number[2] = value.number[3] = 0;

    if (sig_ptr->bits == 0) {
	/* Single bit signal */
	if (sig_ptr->file_type.flag.four_state == 0)
	    state = EXTRACT_2STATE (buf, sig_ptr->file_pos)?STATE_1:STATE_0;
	else {
	    /*
	    if (DTDEBUG &&
		(EXTRACT_4STATE (buf, sig_ptr->file_pos) !=
		 ((EXTRACT_2STATE (buf, sig_ptr->file_pos+1) * 2) +
		  EXTRACT_2STATE (buf, sig_ptr->file_pos))))
		printf ("Mismatch@%d %s: %d!=%d,%d,%d\n",
			sig_ptr->file_pos,
			sig_ptr->signame,
			EXTRACT_4STATE (buf, sig_ptr->file_pos),
			EXTRACT_2STATE (buf, sig_ptr->file_pos + 1),
			EXTRACT_2STATE (buf, sig_ptr->file_pos),
			EXTRACT_2STATE (buf, sig_ptr->file_pos - 1));
			*/

	    switch (EXTRACT_4STATE (buf, sig_ptr->file_pos)) {
	      case 0: state = STATE_0; break;
	      case 1: state = STATE_1; break;
	      case 2: state = STATE_Z; break;
	      case 3: state = STATE_U; break;
	    }
	}	
    }
    else {
	/* Multibit signal */
	if (sig_ptr->file_type.flag.four_state == 0)
	    state = read_2state_to_value (sig_ptr, buf, &value);
	else state = read_4state_to_value (sig_ptr, buf, &value);
    }
	    
    value.siglw.sttime.state = state;
    value.siglw.sttime.time = time;
    fil_add_cptr (sig_ptr, &value, nocheck);
}

void decsim_read_binary (
    TRACE	*trace,
    int		read_fd)
{
    static struct bintrarec *buf, *last_buf;
    static struct bintrarec bufa;
    static struct bintrarec bufb;
    int 	first_data=TRUE;	/* True till first data is loaded */
    int		len;
    int		next_different_lw_pos;	/* LW pos with different value than last time slice */
    int		time;
    SIGNAL	*sig_ptr,*last_sig_ptr;
    int		max_lw_pos=0;		/* Maximum position in buf that has trace data */
    DTime	time_divisor;

    time_divisor = time_units_to_multiplier (global->time_precision);

    /* Signal description data */
    last_sig_ptr = NULL;
    trace->firstsig = NULL;

    /* setup data buffers */
    buf = &bufa;
    last_buf = &bufb;

    for (;;) {
	/* Alternate between buffers so that we have the data from the last time slice */
	if (last_buf == &bufb) {
	    buf = &bufb;
	    last_buf = &bufa;
	}
	else {
	    buf = &bufa;
	    last_buf = &bufb;
	}

	if (read (read_fd, buf, sizeof (struct bintrarec)) == 0) {
	    break;
	}

	switch (buf->tra$b_class) {
	    /***** CLASS: Header *****/
	  case tra$k_mhr:
	    switch (buf->tra$b_type) {
	      case tra$k_mmh:
	      case tra$k_mdr:
		break;
	      default:
		if (DTPRINT) printf ( "%%E, Unknown header type %d\n", buf->tra$b_type);
	    }
	    break;

	    /***** CLASS: Signal *****/
	  case tra$k_sir:
	    switch (buf->tra$b_type) {
		/**** TYPE: Begin Of Signal Section ****/
	      case tra$k_nns:
		break;

		/**** TYPE: End Of Signal Section ****/
	      case tra$k_nss:
		read_make_busses (trace, TRUE);
		break;

		/**** TYPE: Unknown ****/
	      default:
		if (DTPRINT) printf ("%%E, Unknown section identifier type %d\n", buf->tra$b_type);
	    }
	    break;

	    /***** CLASS: Data records *****/
	  case tra$k_dr:
	    switch (buf->tra$b_type) {

		/**** TYPE: Node format data ****/
	      case tra$k_nfd:
		sig_ptr = DNewCalloc (SIGNAL);
		sig_ptr->trace = trace;
		sig_ptr->file_pos = buf->TRA$L_BITPOS;
		/* if (DTPRINT_FILE) printf ("Reading signal format data, ptr=%d\n", sig_ptr); */

		sig_ptr->file_type.flags = 0;
		sig_ptr->file_type.flag.four_state = ! (buf->TRA$B_DATTYP == tra$k_twosta);

		/* Save the maximum position in the trace */
		if ((sig_ptr->file_pos >> 5) > max_lw_pos) {
		    max_lw_pos = ( (sig_ptr->file_pos + (sig_ptr->file_type.flag.four_state ? 2:1)
			       * sig_ptr->bits ) >> 5) + 1 /* so over estimate */;
		}

		/* initialize all the pointers that aren't NULL */
		if (last_sig_ptr) last_sig_ptr->forward = sig_ptr;
		sig_ptr->backward = last_sig_ptr;
		if (trace->firstsig==NULL) trace->firstsig = sig_ptr;
		break;

		/**** TYPE: Node signal name data ****/
	      case tra$k_nnr:
		/* if (DTPRINT_FILE) printf ("Reading signal name data, ptr=%d\n", sig_ptr); */
		len = buf->TRA$W_NODNAMLEN;
		sig_ptr->signame = (char *)XtMalloc(10+len);	/* allow extra space in case becomes vector */
		strncpy (sig_ptr->signame, buf->TRA$T_NODNAMSTR, (size_t) len);
		sig_ptr->signame[len] = '\0';
		
		last_sig_ptr = sig_ptr;
		break;

		/**** TYPE: Node state data ****/
	      case tra$k_nsr:
		time = ((buf->TRA$L_TIME_LO>>1)&0x3FFFFFFF) / time_divisor +
		    ((buf->TRA$L_TIME_HI)&0x3FFFFFFF) * (0x40000000 / time_divisor);

		/* save start/ end time */
		if (first_data) {
		    trace->start_time = time;
		}
		trace->end_time = time;

		/* Compute first LW that has a different value in it */
		next_different_lw_pos = trace->firstsig->file_pos >> 5;	/* Grab first pos in LWs */
		while ( next_different_lw_pos <= max_lw_pos
		       && ( ((long *)buf)[next_different_lw_pos]
			   == ((long *)last_buf)[next_different_lw_pos] )) {
		    next_different_lw_pos++;
		}
		/*
		printf ("ST %d-%d %c%c%c%c%c: ", next_different_lw_pos, max_lw_pos,
			( ((long *)buf)[5] == ((long *)last_buf)[5])?'-':'5',
			( ((long *)buf)[6] == ((long *)last_buf)[6])?'-':'6',
			( ((long *)buf)[7] == ((long *)last_buf)[7])?'-':'7',
			( ((long *)buf)[8] == ((long *)last_buf)[8])?'-':'8',
			( ((long *)buf)[9] == ((long *)last_buf)[9])?'-':'9'
			);
			*/

		/* save data for each signal */
		for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
		    if (( next_different_lw_pos <= (sig_ptr->file_end_pos >> 5)) || first_data) {
			/* A bit in this signal's range has changed.  Decode the value.
			   This signal itself may not have changed though, since there could
			   be 31 of 32 signals in this LW that were not changed.  */

			fil_decsim_binary_add_cptr (sig_ptr, buf, time, first_data);

			/* Compute next different lw */
			if (((sig_ptr->file_end_pos + 1) >> 5) > next_different_lw_pos) {
			    /* This signal completely used the different lw, find another */
			    /* (else there are other signals later also using this lw) */
			    next_different_lw_pos = (sig_ptr->file_end_pos+1) >> 5;	/* Cvt to LW position */
			    while ( next_different_lw_pos <= max_lw_pos
				   && ( ((long *)buf)[next_different_lw_pos]
				       == ((long *)last_buf)[next_different_lw_pos] )) {
				next_different_lw_pos++;
			    }
			    /* printf ("%d ", next_different_lw_pos); */
			}
		    }

		    /* else signal couldn't have changed in this time slice.
		       This speed bypass saves an enourmous amount of time */
		    else {
			/* For debugging, Accumulate statistics on how often we skipped this signal */
			/* sig_ptr->color ++; */
		    }
		}
		/* printf ("\n"); */

		first_data = FALSE;
		break;

		/**** TYPE: Unknown ****/
	      default:
		if (DTDEBUG) printf ("Unknown data record type %d\n", buf->tra$b_type);
	    }
	    break;

	    /**** CLASS: EOF ****/
	  case tra$k_mtr:
	    break;

	    /**** CLASS: Unknown ****/
	  default:
	    if (DTPRINT_FILE) printf ("Unknown record class %d, assuming ASCII\n", buf->tra$b_class);
	    decsim_read_ascii (trace, read_fd, NULL);
	    return;
	}
    }

    if (DTPRINT_FILE) printf ("Times = %d to %d\n", trace->start_time, trace->end_time);

    /* Print skipping statistics * /
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	printf ("%d\tLW %d-%d\tSig %s\n",sig_ptr->color,
		sig_ptr->file_pos >> 5, sig_ptr->file_end_pos >> 5, sig_ptr->signame);
	sig_ptr->color = 0;
	}
	*/
}
#endif /* VMS */


#pragma inline (fil_tempest_binary_add_cptr)
void	fil_tempest_binary_add_cptr (
    /* Add a cptr corresponding to the text at value_strg */
    SIGNAL	*sig_ptr,
    unsigned int *buf,
    DTime	time,
    Boolean	nocheck)		/* don't compare against previous data */
{
    register int state=STATE_U, bit;
    register unsigned int value_index;
    register unsigned int data_index;
    register unsigned int data_mask;
    VALUE	value;

    /* zero the value */
    value.siglw.number = value.number[0] = value.number[1] = value.number[2] = value.number[3] = 0;

    /* determine starting index and bit mask */
    if (sig_ptr->bits == 0) {
	/* Single bit signal */
	if (sig_ptr->file_type.flag.four_state == 0) {
	    data_index = (sig_ptr->file_pos >> 5);
	    data_mask = 1 << ((sig_ptr->file_pos) & 0x1F);	

	    state = (buf[data_index] & data_mask)?STATE_1:STATE_0;
	}

	else { /* Single bit four state (not really supported) */
	    data_index = (sig_ptr->file_pos >> 5);
	    data_mask = 1 << ((sig_ptr->file_pos) & 0x1F);	

	    value_index = (buf[data_index] & data_mask);	/* Used as temp */
	    if (!(data_mask = data_mask << 1)) {
		data_mask = 1;
		data_index++;
	    }
	    value_index = (value_index << 1) + (buf[data_index] & data_mask);

	    switch (value_index) {
	      case 0: state = STATE_0; break;
	      case 1: state = STATE_1; break;
	      case 2: state = STATE_Z; break;
	      case 3: state = STATE_U; break;
	    }
	}	
    }
    else {
	/* Multibit signal */
	if (sig_ptr->file_type.flag.four_state == 0) {
	    buf += (sig_ptr->file_pos >> 5);	/* Point to LSB's LW */
	    bit = (sig_ptr->file_pos & 0x1F);	/* Point to LSB's bit position in LW */
	    state = sig_ptr->type;

	    /* We want to take the data and move it so that the LSB of the data to be extracted is
	       now in value[0].  This is effectively a shift right of "bit" bits.  The fun comes in
	       because we want more then one LW of accuracy.  (Give me a 128 bit architecture!)
	       This method runs fast because it can execute in parallel and has no branches. */

	    value.number[0] = (((buf[0] >> bit) & sig_ptr->pos_mask)
				    | ((buf[1] << (32-bit)) & (~sig_ptr->pos_mask)))
		& sig_ptr->value_mask[0];
	    value.number[1] = (((buf[1] >> bit) & sig_ptr->pos_mask)
				    | ((buf[2] << (32-bit)) & (~sig_ptr->pos_mask)))
		& sig_ptr->value_mask[1];
	    value.number[2] = (((buf[2] >> bit) & sig_ptr->pos_mask)
				    | ((buf[3] << (32-bit)) & (~sig_ptr->pos_mask)))
		& sig_ptr->value_mask[2];
	    value.number[3] = (((buf[3] >> bit) & sig_ptr->pos_mask)
				    | ((buf[4] << (32-bit)) & (~sig_ptr->pos_mask)))
		& sig_ptr->value_mask[3];
	}
	else {
	    state = STATE_U;
	}
    }
	    
    value.siglw.sttime.state = state;
    value.siglw.sttime.time = time;
    fil_add_cptr (sig_ptr, &value, nocheck);

    /*if (DTPRINT_FILE) printf ("SIg %s  State %d  Value %d\n", sig_ptr->signame, value.siglw.sttime.state,
      value.number[0]);*/
}

void tempest_read (
    TRACE	*trace,
    int		read_fd)
{
    int		status;
    int		numBytes,numRows,numBitsRow,numBitsRowPad;
    int		sigChars,sigFlags,sigOffset,sigWidth;
    char		chardata[1024];
    unsigned int	*data;
    unsigned int	*data_xor;
    int		first_data;
    int		i,j;
    int		pad_len;
    unsigned int	time, last_time=EOT;
    register	SIGNAL	*sig_ptr=NULL;
    SIGNAL	*last_sig_ptr=NULL;
    int 	index;
    Boolean	verilator_xor_format;

    /*
     ** Read the file identification block
     */
    status = read (read_fd, chardata, 4);
    chardata[4]='\0';
    verilator_xor_format = !strncmp (chardata,"BX",2);
    if (!status || (!strncmp (chardata,"BT",2) && !strncmp (chardata,"BX",2) )) {
	sprintf (message, "Bad File Format (=%s)\n", chardata);
	dino_error_ack(trace, message);
	return;
    }
    status = read (read_fd, &numBytes, 4);
    status = read (read_fd, &trace->numsig, 4);
    status = read (read_fd, &numRows, 4);
    status = read (read_fd, &numBitsRow, 4);
    status = read (read_fd, &numBitsRowPad, 4);

    if (DTPRINT_FILE) {
	if (verilator_xor_format) printf ("This is Verilator Compressed XOR format.\n");
	printf ("File Sig=%s Bytes=%d Signals=%d Rows=%d Bits/Row=%d Bits/Row(pad)=%d\n",
		chardata,numBytes,trace->numsig,numRows,numBitsRow,numBitsRowPad);
    }

    /*
     ** Read the signal description data - a signal description block is
     ** created for each signal describing the signal and containing ptrs
     ** for the trace data, current trace location, etc.
     */
    trace->firstsig=NULL;
    for (i=0;i<trace->numsig;i++)
    {
	status = read (read_fd, &sigFlags, 4);
	status = read (read_fd, &sigOffset, 4);
	status = read (read_fd, &sigWidth, 4);
	status = read (read_fd, &sigChars, 4);
	status = read (read_fd, chardata, sigChars);
	chardata[sigChars] = '\0';
	
	if (DTPRINT_FILE) {
	    printf ("sigFlags=%x sigOffset=%d sigWidth=%d sigChars=%d sigName=%s\n",
		   sigFlags,sigOffset,sigWidth,sigChars,chardata);
	}
	
	for (index=sigWidth-1 ; index>=0; index--) {
	    /*
	     ** Initialize all pointers and other stuff in the signal
	     ** description block
	     */
	    sig_ptr = DNewCalloc (SIGNAL);
	    sig_ptr->trace = trace;
	    sig_ptr->forward = NULL;
	    if (trace->firstsig==NULL) {
		trace->firstsig = sig_ptr;
		sig_ptr->backward = NULL;
	    }
	    else {
		last_sig_ptr->forward = sig_ptr;
		sig_ptr->backward = last_sig_ptr;
	    }
	    if (sigWidth>1) {
		sig_ptr->bit_index = index;
		sig_ptr->bits = sigWidth;	/* Special, will be decomposed */
	    }
	    else {
		sig_ptr->bit_index = -1;
		sig_ptr->bits = 0;
	    }
	    if (index==sigWidth-1) sig_ptr->file_type.flag.vector_msb = TRUE;
	    sig_ptr->file_pos = sigOffset + index;
	    
	    /* These could be simplified as they map 1:1, but safer not to */
	    sig_ptr->file_type.flags = 0;
	    sig_ptr->file_type.flag.pin_input = ((sigFlags & 1) != 0);
	    sig_ptr->file_type.flag.pin_output = ((sigFlags & 2) != 0);
	    sig_ptr->file_type.flag.pin_psudo = ((sigFlags & 4) != 0);
	    sig_ptr->file_type.flag.pin_timestamp = ((sigFlags & 8) != 0);
	    sig_ptr->file_type.flag.four_state = ((sigFlags & 16) != 0);
	    
	    /*
	     ** Copy the signal name, add EOS delimiter and initialize the pointer to it
	     */
	    sig_ptr->signame = (char *)XtMalloc (10+sigChars); /* allow extra space in case becomes vector */
	    for (j=0;j<sigChars;j++)
		sig_ptr->signame[j] = chardata[j];
	    sig_ptr->signame[sigChars] = '\0';
	    
	    last_sig_ptr = sig_ptr;
	}
    
	/* Checks */
	if (sig_ptr->file_type.flag.four_state != 0) {
	    sprintf (message,"Four state tempest not supported.\nSignal %s will be wrong.",sig_ptr->signame);
	    dino_warning_ack (trace, message);
	}

	/*
	 ** Read the pad bits
	 */
	pad_len = (sigChars%8) ? 8 - (sigChars%8) : 0;
	status = read (read_fd, chardata, pad_len);
    }

    /* Make the busses */
    read_make_busses (trace, FALSE);

    /* Make storage space, with some overhead */
    data     = (unsigned int *)XtCalloc (32 + numBitsRowPad/32, sizeof(unsigned int));
    data_xor = (unsigned int *)XtCalloc (32 + numBitsRowPad/32, sizeof(unsigned int));

    /* Read the signal trace data
     * Pass 0-(numRows-1) reads the data, pass numRows processes last line */
    first_data = TRUE;

    /* Don't use numRows as it is written when CCLI exits, and may be incorrect 
       if CCLI was CTL-Ced */

    while (1) {
	/* Read a row of data */
	if (verilator_xor_format) {
	    status = read (read_fd, data_xor, numBitsRowPad/8);
	    if (status < numBitsRowPad/8) break;	/* End of data */

	    /* Un-exor the data */
	    for (j = 0; j <= (numBitsRowPad/(sizeof(unsigned int)*8)); j++) {
		data[j] ^= data_xor[j];
	    }
	}
	else {
	    /* Regular format */
	    status = read (read_fd, data, numBitsRowPad/8);
	    if (status < numBitsRowPad/8) break;	/* End of data */
	}

	/*
	if (DTPRINT_FILE) {
	    printf ("read: time=%d  status %d data=%08x %08x\n", data[0], 
		    status, data[0], data[1]);
		    }*/
	
	/** Extract the phase - this will be used as a 'time' value and
	 ** is multiplied by 100 to make the trace easier to read
	 */
	time = data[0] * global->tempest_time_mult;
	if (time == last_time) time++;
	last_time = time;
	
	/*
	 ** If this is the first row, save the starting and initial
	 ** time, else eventually the end time will be saved.
	 */
	if (first_data)
	    trace->start_time = time;
	else trace->end_time = time;
	    
	/* Fortunately, Tempest and Decsim have identical binary packed formats. */
	/* Perhaps it's because both were written by SEG CAD. */
	for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	    fil_tempest_binary_add_cptr (sig_ptr, data, time, first_data);
	}

	first_data = FALSE;
    }/* end for */

    DFree (data);
}

