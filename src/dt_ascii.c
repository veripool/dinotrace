#ident "$Id$"
/******************************************************************************
 * DESCRIPTION: Dinotrace source: ascii trace file reading
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
 * gratefuly have agreed to share it, and thus the base version has been
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

#include "functions.h"

/**********************************************************************/

int fil_line_num=0;

#define FIL_SIZE_INC	2048	/* Characters to increase length by */

/****************************** UTILITIES ******************************/

#ifdef VMS
#pragma inline (ascii_string_add_cptr)
#endif
void	ascii_string_add_cptr (
    /* Add a cptr corresponding to the text at value_strg */
    Signal_t	*sig_ptr,
    const char	*value_strg,
    DTime_t	time,
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

    if (sig_ptr->bits<2) {
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
	len = sig_ptr->bits-1;
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
	    if (sig_ptr->bits < 33 ) {
		state = STATE_F32;
		for (bitcnt=0; bitcnt <= (MIN (31,len)); bitcnt++, cp++) {
		    value.number[0] = (value.number[0]<<1) | (*cp=='1') | (*cp=='z') | (*cp=='Z');
		    value.number[1] = (value.number[1]<<1) | (((*cp)|1)!='1');
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

    /*if (DTPRINT_FILE) printf ("time %d sig %s str %s cor %c cand %c state %d n0 %08x n1 %08x\n",				time, sig_ptr->signame, value_strg, chr_or, chr_and, state, value.number[0], value.number[1]); */
    value.siglw.stbits.state = state;
    value.time = time;
    fil_add_cptr (sig_ptr, &value, nocheck);
}

/****************************** DECSIM ASCII ******************************/

static void decsim_read_ascii_header (
    Trace_t	*trace,
    char	*header_start,
    char	*data_begin_ptr,
    int		sig_start_pos,
    int		sig_end_pos,
    int		header_lines)
{
    int		line,col;
    char	*line_ptr, *tmp_ptr;
    char	*signame_array;
    char	*signame;
    Boolean_t	hit_name_block, past_name_block, no_names;

#define	SIGLINE(_l_) (signame_array+(_l_)*(sig_end_pos+5))

    /* make array:  [header_lines+5][sig_end_pos+5]; */
    signame_array = XtMalloc ((header_lines+5)*(sig_end_pos+5));
    signame = XtMalloc ((header_lines+5));

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

    /* Extend short lines with spaces */
    no_names=TRUE;
    for (line=0; line<header_lines; line++) {
	for (col=sig_start_pos; col < sig_end_pos; col++) {
	    if (SIGLINE(line)[col]!='!' && SIGLINE(line)[col]!=' ')
		no_names = FALSE;
	}
	col=strlen(SIGLINE(line));
	if (SIGLINE(line)[0]!='!') col=0;
	for ( ; col<sig_end_pos; col++) SIGLINE(line)[col]=' '; 
    }

    /* Make a signal structure for each signal */
    trace->firstsig = NULL;
    for ( col=sig_start_pos; col < sig_end_pos; col++) {
	union sig_file_type_u file_type;
	file_type.flags = 0;

	/* Read Signal Names Into Structure */
	if (no_names) {
	    sprintf (signame, "SIG %d", col);
	} else {
	    for (line=0; line<header_lines; line++) {
		signame[line] = SIGLINE(line)[col];
	    }
	    signame[line] = '\0';
	}

	sig_new_file (trace, signame,
		      col, 0,
		      1/*bits*/, -1/*msb*/, -1/*lsb*/,
		      file_type);
    }

    DFree (signame);
    DFree (signame_array);
}

void ascii_read (
    Trace_t	*trace,
    int		read_fd,
    FILE	*decsim_z_readfp)	/* Only pre-set if FF_DECSIM_Z */
{
    FILE	*readfp;
    Boolean_t	first_data;
    char	*line_in;
    Signal_t	*sig_ptr;
    long	time_stamp;
    char	*pchar;
    DTime_t	time_divisor;
    int		ch;

    char	*header_start, *header_ptr;
    long	header_length, header_lines=0;
    Boolean_t	got_start=FALSE, hit_start=FALSE, in_comment=FALSE;
    long	base_chars_left=0;

    char	*t;
    Boolean_t	chango_format=FALSE;
    long	time_stamp_last;
    long	smallest_delta_time = -1;
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
    header_length = FIL_SIZE_INC;
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
	if (header_ptr - header_start > header_length-4/*slop*/) {
	    long header_size = header_ptr - header_start;
	    header_length += FIL_SIZE_INC;
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
	DFree (header_start);
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
    time_stamp_last = -100;
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

	    /* save information on each signal */
	    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
		ascii_string_add_cptr (sig_ptr, line_in + sig_ptr->file_pos, time_stamp, first_data);
	    }

	    if (first_data) {
		trace->start_time = time_stamp;
		first_data = FALSE;
	    } else {
		if (smallest_delta_time < 0
		    || ((time_stamp - time_stamp_last) < smallest_delta_time)) {
		    smallest_delta_time = (time_stamp - time_stamp_last);
		}
	    }
	    trace->end_time = time_stamp;
	    time_stamp_last = time_stamp;
	}
    }

    /* No timestamps, or uniform timestamps, so include last line as valid data */
    /* We'll always include some time so we don't loose the last line's info */
    if (smallest_delta_time < 0) smallest_delta_time = 100;	/* Occurs when only 1 line */
    trace->end_time += smallest_delta_time;

    DFree (header_start);

    /* Mark format as may have presumed binary */
    trace->dfile.fileformat = FF_DECSIM_ASCII;
    if (DTPRINT_FILE) printf ("decsim_read_ascii done\n");
}

