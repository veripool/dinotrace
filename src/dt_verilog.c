#pragma ident "$Id$"
/******************************************************************************
 * dt_verilog.c --- Verilog dump file reading
 *
 * This file is part of Dinotrace.  
 *
 * Author: Wilson Snyder <wsnyder@ultranet.com> or <wsnyder@iname.com>
 *
 * Code available from: http://www.ultranet.com/~wsnyder/veripool/dinotrace
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
#define MAXSCOPES 100
static char	scopes[MAXSCOPES][MAXSIGLEN];
static int	scope_level;
static DTime_t	time_divisor, time_scale;

static int	verilog_fd;
static char	*verilog_text = NULL;
static Boolean_t verilog_eof;

static Signal	*last_sig_ptr;		/* last signal read in */

/* Pointer to array of signals sorted by pos. (Special hash table) */
/* *(signal_by_pos[VERILOG_ID_TO_POS ("abc")]) gives signal ABC */
static Signal	**signal_by_pos;	
static uint_t	signal_by_pos_max;
static uint_t	verilog_max_bits;

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

#define VERILOG_POS_TO_SIG(_pos_) \
    (((_pos_)<signal_by_pos_max)?signal_by_pos[(_pos_)]:0)

/**********************************************************************/

#define FGETS_SIZE_INC	10	/* Characters to increase length by (set small for testing) */

static void	verilog_gets_init (int read_fd)
{
    verilog_fd = read_fd;
    verilog_line_num=1;
    verilog_text = NULL;
    verilog_eof = 0;
}

#define VERILOG_ISSPACE(ch) ((ch)<=32)	/* Fast and very dirty */
static char *verilog_gettok ()
{
    static int buf_length = 0;
    static char *buf_ptr = NULL;
    static int buf_valid = 0;
    char *cp;
    int status;

    if (verilog_text==NULL) {
	/* Init */
	if (buf_length==0) {
	    /* First init */
	    fgets_dynamic_extend (&buf_ptr, &buf_length, FGETS_SIZE_INC);
	}
	buf_valid = 0;
	verilog_text = buf_ptr;
	verilog_text[0] = '\0';
    }

    /* Advance text pointer to EOL + 1 */
    while (*verilog_text) verilog_text++;
    verilog_text++;
    while (1) {
	/*int chrs = (buf_ptr + buf_length - verilog_text);
	  printf ("Buf %p %d  Txt %p %d rem %d\n", buf_ptr, buf_length, verilog_text, buf_valid, chrs);*/
	    
	/* Skip any additional spaces */
	while (VERILOG_ISSPACE(*verilog_text) && verilog_text < (buf_ptr + buf_valid)) {
	    if (*verilog_text == '\n') verilog_line_num++;
	    verilog_text++;
	}
	/* Is there a space or newline after this? */
	cp = verilog_text;
	while (cp < (buf_ptr + buf_valid)) {
	    if (!VERILOG_ISSPACE(*cp)) { cp++; continue;}
	    /* Done! */
	    /* Convert newline to null & return */
	    *cp = '\0';
	    if (*verilog_text == '\n') verilog_line_num++;
	    if (DTPRINT_FILE) printf ("Got line '%s'\n", verilog_text);
	    return (verilog_text);
	}
	if (verilog_text == buf_ptr) {
	    if (verilog_eof) {
		if (DTPRINT_FILE) printf ("EOF\n");
		buf_ptr[0] = '\0';
		return (buf_ptr);
	    }
	    /* Started at buffer beginning, no spaces, so increase buffer, read new space */
	    fgets_dynamic_extend (&buf_ptr, &buf_length, buf_length + FGETS_SIZE_INC);
	    verilog_text = buf_ptr;
	}
	else {
	    /* Move stuff at end of buffer to beginning */
	    int mov_cnt = (buf_ptr + buf_valid - verilog_text);
	    if (mov_cnt>0) {
		char *cp;
		char *sp;
		/* Can't memcpy... may not allow overlap */
		buf_valid = mov_cnt;
		for (sp = verilog_text, cp = buf_ptr; mov_cnt--; ) {
		    *cp++ = *sp++;
		}
	    }
	    else buf_valid = 0;
	    verilog_text = buf_ptr;
	}
	status = read (verilog_fd, buf_ptr+buf_valid, buf_length-buf_valid);
	/*printf ("Read init %p %d/%d ptr %p ch %d got %d\n", buf_ptr, buf_length, buf_valid, buf_ptr+buf_valid, buf_length-buf_valid, status);
	  printf ("Buf = %s\n", buf_ptr);*/
	if (status > 0) buf_valid += status;
	else {
	    verilog_eof = 1;
	    *(buf_ptr+buf_valid) = '\n';	/* We alloc +1, so this won't trash anything if full buffer */
	}
	continue;
    }
    return (NULL); /* True return case is above */
}

static void	verilog_read_till_end ()
{
    char *line;
    while (!verilog_eof) {
	line = verilog_gettok();
	if (!strncmp (line, "$end", 4)) {
	    return;
	}
    }
}

static void	verilog_read_till_eol ()
{
    char *line;
    while (!verilog_eof) {
	line = verilog_gettok();
	/*printf ("Skipped '%s'\n", line);*/
	if (!strncmp (line, "$end", 4)) {
	    return;
	}
    }
}

static void	verilog_read_timescale (
    Trace	*trace)
{
    char *line = verilog_gettok();
    time_scale = atol (line);
    while (isdigit (*line)) line++;
    if (!*line) line = verilog_gettok();	/* Allow '10 ns' */
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

    verilog_read_till_end ();
}

static void	verilog_process_var (
    /* Process a VAR statement (read a signal) */
    Trace	*trace)
{
    char	*cmd;
    Boolean_t	is_real;
    int		bits;
    Signal	*new_sig_ptr;
    char	signame[10000];
    int		t, len;
    char	code[10];

    /* Read <vartype> */
    cmd = verilog_gettok();
    is_real = (!strcmp(cmd,"real"));
	
    /* Read <size> */
    cmd = verilog_gettok();
    bits = atoi (cmd);

    /* Read <identifier_code>  (uses chars 33-126 = '!' - '~') */
    cmd = verilog_gettok();
    strcpy (code, cmd);
    
    /* Signal name begins with the present scope */
    signame[0] = '\0';
    for (t=0; t<scope_level; t++) {
	strcat (signame, scopes[t]);
	strcat (signame, ".");
    }
    
    /* Read <reference> */
    cmd = verilog_gettok();
    strcat (signame, cmd);
    cmd = verilog_gettok();
    if (*cmd != '$' && strcmp(cmd, "[-1]")) strcat (signame, cmd);
    
    /* Allocate new signal structure */
    new_sig_ptr = DNewCalloc (Signal);
    new_sig_ptr->trace = trace;
    new_sig_ptr->dfile = &(trace->dfile);
    new_sig_ptr->radix = global->radixs[0];
    if (is_real) {
	new_sig_ptr->radix = global->radixs[RADIX_REAL];
    }

    new_sig_ptr->file_pos = VERILOG_ID_TO_POS(code);
    new_sig_ptr->bits = bits;
    new_sig_ptr->msb_index = 0;
    new_sig_ptr->lsb_index = 0;

    new_sig_ptr->file_type.flags = 0;
    new_sig_ptr->file_type.flag.perm_vector = (bits>1);	/* If a vector already then we won't vectorize it */

    if (DTPRINT_FILE) printf ("var %s %s/%d %s %s\n", is_real?"real":"reg/wire", cmd,bits, code, signame);

    val_zero (&(new_sig_ptr->file_value));

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

    /*sig_print_names (NULL, trace);*/
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
	    new_sig_ptr->copyof = sig_ptr->copyof;
	    new_sig_ptr->verilog_next = sig_ptr->verilog_next;
	    sig_ptr->verilog_next = new_sig_ptr;

	    /* How much to remove */
	    chop = (sig_ptr->bits) % 128;
	    if (chop==0) chop=128;
	    /*printf ("chopping %d\n",chop);*/

 	    /* Make new be remainders */
 	    new_sig_ptr->bits = sig_ptr->bits - chop;
 	    new_sig_ptr->msb_index = sig_ptr->msb_index - chop;
  
  	    /* Shorten bits of old */
 	    sig_ptr->bits = chop;
 	    sig_ptr->lsb_index = sig_ptr->msb_index - sig_ptr->bits + 1;

	    /* Next is new guy, may be >> 128 also */
	    sig_ptr = new_sig_ptr->backward;
	}
	else {
	    sig_ptr = sig_ptr->forward;
	}
    }

    /*sig_print_names (NULL, trace);*/
}


static void	verilog_print_pos (void)
    /* Print the pos array */
{
    int pos;
    Signal	*pos_sig_ptr, *sig_ptr;

    printf ("POS array:\n");
    for (pos=0; pos<signal_by_pos_max; pos++) {
	pos_sig_ptr = signal_by_pos[pos];
	if (pos_sig_ptr) {
	    printf ("\t%d\t%s\n", pos, pos_sig_ptr->signame);
	    for (sig_ptr=pos_sig_ptr->verilog_next; sig_ptr; sig_ptr=sig_ptr->verilog_next) {
		printf ("\t\t\t%s\n", sig_ptr->signame);
	    }
	}
    }
}

static void	verilog_process_definitions (
    /* Process the definitions */
    Trace	*trace)
{
    Signal	*sig_ptr;
    Signal	*pos_sig_ptr, *last_sig_ptr;
    uint_t	pos_level, level;
    uint_t	pos;
    char	*tp;
    
    /* if (DTPRINT_FILE) sig_print_names (trace); */
    
    /* Find the highest pos used & max number of bits */
    signal_by_pos_max = 0;
    verilog_max_bits = 0;
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	signal_by_pos_max = MAX (signal_by_pos_max, sig_ptr->file_pos + sig_ptr->bits - 1 + 1);
	verilog_max_bits = MAX (verilog_max_bits, sig_ptr->bits);
    }
    
    /* Allocate space for one signal pointer for each of the possible codes */
    /* This will, of course, use a lot of memory for large traces.  It will be very fast though */
    signal_by_pos = (Signal **)XtMalloc (sizeof (Signal *) * (signal_by_pos_max + 1));
    memset (signal_by_pos, 0, sizeof (Signal *) * (signal_by_pos_max + 1));

    /* Also set aside space so every signal could change in a single cycle */
    signal_update_array = (Signal **)XtMalloc (sizeof (Signal *) * (signal_by_pos_max + 1));
    signal_update_array_last_pptr = signal_update_array;
    
    /* Assign signals to the pos array.  The array points to the "original" */
    /*	-> Note that a single position can have multiple bits (if sig_ptr->file_type is true) */
    /* The original then has a linked list to other copies, the sig_ptr->verilog_next field */
    /* The original is signal highest in hiearchy, OR first to occur if at the same level */
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	for (pos = sig_ptr->file_pos; 
	     pos <= sig_ptr->file_pos
		 + ((sig_ptr->file_type.flag.perm_vector)?0:(sig_ptr->bits-1));
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
		/*printf ("Compare %d %d  %s %s\n",level, pos_level, sig_ptr->signame, pos_sig_ptr->signame);*/
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
    /* if (DTPRINT_FILE) verilog_print_pos (); */

    /* Kill all but the first copy of the signals.  This may be changed in later releases. */
    for (pos=0; pos<signal_by_pos_max; pos++) {
	pos_sig_ptr = signal_by_pos[pos];
	if (pos_sig_ptr) {
	    last_sig_ptr = pos_sig_ptr;
	    while (NULL!=(sig_ptr=last_sig_ptr->verilog_next)) {
		/* Delete this signal */
		if (global->save_duplicates
		    && sig_ptr->bits == pos_sig_ptr->bits) {
		    sig_ptr->copyof = pos_sig_ptr;
		    last_sig_ptr = sig_ptr;
		    /*printf ("Dup %s\t  %s\n", pos_sig_ptr->signame, sig_ptr->signame);*/
		} else {
		    last_sig_ptr->verilog_next = sig_ptr->verilog_next;
		    sig_free (trace, sig_ptr, FALSE, FALSE);
		}
	    }
	    pos_sig_ptr->verilog_next = NULL;		/* Zero in prep of make_busses */
	}
	signal_by_pos[pos] = NULL;			/* Zero in prep of make_busses */
    }
    
    /* Make the busses */
    /* The pos creation is first because there may be vectors that map to single signals. */
    fil_make_busses (trace, TRUE);

    /* Assign signals to the pos array **AGAIN**. */
    /* Since busses now exist, the pointers will all be different */
    if (DTPRINT_FILE) printf ("Reassigning signals to pos array.\n");
    memset (signal_by_pos, 0, sizeof (Signal *) * (signal_by_pos_max + 1));
    for (pos_sig_ptr = trace->firstsig; pos_sig_ptr; pos_sig_ptr = pos_sig_ptr->forward) {
	for (pos = pos_sig_ptr->file_pos; 
	     pos <= pos_sig_ptr->file_pos
		 + ((pos_sig_ptr->file_type.flag.perm_vector)?0:(pos_sig_ptr->bits-1));
	     pos++) {
	    if (pos_sig_ptr->copyof) {
		pos_sig_ptr->bptr = pos_sig_ptr->copyof->bptr;
		pos_sig_ptr->cptr = pos_sig_ptr->copyof->cptr;
		if (DTPRINT_FILE) printf ("StillDup %s\t  %s\n", pos_sig_ptr->signame, pos_sig_ptr->copyof->signame);
	    } else {
		if (!signal_by_pos[pos]) {
		    signal_by_pos[pos] = pos_sig_ptr;
		} else {
		    /* If already assigned, this is a signal that was womp_128ed. */
		    /* or, we're saving duplicates */
		}
	    }
	}
    }

    /* Print the pos array */
    if (DTPRINT_FILE) verilog_print_pos ();
}

static inline void verilog_prep_busses (
    Signal	*sig_ptr,
    int bit,
    int state,
    int first_data)
    /* About to do sub-bit changes on this signal, get last value */	
{
    int b;
    if (sig_ptr->file_value.siglw.stbits.state == 0) {
	/* First change to this signal */
	*(signal_update_array_last_pptr++) = sig_ptr;
	val_zero (&(sig_ptr->file_value));
	if (first_data) { /* Default to X of appropriate width */
	    for (b=0; b < sig_ptr->bits; b++) {
		sig_ptr->file_value.number[4 + (b>>5)] |= (1<<(b&31));
	    }
	} else {
	    Value_t *pcptr;
	    pcptr = sig_ptr->cptr;
	    /*if (DTPRINT_FILE) { printf ("Copied: "); print_cptr (pcptr); print_sig_info (sig_ptr);} */
	    /*Extract to T128 form for next comparison */
	    switch (pcptr->siglw.stbits.state) {
	    case STATE_0:
		break;
	    case STATE_1:
		sig_ptr->file_value.number[0] = 1;
		break;
	    case STATE_U:
		for (b=0; b < sig_ptr->bits; b++) {
		    sig_ptr->file_value.number[4 + (b>>5)] |= (1<<(b&31));
		}
		break;
	    case STATE_Z:
		for (b=0; b < sig_ptr->bits; b++) {
		    sig_ptr->file_value.number[0 + (b>>5)] |= (1<<(b&31));
		    sig_ptr->file_value.number[4 + (b>>5)] |= (1<<(b&31));
		}
		break;
	    case STATE_F32:
		sig_ptr->file_value.number[0] = pcptr->number[0];
		sig_ptr->file_value.number[4] = pcptr->number[1];
		break;
	    case STATE_F128:
		/* FALLTHRU */
		sig_ptr->file_value.number[7] = pcptr->number[7];
		sig_ptr->file_value.number[6] = pcptr->number[6];
		sig_ptr->file_value.number[5] = pcptr->number[5];
		sig_ptr->file_value.number[4] = pcptr->number[4];
	    case STATE_B128:
		/* FALLTHRU */
		sig_ptr->file_value.number[3] = pcptr->number[3];
		sig_ptr->file_value.number[2] = pcptr->number[2];
		sig_ptr->file_value.number[1] = pcptr->number[1];
	    case STATE_B32:
		sig_ptr->file_value.number[0] = pcptr->number[0];
		break;
	    }
	}
	sig_ptr->file_value.siglw.stbits.state = STATE_F128;
    }
    /*if (DTPRINT_FILE) { printf ("Prechg: "); print_cptr (&(sig_ptr->file_value)); } */
    if (state & 1) {
	sig_ptr->file_value.number[(bit>>5)] |= (1<<(bit&31));
    } else {
	sig_ptr->file_value.number[(bit>>5)] &= ~(1<<(bit&31));
    }
    if (state & 2) {
	sig_ptr->file_value.number[4 + (bit>>5)] |= (1<<(bit&31));
    } else {
	sig_ptr->file_value.number[4 + (bit>>5)] &= ~(1<<(bit&31));
    }
    if (bit>127) printf ("%%E, Signal too wide on line %d of %s\n",
			 verilog_line_num, current_file);
    /*if (DTPRINT_FILE) { printf ("Aftchg: "); print_cptr (&(sig_ptr->file_value)); } */
}

static void	verilog_enter_busses (
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
	if (sig_ptr->file_value.siglw.stbits.state) {
	    /*if (DTPRINT_FILE) { printf ("Entered: "); print_cptr (&(sig_ptr->file_value)); } */
	    if (DTPRINT_FILE) { printf ("Entered: "); print_cptr (&(sig_ptr->file_value)); } 

	    /* Make cptr have correct state */
	    val_minimize (&(sig_ptr->file_value));

	    /* Enter the cptr */
	    sig_ptr->file_value.time = time;
	    fil_add_cptr (sig_ptr, &(sig_ptr->file_value), first_data);

	    /* Zero the state for next time */
	    sig_ptr->file_value.siglw.number = 0;
	    /*if (DTPRINT_FILE) { printf ("Exited: "); print_cptr (&(sig_ptr->file_value)); } */
	}
    }
    /* All updated */
    signal_update_array_last_pptr = signal_update_array;
}

static void	verilog_read_data (
    Trace	*trace)
{
    char	*value_strg, *line, *code;
    DTime_t	time;
    Boolean_t	first_data=TRUE;
    Boolean_t	got_data=FALSE;
    Boolean_t	got_time=FALSE;
    Value_t	value;
    Signal	*sig_ptr;
    int		pos;
    int		state = STATE_Z;
    char	*scratchline;
    char	*scratchline2;
    double	dnum; 

    if (DTPRINT_ENTRY) printf ("In verilog_read_data\n");

    time = 0;
    scratchline = (char *)XtMalloc(100+verilog_max_bits);
    scratchline2 = (char *)XtMalloc(100+verilog_max_bits);
    
    while (!verilog_eof) {
	line = verilog_gettok();
	if (verilog_eof) return;

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

	    code = line;
	    pos = VERILOG_ID_TO_POS(code);
	    sig_ptr = VERILOG_POS_TO_SIG(pos);
	    if (sig_ptr) {
		/* printf ("\tsignal '%s'=%d %s  state %d\n", code, pos, sig_ptr->signame, state); */
		if (sig_ptr->bits < 2) {
		    /* Not a vector.  This is easy */
		    val_zero (&value);
		    value.siglw.stbits.state = state;
		    value.time = time;
		    fil_add_cptr (sig_ptr, &value, first_data);
		}
		else {	/* Unary signal made into a vector */
		    register int bit = sig_ptr->bits - 1 - (pos - sig_ptr->file_pos); 
		    /* Mark this for update at next time stamp */
		    verilog_prep_busses (sig_ptr, bit, state, first_data);
		}
		/* if (DTPRINT_FILE) print_cptr (&(sig_ptr->file_value)); */
	    } /* if sig_ptr */
	    else if (DTDEBUG || DTPRINT_FILE) printf ("%%E, Unknown <identifier_code> '%s'\n", code);
	    break;

	case 'b':
	    got_data = TRUE;

	    value_strg = line;
	    strcpy (scratchline2, line);
	    value_strg = scratchline2;
	    code = verilog_gettok();
	    pos = VERILOG_ID_TO_POS(code);
	    sig_ptr = VERILOG_POS_TO_SIG(pos);
	    if (sig_ptr) {
		if ((sig_ptr->bits < 2) || !sig_ptr->file_type.flag.perm_vector) {
		    /* For some idiotic reason vpd2vcd likes to do this! */
		    /* This costs extra bytes and time... Oh well... */
		    switch (value_strg[0]) {
		    case '0': state = STATE_0; break;
		    case '1': state = STATE_1; break;
		    case 'z': state = STATE_Z; break;
		    case 'x': state = STATE_U; break;
		    default: printf ("%%E, Strange bit value %c on line %d of %s\n",
				     value_strg[0], verilog_line_num, current_file);
		    }
		    val_zero (&value);
		    value.siglw.stbits.state = state;
		    value.time = time;
		    fil_add_cptr (sig_ptr, &value, first_data);
		}
		else {
		    register int len;

		    for (; sig_ptr; sig_ptr=sig_ptr->verilog_next) {
			if (!sig_ptr->copyof) {
			    /* Verilog requires us to extend the value if it is too short */
			    len = (sig_ptr->bits) - strlen (value_strg);
			    if (len > 0) {
				/* This is rare, so not fast (see bradshaw.dmp)*/
				/* 1's extend as 0's, x as x */
				register char extend_char = (value_strg[0]=='1')?'0':value_strg[0];
				char *cp;
				strcpy (scratchline, value_strg);
				cp = value_strg;
				while (len>0) {
				    *cp++ = extend_char;
				    len--;
				}
				*cp++ = '\0';
				
				strcat (value_strg, scratchline);	/* tack on the original value */
				if (DTPRINT_FILE) printf ("Sign extended %s to %s\n", scratchline, value_strg);
			    }
			    
			    /* Store the file information */
			    /*if (DTPRINT_FILE) printf ("\tsignal '%s'=%d %d %s  value %s\n", code, pos, time, sig_ptr->signame, value_strg);*/
			    ascii_string_add_cptr (sig_ptr, value_strg, time, first_data);
			    
			    /* Push string past this signal's bits */
			    value_strg += sig_ptr->bits;
			}
		    }
		}
	    }
	    else if (DTDEBUG || DTPRINT_FILE)
		printf ("%%E, Unknown <identifier_code> '%s' on line %d of %s\n",
			code, verilog_line_num, current_file);
	    break;

	    /* Times are next most common */
	case '#':	/* Time stamp */
	    verilog_enter_busses (trace, first_data, time);
	    time = (atol (line) * time_scale) / time_divisor;	/* 1% of time in this division! */
	    if (first_data && got_time && got_data) {
		first_data = FALSE;
	    } else if (first_data) {
		trace->start_time = time;
		got_time = TRUE;
		got_data = FALSE;
	    }
	    trace->end_time = time;
	    if (DTPRINT_FILE) {
		printf ("Time %d start %d first %d got %d  ", time, trace->start_time, first_data, got_data);
		printf (" %ld * ( %d / %d )\n", atol(line), time_scale, time_divisor);
	    }
	    break;

	case 'r':	/* Real number */
	    dnum = atof (line);
	    code = verilog_gettok();
	    pos = VERILOG_ID_TO_POS(code);
	    sig_ptr = VERILOG_POS_TO_SIG(pos);
	    if (sig_ptr && sig_ptr->bits == 64) {
		val_zero (&value);
		if (dnum==0) value.siglw.stbits.state = STATE_0;
		else {
		    value.siglw.stbits.state = STATE_B128;
		    *((double*)(&value.number[0])) = dnum;
		}
		value.time = time;
		fil_add_cptr (sig_ptr, &value, first_data);
	    }
	    
	    break;
	    
	    /* Things to ignore, uncommon */
	case '\n':
	case '$':	/* Command, $end, $dump, etc (ignore) */
	    break;

	case 'D':	/* 'Done' from vpd2vcd */
	    verilog_read_till_eol();
	    break;

	default:
	    printf ("%%E, Unknown line character in verilog trace '%c' on line %d of %s\n",
		    *(line-1), verilog_line_num, current_file);
	}
    }

    /* May be left overs from the last line */
    verilog_enter_busses (trace, first_data, time);

    DFree (scratchline2);
    DFree (scratchline);
}

static void	verilog_process_lines (
    Trace	*trace)
{
    char	*cmd;

    while (!verilog_eof) {
	/* Read line & kill EOL at end */
	cmd = verilog_gettok ();

	if (cmd[0] != '$') continue;
	cmd++;

	/* Note that these are in most frequent first ordering */
	if (!strcmp(cmd, "end")) {
	}
	else if (!strcmp(cmd, "var")) {
	    verilog_process_var (trace);
	}
	else if (!strcmp(cmd, "upscope")) {
	    if (scope_level > 0) scope_level--;
	}
	else if (!strcmp(cmd, "scope")) {
	    /* Skip <scopetype> = module, task, function, begin, fork */
	    verilog_gettok();
	    
	    /* Null terminate <identifier> */
	    cmd = verilog_gettok();
	    
	    if (scope_level < MAXSCOPES) {
		strcpy (scopes[scope_level], cmd);
		if (DTPRINT_FILE) printf ("added scope, %d='%s'\n", scope_level, scopes[scope_level]);
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
	    verilog_read_till_end ();
	}
	else if (!strcmp(cmd, "timescale")) {
	    verilog_read_timescale (trace);
	}
	else if (!strcmp (cmd, "enddefinitions")) {
	    verilog_process_definitions (trace);
	    verilog_read_data (trace);
	}
	else {
	    if (DTPRINT_FILE) printf ("%%E, Unknown command '%s' on verilog line %d\n", cmd, verilog_line_num);
	    
	    sprintf (message, "Unknown command '%s' on line %d of %s\n",
		     cmd, verilog_line_num, current_file);
	    dino_error_ack (trace,message);
	}
    }
}

void verilog_read (
    Trace	*trace,
    int		read_fd)
{
    time_divisor = time_units_to_multiplier (global->time_precision);
    time_scale = time_divisor;

    /* Signal description data */
    last_sig_ptr = NULL;
    trace->firstsig = NULL;
    signal_by_pos = NULL;

    current_file = trace->dfile.filename;
    verilog_gets_init (read_fd);
    verilog_process_lines (trace);

    /* Free up */
    DFree (signal_by_pos);
}


