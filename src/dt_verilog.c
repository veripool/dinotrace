#ident "$Id$"
/******************************************************************************
 * dt_verilog.c --- Verilog dump file reading
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
/* File locals */
static int verilog_line_num=0;
static char *current_file="";

/* Stack of scopes, 0=top level.  Grows up to MAXSCOPES */
#define MAXSCOPES 20
static char	scopes[MAXSCOPES][MAXSIGLEN];
static int	scope_level;
static DTime	time_divisor, time_scale;

static char	*verilog_line_storage;
static uint_t	verilog_line_length;

static Signal	*last_sig_ptr;		/* last signal read in */

/* Pointer to array of signals sorted by pos. (Special hash table) */
/* *(signal_by_pos[VERILOG_ID_TO_POS ("abc")]) gives signal ABC */
static Signal	**signal_by_pos;	

/* List of signals that need updating */
static Signal	**signal_update_array;	
static Signal	**signal_update_array_last_pptr;

/* Convert a 4 letter <identifier_code> to an id number */
/* <identifier_code> uses chars 33-126 = '!' - '~' */
#define	VERILOG_ID_TO_POS(_code_) \
    (_code_[0]?((_code_[0]-32) + 94 * \
		(_code_[1]?((_code_[1]-32) + 94 * \
			    (_code_[2]?((_code_[2]-32) + 94 * \
					(_code_[3]?((_code_[3]-32) \
						    ):0) \
					):0) \
			    ):0) \
		):0)

void	verilog_read_till_end (
    char	*line,
    FILE	*readfp)
{
    char *tp;

    while (1) {
	while (*line) {
	    while (*line && *line!='$') line++;
	    if (*line) {
		if (!strncmp (line, "$end", 4)) {
		    return;
		}
		else line++;
	    }
	}

	fgets_dynamic (&verilog_line_storage, &verilog_line_length, readfp);
	line = verilog_line_storage;
	if (feof (readfp)) return;
	if (*(tp=(line+strlen(line)-1))=='\n') *tp='\0';
	/* if (DTPRINT_FILE) printf ("line='%s'\n",line); */
	verilog_line_num++;
    }
}

void	verilog_read_timescale (
    Trace	*trace,
    char	*line,
    FILE	*readfp)
{
    while (isspace (*line)) line++;
    if (!*line) {
	fgets_dynamic (&verilog_line_storage, &verilog_line_length, readfp);
	line = verilog_line_storage;
	verilog_line_num++;
	if (feof (readfp)) return;
    }

    while (isspace (*line)) line++;
    time_scale = atol (line);
    while (isdigit (*line)) line++;
    switch (*line) {
      case 's':
	time_scale *= 1000;
      case 'm':
	time_scale *= 1000;
	dino_warning_ack (trace, "Timescale of trace may be too big.");
      case 'u':
	time_scale *= 1000;
      case 'n':
	time_scale *= 1000;
      case 'p':
	break;
      case 'f':
      default:
	sprintf (message,"Unknown time scale unit '%c' on line %d of %s\n",
		 *line, verilog_line_num, current_file);
	dino_error_ack (trace, message);
    }
    if (time_scale == 0) time_scale = 1;
    if (DTPRINT_FILE) printf ("timescale=%d\n",time_scale);

    verilog_read_till_end (line, readfp);
}

#define verilog_skip_parameter(_line_) \
    while (isspace(*_line_)) _line_++; \
    while (*_line_ && !isspace(*_line_)) _line_++; \
    while (isspace(*_line_)) _line_++

#define verilog_read_parameter(_line_, cmd) \
   {\
	while (isspace(*_line_)) _line_++; \
	cmd=_line_; \
	while (*_line_ && !isspace(*_line_)) _line_++; \
	if (*_line_) *_line_++ = '\0'; \
					   }

void	verilog_process_var (
    /* Process a VAR statement (read a signal) */
    Trace	*trace,
    char	*line)
{
    char	*type, *cmd, *code;
    int		bits;
    Signal	*new_sig_ptr;
    char	signame[10000];
    int		t, len;

    /* Read <vartype> */
    verilog_read_parameter (line, type);
	
    /* Read <size> */
    verilog_read_parameter (line, cmd);
    bits = atoi (cmd);

    /* Read <identifier_code>  (uses chars 33-126 = '!' - '~') */
    verilog_read_parameter (line, code);
    
    /* Signal name begins with the present scope */
    signame[0] = '\0';
    for (t=0; t<scope_level; t++) {
	strcat (signame, scopes[t]);
	strcat (signame, ".");
    }
    
    /* Read <reference> */
    verilog_read_parameter (line, cmd);
    strcat (signame, cmd);
    verilog_read_parameter (line, cmd);
    if (*cmd != '$' && strcmp(cmd, "[-1]")) strcat (signame, cmd);
    
    if (!strncmp (type, "real", 4)) {
	printf ("%%W, Reals not supported, signal %s ignored.\n", signame);
	return;
    }

    /* Allocate new signal structure */
    new_sig_ptr = DNewCalloc (Signal);
    new_sig_ptr->trace = trace;
    new_sig_ptr->file_pos = VERILOG_ID_TO_POS(code);
    new_sig_ptr->bits = bits - 1;
    new_sig_ptr->msb_index = 0;
    new_sig_ptr->lsb_index = 0;

    new_sig_ptr->file_type.flags = 0;
    new_sig_ptr->file_type.flag.perm_vector = (bits>1);	/* If a vector already then we won't vectorize it */

    new_sig_ptr->file_value.siglw.number = new_sig_ptr->file_value.number[0] =
	new_sig_ptr->file_value.number[1] =  new_sig_ptr->file_value.number[2] =
	    new_sig_ptr->file_value.number[3] = 0;
    
    /* initialize all the pointers that aren't NULL */
    if (last_sig_ptr) last_sig_ptr->forward = new_sig_ptr;
    new_sig_ptr->backward = last_sig_ptr;
    if (trace->firstsig==NULL) trace->firstsig = new_sig_ptr;
    last_sig_ptr = new_sig_ptr;

    /* copy signal info */
    len = strlen (signame);
    new_sig_ptr->signame = (char *)XtMalloc(10+len);	/* allow extra space in case becomes vector */
    strcpy (new_sig_ptr->signame, signame);
}


/*
For a large trace, here's a table of frequency of signals of each number of bits.

55147 1		 150  8 	 125  16	  28  24	 175  32	   1  44 
 272  2		   3  9 	  72  17	   3  25	  17  34	   2  54 
 272  3		  57  10	   2  18	   1  26	   2  35	   1  62 
 316  4		  21  11	   3  19			   9  36	  75  64 
  32  5		  85  12	 		  10  28			  36  72 
  74  6		  42  13	   3  21	   1  29			   1  111
  38  7		  84  14	 135  22	  13  30			 121  128
		   4  15	 		   5  31			   1  145
										  12  256
*/

void	verilog_womp_128s (
    /* Take signals of 128+ signals and split into several signals */
    Trace	*trace)
{
    Signal	*new_sig_ptr;
    Signal	*sig_ptr;
    int		len;
    int		chop;

    /*print_sig_names (NULL, trace);*/
    for (sig_ptr = trace->firstsig; sig_ptr; /* increment below */ ) {

	if (sig_ptr->bits > 128) {
	    if (DTPRINT_FILE) printf ("Adjusting signal %s [%d - %d]   %d > 128\n",
				      sig_ptr->signame, sig_ptr->lsb_index, sig_ptr->msb_index, sig_ptr->bits);
	    /* Allocate new signal structure */
	    new_sig_ptr = XtNew (Signal);
	    memcpy (new_sig_ptr, sig_ptr, sizeof (Signal));
	    len = strlen (sig_ptr->signame);
	    new_sig_ptr->signame = (char *)XtMalloc(10+len);	/* allow extra space in case becomes vector */
	    strcpy (new_sig_ptr->signame, sig_ptr->signame);

	    new_sig_ptr->forward = sig_ptr->forward;
	    sig_ptr->forward = new_sig_ptr;
	    new_sig_ptr->backward = sig_ptr;
	    if (new_sig_ptr->forward) new_sig_ptr->forward->backward = new_sig_ptr;
	    
	    /* Remember there are multiple signals under this coding */
	    new_sig_ptr->verilog_next = sig_ptr->verilog_next;
	    sig_ptr->verilog_next = new_sig_ptr;

	    /* How much to remove */
	    chop = (sig_ptr->bits+1) % 128;
	    if (chop==0) chop=128;
	    /*printf ("chopping %d\n",chop);*/

 	    /* Make new be remainders */
 	    new_sig_ptr->bits = sig_ptr->bits - chop;
 	    new_sig_ptr->msb_index = sig_ptr->msb_index - chop;
  
  	    /* Shorten bits of old */
 	    sig_ptr->bits = chop - 1;
 	    sig_ptr->lsb_index = sig_ptr->msb_index - sig_ptr->bits;

	    /* Next is new guy, may be >> 128 also */
	    sig_ptr = new_sig_ptr->backward;
	}
	else {
	    sig_ptr = sig_ptr->forward;
	}
    }

    /*print_sig_names (NULL, trace);*/
}


void	verilog_print_pos (
    int max_pos)
    /* Print the pos array */
{
    int pos;
    Signal	*pos_sig_ptr, *sig_ptr;

    printf ("POS array:\n");
    for (pos=0; pos<=max_pos; pos++) {
	pos_sig_ptr = signal_by_pos[pos];
	if (pos_sig_ptr) {
	    printf ("\t%d\t%s\n", pos, pos_sig_ptr->signame);
	    for (sig_ptr=pos_sig_ptr->verilog_next; sig_ptr; sig_ptr=sig_ptr->verilog_next) {
		printf ("\t\t\t%s\n", sig_ptr->signame);
	    }
	}
    }
}

void	verilog_process_definitions (
    /* Process the definitions */
    Trace	*trace)
{
    uint_t	max_pos;
    Signal	*sig_ptr;
    Signal	*pos_sig_ptr, *next_sig_ptr;
    uint_t	pos_level, level;
    uint_t	pos;
    char	*tp;
    
    /* if (DTPRINT_FILE) print_sig_names (NULL, trace); */
    
    /* Find the highest pos used */
    max_pos = 0;
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	max_pos = MAX (max_pos, sig_ptr->file_pos + sig_ptr->bits);
    }
    
    /* Allocate space for one signal pointer for each of the possible codes */
    /* This will, of course, use a lot of memory for large traces.  It will be very fast though */
    signal_by_pos = (Signal **)XtMalloc (sizeof (Signal *) * (max_pos + 1));
    memset (signal_by_pos, 0, sizeof (Signal *) * (max_pos + 1));

    /* Also set aside space so every signal could change in a single cycle */
    signal_update_array = (Signal **)XtMalloc (sizeof (Signal *) * (max_pos + 1));
    signal_update_array_last_pptr = signal_update_array;
    
    /* Assign signals to the pos array.  The array points to the "original" */
    /*	-> Note that a single position can have multiple bits (if sig_ptr->file_type is true) */
    /* The original then has a linked list to other copies, the sig_ptr->verilog_next field */
    /* The original is signal highest in hiearchy, OR first to occur if at the same level */
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	for (pos = sig_ptr->file_pos; 
	     pos <= sig_ptr->file_pos + ((sig_ptr->file_type.flag.perm_vector)?0:sig_ptr->bits);
	     pos++) {
	    pos_sig_ptr = signal_by_pos[pos];
	    if (!pos_sig_ptr) {
		signal_by_pos[pos] = sig_ptr;
	    }
	    else {
		for (level=0, tp=sig_ptr->signame; *tp; tp++) {
		    if (*tp=='.') level++;
		}
		for (pos_level=0, tp=pos_sig_ptr->signame; *tp; tp++) {
		    if (*tp=='.') pos_level++;
		}
		if (level < pos_level) {
		    signal_by_pos[pos] = sig_ptr;
		    sig_ptr->verilog_next = pos_sig_ptr;
		}
		else {
		    sig_ptr->verilog_next = pos_sig_ptr->verilog_next;
		    pos_sig_ptr->verilog_next = sig_ptr;
		}
	    }
	}
    }

    /* Print the pos array */
    /* if (DTPRINT_FILE) verilog_print_pos (max_pos); */

    /* Kill all but the first copy of the signals.  This may be changed in later releases. */
    for (pos=0; pos<=max_pos; pos++) {
	pos_sig_ptr = signal_by_pos[pos];
	if (pos_sig_ptr) {
	    for (sig_ptr=pos_sig_ptr->verilog_next; sig_ptr; sig_ptr=next_sig_ptr) {
		/* Delete this signal */
		next_sig_ptr = sig_ptr->verilog_next;
		sig_free (trace, sig_ptr, FALSE, FALSE);
	    }
	    pos_sig_ptr->verilog_next = NULL;		/* Zero in prep of make_busses */
	}
	signal_by_pos[pos] = NULL;			/* Zero in prep of make_busses */
    }
    
    /* Make the busses */
    /* The pos creation is first because there may be vectors that map to single signals. */
    read_make_busses (trace, TRUE);

    /* Assign signals to the pos array **AGAIN**. */
    /* Since busses now exist, the pointers will all be different */
    if (DTPRINT_FILE) printf ("Reassigning signals to pos array./n");
    memset (signal_by_pos, 0, sizeof (Signal *) * (max_pos + 1));
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	for (pos = sig_ptr->file_pos; 
	     pos <= sig_ptr->file_pos + ((sig_ptr->file_type.flag.perm_vector)?0:sig_ptr->bits);
	     pos++) {
	    /* If already assigned, this is a signal that was womp_128ed. */
	    if (!signal_by_pos[pos]) signal_by_pos[pos] = sig_ptr;
	}
    }

    /* Print the pos array */
    if (DTPRINT_FILE) verilog_print_pos (max_pos);
}

void	verilog_enter_busses (
    /* If at a new time a signal has had its state non zero then */
    /* enter the file_value as a new cptr */
    Trace	*trace,
    int		first_data,
    int		time)
{
    Signal	*sig_ptr;
    Signal	**sig_upd_pptr;

    for (sig_upd_pptr = signal_update_array; sig_upd_pptr < signal_update_array_last_pptr; sig_upd_pptr++) {
	sig_ptr = *sig_upd_pptr;
	if (sig_ptr->file_value.siglw.sttime.state) {
	    /*if (DTPRINT_FILE) { printf ("Entered: "); print_cptr (&(sig_ptr->file_value)); } */

	    /* Enter the cptr */
	    sig_ptr->file_value.siglw.sttime.time = time;
	    fil_add_cptr (sig_ptr, &(sig_ptr->file_value), first_data);

	    /* Zero the state and keep the value for next time */
	    sig_ptr->file_value.siglw.number = 0;
	    /*if (DTPRINT_FILE) { printf ("Exited: "); print_cptr (&(sig_ptr->file_value)); } */
	}
    }
    /* All updated */
    signal_update_array_last_pptr = signal_update_array;
}

void	verilog_read_data (
    Trace	*trace,
    FILE	*readfp)
{
    char	*value_strg, *line, *code;
    DTime	time;
    Boolean	first_data=TRUE;
    Boolean	got_data=FALSE;
    Boolean	got_time=FALSE;
    Value	value;
    Signal	*sig_ptr;
    int		pos;
    int		state;

    if (DTPRINT_ENTRY) printf ("In verilog_read_data\n");

    time = 0;

    while (1) {
	fgets_dynamic (&verilog_line_storage, &verilog_line_length, readfp);
	verilog_line_num++;
	line = verilog_line_storage;
	if (feof(readfp)) break;
	/*if (verilog_line_num % 5000 == 0) printf ("Line %d\n", verilog_line_num);*/
	if (DTPRINT_FILE) {
	    line[strlen(line)-1]= '\0';
	    printf ("line='%s'\n",line);
	}

	switch (*line++) {
	    /* Single bits are most common */
	  case '0':
	    state = STATE_0;
	    goto common;
	  case '1':
	    state = STATE_1;
	    goto common;
	  case 'z':
	    state = STATE_Z;
	    goto common;
	  case 'x':
	    state = STATE_U;
	    goto common;

	  common:
	    got_data = TRUE;

	    verilog_read_parameter (line, code);
	    sig_ptr = trace->firstsig;
	    pos = VERILOG_ID_TO_POS(code);
	    sig_ptr = signal_by_pos[ pos ];
	    if (sig_ptr) {
		/* printf ("\tsignal '%s'=%d %s  state %d\n", code, pos, sig_ptr->signame, state); */
		if (sig_ptr->bits == 0) {
		    /* Not a vector.  This is easy */
		    value.siglw.sttime.state = state;
		    value.siglw.sttime.time = time;
		    fil_add_cptr (sig_ptr, &value, first_data);
		}
		else {	/* Unary signal made into a vector */
		    /* Mark this for update at next time stamp */
		    *(signal_update_array_last_pptr++) = sig_ptr;
		    /* printf ("Pre: "); print_cptr (&(sig_ptr->file_value)); */
		    switch (state) {
		      case STATE_U:
		      default:
			sig_ptr->file_value.siglw.sttime.state = STATE_U;
			break;

		      case STATE_Z:
			/* Make a U if the signal has any 0, 1, or Us */
			sig_ptr->file_value.siglw.sttime.state = 
			    ( (sig_ptr->file_value.siglw.sttime.state == STATE_Z)
			     || (sig_ptr->file_value.siglw.sttime.state == STATE_0) )
				? STATE_Z : STATE_U;
			break;

		      case STATE_0:
			if ( (sig_ptr->file_value.siglw.sttime.state == STATE_Z)
			    || (sig_ptr->file_value.siglw.sttime.state == STATE_U)) {
			    sig_ptr->file_value.siglw.sttime.state = STATE_U;
			}
			else {
			    register int bit = sig_ptr->bits - (pos - sig_ptr->file_pos); 

			    sig_ptr->file_value.siglw.sttime.state = sig_ptr->type;
			    if (bit < 32) {
				sig_ptr->file_value.number[0] = 
				    ( sig_ptr->file_value.number[0] & (~ (1<<bit)) );
			    }
			    else if (bit < 64) {
				bit -= 32;
				sig_ptr->file_value.number[1] = 
				    ( sig_ptr->file_value.number[1] & (~ (1<<bit)) );
			    }
			    else if (bit < 96) {
				bit -= 64;
				sig_ptr->file_value.number[2] = 
				    ( sig_ptr->file_value.number[2] & (~ (1<<bit)) );
			    }
			    else if (bit < 128) {
				bit -= 96;
				sig_ptr->file_value.number[3] = 
				    ( sig_ptr->file_value.number[3] & (~ (1<<bit)) );
			    }
			    else printf ("%%E, Signal too wide on line %d of %s\n",
					 verilog_line_num, current_file);
			}
			break;

		      case STATE_1:
			if ( (sig_ptr->file_value.siglw.sttime.state == STATE_Z)
			    || (sig_ptr->file_value.siglw.sttime.state == STATE_U)) {
			    sig_ptr->file_value.siglw.sttime.state = STATE_U;
			}
			else {
			    register int bit = sig_ptr->bits - (pos - sig_ptr->file_pos); 

			    sig_ptr->file_value.siglw.sttime.state = sig_ptr->type;
			    if (bit < 32) {
				sig_ptr->file_value.number[0] = 
				    ( sig_ptr->file_value.number[0] | (1<<bit) );
			    }
			    else if (bit < 64) {
				bit -= 32;
				sig_ptr->file_value.number[1] = 
				    ( sig_ptr->file_value.number[1] | (1<<bit) );
			    }
			    else if (bit < 96) {
				bit -= 64;
				sig_ptr->file_value.number[2] = 
				    ( sig_ptr->file_value.number[2] | (1<<bit) );
			    }
			    else if (bit < 128) {
				bit -= 96;
				sig_ptr->file_value.number[3] = 
				    ( sig_ptr->file_value.number[3] | (1<<bit) );
			    }
			    else printf ("%%E, Signal too wide on line %d of %s\n",
					 verilog_line_num, current_file);
			}
			break;
		    }
		    /* if (DTPRINT_FILE) print_cptr (&(sig_ptr->file_value)); */
		}
	    }
	    else printf ("%%E, Unknown <identifier_code> '%s'\n", code);
	    break;

	  case 'b':
	    if (first_data) {
		trace->start_time = time;
		trace->end_time = time;
		first_data = FALSE;
	    }
	    got_data = TRUE;

	    verilog_read_parameter (line, value_strg);
	    verilog_read_parameter (line, code);
	    sig_ptr = trace->firstsig;
	    pos = VERILOG_ID_TO_POS(code);
	    sig_ptr = signal_by_pos[ pos ];
	    if (sig_ptr) {
		if ((sig_ptr->bits == 0) || !sig_ptr->file_type.flag.perm_vector) {
		    printf ("%%E, Vector decode on single-bit or non perm_vector signal on line %d of %s\n",
			    verilog_line_num, current_file);
		}
		else {
		    register int len;

		    for (; sig_ptr; sig_ptr=sig_ptr->verilog_next) {
			/* Verilog requires us to extend the value if it is too short */
			len = (sig_ptr->bits+1) - strlen (value_strg);
			if (len > 0) {
			    /* This is rare, so not fast */
			    /* 1's extend as 0's, x as x */
			    register char extend_char = (value_strg[0]=='1')?'0':value_strg[0];
			    char *line_copy;
			    
			    line_copy = strdup(value_strg);
			
			    line = value_strg;
			    while (len>0) {
				*line++ = extend_char;
				len--;
			    }
			    *line++ = '\0';
			    
			    strcat (line, line_copy);	/* tack on the original value */
			    if (DTPRINT_FILE) printf ("Sign extended %s to %s\n", value_strg, line);
			    XtFree (line_copy);
			}
	
			/* Store the file information */
			/* if (DTPRINT_FILE) printf ("\tsignal '%s'=%d %s  value %s\n", code, pos, sig_ptr->signame, value_strg); */
			fil_string_add_cptr (sig_ptr, value_strg, time, first_data);

			/* Push string past this signal's bits */
			value_strg += sig_ptr->bits+1;
		    }
		}
	    }
	    else printf ("%%E, Unknown <identifier_code> '%s' on line %d of %s\n",
			 code, verilog_line_num, current_file);
	    break;

	    /* Times are next most common */
	  case '#':	/* Time stamp */
	    verilog_enter_busses (trace, first_data, time);
	    time = (atol (line) * time_scale) / time_divisor;	/* 1% of time in this division! */
	    if (DTPRINT_FILE) {
		printf (" %ld * ( %d / %d )\n", atol(line), time_scale, time_divisor);
		printf ("Time %d start %d first %d got %d\n", time, trace->start_time, first_data, got_data);
	    }
	    if (first_data) {
		if (got_time) {
		    if (got_data) first_data = FALSE;
		    got_data = FALSE;
		}
		got_time = TRUE;
		trace->start_time = time;
	    }
	    else {
		trace->end_time = time;
	    }
	    break;

	    /* Things to ignore, uncommon */
	  case '\n':
	  case '$':	/* Command, $end, $dump, etc (ignore) */
	  case 'r':	/* Real number */
	    break;

	  default:
	    printf ("%%E, Unknown line character in verilog trace '%c' on line %d of %s\n",
		    *(line-1), verilog_line_num, current_file);
	}
    }

    /* May be left overs from the last line */
    verilog_enter_busses (trace, first_data, time);
}

void	verilog_process_lines (
    Trace	*trace,
    FILE	*readfp)
{
    char	*cmd, *tp;
    char	*line;

    line = NULL;
    verilog_line_length = 0;

    while (!feof (readfp)) {
	/* Read line & kill EOL at end */
	fgets_dynamic (&verilog_line_storage, &verilog_line_length, readfp);
	line = verilog_line_storage;
	if (*(tp=(line+strlen(line)-1))=='\n') *tp='\0';
	verilog_line_num++;

	/* if (DTPRINT_FILE) printf ("line='%s'\n",line); */

	/* Find first command */
	while (*line && *line!='$') line++;
	if (!*line) continue;
	
	/* extract command */
	line++;
	verilog_read_parameter (line, cmd);
	
	/* Note that these are in most frequent first ordering */
	if (!strcmp(cmd, "end")) {
	}
	else if (!strcmp(cmd, "var")) {
	    verilog_process_var (trace, line);
	}
	else if (!strcmp(cmd, "upscope")) {
	    if (scope_level > 0) scope_level--;
	}
	else if (!strcmp(cmd, "scope")) {
	    /* Skip <scopetype> = module, task, function, begin, fork */
	    verilog_skip_parameter (line);
	    
	    /* Null terminate <identifier> */
	    verilog_read_parameter (line, cmd);
	    
	    if (scope_level < MAXSCOPES) {
		strcpy (scopes[scope_level], cmd);
		/* if (DTPRINT_FILE) printf ("added scope, %d='%s'\n", scope_level, scopes[scope_level]); */
		scope_level++;
	    }
	    else {
		if (DTPRINT_FILE) printf ("%%E, Too many scope levels on verilog line %d\n", verilog_line_num);
		
		sprintf (message, "Too many scope levels on line %d of %s\n",
			 verilog_line_num, current_file);
		dino_error_ack (trace,message);
	    }
	}
	else if (!strcmp (cmd, "date")
		 || !strcmp (cmd, "version")
		 || !strcmp (cmd, "comment")) {
	    verilog_read_till_end (line, readfp);
	}
	else if (!strcmp(cmd, "timescale")) {
	    verilog_read_timescale (trace, line, readfp);
	}
	else if (!strcmp (cmd, "enddefinitions")) {
	    verilog_process_definitions (trace);
	    verilog_read_data (trace, readfp);
	}
	else {
	    if (DTPRINT_FILE) printf ("%%E, Unknown command '%s' on verilog line %d\n", cmd, verilog_line_num);
	    
	    sprintf (message, "Unknown command '%s' on line %d of %s\n",
		     cmd, verilog_line_num, current_file);
	    dino_error_ack (trace,message);
	}
    }

    verilog_line_length = 0;
    XtFree (verilog_line_storage);
}

void verilog_read (
    Trace	*trace,
    int		read_fd)
{
    FILE	*readfp;

    time_divisor = time_units_to_multiplier (global->time_precision);
    time_scale = time_divisor;

    /* Make file into descriptor */
    readfp = fdopen (read_fd, "r");
    if (!readfp) {
	return;
    }

    /* Signal description data */
    last_sig_ptr = NULL;
    trace->firstsig = NULL;
    signal_by_pos = NULL;

    verilog_line_num=0;
    current_file = trace->filename;
    verilog_process_lines (trace, readfp);

    /* Free up */
    DFree (signal_by_pos);
}


