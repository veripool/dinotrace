#ident "$Id$"
/******************************************************************************
 * DESCRIPTION: Dinotrace source: Verilog dump file reading
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
/* File locals */
static int verilog_line_num=0;
static char *current_file="";

/* Stack of scopes, 0=top level.  Grows up to MAXSCOPES */
#define MAXSCOPES 100
static char	scopes[MAXSCOPES][MAXSIGLEN];
static int	scope_level;
static double	time_scale;

static int	verilog_fd;
static char	*verilog_text = NULL;
static Boolean_t verilog_eof;

/* Pointer to array of signals sorted by pos. (Special hash table) */
/* *(signal_by_code[VERILOG_ID_TO_POS ("abc")]) gives signal ABC */
static Signal_t	**signal_by_code;	
static uint_t	signal_by_code_max;
static uint_t	verilog_max_bits = 128;

/* List of signals that need updating */
static Signal_t	**signal_update_array;	
static Signal_t	**signal_update_array_last_pptr;

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
    (((_pos_)<signal_by_code_max)?signal_by_code[(_pos_)]:0)

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
	while (verilog_text < (buf_ptr + buf_valid) && VERILOG_ISSPACE(*verilog_text)) {
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
    Trace_t	*trace)
{
    char *line = verilog_gettok();
    /*time_scale is global*/

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
	time_scale = 0.001;
	break;
    default:
	sprintf (message,"Unknown time scale unit '%c' on line %d of %s\n",
		 *line, verilog_line_num, current_file);
	dino_error_ack (trace, message);
    }
    if (time_scale == 0) time_scale = 1;
    if (DTPRINT_FILE) printf ("timescale=%f\n",time_scale);

    verilog_read_till_end ();
}

/* Create a new signal for reading in from a file.  Does not return the
   signal structure, as many records can be created for one signal.  All
   relevant info must be passed in. */
void sig_new_file (
    Trace_t	*trace,
    char 	*signame,
    int		file_pos,
    int		file_code,
    int		bits, int msb, int lsb,
    union sig_file_type_u file_type
    )
{
    Signal_t 	*new_sig_ptr;
    char	*endcp, *sep, *bbeg;
    char	*signame_buspos;	/* Text after the bus information */
    char	*pos;

    if (DTPRINT_BUSSES) printf ("sig_new_file    (%s, %d, (%d)%d-%d )\n", signame, file_pos, bits,msb,lsb);

    /* Preprocess signal name */
    if ((pos = strchr(signame, ' ')) != 0)		/* Drop leading spaces */
	signame = pos;
    if ((pos = strrchr(signame, ' ')) != 0)		/* Drop trailing spaces */
	*pos = '\0';

    /* Use the separator character to split signals into vector and base */
    /* IE "signal_1" becomes "signal" with index=1 if the separator is _ */
    {
	char	sepchar = trace->dfile.vector_separator;
	if (sepchar == '\0') sepchar='<';	/* Allow both '\0' and '<' for DANGER::DORMITZER */
	sep = strrchr (signame, sepchar);
	bbeg = sep+1;
	if (!sep || !isdigit (*bbeg)) {
	    if (trace->dfile.vector_separator == '\0') {
		/* Allow numbers at end to be stripped off as the vector bit */
		for (sep = signame + strlen (signame) - 1;
		     (sep > signame) && isdigit (*sep);
		     sep --) ;
		if (sep) sep++;
		bbeg = sep;
	    }
	}
    }
	
    /* Extract the bit subscripts from the name of the signal */
    signame_buspos = 0;	/* Presume nothing after the vector */
    if (sep && isdigit (*(bbeg))) {
	/* Is a named bus, with <subscript> */
	msb = atoi (bbeg);
	lsb = msb - bits + 1;
	if (lsb < 0) {
	    msb += -lsb;
	    lsb += -lsb;
	}
	/* Don't need to search for :'s, as the bits should already be computed */
	/* Mark this first digit, _, whatever as null (truncate the name) */
	*sep++ = '\0';
	/* Hunt for the end of the vector */
	while (*sep && *sep!=trace->dfile.vectorend_separator) sep++;
	while (*sep && *sep==trace->dfile.vectorend_separator) sep++;
	/* Remember if there is stuff after the vector */
	if (*sep) signame_buspos = sep;
    }
    else {
	if (bits>1) {
	    /* Is a unnamed bus */
	    msb = bits - 1;
	    lsb = 0;
	} else {
	    /* Is a unnamed single signals */
	    msb = -1;
	    lsb = -1;
	}
    }

    /* Remove null names and huge memories */
    /* FIX? What if composed of many small bits? */
    if (*signame == '\0') {
	if (DTDEBUG) printf ("Dropping null signal name\n");
	return;
    }
    if (bits > 2048) {
	if (DTDEBUG) printf ("Dropping huge memory %s\n", signame);
	return;
    }

    /* Vectorize signals */
    {
	Signal_t	*bus_sig_ptr;	/* ptr to signal which is being bussed (upper bit number) */
	bus_sig_ptr = trace->lastsig;
	/* Can we merge this new signal to be created with the previous signal? */
	if (bus_sig_ptr) {
	    if (DTPRINT_BUSSES) printf ("  '%s'%d:/#%d/p%d/c%d\t'%s'%d:/#%d/p%d/c%d\n",
					bus_sig_ptr->signame, bus_sig_ptr->msb_index,
					bus_sig_ptr->bits, bus_sig_ptr->file_pos, bus_sig_ptr->file_code, 
					signame, msb, bits, file_pos, file_code);
	    /* Combine signals with same base name, if */
	    /*  are a vector */
	    if (msb >= 0
		/*  & the result would have < 128 bits */
		&& (ABS(bus_sig_ptr->msb_index - lsb) < 128 )
		/* 	& have subscripts that are different by 1  (<20:10> and <9:5> are separated by 10-9=1) */
		&& ( ((bus_sig_ptr->lsb_index >= msb)
		      && ((bus_sig_ptr->lsb_index - 1) == msb)))
		/*	& are placed next to each other in the source */
		/*	& not a tempest trace (because the bit ordering is backwards, <31:0> would look line 0:31) */
		&& ((trace->dfile.fileformat != FF_TEMPEST)
		    || trace->dfile.fileformat == FF_VERILOG || trace->dfile.fileformat == FF_VERILOG_VPD
		    || (((bus_sig_ptr->file_pos) == (file_pos + bits))
			&& (trace->dfile.vector_separator=='<'
			    || trace->dfile.vector_separator=='('
			    )))
		&& ((trace->dfile.fileformat != FF_VERILOG && trace->dfile.fileformat != FF_VERILOG_VPD)
		    || (( (   (bus_sig_ptr->file_pos + bus_sig_ptr->bits) == file_pos)
			  || ((bus_sig_ptr->file_code + bus_sig_ptr->bits) == file_code))
			&& (trace->dfile.vector_separator=='[')))
		/*	& not (verilog trace which had a signal already as a vector) */
		&& ! (file_type.flag.perm_vector || bus_sig_ptr->file_type.flag.perm_vector)
		/* Signal name following bus separator is identical (slow) */
		&& (signame_buspos==NULL || 0==strcmp (signame_buspos, bus_sig_ptr->signame_buspos))
		/* Signal name match (last, as is slowest) */
		&& 0==strcmp (signame, bus_sig_ptr->signame)
		) {

		/* Can be bussed with previous signal */
		bus_sig_ptr->bits += bits;
		bus_sig_ptr->lsb_index = lsb;
		if (trace->dfile.fileformat == FF_TEMPEST) bus_sig_ptr->file_pos = file_pos;

		/* We can "delete" this signal.  Never added it, so just return */
		if (DTPRINT_BUSSES) printf ("       bussed    %s\n", signame);
		return;
	    }
	}
    }

    {
	int	file_pos_orig = file_pos;
	int	lsb_orig = lsb;

	while (bits) {
	    /* May need multiple signals if there are > 128 bits to be added */
	    int bits_this = bits;
	    int msb_this = msb;
	    int file_pos_this = file_pos;
	    if (bits>128) {
		int chop = bits % 128;
		if (chop==0) chop = 128;
		bits_this = chop;
		bits -= chop;
		msb -= chop;
		file_pos += chop;
	    } else {
		bits = 0;
	    }
	    if (bits_this>1) lsb = msb_this - bits_this + 1;
	    else lsb = msb_this;
	    
	    /* Tempest stores LW0 in position 0, LW1 in position 32, .... so we need to adjust for that */
	    /*      b31 .... b2 b1 b0   ||   b63 b62 .... b32 */
	    if (trace->dfile.fileformat == FF_TEMPEST) {
		file_pos_this = file_pos_orig + (lsb - lsb_orig);
	    }

	    /* Create the new signal */
	    new_sig_ptr = DNewCalloc (Signal_t);
	    new_sig_ptr->trace = trace;
	    new_sig_ptr->dfile = &(trace->dfile);
	    new_sig_ptr->file_type = file_type;
	    if (file_type.flag.real) {
		new_sig_ptr->radix = global->radixs[RADIX_REAL];
	    } else {
		new_sig_ptr->radix = global->radixs[0];
	    }
	    new_sig_ptr->file_pos = file_pos_this;
	    new_sig_ptr->file_code = file_code;	/* Codes are constant, so don't change with bit loop */
	    new_sig_ptr->bits = bits_this;
	    new_sig_ptr->msb_index = msb_this;
	    new_sig_ptr->lsb_index = lsb;
	    
	    /* initialize all the pointers that aren't NULL */
	    if (trace->lastsig) trace->lastsig->forward = new_sig_ptr;
	    new_sig_ptr->backward = trace->lastsig;
	    if (trace->firstsig==NULL) trace->firstsig = new_sig_ptr;
	    trace->lastsig = new_sig_ptr;
	    
	    /* copy signal info */
	    new_sig_ptr->signame = (char *)XtMalloc(16+strlen (signame));	/* allow extra space in case becomes vector */
	    strcpy (new_sig_ptr->signame, signame);
	    new_sig_ptr->signame_buspos = (signame_buspos
					   ? strdup(signame_buspos) : NULL);
	    if (DTPRINT_BUSSES) printf ("       donefile (%s %s, %d, (%d)%d-%d )\n",
					new_sig_ptr->signame, DeNull(new_sig_ptr->signame_buspos),
					new_sig_ptr->file_pos, new_sig_ptr->bits, new_sig_ptr->msb_index, new_sig_ptr->lsb_index);
	}
    }
}

static void	verilog_process_var (
    /* Process a VAR statement (read a signal) */
    Trace_t	*trace)
{
    char	*cmd;
    int		bits;
    int		msb = 0, lsb = 0;
    char	signame[10000] = "";
    char	basename[10000] = "";
    char 	str[2];
    int		t;
    char	code[10];
    char	*refer;
    union sig_file_type_u file_type;

    /*
      $var reg       5 :    r [4:0] $end
      $var reg   5 : 2 x    r [4:0] $end		<<<< shows a range
      $var wire      1 v&   Z [-1] $end			<<<<< ignore [-1]
      $var wire      1 R    addr_h [6] $end
      $var reg       1 !    clk  $end
      $var reg      11 "    count[10:0] $end
      $var reg      11 "    count [10:0] $end
      $var real     64 /$   ad_off_delay $end
    */

    /* Read <vartype> */
    cmd = verilog_gettok();
    file_type.flags = 0;
    file_type.flag.real = (!strcmp(cmd,"real"));
	
    /* Read <size> */
    cmd = verilog_gettok();
    bits = atoi (cmd);
    verilog_max_bits = MAX (verilog_max_bits, bits);	/* Count before we break into 128 bit pieces */
    if (bits<0) {
	sprintf (message,"Negative bit count on line %d of %s\n",
		 verilog_line_num, current_file);
	dino_error_ack (trace, message);
	return;
    }

    /* read next token */
    /* if token == ":", msb and lsb are given */
    cmd = verilog_gettok();
    refer = NULL;
    if(!strcmp(cmd, ":")) {
	msb = bits;

	/* Read lsb */
	cmd = verilog_gettok();
	if (isdigit(cmd[0])) {
	    lsb = atoi (cmd);
	    bits = msb - lsb + 1;
 
	    /* Read <identifier_code>  (uses chars 33-126 = '!' - '~') */
	    cmd = verilog_gettok();
	    strcpy (code, cmd);
	} else {
	    /* Faked us out, : is the identifier code */
	    strcpy(code, ":");
	    /* We read what should have been the reference */
	    refer = cmd;
	}
    }
    else { /* must have been identifier */
	strcpy(code, cmd);
    }
      
    /* Signal name begins with the present scope */
    basename[0] = '\0';
    str[0] = trace->dfile.hierarchy_separator;
    str[1] = '\0';
    for (t=0; t<scope_level; t++) {
	strcat (basename, scopes[t]);
	strcat (basename, str);
    }
    
    /* Read <reference> */
    if (refer==NULL) refer = verilog_gettok();
    strcpy (signame, basename);
    strcat (signame, refer);

    /* Read vector or $end */
    cmd = verilog_gettok();
    if (*cmd != '$' && strcmp(cmd, "[-1]")) {
	/* Add vector to signal */
	strcat (signame, cmd);
    }
    
    if (DTPRINT_FILE) printf ("var %s %d %s %s\n",
			      (file_type.flag.real)?"real":"reg/wire", bits, code, signame);

    /* Allocate new signal structure */
    file_type.flag.perm_vector = (bits>1);	/* If a vector already then we won't vectorize it */
    sig_new_file (trace, signame,
		  0, VERILOG_ID_TO_POS(code),
		  bits, msb, lsb, file_type);
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

static void	verilog_print_pos (void)
    /* Print the pos array */
{
    int pos;
    Signal_t	*pos_sig_ptr, *sig_ptr;

    printf ("POS array:\n");
    for (pos=0; pos<signal_by_code_max; pos++) {
	pos_sig_ptr = signal_by_code[pos];
	if (pos_sig_ptr) {
	    printf ("\t%d\t%s\n", pos, pos_sig_ptr->signame);
	    for (sig_ptr=pos_sig_ptr->verilog_next; sig_ptr; sig_ptr=sig_ptr->verilog_next) {
		printf ("\t\t\t%p  %s\n", sig_ptr, sig_ptr->signame);
	    }
	}
    }
}

static void	verilog_process_definitions (
    /* Process the definitions */
    Trace_t	*trace)
{
    Signal_t	*sig_ptr;
    Signal_t	*pos_sig_ptr;
    uint_t	pos_level, level;
    uint_t	pos;
    char	*tp;
    
    /* if (DTPRINT_FILE) sig_print_names (trace); */
    
    /* Find the highest pos used & max number of bits */
    signal_by_code_max = 0;
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	signal_by_code_max = MAX (signal_by_code_max, sig_ptr->file_code + sig_ptr->bits - 1 + 1);
    }
    
    /* Allocate space for one signal pointer for each of the possible codes */
    /* This will, of course, use a lot of memory for large traces.  It will be very fast though */
    signal_by_code = (Signal_t **)XtCalloc (sizeof (Signal_t *), (signal_by_code_max + 1));

    /* Also set aside space so every signal could change in a single cycle */
    signal_update_array = (Signal_t **)XtMalloc (sizeof (Signal_t *) * (signal_by_code_max + 1));
    signal_update_array_last_pptr = signal_update_array;
    
    /* Assign signals to the pos array.  The array points to the "original" */
    /*	-> Note that a single position can have multiple bits (if sig_ptr->file_type is true) */
    /* The original then has a linked list to other copies, the sig_ptr->verilog_next field */
    /* The original is signal highest in hiearchy, OR first to occur if at the same level */
    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	for (pos = sig_ptr->file_code; 
	     pos <= sig_ptr->file_code
		 + ((sig_ptr->file_type.flag.perm_vector)?0:(sig_ptr->bits-1));
	     pos++) {
	    pos_sig_ptr = signal_by_code[pos];
	    if (!pos_sig_ptr) {
		pos_sig_ptr = sig_ptr;
		signal_by_code[pos] = pos_sig_ptr;
		sig_ptr->verilog_next = NULL;
	    }
	    else {
		for (level=0, tp=sig_ptr->signame; *tp; tp++) {
		    if (*tp=='.') level++;
		}
		for (pos_level=0, tp=pos_sig_ptr->signame; *tp; tp++) {
		    if (*tp=='.') pos_level++;
		}
		/*printf ("Compare %d %d  %s %s\n",level, pos_level, sig_ptr->signame, pos_sig_ptr->signame);*/
		if (pos == sig_ptr->file_code) {
		    if (level < pos_level) {
			/* Put first */
			signal_by_code[pos] = sig_ptr;
			sig_ptr->verilog_next = pos_sig_ptr;
		    }
		    else { /* Put second */
			sig_ptr->verilog_next = pos_sig_ptr->verilog_next;
			pos_sig_ptr->verilog_next = sig_ptr;
		    }
		}
		// Else it's multi bit, we already used the verilog_next
	    }
	}
    }

    /* Print the pos array */
    if (DTPRINT_FILE) verilog_print_pos ();

    /* Kill all but the first copy of the signals.  This may be changed in later releases. */
    for (pos=0; pos<signal_by_code_max; pos++) {
	Signal_t *next_sig_ptr;
	pos_sig_ptr = signal_by_code[pos];
	if (pos_sig_ptr) {
	    for (sig_ptr = pos_sig_ptr->verilog_next; sig_ptr; sig_ptr=next_sig_ptr) {
		next_sig_ptr = sig_ptr->verilog_next;
		sig_ptr->verilog_next = NULL;		/* Zero in prep of make_busses */
		if (sig_ptr->file_pos == pos_sig_ptr->file_pos) {	/* Else is a > 128bit breakup copy */
		    /* Delete this signal */
		    if (global->save_duplicates
			&& sig_ptr->bits == pos_sig_ptr->bits) {
			sig_ptr->copyof = pos_sig_ptr;
			sig_ptr->file_copy = TRUE;
			/*printf ("Dup %s\t  %s\n", pos_sig_ptr->signame, sig_ptr->signame);*/
		    } else {
			sig_free (trace, sig_ptr, FALSE, FALSE); sig_ptr=NULL;
		    }
		}
	    }
	    pos_sig_ptr->verilog_next = NULL;		/* Zero in prep of make_busses */
	    signal_by_code[pos] = NULL;			/* Zero in prep of make_busses */
	}
    }
    
    /* Make the busses */
    /* The pos creation is first because there may be vectors that map to single signals. */
    fil_make_busses (trace, TRUE);

    /* Assign signals to the pos array **AGAIN**. */
    /* Since busses now exist, the pointers will all be different */
    if (DTPRINT_FILE) printf ("Reassigning signals to pos array.\n");
    memset (signal_by_code, 0, sizeof (Signal_t *) * (signal_by_code_max + 1));
    for (pos_sig_ptr = trace->firstsig; pos_sig_ptr; pos_sig_ptr = pos_sig_ptr->forward) {
	for (pos = pos_sig_ptr->file_code; 
	     pos <= pos_sig_ptr->file_code
		 + ((pos_sig_ptr->file_type.flag.perm_vector)?0:(pos_sig_ptr->bits-1));
	     pos++) {
	    if (pos_sig_ptr->copyof) {
		pos_sig_ptr->bptr = pos_sig_ptr->copyof->bptr;
		pos_sig_ptr->cptr = pos_sig_ptr->copyof->cptr;
		if (DTPRINT_FILE) printf ("StillDup %s\t  %s\n", pos_sig_ptr->signame, pos_sig_ptr->copyof->signame);
	    } else {
		if (!signal_by_code[pos]) {
		    signal_by_code[pos] = pos_sig_ptr;
		} else {
		    /* If already assigned, this is a signal that was womp_128ed. */
		    /* or, we're saving duplicates */
		    pos_sig_ptr->verilog_next = signal_by_code[pos];
		    signal_by_code[pos] = pos_sig_ptr;
		}
	    }
	}
    }

    /* Print the pos array */
    if (DTPRINT_FILE) verilog_print_pos ();
}

static inline void verilog_prep_busses (
    Signal_t	*sig_ptr,
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
    Trace_t	*trace,
    int		first_data,
    int		time)
{
    Signal_t	*sig_ptr;
    Signal_t	**sig_upd_pptr;

    for (sig_upd_pptr = signal_update_array; sig_upd_pptr < signal_update_array_last_pptr; sig_upd_pptr++) {
	sig_ptr = *sig_upd_pptr;
	if (sig_ptr->file_value.siglw.stbits.state) {
	    /*if (DTPRINT_FILE) { printf ("Entered: "); print_cptr (&(sig_ptr->file_value)); } */
	    if (DTPRINT_FILE) { printf ("Entered: "); print_cptr (&(sig_ptr->file_value)); } 

	    /* Make cptr have correct state */
	    val_minimize (&(sig_ptr->file_value), sig_ptr);

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
    Trace_t	*trace)
{
    char	*value_strg, *line, *code;
    DTime_t	time;
    Boolean_t	first_data=TRUE;
    Boolean_t	got_data=FALSE;
    Boolean_t	got_time=FALSE;
    Boolean_t	told_wrap=FALSE;
    Value_t	value;
    Signal_t	*sig_ptr;
    int		poscode;
    int		state = STATE_Z;
    char	*scratchline;
    char	*scratchline2;
    double	dnum; 
    double	time_mult;
    double	time_divisor;

    if (DTPRINT_ENTRY) printf ("In verilog_read_data (max_bits = %d)\n", verilog_max_bits);

    /* Compute time scales */
    time_divisor = global->time_precision;
    time_mult = ((double)time_scale / (double)time_divisor);

    time = 0;
    scratchline = (char *)XtMalloc(100+verilog_max_bits);
    scratchline2 = (char *)XtMalloc(100+verilog_max_bits);
    
    trace->end_time = 0;

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
	case 'Z':
	    state = STATE_Z;
	    goto common;
	case 'x':
	case 'X':
	    state = STATE_U;
	    goto common;

	  common:
	    got_data = TRUE;

	    code = line;
	    poscode = VERILOG_ID_TO_POS(code);
	    sig_ptr = VERILOG_POS_TO_SIG(poscode);
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
		    register int bit = sig_ptr->bits - 1 - (poscode - sig_ptr->file_code); 
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
	    poscode = VERILOG_ID_TO_POS(code);
	    sig_ptr = VERILOG_POS_TO_SIG(poscode);
	    if (sig_ptr) {
		if ((sig_ptr->bits < 2) || !sig_ptr->file_type.flag.perm_vector) {
		    /* For some idiotic reason vpd2vcd likes to do this! */
		    /* This costs extra bytes and time... Oh well... */
		    switch (value_strg[0]) {
		    case '0': state = STATE_0; break;
		    case '1': state = STATE_1; break;
		    case 'z': state = STATE_Z; break;
		    case 'Z': state = STATE_Z; break;
		    case 'x': state = STATE_U; break;
		    case 'X': state = STATE_U; break;
		    default: printf ("%%E, Strange bit value %c on line %d of %s\n",
				     value_strg[0], verilog_line_num, current_file);
		    }
		    val_zero (&value);
		    value.siglw.stbits.state = state;
		    value.time = time;
		    fil_add_cptr (sig_ptr, &value, first_data);
		}
		else {
		    /* compute size across all bits of signal */
		    int size = 0;
		    int offset;
		    Signal_t	*loop_sig_ptr;
		    for (loop_sig_ptr=sig_ptr; loop_sig_ptr; loop_sig_ptr=loop_sig_ptr->verilog_next) {
			if (!loop_sig_ptr->copyof) {
			    size += loop_sig_ptr->bits;
			}
		    }
		    offset = size - strlen(value_strg);
		    if (offset != 0) {
			char extend_char = (value_strg[0]=='1')?'0':value_strg[0];
			memmove (value_strg + offset, value_strg, strlen(value_strg)+1);
			memset (value_strg, extend_char, offset);
		    }
		    
		    for (; sig_ptr; sig_ptr=sig_ptr->verilog_next) {
			if (!sig_ptr->copyof) {
			    /* Store the file information */
			    /*if (DTPRINT_FILE) printf ("\tsignal '%s'=%d @%d %s  fp %d value %s\n", code, poscode, time, sig_ptr->signame, sig_ptr->file_pos, value_strg);*/
			    ascii_string_add_cptr (sig_ptr, value_strg + sig_ptr->file_pos, time, first_data);
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
	    time = (atof (line) * time_mult);	/* 1% of time in this division! */
	    if (first_data && got_time && got_data) {
		first_data = FALSE;
	    } else if (first_data) {
		trace->start_time = time;
		got_time = TRUE;
		got_data = FALSE;
	    }
	    if (trace->end_time > time
		&& !told_wrap) {
		told_wrap = TRUE;
		dino_warning_ack (trace, "Time has wrapped dinotrace's integer size.\nTry increasing Timescale.");
		printf ("Time out %d  Time in %lu  TS %g  TD %g\n", time, atol(line), time_scale, time_divisor);
	    }
	    trace->end_time = time;
	    if (DTPRINT_FILE) {
		printf ("Time %d start %d first %d got %d  ", time, trace->start_time, first_data, got_data);
		printf (" %ld * ( %g / %g )\n", atol(line), time_scale, time_divisor);
	    }
	    break;

	case 'r':	/* Real number */
	    dnum = atof (line);
	    code = verilog_gettok();
	    poscode = VERILOG_ID_TO_POS(code);
	    sig_ptr = VERILOG_POS_TO_SIG(poscode);
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
	    if (0==strncmp (line, "comment", 7)) {
		verilog_read_till_end();
	    }
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
    Trace_t	*trace)
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
    Trace_t	*trace,
    int		read_fd)
{
    time_scale = time_units_to_multiplier (global->time_precision);

    /* Signal description data */
    trace->firstsig = NULL;
    trace->lastsig = NULL;
    signal_by_code = NULL;
    scope_level = 0;

    current_file = trace->dfile.filename;
    verilog_gets_init (read_fd);
    verilog_process_lines (trace);

    /* Free up */
    DFree (signal_by_code);
}


