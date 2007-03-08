#ident "$Id$"
/******************************************************************************
 * DESCRIPTION: Dinotrace source: value-at-a-time decoding
 *
 * This file is part of Dinotrace.
 *
 * Author: Wilson Snyder <wsnyder@wsnyder.org>
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

#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <Xm/SelectioB.h>
#include <Xm/Text.h>
#include <Xm/BulletinB.h>
#include <Xm/RowColumn.h>
#include <Xm/RowColumnP.h>
#include <Xm/Label.h>
#include <Xm/LabelP.h>
#include <Xm/MenuShellP.h>
#include <Xm/GrabShell.h>

#include "functions.h"

char *val_state_name[] = { "STATE_0", "STATE_1", "STATE_U", "STATE_Z",
			   "STATE_B32", "STATE_F32", "STATE_B128", "STATE_F128"};

/****************************** UTILITIES ******************************/

static uint_t val_bit (
    const Value_t *value_ptr,
    int bit)
    /* Return bit value, understanding 4-state, may be 0/1/U/Z */
{
    int bitpos = bit & 0x1f;
    if (bit<0) return(0);
    switch (value_ptr->siglw.stbits.state) {
    case STATE_0:
	return (0);
    case STATE_1:
	return (bit==0 ? 1:0);
    case STATE_Z:
	return (3);
    case STATE_B32:
	if (bit<32)  return (    (value_ptr->number[0] >>bitpos) & 1);
	return (0);
    case STATE_F32:
	if (bit<32)  return ((   (value_ptr->number[0] >>bitpos) & 1)
			     | (((value_ptr->number[1] >>bitpos) & 1)<<1));
	return (0);
    case STATE_B128:
	if (bit<32)  return (    (value_ptr->number[0] >>bitpos) & 1);
	if (bit<64)  return (    (value_ptr->number[1] >>bitpos) & 1);
	if (bit<96)  return (    (value_ptr->number[2] >>bitpos) & 1);
	if (bit<128) return (    (value_ptr->number[3] >>bitpos) & 1);
	return (0);
    case STATE_F128:
	if (bit<32)  return ((   (value_ptr->number[0] >>bitpos) & 1)
			     | (((value_ptr->number[4] >>bitpos) & 1)<<1));
	if (bit<64)  return ((   (value_ptr->number[1] >>bitpos) & 1)
			     | (((value_ptr->number[5] >>bitpos) & 1)<<1));
	if (bit<96)  return ((   (value_ptr->number[2] >>bitpos) & 1)
			     | (((value_ptr->number[6] >>bitpos) & 1)<<1));
	if (bit<128) return ((   (value_ptr->number[3] >>bitpos) & 1)
			     | (((value_ptr->number[7] >>bitpos) & 1)<<1));
	return (0);
    default:
	return (2);/*X*/
    }
}

static char	val_str_digit_ascii (
    const Value_t *value_ptr,
    int lsb)
{
    uint_t val = 0;
    uint_t bit;
    int bitnum;

    for (bitnum=lsb+7; bitnum>=lsb; bitnum--) {
	bit = val_bit (value_ptr, bitnum);
	if (bit>1) return ('X');
	val = val<<1 | bit;
    }
    val &= 0xff;
    if (val==0) return (' ');
    if (!isprint(val)) return ('?');
    return (val);
}

static void	val_str_ascii (
    char *strg,
    const Value_t *value_ptr,
    Boolean_t compressed)		/* Drawing on screen; keep small & tidy */
{
    Boolean_t keepspace = FALSE;
    char *cp = strg;
    switch (value_ptr->siglw.stbits.state) {
    case STATE_U:
	*cp++ = 'x';
	break;
    case STATE_Z:
	*cp++ = 'z';
	break;
    case STATE_0:
	break;
    default: {
	int bitnum;
	char c;
	if (!compressed) *cp++ = '\"';
	for (bitnum = 128-8; bitnum >= 0; bitnum-= 8) {
	    c = val_str_digit_ascii (value_ptr, bitnum);
	    if (c!=' ' || keepspace) {
		*cp++ = c;
		keepspace = FALSE;
	    }
	}
	while (cp>strg && *(cp-1) == ' ') cp--;
	if (!compressed) *cp++ = '\"';
	}
    }
    *cp++ = '\0';
}

static char	val_str_digit (
    int lsb,
    uint_t mask,
    uint_t lw,
    uint_t enlw
    )
{
    lw = (lw>>lsb) & mask;
    enlw = (enlw>>lsb) & mask;
    if (enlw) {
	if ((enlw == mask) && (lw == mask)) {
	    return ('z');	/* All z's */
	} else if ((enlw == mask) && (lw == 0)) {
	    return ('x');	/* All u's */
	}
	return ('X');	/* U/Z/0/1 mix */
    }

    return (lw + ((lw < 10) ? '0':('a'-10)));
}

static void    val_str_lw (
    const Radix_t *radix_ptr,
    char *strg,
    char sep,
    Boolean_t middle,	/* 2nd, 3rd, etc value on this line */
    int width,
    uint_t lw,
    uint_t enlw
    )
{
    int bit = 31;

    /* Separate if needed */
    if (middle) {
	*strg++ = sep;
    }
    if (!middle) {
	if (width) {
	    /* Start at appropriate bit */
	    bit = ((width-1) & 31);
	} else {
	    /* Skip leading 0's */
	    for (; bit>0; bit--) {
		if ((lw|enlw) & (1L<<bit)) break;
	    }
	}
    }

    switch (radix_ptr->type) {
    case RADIX_HEX_UN:
	for (; bit>=0; bit--) {
	    bit = (bit / 4) *4;	/* Next hex digit */
	    *strg++ = val_str_digit (bit, 0xf, lw, enlw);
	}
	*strg++ = '\0';
	break;
    case RADIX_OCT_UN:
	for (; bit>=0; bit--) {
	    bit = (bit / 3) *3;	/* Next octal digit */
	    *strg++ = val_str_digit (bit, 0x7, lw, enlw);
	}
	*strg++ = '\0';
	break;
    case RADIX_DEC_UN:
	if (enlw) {
	    strcpy (strg,"Mixed01XU");
	} else {
	    sprintf (strg,middle?"%010d":"%d", lw);
	}
	break;
    case RADIX_DEC_SN:
	if (enlw) {
	    strcpy (strg,"Mixed01XU");
	} else {
	    if (lw & (1<<(width-1))) {  // Negative
		int masked = ~lw & ((1<<(width-1))-1);
		sprintf (strg,"-%d", masked+1);
	    } else {  // Signed and positive
		sprintf (strg,middle?"%010d":"%d", lw);
	    }
	}
	break;
    case RADIX_BIN_UN: {
	for (;bit>=0;bit--) {
	    *strg++ = val_str_digit (bit, 0x1, lw, enlw);
	}
	*strg++ = '\0';
	break;
    }
    default: strcat (strg, "bad radix");
    }
}

void    val_to_string (
    const Radix_t *radix_ptr,
    char *strg,
    const Value_t *value_ptr,
    int width,				/* or 0 to drop leading 0s */
    Boolean_t compressed,		/* Drawing on screen; keep small & tidy */
    Boolean_t dropleading)		/* Drop leading spaces */
    /* Convert a string to a value */
    /* Understand 'h 'b and other similar tags */
    /* Try to convert a signal state also */
    /* This isn't fast, so don't use when reading files!!! */
{
    char sep = compressed ? ' ' : '_';
    if (radix_ptr==NULL) {strcpy (strg,"nullradix_ptr"); return;}

    if (radix_ptr->type == RADIX_ASCII) {
	val_str_ascii (strg, value_ptr, compressed);
	return;
    }

    switch (value_ptr->siglw.stbits.state) {
    case STATE_0:
	*strg++ = '0';
	break;
    case STATE_1:
	*strg++ = '1';
	break;
    case STATE_U:
	*strg++ = 'x';
	break;
    case STATE_Z:
	*strg++ = 'z';
	break;

    case STATE_B32:
    case STATE_F32:
    case STATE_B128:
    case STATE_F128: {
	Boolean_t middle = FALSE;
	if (!compressed) {
	    strcpy (strg, radix_ptr->prefix);
	    strg += strlen (radix_ptr->prefix);
	}
	if (radix_ptr->type == RADIX_REAL
	    && value_ptr->siglw.stbits.state == STATE_B128) {
	    sprintf (strg, "%g", *(double*)(&value_ptr->number[0]));
	} else {
	    uint_t enlw;
	    if (value_ptr->siglw.stbits.state == STATE_B128
		|| value_ptr->siglw.stbits.state == STATE_F128) {
		if (value_ptr->number[3]
		    || (!dropleading && width>32*3)) {
		    val_str_lw (radix_ptr, strg, sep, middle, width,
				value_ptr->number[3],
				((value_ptr->siglw.stbits.state == STATE_F128)
				 ?value_ptr->number[7]:0));
		    strg += strlen (strg);
		    middle = TRUE;
		}
		if (value_ptr->number[3] || value_ptr->number[2]
		    || (!dropleading && width>32*2)) {
		    val_str_lw (radix_ptr, strg, sep, middle, width,
				value_ptr->number[2],
				((value_ptr->siglw.stbits.state == STATE_F128)
				 ?value_ptr->number[6]:0));
		    strg += strlen (strg);
		    middle = TRUE;
		}
		if (value_ptr->number[3] || value_ptr->number[2]
		    || value_ptr->number[1]
		    || (!dropleading && width>32*1)) {
		    val_str_lw (radix_ptr, strg, sep, middle, width,
				value_ptr->number[1],
				((value_ptr->siglw.stbits.state == STATE_F128)
				 ?value_ptr->number[5]:0));
		    strg += strlen (strg);
		    middle = TRUE;
		}
	    }
	    enlw = 0;
	    if (value_ptr->siglw.stbits.state == STATE_F128) {
		enlw = value_ptr->number[4];
	    }
	    if (value_ptr->siglw.stbits.state == STATE_F32) {
		enlw = value_ptr->number[1];
	    }
	    val_str_lw (radix_ptr, strg, sep, middle, width,
			value_ptr->number[0], enlw);
	}
	strg += strlen (strg);
	break;
    }
    default:
	*strg++ = '?';
	break;
    }
    *strg++ = '\0';
}

void	val_minimize (
    Value_t	*value_ptr,
    Signal_t	*sig_ptr
    )
    /* Given a STATE_B128, try to shrink to something smaller if we can */
{
    Boolean_t num123 = (value_ptr->number[1]
			|| value_ptr->number[2]
			|| value_ptr->number[3]);
    Boolean_t num567 = (value_ptr->number[5]
			|| value_ptr->number[6]
			|| value_ptr->number[7]);
    if (!value_ptr->number[4] && !num567) {
	if (!num123) {
	    if (!value_ptr->number[0])
		value_ptr->siglw.stbits.state = STATE_0;
	    else value_ptr->siglw.stbits.state = STATE_B32;
	}
	else value_ptr->siglw.stbits.state = STATE_B128;
    } else {
	if ((value_ptr->number[0] == value_ptr->number[4])
	    && (value_ptr->number[1] == value_ptr->number[5])
	    && (value_ptr->number[2] == value_ptr->number[6])
	    && (value_ptr->number[3] == value_ptr->number[7])
	    && sig_ptr
	    && (value_ptr->number[0] == sig_ptr->value_mask[0])
	    && (value_ptr->number[1] == sig_ptr->value_mask[1])
	    && (value_ptr->number[2] == sig_ptr->value_mask[2])
	    && (value_ptr->number[3] == sig_ptr->value_mask[3])
	    ) {
	    value_ptr->siglw.stbits.state = STATE_Z;
	} else if (!num123 && !num567) {
	    value_ptr->siglw.stbits.state = STATE_F32;
	    value_ptr->number[1] = value_ptr->number[4];
	    value_ptr->number[4] = 0;
	} else {
	    value_ptr->siglw.stbits.state = STATE_F128;
	}
    }
    value_ptr->siglw.stbits.size = STATE_SIZE(value_ptr->siglw.stbits.state);
}

void    string_to_value (
    Radix_t **radix_pptr,
    const char *strg,
    Value_t *value_ptr)
{
    register char value;
    uint_t MSO = (7L<<29);	/* Most significant octal digit */
    uint_t MSH = (15L<<28);	/* Most significant hex digit */
    uint_t MSB = (1L<<31);	/* Most significant binary digit */
    uint_t MSC = (0xffL<<24);	/* Most significant char */
    const char *cp = strg;
    char radix_id;
    uint_t *nptr = &(value_ptr->number[0]);
    Boolean_t negate = FALSE;

    val_zero (value_ptr);

    radix_id = 'x';
    *radix_pptr = global->radixs[RADIX_HEX_UN];
    if (*cp=='\'') {
	radix_id = cp[1];
	cp+=2;
    } else if (isalpha(*cp) && cp[1]=='"') {
	radix_id = cp[0];
	cp+=2;
    } else if (*cp=='"') {
	radix_id = cp[0];
	cp++;
    }
    switch (toupper(radix_id)) {
    case 'o': *radix_pptr = global->radixs[RADIX_OCT_UN]; break;
    case 'd': *radix_pptr = global->radixs[RADIX_DEC_UN]; break;
    case 'i': *radix_pptr = global->radixs[RADIX_DEC_SN]; break;
    case 'b': *radix_pptr = global->radixs[RADIX_BIN_UN]; break;
    case 'r': *radix_pptr = global->radixs[RADIX_REAL]; break;
    case '"':
    case 's':
	*radix_pptr = global->radixs[RADIX_ASCII];  break;
    default:
    case 'h':
    case 'x':
	*radix_pptr = global->radixs[RADIX_HEX_UN];  break;
    }

    if ((*radix_pptr)->type != RADIX_ASCII
	|| (cp == strg)) {
        /*Value literals: Not in a string, or unquoted:*/
	if ((*cp == 'z' || *cp == 'Z') && cp[1]=='\0') {
	    value_ptr->siglw.stbits.state = STATE_Z;
	    return;
	}
	if ((*cp == 'u' || *cp == 'U' || *cp == 'x' || *cp == 'X') && cp[1]=='\0') {
	    value_ptr->siglw.stbits.state = STATE_U;
	    return;
	}
    }

    if ((*radix_pptr)->type == RADIX_REAL) {
	double dnum = atof (cp);
	if (dnum == 0) 	value_ptr->siglw.stbits.state = STATE_0;
	else {
	    value_ptr->siglw.stbits.state = STATE_B128;
	    *((double*)&value_ptr->number[0]) = dnum;
	}
	return;
    }

    for (; *cp; cp++) {
	value = -1;
	if (*cp >= '0' && *cp <= '9')
	    value = *cp - '0';
	else if (*cp >= 'A' && *cp <= 'F')
	    value = *cp - ('A' - 10);
	else if (*cp >= 'a' && *cp <= 'f')
	    value = *cp - ('a' - 10);
	else if (*cp == '-')
	    negate = !negate;

	switch ((*radix_pptr)->type) {
	case RADIX_HEX_UN:
	    if (value >=0 && value <= 15) {
		nptr[3] = (nptr[3]<<4) + ((nptr[2] & MSH)>>28);
		nptr[2] = (nptr[2]<<4) + ((nptr[1] & MSH)>>28);
		nptr[1] = (nptr[1]<<4) + ((nptr[0] & MSH)>>28);
		nptr[0] = (nptr[0]<<4) + value;
	    }
	    break;
	case RADIX_OCT_UN:
	    if (value >=0 && value <= 7) {
		nptr[3] = (nptr[3]<<3) + ((nptr[2] & MSO)>>29);
		nptr[2] = (nptr[2]<<3) + ((nptr[1] & MSO)>>29);
		nptr[1] = (nptr[1]<<3) + ((nptr[0] & MSO)>>29);
		nptr[0] = (nptr[0]<<3) + value;
	    }
	    break;
	case RADIX_BIN_UN:
	    if (value >=0 && value <= 1) {
		nptr[3] = (nptr[3]<<1) + ((nptr[2] & MSB)>>31);
		nptr[2] = (nptr[2]<<1) + ((nptr[1] & MSB)>>31);
		nptr[1] = (nptr[1]<<1) + ((nptr[0] & MSB)>>31);
		nptr[0] = (nptr[0]<<1) + value;
	    }
	    break;
	case RADIX_DEC_UN:
	case RADIX_DEC_SN:
	    if (value >=0 && value <= 9) {
		/* This is buggy for large numbers */
		/* Note in spec that binary is for 32 bit or less numbers */
		nptr[3] = (nptr[3]*10) + ((cp>=(strg+30))?cp[-30]:0);
		nptr[2] = (nptr[2]*10) + ((cp>=(strg+20))?cp[-20]:0);
		nptr[1] = (nptr[1]*10) + ((cp>=(strg+10))?cp[-10]:0);
		nptr[0] = (nptr[0]*10) + value;
	    }
	    break;
	case RADIX_ASCII:
	    if (*cp != '\"') {
		nptr[3] = (nptr[3]<<8) + ((nptr[2] & MSC)>>24);
		nptr[2] = (nptr[2]<<8) + ((nptr[1] & MSC)>>24);
		nptr[1] = (nptr[1]<<8) + ((nptr[0] & MSC)>>24);
		nptr[0] = (nptr[0]<<8) + (*cp & 0xff);
	    }
	    break;
	case RADIX_MAX:
	case RADIX_REAL:
	    break;
	}
    }

    if (negate && (nptr[0] || nptr[1] || nptr[2] || nptr[3])) {
	nptr[0] = ~nptr[0];
	nptr[1] = ~nptr[1];
	nptr[2] = ~nptr[2];
	nptr[3] = ~nptr[3];
	nptr[0]++;
	if (!nptr[0]) nptr[1]++;
	if (!nptr[0] && !nptr[1]) nptr[2]++;
	if (!nptr[0] && !nptr[1] && !nptr[2]) nptr[3]++;
    }

    val_minimize (value_ptr, NULL);
}

Boolean_t  val_zero_or_one (
    const Value_t	*vptr)
{
    return ( (  vptr->siglw.stbits.state == STATE_0)
	     || (vptr->siglw.stbits.state == STATE_1)
	     || (vptr->siglw.stbits.state == STATE_B32
		 && (vptr->number[0] == 1)));
}

Boolean_t  val_equal (
    const Value_t	*vptra,
    const Value_t	*vptrb)
{
    return ((vptra->siglw.stbits.state == vptrb->siglw.stbits.state)
	    && (( (  vptra->siglw.stbits.state == STATE_0)
		  | (vptra->siglw.stbits.state == STATE_1)
		  | (vptra->siglw.stbits.state == STATE_U)
		  | (vptra->siglw.stbits.state == STATE_Z))
		|| ((vptra->siglw.stbits.state == STATE_B32)
		    & (vptra->number[0] == vptrb->number[0]))
		|| ((vptra->siglw.stbits.state == STATE_B128)
		    & (vptra->number[0] == vptrb->number[0])
		    & (vptra->number[1] == vptrb->number[1])
		    & (vptra->number[2] == vptrb->number[2])
		    & (vptra->number[3] == vptrb->number[3]))
		|| ((vptra->siglw.stbits.state == STATE_F32)
		    & (vptra->number[0] == vptrb->number[0])
		    & (vptra->number[1] == vptrb->number[1]))
		|| ((vptra->siglw.stbits.state == STATE_F128)
		    & (vptra->number[0] == vptrb->number[0])
		    & (vptra->number[1] == vptrb->number[1])
		    & (vptra->number[2] == vptrb->number[2])
		    & (vptra->number[3] == vptrb->number[3])
		    & (vptra->number[4] == vptrb->number[4])
		    & (vptra->number[5] == vptrb->number[5])
		    & (vptra->number[6] == vptrb->number[6])
		    & (vptra->number[7] == vptrb->number[7]))
		));
}

void	val_update_search ()
{
    Trace_t	*trace;
    Signal_t	*sig_ptr;
    Value_t	*cptr;
    int		cursorize;
    register int i;
    DCursor_t	*csr_ptr;
    Boolean_t	any_enabled;
    Boolean_t	zero_or_one_search = FALSE;
    Boolean_t	matches[MAX_SRCH];	/* Cache the wildmat for each bit, so searching is faster */
    static Boolean_t enabled_lasttime = FALSE;
    int		prev_cursor;

    if (DTPRINT_ENTRY) printf ("In val_update_search\n");

    /* This sometimes takes a while (don't XSync in case it doesn't) */
    prev_cursor = last_set_cursor ();
    set_cursor (DC_BUSY);

    /* If no searches are enabled, skip the cptr loop.  */
    /* This saves 3% of the first reading time on large traces */
    any_enabled = enabled_lasttime; 	/* if all get disabled, we still need to clean up enables */
    for (i=0; i<MAX_SRCH; i++) {
	if ((global->val_srch[i].color != 0)
	    || (global->val_srch[i].cursor != 0)) {
	    any_enabled = TRUE;
	    if (val_zero_or_one(&global->val_srch[i].value)) zero_or_one_search = TRUE;
	}
    }
    enabled_lasttime = FALSE;

    /* Mark all cursors that are a result of a search as old (-1) */
    for (csr_ptr = global->cursor_head; csr_ptr; csr_ptr = csr_ptr->next) {
	if (csr_ptr->type==SEARCH) csr_ptr->type = SEARCHOLD;
    }

    /* Search every trace for the value, mark the signal if it has it to speed up displaying */
    for (trace = global->deleted_trace_head; trace; trace = trace->next_trace) {
	for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	    if (any_enabled) {
		if (!zero_or_one_search && sig_ptr->bits<2) {
		    /* Single bit signal, don't search for values > 1 for speed */
		    continue;
		}

		cursorize=0;
		for (i=0; i<MAX_SRCH; i++) {
		    matches[i] = 0;
		}

		for (cptr = sig_ptr->bptr; (CPTR_TIME(cptr) != EOT);
		     cptr = CPTR_NEXT(cptr)) {
		    int color_to_assign = 0;
		    switch (cptr->siglw.stbits.state) {
		    case STATE_0:
		    case STATE_1:
			if (!zero_or_one_search) break;
			/* FALLTHRU */
		    case STATE_B32:
		    case STATE_B128:
		    case STATE_F32:
		    case STATE_F128:
			for (i=0; i<MAX_SRCH; i++) {
			    if (val_equal (cptr, &global->val_srch[i].value)
				&& ( matches[i] || wildmat (sig_ptr->signame, global->val_srch[i].signal))  ) {
				matches[i] = TRUE;
				if ( global->val_srch[i].color != 0) {
				    color_to_assign = global->val_srch[i].color;
				    enabled_lasttime = TRUE;
				}
				if ( global->val_srch[i].cursor != 0) {
				    cursorize = global->val_srch[i].cursor;
				    enabled_lasttime = TRUE;
				}
				/* don't break, because if same value on two lines, one with cursor and one without will fail */
			    }
			}
			break;
		    } /* switch */

		    if (color_to_assign != (int)cptr->siglw.stbits.color) {
			/* Don't write unless changes to save cache flushing */
			cptr->siglw.stbits.color = color_to_assign;
		    }

		    if (cursorize) {
			if (NULL != (csr_ptr = time_to_cursor (CPTR_TIME(cptr)))) {
			    if (csr_ptr->type == SEARCHOLD) {
				/* mark the old cursor as new so won't be deleted */
				csr_ptr->type = SEARCH;
			    }
			}
			else {
			    /* Make new cursor at this location */
			    cur_new (CPTR_TIME(cptr), cursorize, SEARCH, NULL);
			}
			cursorize = 0;
		    }

		} /* for cptr */
	    } /* if enabled */
	} /* for sig */
    } /* for trace */

    /* Delete all old cursors */
    cur_delete_of_type (SEARCHOLD);

    set_cursor (prev_cursor);
}

void	val_transition_cnt (Signal_t* sig_ptr, Value_t* stop_cptr, int* negedgesp, int* posedgesp)
{
    Value_t*	cptr;
    int		posedges = 0;
    int		negedges = 0;
    int		last_state = -1;
    for (cptr = sig_ptr->bptr; (CPTR_TIME(cptr) != EOT); cptr = CPTR_NEXT(cptr)) {
	/**/
	switch (cptr->siglw.stbits.state) {
	case STATE_0:
	    if (last_state==STATE_1 || last_state<0) negedges++;
	    break;
	case STATE_1:
	    if (last_state==STATE_0 || last_state<0) posedges++;
	    break;
	case STATE_Z:
	case STATE_B32:
	case STATE_B128:
	case STATE_F32:
	case STATE_F128:
	    break;
	}
	last_state = cptr->siglw.stbits.state;
	if (cptr == stop_cptr) break;
    }
    *negedgesp = negedges;
    *posedgesp = posedges;
}

/****************************** RADIXS ******************************/

Radix_t *val_radix_find (
    const char *name)
{
    Radix_t *radix_ptr;
    for (radix_ptr=global->radixs[0]; radix_ptr; radix_ptr = radix_ptr->next) {
	if (!strcasecmp(radix_ptr->name, name)) {
	    return (radix_ptr);
	}
    }
    return (NULL);
}

static Radix_t *val_radix_add (
    RadixType_t type,
    const char *name,
    const char *prefix)
{
    int radixnum;
    Radix_t *radix_ptr;
    Radix_t *found_radix_ptr;

    found_radix_ptr = val_radix_find (name); /* Null if not found */
    radix_ptr = found_radix_ptr;

    if (found_radix_ptr == NULL) {
	Radix_t *radix_end_ptr;
	radix_ptr = DNewCalloc (Radix_t);

	for (radix_end_ptr=global->radixs[0]; radix_end_ptr; radix_end_ptr = radix_end_ptr->next) {
	    if (!radix_end_ptr->next) {
		radix_end_ptr->next = radix_ptr;
		break;
	    }
	}

	/* Can we fit it in the menu? */
	for (radixnum=0; radixnum < RADIX_MAX_MENU; radixnum++) {
	    if (global->radixs[radixnum]==NULL) {
		global->radixs[radixnum]=radix_ptr;
		break;
	    }
	}
    }
    radix_ptr->name = strdup (name);
    radix_ptr->type = type;
    radix_ptr->prefix = strdup(prefix);

    return (radix_ptr);
}

void	val_radix_init ()
{
    int		radixnum;
    /* Define default radixs */
    for (radixnum=0; radixnum<RADIX_MAX; radixnum++) {
	global->radixs[radixnum] = NULL;
    }
    /* This order MUST match the enum order of RadixType_t */
    /* The first one listed is the system default */
#define DEFAULT_RADIX_PREFIX "'h"
    val_radix_add (RADIX_HEX_UN, "Hex", "");
    val_radix_add (RADIX_OCT_UN, "Octal",	"'o");
    val_radix_add (RADIX_BIN_UN, "Binary", "'b");
    val_radix_add (RADIX_DEC_UN, "Decimal", "'d");
    val_radix_add (RADIX_DEC_SN, "Signed Decimal", "'i");
    val_radix_add (RADIX_ASCII,  "Ascii", "\"");
    val_radix_add (RADIX_REAL,   "Real", "'r");
}

/****************************** STATES ******************************/

void	val_states_update ()
{
    Trace_t	*trace;
    Signal_t	*sig_ptr;

    if (DTPRINT_ENTRY) printf ("In val_states_update\n");

     for (trace = global->deleted_trace_head; trace; trace = trace->next_trace) {
	 for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	     if (NULL != (sig_ptr->decode = signalstate_find (trace, sig_ptr->signame))) {
		 /* if (DTPRINT_FILE) printf ("Signal %s is patterned\n",sig_ptr->signame); */
	     }
	 }
     }
}


/****************************** MENU OPTIONS ******************************/

void    val_examine_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);

    if (DTPRINT_ENTRY) printf ("In val_examine_cb - trace=%p\n",trace);

    /* process all subsequent button presses as cursor moves */
    remove_all_events (trace);
    set_cursor (DC_VAL_EXAM);
    add_event (ButtonPressMask, val_examine_ev);
}

void    val_highlight_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In val_highlight_cb - trace=%p\n",trace);

    /* Grab color number from the menu button pointer */
    global->highlight_color = submenu_to_color (trace, w, 0, trace->menu.val_highlight_pds);

    /* process all subsequent button presses as signal deletions */
    remove_all_events (trace);
    set_cursor (DC_VAL_HIGHLIGHT);
    add_event (ButtonPressMask, val_highlight_ev);
}


/****************************** EVENTS ******************************/

char *val_examine_string (
    /* Return string with examine information in it */
    Trace_t	*trace,
    Signal_t	*sig_ptr,
    DTime_t	time)
{
    Value_t	*cptr;
    static char	strg[2000];
    char	strg2[2000];
    int		rows, cols, bit, row, col;
    int		lesser_index, greater_index;
    char	*format;

    if (DTPRINT_ENTRY) printf ("\ttime = %d, signal = %s\n", time, sig_ptr->signame);

    /* Get information */
    cptr = value_at_time (sig_ptr, time);

    lesser_index = MIN(sig_ptr->msb_index,sig_ptr->lsb_index);
    greater_index = MAX(sig_ptr->msb_index,sig_ptr->lsb_index);
    strcpy (strg, sig_ptr->signame);

    if (CPTR_TIME(cptr) == EOT) {
	strcat (strg, "\nValue at EOT:\n");
    }
    else {
	strcat (strg, "\nValue at times ");
	time_to_string (trace, strg2, CPTR_TIME(cptr), FALSE);
	strcat (strg, strg2);
	strcat (strg, " - ");
	time_to_string (trace, strg2, CPTR_TIME(CPTR_NEXT(cptr)), FALSE);
	strcat (strg, strg2);
	strcat (strg, ":\n");
    }

    strcat (strg, "= ");
    val_to_string (sig_ptr->radix, strg2, cptr, sig_ptr->bits, FALSE, FALSE);
    strcat (strg, strg2);
    strcat (strg, "\n");
    if (sig_ptr->radix->type != global->radixs[0]->type) {
	strcat (strg, "= ");
	strcat (strg, DEFAULT_RADIX_PREFIX);
	val_to_string (global->radixs[0], strg2, cptr, sig_ptr->bits, FALSE, FALSE);
	strcat (strg, strg2);
	strcat (strg, "\n");
    }

    if (sig_ptr->bits>1
	&& (lesser_index > 0)
	&& ((sig_ptr->bits + lesser_index) <= 32)
	&& (cptr->siglw.stbits.state == STATE_B32)) {
	Value_t shifted;
	val_zero (&shifted);
	(&shifted)->siglw     = cptr->siglw;
	(&shifted)->number[0] = cptr->number[0] << lesser_index;
	/* Show decode */
	sprintf (strg2, "[%d:0] = ", greater_index);
	strcat (strg, strg2);
	val_to_string (sig_ptr->radix, strg2, &shifted,
		       sig_ptr->bits + lesser_index, FALSE, FALSE);
	strcat (strg, strg2);
	strcat (strg, "\n");
    }

    if (sig_ptr->bits>1
	&& (cptr->siglw.stbits.state == STATE_B32
	    || cptr->siglw.stbits.state == STATE_0)) {
	uint_t num0 = 0;
	if (cptr->siglw.stbits.state != STATE_0) num0 = cptr->number[0];
	if ((sig_ptr->decode != NULL)
	    && (num0 < sig_ptr->decode->numstates)
	    && (sig_ptr->decode->statename[num0][0] != '\0') ) {
	    /* Show decode */
	    sprintf (strg2, "State = %s\n", sig_ptr->decode->statename[num0] );
	    strcat (strg, strg2);
	}
    }

    switch (cptr->siglw.stbits.state) {
    case STATE_B32:
    case STATE_F32:
    case STATE_B128:
    case STATE_F128:
	/* Bitwise information */
	rows = (int)(ceil (sqrt ((double)(sig_ptr->bits))));
	cols = (int)(ceil ((double)(rows) / 4.0) * 4);
	rows = (int)(ceil ((double)(sig_ptr->bits)/ (double)cols));

	format = "[%01d]=%c ";
	if (greater_index >= 10)  format = "[%02d]=%c ";
	if (greater_index >= 100) format = "[%03d]=%c ";

	bit = 0;
	for (row=rows - 1; row >= 0; row--) {
	    for (col = cols - 1; col >= 0; col--) {
		bit = (row * cols + col);
		if ((bit>=0) && (bit < sig_ptr->bits)) {
		    uint_t bit_value = val_bit (cptr, bit);
		    sprintf (strg2, format, greater_index +
			     ((greater_index >= lesser_index)
			      ? (bit - sig_ptr->bits + 1) : (sig_ptr->bits - bit - 1)),
			     "01uz"[bit_value]);
		    strcat (strg, strg2);
		    if (col==4 || col==8) strcat (strg, "  ");
		}
	    }
	    strcat (strg, "\n");
	}
	break;
    }

    if (sig_ptr->bits > 2) {
	int par = 0;
	Boolean_t unknown = FALSE;
	for (bit=0; bit<=sig_ptr->bits; bit++) {
	    uint_t bit_value = val_bit (cptr, bit);
	    if (bit_value==0 || bit_value==1) par ^= bit_value;
	    else unknown = TRUE;
	}
	if (!unknown) {
	    if (par) strcat (strg, "Odd Parity\n");
	    else  strcat (strg, "Even Parity\n");
	}
    }

    if (sig_ptr->bits == 1) {
	int	posedges = 0;
	int	negedges = 0;
	val_transition_cnt (sig_ptr, cptr, &negedges, &posedges);
	if (posedges && posedges>negedges) {
	    sprintf (strg2, "Posedge # %d\n", posedges);
	    strcat (strg, strg2);
	} else if (negedges) {
	    sprintf (strg2, "Negedge # %d\n", negedges);
	    strcat (strg, strg2);
	}
    }

    /* Debugging information */
    if (DTDEBUG) {
	strcat (strg, "\n");
	sprintf (strg2, "Value_stbits %08x  %s\n",
		 cptr->siglw.number,
		 val_state_name[cptr->siglw.stbits.state]);
	strcat (strg, strg2);
    }

    return (strg);
}

static void    val_examine_popdown (
    Trace_t	*trace)
{
    if (trace->examine.popup) {
	XtPopdown (trace->examine.popup);
	if (XtIsManaged(trace->examine.rowcol)) {
	    XtUnmanageChild (trace->examine.label);
	    XtUnmanageChild (trace->examine.rowcol);
	}

	/* We'll regenerate, as the text gets hosed elsewise */
	XtDestroyWidget (trace->examine.label);
	XtDestroyWidget (trace->examine.rowcol);
	XtDestroyWidget (trace->examine.popup);
	trace->examine.popup = NULL;
    }
}

static void    val_examine_popup (
    /* Create the popup menu for val_examine, based on cursor position x,y */
    Trace_t	*trace,
    XButtonPressedEvent	*ev)
{
    DTime_t	time;
    Signal_t	*sig_ptr;
    char	*strg = "No information here";
    XmString	xs;
    int		x,y;	/* Must allow signed */

    time = posx_to_time (trace, ev->x);
    sig_ptr = posy_to_signal (trace, ev->y);
    if (DTPRINT_ENTRY) printf ("In val_examine_popup %d %d time %d\n", __LINE__, ev->x, time);

    val_examine_popdown (trace);

    /* Get the text to display */
    /* Brace yourself for something that should be object oriented... */
    if (sig_ptr) {
	/* Get information */
	if (time>=0) {
	    strg = val_examine_string (trace, sig_ptr, time);
	} else {
	    strg = sig_examine_string (trace, sig_ptr);
	}
    } else {
	if (ev->y <= trace->ystart) {
	    DTime_t grid_time;
	    Grid_t *grid_ptr = posx_to_grid (trace, ev->x, &grid_time);
	    if (grid_ptr) strg = grid_examine_string (trace, grid_ptr, grid_time);
	} else {
	    DCursor_t *csr_ptr = posx_to_cursor (trace, ev->x);
	    if (csr_ptr) strg = cur_examine_string (trace, csr_ptr);
	}
    }


    /* First, a override for the shell */
    /* It's probably tempting to use XmCreatePopupMenu. */
    /* Don't.  It was that way, but the keyboard grab proved problematic */
    /* Don't manage this guy, he's just a container */
    XtSetArg (arglist[0], XmNallowShellResize, FALSE);
    XtSetArg (arglist[1], XmNshadowThickness, 2);
#if 1  /* If you get a error on the next line, try changing the 1 to a 0 */
    trace->examine.popup = (Widget)XmCreateGrabShell (trace->main, "examinepopup", arglist, 2);
#else
    trace->examine.popup = XtCreatePopupShell
	("examinepopup", overrideShellWidgetClass, trace->main, arglist, 1);
#endif

    /* Row column for a nice border */
    /* For Lesstif we used to have MENU_POPUP, but that has mouse side effects */
    XtSetArg (arglist[0], XmNrowColumnType, XmWORK_AREA);
    XtSetArg (arglist[1], XmNborderWidth, 1);
    XtSetArg (arglist[2], XmNentryAlignment, XmALIGNMENT_CENTER);
    XtSetArg (arglist[3], XmNshadowThickness, 2);
    trace->examine.rowcol = XmCreateRowColumn (trace->examine.popup,"rowcol",arglist,4);

    /* Finally the label */
    xs = string_create_with_cr (strg);
    XtSetArg (arglist[0], XmNlabelString, xs);
    trace->examine.label = XmCreateLabel (trace->examine.rowcol,"popuplabel",arglist,1);
    DManageChild (trace->examine.label, trace, MC_GLOBALKEYS);
    XmStringFree (xs);

    /* Don't stretch past right screen edge if possible */
    x = ev->x_root;  y = ev->y_root;
    if (x + trace->examine.label->core.width >= (XtScreen(trace->examine.popup)->width)) {
	x = (XtScreen(trace->examine.popup)->width) - trace->examine.label->core.width - 1;
    }
    if (y + trace->examine.label->core.height >= (XtScreen(trace->examine.popup)->height)) {
	y = (XtScreen(trace->examine.popup)->height) - trace->examine.label->core.height - 1;
    }
    x = MAX(x,0); y = MAX(y,0);
    XtSetArg (arglist[0], XmNx, x);
    XtSetArg (arglist[1], XmNy, y);
    XtSetValues (trace->examine.popup, arglist, 2);

    /* Putem up */
    DManageChild (trace->examine.rowcol, trace, MC_GLOBALKEYS);
    XtPopup (trace->examine.popup, XtGrabNone); /* Don't manage! */

    /* Make sure we find out when button gets released */
    XSelectInput (XtDisplay (trace->work),XtWindow (trace->examine.popup),
		 ButtonReleaseMask|PointerMotionMask|StructureNotifyMask|ExposureMask);

    /* We definately shouldn't have to force exposure of the popup. */
    /* However, the reality is lessTif doesn't seem to draw the text unless we do */
    /* This is a unknown bug in lessTif; it works fine on the true Motif */
    (xmRowColumnClassRec.core_class.expose) (trace->examine.rowcol, (XEvent*) ev, NULL);
    (xmLabelClassRec.core_class.expose) (trace->examine.label, (XEvent*) ev, NULL);
}

void    val_examine_ev (
    Widget		w,
    Trace_t		*trace,
    XButtonPressedEvent	*ev)
{
    XEvent	event;
    XButtonPressedEvent *em;
    int		update_pending = FALSE;
    extern char *events[];
    if (events) {}  /* Prevent unused warning */

    if (DTPRINT_ENTRY) printf ("In val_examine_ev, button=%d state=%d\n", ev->button, ev->state);
    if (ev->type != ButtonPress) return;	/* Used for both button 1 & 2. */

    /* not sure why this has to be done but it must be done */
    XSync (global->display, FALSE);
    XUngrabPointer (XtDisplay (trace->work),CurrentTime);

    /* select the events the widget will respond to */
    XSelectInput (XtDisplay (trace->work),XtWindow (trace->work),
		 ButtonReleaseMask|PointerMotionMask|StructureNotifyMask|ExposureMask);

    /* Create */
    val_examine_popup (trace, ev);

    /* loop and service events until button is released */
    while ( 1 ) {
	/* wait for next event */
	XNextEvent (global->display, &event);
	/* if (DTPRINT) { printf ("[ExEvent %s] ", events[ev->type]);fflush(stdout); } */

	/* Mark an update as needed */
	if (event.type == MotionNotify) {
	    update_pending = TRUE;
	}

	/* if window was exposed, must redraw it */
	if (event.type == Expose) win_expose_cb (0,trace);
	/* if window was resized, must redraw it */
	if (event.type == ConfigureNotify) win_resize_cb (0,trace);

	/* button released - calculate cursor position and leave the loop */
	if (event.type == ButtonRelease || event.type == ButtonPress) {
	    /* ButtonPress in case user is freaking out, some strange X behavior caused the ButtonRelease to be lost */
	    break;
	}

	/* If a update is needed, redraw the menu. */
	/* Do it later if events pending, otherwise dragging is SLOWWWW */
	if (update_pending && !XPending (global->display)) {
	    update_pending = FALSE;
	    em = (XButtonPressedEvent *)&event;
	    val_examine_popup (trace, em);
	}

	if (global->redraw_needed && !XtAppPending (global->appcontext)) {
	    draw_perform();
	}
    }

    /* reset the events the widget will respond to */
    XSync (global->display, FALSE);
    XSelectInput (XtDisplay (trace->work),XtWindow (trace->work),
		 ButtonPressMask|StructureNotifyMask|ExposureMask);

    /* unmanage popup */
    val_examine_popdown (trace);
    draw_needed (trace);
}

void    val_examine_popup_act (
    Widget		w,
    XButtonPressedEvent	*ev,
    String		params,
    Cardinal		*num_params)
{
    Trace_t	*trace;		/* Display information */
    int		prev_cursor;

    if (ev->type != ButtonPress) return;

    if (DTPRINT_ENTRY) printf ("In val_examine_popup_ev, button=%d state=%d\n", ev->button, ev->state);

    if (!(trace = widget_to_trace (w))) return;

    /* Create */
    prev_cursor = last_set_cursor ();
    set_cursor (DC_VAL_EXAM);

    val_examine_ev (w, trace, ev);

    set_cursor (prev_cursor);
}

static void    val_search_widget_update (
    Trace_t	*trace)
{
    VSearchNum_t search_pos;
    char	strg[MAXVALUELEN];

    /* Copy settings to local area to allow cancel to work */
    for (search_pos=0; search_pos<MAX_SRCH; search_pos++) {
	ValSearch_t *vs_ptr = &global->val_srch[search_pos];

	/* Update with current search enables */
	XtSetArg (arglist[0], XmNset, (vs_ptr->color != 0));
	XtSetValues (trace->value.enable[search_pos], arglist, 1);

	/* Update with current cursor enables */
	XtSetArg (arglist[0], XmNset, (vs_ptr->cursor != 0));
	XtSetValues (trace->value.cursor[search_pos], arglist, 1);

	/* Update with current search values */
	val_to_string (vs_ptr->radix, strg, &vs_ptr->value, 0, FALSE, TRUE);
	XmTextSetString (trace->value.text[search_pos], strg);

	/* Update with current signal values */
	XmTextSetString (trace->value.signal[search_pos], vs_ptr->signal);
    }
}

void    val_search_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    int		i;

    if (DTPRINT_ENTRY) printf ("In val_search_cb - trace=%p\n",trace);

    if (!trace->value.dialog) {
	XtSetArg (arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg (arglist[1], XmNdialogTitle, XmStringCreateSimple ("Value Search Requester") );
	XtSetArg (arglist[2], XmNverticalSpacing, 7);
	XtSetArg (arglist[3], XmNhorizontalSpacing, 10);
	trace->value.dialog = XmCreateFormDialog (trace->work,"search",arglist,4);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Color"));
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_FORM );
	trace->value.label1 = XmCreateLabel (trace->value.dialog,"label1",arglist,3);
	DManageChild (trace->value.label1, trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Place"));
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[2], XmNleftWidget, trace->value.label1);
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_FORM );
	trace->value.label2 = XmCreateLabel (trace->value.dialog,"label2",arglist,4);
	DManageChild (trace->value.label2, trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Value"));
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopWidget, trace->value.label1);
	trace->value.label4 = XmCreateLabel (trace->value.dialog,"label4",arglist,4);
	DManageChild (trace->value.label4, trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Cursor"));
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[2], XmNleftWidget, trace->value.label4);
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNtopWidget, trace->value.label2);
	trace->value.label5 = XmCreateLabel (trace->value.dialog,"label5",arglist,5);
	DManageChild (trace->value.label5, trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple (
	    "Search value, Hex default, 'd=decimal, 'b=binary"));
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[2], XmNleftWidget, trace->value.label5);
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNtopWidget, trace->value.label1);
	trace->value.label3 = XmCreateLabel (trace->value.dialog,"label3",arglist,5);
	DManageChild (trace->value.label3, trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Signal Wildcard"));
	XtSetArg (arglist[1], XmNrightAttachment, XmATTACH_FORM);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopWidget, trace->value.label1);
	trace->value.label6 = XmCreateLabel (trace->value.dialog,"label6",arglist,4);
	DManageChild (trace->value.label6, trace, MC_NOKEYS);

	for (i=0; i<MAX_SRCH; i++) {
	    /* enable button */
	    XtSetArg (arglist[0], XmNleftAttachment, XmATTACH_FORM);
	    XtSetArg (arglist[1], XmNselectColor, trace->xcolornums[i+1]);
	    XtSetArg (arglist[2], XmNlabelString, XmStringCreateSimple (" "));
	    XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[4], XmNtopWidget, ((i==0)?(trace->value.label4):(trace->value.signal[i-1])));
	    trace->value.enable[i] = XmCreateToggleButton (trace->value.dialog,"togglen",arglist,5);
	    DManageChild (trace->value.enable[i], trace, MC_NOKEYS);

	    /* enable button */
	    XtSetArg (arglist[0], XmNselectColor, trace->xcolornums[i+1]);
	    XtSetArg (arglist[1], XmNlabelString, XmStringCreateSimple (" "));
	    XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[3], XmNtopWidget, ((i==0)?(trace->value.label4):(trace->value.signal[i-1])));
	    XtSetArg (arglist[4], XmNleftAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[5], XmNleftWidget, trace->value.enable[i]);
	    trace->value.cursor[i] = XmCreateToggleButton (trace->value.dialog,"cursorn",arglist,6);
	    DManageChild (trace->value.cursor[i], trace, MC_NOKEYS);

	    /* create the file name text widget */
	    XtSetArg (arglist[0], XmNrows, 1);
	    XtSetArg (arglist[1], XmNcolumns, MAXVALUELEN);
	    XtSetArg (arglist[2], XmNresizeHeight, FALSE);
	    XtSetArg (arglist[3], XmNeditMode, XmSINGLE_LINE_EDIT);
	    XtSetArg (arglist[4], XmNtopAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[5], XmNtopWidget, ((i==0)?(trace->value.label4):(trace->value.signal[i-1])));
	    XtSetArg (arglist[6], XmNleftAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[7], XmNleftOffset, 20);
	    XtSetArg (arglist[8], XmNleftWidget, trace->value.cursor[i]);
	    trace->value.text[i] = XmCreateText (trace->value.dialog,"textn",arglist,9);
	    DAddCallback (trace->value.text[i], XmNactivateCallback, val_search_ok_cb, trace);
	    DManageChild (trace->value.text[i], trace, MC_NOKEYS);

	    /* create the signal text widget */
	    XtSetArg (arglist[0], XmNrows, 1);
	    XtSetArg (arglist[1], XmNcolumns, 30);
	    XtSetArg (arglist[2], XmNresizeHeight, FALSE);
	    XtSetArg (arglist[3], XmNeditMode, XmSINGLE_LINE_EDIT);
	    XtSetArg (arglist[4], XmNtopAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[5], XmNtopWidget, ((i==0)?(trace->value.label4):(trace->value.signal[i-1])));
	    XtSetArg (arglist[6], XmNleftAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[7], XmNleftOffset, 20);
	    XtSetArg (arglist[8], XmNleftWidget, trace->value.text[i]);
	    XtSetArg (arglist[9], XmNrightAttachment, XmATTACH_FORM );
	    trace->value.signal[i] = XmCreateText (trace->value.dialog,"texts",arglist,10);
	    DAddCallback (trace->value.signal[i], XmNactivateCallback, val_search_ok_cb, trace);
	    DManageChild (trace->value.signal[i], trace, MC_NOKEYS);
	}

	/* Ok/apply/cancel */
	ok_apply_cancel (&trace->value.okapply, trace->value.dialog,
			 dmanage_last,
			 (XtCallbackProc)val_search_ok_cb, trace,
			 (XtCallbackProc)val_search_apply_cb, trace,
			 NULL,NULL,
			 (XtCallbackProc)unmanage_cb, (Trace_t*)trace->value.dialog);
    }

    /* Copy settings to local area to allow cancel to work */
    val_search_widget_update (trace);

    /* manage the popup on the screen */
    DManageChild (trace->value.dialog, trace, MC_NOKEYS);
}

void    val_search_ok_cb (
    Widget	w,
    Trace_t	*trace,
    XmSelectionBoxCallbackStruct *cb)
{
    char	*strg;
    int		i;
    char	origstrg[MAXVALUELEN];

    if (DTPRINT_ENTRY) printf ("In val_search_ok_cb - trace=%p\n",trace);

    for (i=0; i<MAX_SRCH; i++) {
	ValSearch_t *vs_ptr = &global->val_srch[i];

	/* Update with current search enables */
	vs_ptr->color = (XmToggleButtonGetState (trace->value.enable[i])) ? i+1 : 0;

	/* Update with current cursor enables */
	vs_ptr->cursor = (XmToggleButtonGetState (trace->value.cursor[i])) ? i+1 : 0;

	/* Update with current search values */
	val_to_string (vs_ptr->radix, origstrg, &vs_ptr->value, 0, FALSE, TRUE);
	strg = XmTextGetString (trace->value.text[i]);
	if (strcmp (origstrg, strg)) {
	    /* Only update if it has changed, so that a search for a pattern */
	    /* of X & U's won't disappear when rendered as a simple "X" string */
	    string_to_value (&vs_ptr->radix, strg, &vs_ptr->value);
	}

	/* Update with current search values */
	strg = XmTextGetString (trace->value.signal[i]);
	strcpy (vs_ptr->signal, strg);

	if (DTPRINT_SEARCH) {
	    char strg2[MAXVALUELEN];
	    val_to_string (global->radixs[0], strg2, &vs_ptr->value, 0, FALSE, TRUE);
	    printf ("Search %d) %d  %s  '%s' -> '%s'\n", i, vs_ptr->color,
		    vs_ptr->radix->name, strg, strg2);
	}
    }

    XtUnmanageChild (trace->value.dialog);

    draw_needupd_val_search ();
    draw_all_needed ();
}

void    val_search_apply_cb (
    Widget	w,
    Trace_t	*trace,
    XmSelectionBoxCallbackStruct *cb)
{
    if (DTPRINT_ENTRY) printf ("In val_search_apply_cb - trace=%p\n",trace);

    val_search_ok_cb (w,trace,cb);
    val_search_cb (trace->main);
}

void    val_highlight_ev (
    Widget	w,
    Trace_t	*trace,
    XButtonPressedEvent	*ev)
{
    DTime_t	time;
    Signal_t	*sig_ptr;
    Value_t	*cptr;

    if (DTPRINT_ENTRY) printf ("In val_highlight_ev - trace=%p\n",trace);
    if (ev->type != ButtonPress || ev->button!=1) return;

    time = posx_to_time (trace, ev->x);
    sig_ptr = posy_to_signal (trace, ev->y);
    if (!sig_ptr && time<=0) return;
    cptr = value_at_time (sig_ptr, time);
    if (!cptr) return;

    /* Change the color */
    if (global->highlight_color > 0) {
	ValSearch_t *vs_ptr = &global->val_srch[global->highlight_color - 1];

	val_copy (&vs_ptr->value, cptr);
	vs_ptr->radix = sig_ptr->radix;
	strcpy (vs_ptr->signal, sig_ptr->signame);
	if (!vs_ptr->color && !vs_ptr->cursor ) {
	    /* presume user really wants color if neither is on */
	    vs_ptr->color = global->highlight_color;
	    vs_ptr->cursor = global->highlight_color;
	}
    }

    /* If search requester is on the screen, update it too */
    if (trace->value.dialog && XtIsManaged (trace->value.dialog)) {
	val_search_widget_update (trace);
    }

    /* redraw the screen */
    draw_needupd_val_search ();
    draw_all_needed ();
}


/****************************** ANNOTATION ******************************/

void    val_annotate_cb (
    Widget	w)
{
    Trace_t *trace = widget_to_trace(w);
    Widget last;
    int i;

    if (DTPRINT_ENTRY) printf ("In val_annotate_cb - trace=%p\n",trace);

    if (!trace->annotate.dialog) {
	XtSetArg (arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg (arglist[1], XmNdialogTitle, XmStringCreateSimple ("Annotate Menu"));
	XtSetArg (arglist[2], XmNverticalSpacing, 7);
	XtSetArg (arglist[3], XmNhorizontalSpacing, 10);
	trace->annotate.dialog = XmCreateFormDialog (trace->work, "annotate",arglist,4);

	/* create label widget for text widget */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("File Name") );
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_FORM );
	trace->annotate.label1 = XmCreateLabel (trace->annotate.dialog,"l1",arglist,3);
	DManageChild (trace->annotate.label1, trace, MC_NOKEYS);

	/* create the file name text widget */
	XtSetArg (arglist[0], XmNrows, 1);
	XtSetArg (arglist[1], XmNcolumns, 30);
	XtSetArg (arglist[2], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[3], XmNrightAttachment, XmATTACH_FORM );
	XtSetArg (arglist[4], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[5], XmNtopWidget, dmanage_last );
	XtSetArg (arglist[6], XmNresizeHeight, FALSE);
	XtSetArg (arglist[7], XmNeditMode, XmSINGLE_LINE_EDIT);
	trace->annotate.text = XmCreateText (trace->annotate.dialog,"filename",arglist,8);
	DManageChild (trace->annotate.text, trace, MC_NOKEYS);
	DAddCallback (trace->annotate.text, XmNactivateCallback, val_annotate_ok_cb, trace);

	/* Cursor enables */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Include which user (solid) cursor colors:") );
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopWidget, dmanage_last );
	trace->annotate.label2 = XmCreateLabel (trace->annotate.dialog,"l2",arglist,4);
	DManageChild (trace->annotate.label2, trace, MC_NOKEYS);

	last = trace->annotate.dialog;
	for (i=0; i<=MAX_SRCH; i++) {
	    /* enable button */
	    XtSetArg (arglist[0], XmNx, 15+30*i);
	    XtSetArg (arglist[1], XmNtopAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[2], XmNtopWidget, trace->annotate.label2 );
	    XtSetArg (arglist[3], XmNselectColor, trace->xcolornums[i]);
	    XtSetArg (arglist[4], XmNlabelString, XmStringCreateSimple (" "));
	    trace->annotate.cursors[i] = XmCreateToggleButton (trace->annotate.dialog,"togglenc",arglist,5);
	    DManageChild (trace->annotate.cursors[i], trace, MC_NOKEYS);
	    last = trace->annotate.cursors[i];
	}

	/* Cursor enables */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Include which auto (dotted) cursor colors:") );
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopWidget, trace->annotate.cursors[0] );
	trace->annotate.label4 = XmCreateLabel (trace->annotate.dialog,"l4",arglist,4);
	DManageChild (trace->annotate.label4, trace, MC_NOKEYS);

	for (i=0; i<=MAX_SRCH; i++) {
	    /* enable button */
	    XtSetArg (arglist[0], XmNx, 15+30*i);
	    XtSetArg (arglist[1], XmNtopAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[2], XmNtopWidget, trace->annotate.label4 );
	    XtSetArg (arglist[3], XmNselectColor, trace->xcolornums[i]);
	    XtSetArg (arglist[4], XmNlabelString, XmStringCreateSimple (" "));
	    trace->annotate.cursors_dotted[i] = XmCreateToggleButton (trace->annotate.dialog,"togglencd",arglist,5);
	    DManageChild (trace->annotate.cursors_dotted[i], trace, MC_NOKEYS);
	}

	/* Signal Enables */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Include which signal colors:") );
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopWidget, trace->annotate.cursors_dotted[0] );
	trace->annotate.label3 = XmCreateLabel (trace->annotate.dialog,"",arglist,4);
	DManageChild (trace->annotate.label3, trace, MC_NOKEYS);

	for (i=1; i<=MAX_SRCH; i++) {
	    /* enable button */
	    XtSetArg (arglist[0], XmNx, 15+30*i);
	    XtSetArg (arglist[1], XmNtopAttachment, XmATTACH_WIDGET );
	    XtSetArg (arglist[2], XmNtopWidget, trace->annotate.label3 );
	    XtSetArg (arglist[3], XmNselectColor, trace->xcolornums[i]);
	    XtSetArg (arglist[4], XmNlabelString, XmStringCreateSimple (" "));
	    trace->annotate.signals[i] = XmCreateToggleButton (trace->annotate.dialog,"togglen",arglist,5);
	    DManageChild (trace->annotate.signals[i], trace, MC_NOKEYS);
	}

	/* Label */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Include signals from:") );
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopWidget, trace->annotate.signals[1]);
	trace->annotate.trace_label = XmCreateLabel (trace->annotate.dialog,"",arglist,4);
	DManageChild (trace->annotate.trace_label, trace, MC_NOKEYS);

	/* Begin pulldown */
	trace->annotate.trace_pulldown = XmCreatePulldownMenu (trace->annotate.dialog,"trace_pulldown",arglist,0);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("All Traces & Deleted") );
	trace->annotate.trace_button[TRACESEL_ALLDEL] =
	    XmCreatePushButtonGadget (trace->annotate.trace_pulldown,"pdbutton2",arglist,1);
	DManageChild (trace->annotate.trace_button[TRACESEL_ALLDEL], trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("All Traces") );
	trace->annotate.trace_button[TRACESEL_ALL] =
	    XmCreatePushButtonGadget (trace->annotate.trace_pulldown,"pdbutton1",arglist,1);
	DManageChild (trace->annotate.trace_button[TRACESEL_ALL], trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("This Trace") );
	trace->annotate.trace_button[TRACESEL_THIS] =
	    XmCreatePushButtonGadget (trace->annotate.trace_pulldown,"pdbutton0",arglist,1);
	DManageChild (trace->annotate.trace_button[TRACESEL_THIS], trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNsubMenuId, trace->annotate.trace_pulldown);
	XtSetArg (arglist[1], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[2], XmNtopWidget, trace->annotate.trace_label );
	XtSetArg (arglist[3], XmNleftAttachment, XmATTACH_FORM );
	trace->annotate.trace_option = XmCreateOptionMenu (trace->annotate.dialog,"options",arglist,4);
	DManageChild (trace->annotate.trace_option, trace, MC_NOKEYS);

	/* Ok/apply/cancel */
	ok_apply_cancel (&trace->annotate.okapply, trace->annotate.dialog,
			 dmanage_last,
			 (XtCallbackProc)val_annotate_ok_cb, trace,
			 (XtCallbackProc)val_annotate_apply_cb, trace,
			 NULL,NULL,
			 (XtCallbackProc)unmanage_cb, (Trace_t*)trace->annotate.dialog);
    }

    /* reset file name */
    XtSetArg (arglist[0], XmNvalue, global->anno_filename);
    XtSetValues (trace->annotate.text,arglist,1);

    /* Menu */
    XtSetArg (arglist[0], XmNmenuHistory,
	      trace->annotate.trace_button[global->anno_traces]);
    XtSetValues (trace->annotate.trace_option, arglist, 1);

    /* reset enables */
    for (i=0; i<=MAX_SRCH; i++) {
	XtSetArg (arglist[0], XmNset, (global->anno_ena_cursor[i] != 0));
	XtSetValues (trace->annotate.cursors[i], arglist, 1);

	XtSetArg (arglist[0], XmNset, (global->anno_ena_cursor_dotted[i] != 0));
	XtSetValues (trace->annotate.cursors_dotted[i], arglist, 1);
    }
    for (i=1; i<=MAX_SRCH; i++) {
	XtSetArg (arglist[0], XmNset, (global->anno_ena_signal[i] != 0));
	XtSetValues (trace->annotate.signals[i], arglist, 1);
    }

    /* manage the popup on the screen */
    DManageChild (trace->annotate.dialog, trace, MC_NOKEYS);
}

void    val_annotate_ok_cb (
    Widget	w,
    Trace_t	*trace,
    XmAnyCallbackStruct *cb)
{
    char	*strg;
    int		i;
    Widget	clicked;

    if (DTPRINT_ENTRY) printf ("In sig_search_ok_cb - trace=%p\n",trace);

    /* Update with current search enables */
    for (i=0; i<=MAX_SRCH; i++) {
	global->anno_ena_cursor[i] = XmToggleButtonGetState (trace->annotate.cursors[i]);
	global->anno_ena_cursor_dotted[i] = XmToggleButtonGetState (trace->annotate.cursors_dotted[i]);
    }
    for (i=1; i<=MAX_SRCH; i++) {
	global->anno_ena_signal[i] = XmToggleButtonGetState (trace->annotate.signals[i]);
    }

    /* Update with current search values */
    strg = XmTextGetString (trace->annotate.text);
    strcpy (global->anno_filename, strg);

    XtSetArg (arglist[0], XmNmenuHistory, &clicked);
    XtGetValues (trace->annotate.trace_option, arglist, 1);
    for (i=0; i<3; i++) {
        if (clicked == trace->annotate.trace_button[i]) {
	    global->anno_traces = (TraceSel_t)(i);
        }
    }

    global->anno_last_trace = trace;

    XtUnmanageChild (trace->annotate.dialog);

    val_annotate_do_cb (w,trace,cb);
}

void    val_annotate_apply_cb (
    Widget	w,
    Trace_t	*trace,
    XmAnyCallbackStruct	*cb)
{
    if (DTPRINT_ENTRY) printf ("In sig_search_apply_cb - trace=%p\n",trace);

    val_annotate_ok_cb (w,trace,cb);
    val_annotate_cb (trace->main);
}

static void	val_print_quoted (
    FILE *fp,
    char *strg)
{
    char *cp;
    for (cp=strg; *cp; cp++) {
	if (*cp == '"') {
	    fputc ('\\', fp);
	}
	fputc (*cp, fp);
    }
}

void    val_annotate_do_cb (
    Widget	w,
    Trace_t	*trace,
    XmAnyCallbackStruct	*cb)
{
    int		i;
    Signal_t	*sig_ptr;
    Trace_t	*trace_loop;
    Value_t	*cptr;
    FILE	*dump_fp;
    DCursor_t 	*csr_ptr;		/* Current cursor being printed */
    char	strg[1000];
    int		csr_num, csr_num_incl;

    if (DTPRINT_ENTRY) printf ("In val_annotate_cb - trace=%p  file=%s\n",trace,global->anno_filename);

    draw_update ();

    /* Socket connection */
#if HAVE_SOCKETS
    socket_create ();
#endif

    if (global->anno_last_trace == NULL) global->anno_last_trace = global->trace_head;

    if (! (dump_fp=fopen (global->anno_filename, "w"))) {
	sprintf (message,"Bad Filename: %s\n", global->anno_filename);
	dino_error_ack (trace,message);
	return;
    }

    fprintf (dump_fp, "(setq dinotrace-program-version \"%s\")\n\n", DTVERSION);

    /* Socket info */
    if (!global->anno_socket[0] || strchr (global->anno_socket, '*') ) {
	fprintf (dump_fp, "(setq dinotrace-socket-name nil)\n");
    }
    else {
	fprintf (dump_fp, "(setq dinotrace-socket-name \"%s\")\n\n",
		 global->anno_socket);
    }

    fprintf (dump_fp, "(setq dinotrace-hierarchy-separator \"%c\")\n\n",
	     global->trace_head->dfile.hierarchy_separator);

    /* Trace info */
    fprintf (dump_fp, "(setq dinotrace-traces '(\n");
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	if (trace->loaded) {
	    fprintf (dump_fp, "	[\"%s\"\t\"%s\"]\n",
		     trace->dfile.filename,
		     date_string (trace->dfile.filestat.st_ctime));
	}
    }
    fprintf (dump_fp, "\t))\n");

#define colornum_to_name(_color_)  (((_color_)==0)?"":global->color_names[(_color_)])
    /* Cursor info */
    /* [time color-num color-name nil(font) note] */
    fprintf (dump_fp, "(setq dinotrace-cursors [\n");
    for (csr_ptr = global->cursor_head; csr_ptr; csr_ptr = csr_ptr->next) {
	if ((csr_ptr->type==USER) ? global->anno_ena_cursor[csr_ptr->color] : global->anno_ena_cursor_dotted[csr_ptr->color] ) {
	    time_to_string (global->trace_head, strg, csr_ptr->time, FALSE);
	    fprintf (dump_fp, "\t[\"%s\"\t%d\t\"%s\"\tnil\t", strg,
		     csr_ptr->color, colornum_to_name(csr_ptr->color));
	    if (csr_ptr->note) fprintf (dump_fp, "\"%s\"", csr_ptr->note);
	    else fprintf (dump_fp, "nil");
	    fprintf (dump_fp, "]\n");
	}
    }
    fprintf (dump_fp, "\t])\n");

    /* Value search info */
    /* [value color-num color-name] */
    fprintf (dump_fp, "(setq dinotrace-value-searches '(\n");
    for (i=1; i<=MAX_SRCH; i++) {
	ValSearch_t *vs_ptr = &global->val_srch[i-1];
	if (vs_ptr->color) {
	    val_to_string (vs_ptr->radix, strg, &vs_ptr->value, 0, FALSE, FALSE);
	    fprintf (dump_fp, "\t[\"");
	    val_print_quoted (dump_fp, strg);
	    fprintf (dump_fp, "\"\t%d\t\"%s\"]\n", i, colornum_to_name(i));
	}
    }
    fprintf (dump_fp, "\t))\n");

    /* Signal color info */
    /* 0's never actually used, but needed in the array so aref will work in emacs */
    /* [color-num color-name] */
    fprintf (dump_fp, "(setq dinotrace-signal-colors [\n");
    for (i=0; i<=MAX_SRCH; i++) {
	fprintf (dump_fp, "\t[%d\t\"%s\"\tnil]\n", i, (i==0 || !global->color_names[i])?"":global->color_names[i]);
    }
    fprintf (dump_fp, "\t])\n");

    /* Find number of cursors that will be included */
    csr_num_incl = 0;
    for (csr_ptr = global->cursor_head; csr_ptr; csr_ptr = csr_ptr->next) {
	if ((csr_ptr->type==USER) ? global->anno_ena_cursor[csr_ptr->color] : global->anno_ena_cursor_dotted[csr_ptr->color] ) {
	    csr_num_incl++;
	}
    }

    /* Signal values */
    /* (basename [name color-num note values] nil(propertied)) */
    fprintf (dump_fp, "(setq dinotrace-signal-values '(\n");
    for (trace_loop = global->deleted_trace_head; trace_loop; trace_loop = trace_loop->next_trace) {
	/* Process the deleted trace last, so visible values override deleted values */
	trace = trace_loop->next_trace ? trace_loop->next_trace : global->deleted_trace_head;
        if ((   global->anno_traces == TRACESEL_THIS && trace!=global->anno_last_trace)
	    || (global->anno_traces == TRACESEL_ALL && trace==global->deleted_trace_head)) {
	    continue;
	}

	for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	    char *basename, *p;
	    basename = strdup (sig_basename (trace, sig_ptr));
	    p = strrchr (basename, '[');	/* Not vector_separator: it's standardized by here*/
	    if (p) *p = '\0';
	    fprintf (dump_fp, "\t(\"%s\"\t", basename);

	    fprintf (dump_fp, "\t[\"%s\"\t", sig_ptr->signame);
	    if (global->anno_ena_signal[sig_ptr->color]) fprintf (dump_fp, "%d\t", sig_ptr->color);
	    else     fprintf (dump_fp, "nil\t");

	    if (sig_ptr->note) fprintf (dump_fp, "\"%s\"\t", sig_ptr->note);
	    else fprintf (dump_fp, "nil\t");

	    fprintf (dump_fp, "(");
	    csr_num=0;
	    cptr = sig_ptr->cptr;
	    for (csr_ptr = global->cursor_head; csr_ptr; csr_ptr = csr_ptr->next) {

		if ((csr_ptr->type==USER) ? global->anno_ena_cursor[csr_ptr->color] : global->anno_ena_cursor_dotted[csr_ptr->color] ) {
		    csr_num++;

		    /* Note grabs value to right of cursor */
		    while ( (CPTR_TIME(cptr) < csr_ptr->time)
			    && (CPTR_TIME(cptr) != EOT)) {
			cptr = CPTR_NEXT(cptr);
		    }
		    if ( (CPTR_TIME(cptr) > csr_ptr->time)
			 && ( cptr > sig_ptr->bptr)) {
			cptr = CPTR_PREV(cptr);
		    }

		    val_to_string (sig_ptr->radix, strg, cptr, sig_ptr->bits, FALSE, FALSE);

		    /* First value must have `, last must have ', commas in middle */
		    fprintf (dump_fp, "\"");
		    if (csr_num==1) fprintf (dump_fp, "`");
		    else  fprintf (dump_fp, ",");
		    val_print_quoted (dump_fp, strg);
		    if (csr_num==csr_num_incl) fprintf (dump_fp, "'");
		    fprintf (dump_fp, "\"\t");
		}
	    }
	    fprintf (dump_fp, ") nil])\n");
	}
    }
    fprintf (dump_fp, "\t))\n");

    fclose (dump_fp);
}

