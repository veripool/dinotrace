#pragma ident "$Id$"
/******************************************************************************
 * dt_printscreen.c --- postscript output routines
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

#include "dinotrace.h"

#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include <Xm/Text.h>
#include <Xm/BulletinB.h>
#include <Xm/Scale.h>
#include <Xm/Label.h>
#include <Xm/Separator.h>

#include "functions.h"

#include "dt_post.h"

/**********************************************************************/

/* If XTextWidth is called in this file, then something's wrong, as X widths != print widths */


static void print_signame_scrolled (
    Trace	*trace,
    FILE	*psfile,
    Signal	*sig_ptr)
/* Print signame scrolled to right point */
{
    char *basename;
    char *out;
    char *cp;
    int outpos;
    int frontspaces;	/* How many spaces should be in front of sig to align hiearchy */
    int frontnamepos;	/* What namepos is, with 0 at frontspaces[0] */
    int siglen, baselen;

    fprintf (psfile, "(");

    basename = sig_basename (trace, sig_ptr);
    siglen = strlen(sig_ptr->signame);
    baselen = strlen(basename);
    out = (char *)XtMalloc(10+global->namepos_visible);

    frontspaces = global->namepos_hier - (siglen - baselen);
    frontnamepos = ((global->namepos_hier + global->namepos_base
		     - global->namepos_visible)
		    - global->namepos);
    /* Copy it */
    for (outpos=0, cp=out; outpos < global->namepos_visible; outpos++, cp++) {
	int namepos = frontnamepos + outpos - frontspaces;
	if (namepos==(siglen - baselen)) {
	    fprintf (psfile, ") (");
	}
	if (namepos>=0 && namepos<siglen) 
	    fputc (sig_ptr->signame[namepos], psfile);
	else fputc (' ', psfile);
    }

    fprintf (psfile, ")");
}

void    print_reset (
    Trace *trace)
{
    char 		*pchar;
    if (DTPRINT_ENTRY) printf ("In print_reset - trace=%p",trace);
    
    /* Init print name */
#ifdef VMS
    strcpy (trace->printname, "sys$login:dinotrace.ps");
#else
    trace->printname[0] = '\0';
    if (NULL != (pchar = getenv ("HOME"))) strcpy (trace->printname, pchar);
    if (trace->printname[0]) strcat (trace->printname, "/");
    strcat (trace->printname, "dinotrace.ps");
#endif
}

static void print_draw_grid (
    Trace	*trace,
    FILE	*psfile,
    DTime	printtime,	/* Time to start on */
    Grid	*grid_ptr,		/* Grid information */
    Boolean_t	draw_numbers)		/* Whether to print the times or not */
{ 
    char 	strg[MAXTIMELEN];	/* String value to print out */
    int		end_time;
    DTime	xtime;
    Position	x2;			/* Coordinate of current time */
    Position	ytop,ymid,ybot;

    if (grid_ptr->period < 1) return;

    /* Is the grid too small or invisible?  If so, skip it */
    if ((! grid_ptr->visible) || ((grid_ptr->period * global->res) < MIN_GRID_RES)) {
	return;
    }

    /* Other coordinates */
    ytop = trace->ystart - Y_GRID_TOP;
    ymid = trace->ystart - Y_DASH_TOP;
    ybot = trace->yend + Y_GRID_BOTTOM;

    /* Start grid */
    fprintf (psfile,"\n[1 3] 0 setdash\n");
    fprintf (psfile,"%d %d %d START_GRID\n", ytop, ybot, trace->ygridtime);

    /* Start to left of right edge */
    xtime = printtime;
    end_time = printtime + TIME_WIDTH (trace);

    /***** Draw the grid lines */

    switch (grid_ptr->period_auto) {
    case PA_EDGE:
	/* Edges not supported yet */
	break;

    case PA_USER:
    case PA_AUTO:
    default:
	/* Do a regular grid every so many grid units */

	/* Move starting point to the right to hit where a grid line is aligned */
	xtime = ((xtime / grid_ptr->period) * grid_ptr->period) + (grid_ptr->alignment % grid_ptr->period);

	/* Unlike the display, we don't move back one, it's too hard to read signal names */
	/* Loop through grid times */
	for ( ; xtime <= end_time; xtime += grid_ptr->period) {
	    x2 = TIME_TO_XPOS_REL (xtime, printtime);
	    /* compute the time value and draw it if it fits */
	    if (draw_numbers) {
		time_to_string (trace, strg, xtime, FALSE);
	    } else strg[0] = '\0';

	    /* Draw if space, centered on grid line */
	    fprintf (psfile,"%d (%s) GRID\n", x2, strg);
	}
	break;
    }
}

static void print_draw_grids (
    Trace	*trace,
    FILE	*psfile,
    DTime	printtime)
{           
    int		grid_num;
    Grid	*grid_ptr;
    Grid	*grid_smallest_ptr;

    /* WARNING, major weedyness follows: */
    /* Determine which grid has the smallest period and only plot its times. */
    /* If we don't do this, labels may overlap */

    grid_smallest_ptr = &(trace->grid[0]);
    for (grid_num=0; grid_num<MAXGRIDS; grid_num++) {
	grid_ptr = &(trace->grid[grid_num]);
	if ( grid_ptr->visible
	    && ((grid_ptr->period * global->res) >= MIN_GRID_RES)	/* not too small */
	    && (grid_ptr->period > 0)
	    && (grid_ptr->period < grid_smallest_ptr->period) ) {
	    grid_smallest_ptr = grid_ptr;
	}
    }

    /* Draw each grid */
    for (grid_num=0; grid_num<MAXGRIDS; grid_num++) {
	grid_ptr = &(trace->grid[grid_num]);
	print_draw_grid (trace, psfile, printtime, grid_ptr, (grid_ptr == grid_smallest_ptr ) );
    }
}


static void print_draw_val (
    Trace	*trace,
    FILE	*psfile,
    Signal	*sig_ptr,	/* Vertical signal to start on */
    Signal	*sig_end_ptr,	/* Last signal to print */
    DTime	printtime)	/* Time to start on */
{
    int ymdpt,xend;
    int yspace;
    int xsigrf;
    int xleft, xright;
    const Value_t *cptr,*nptr;
    char strg[MAXVALUELEN];
    int unstroked=0;		/* Number commands not stroked */
    int numprt;
    
    if (DTPRINT_ENTRY) printf ("In print_draw - filename=%s, printtime=%d sig=%s\n",trace->filename, printtime, sig_ptr->signame);
    
    xend = trace->width - XMARGIN;
    xsigrf = MAX(1,global->sigrf);
    
    /* Loop and draw each signal individually */
    for (numprt=0; sig_ptr && sig_ptr!=sig_end_ptr;
	 sig_ptr = sig_ptr->forward, numprt++) {
	int yhigh, ylow;
	/* Calculate line position */
	yhigh = trace->ystart + numprt * global->sighgt;
	ylow = yhigh + global->sighgt - Y_SIG_SPACE;
	/* Steal space from Y_SIG_SPACE if we can, down to 2 pixels */
	yspace = global->sighgt - global->signal_font->max_bounds.ascent;
	yspace = MIN( yspace, Y_SIG_SPACE);  /* Bound reasonably */
	yspace = MAX( yspace, 2);  /* Bound reasonably */
	ymdpt = (yhigh + ylow)/2;

	/* Compute starting points for signal */
	xright = global->xstart;
	
	/* find data start */
	cptr = sig_ptr->bptr;
	if (printtime >= CPTR_TIME(cptr) ) {
	    while ((CPTR_TIME(cptr) != EOT) &&
		   (printtime > CPTR_TIME(CPTR_NEXT(cptr)))) {
		cptr = CPTR_NEXT(cptr);
	    }
	}

	/* output y information - note reverse from draw() due to y-axis */
	/* output starting positional information */
	fprintf (psfile,"%d %d %d %d START_SIG\n",
		 ymdpt, yhigh, ylow, xright);
	
	/* Loop as long as the time and end of trace are in current screen */
	for (; (CPTR_TIME(cptr) != EOT && xright < xend);
	     cptr = nptr) {
	    uint_t dr_mask = 0;	/* Bitmask of how to draw transitions */
#define DR_HIGH		0x01
#define DR_HIGHHIGH	0x02	/* wide high line */
#define DR_LOW		0x04
#define DR_Z		0x08
#define DR_U		0x10

	    /* find the next transition */
	    nptr = CPTR_NEXT(cptr);
	    
	    /* if next transition is the end, don't draw */
	    if (CPTR_TIME(nptr) == EOT) break;

	    /* find the x location for the left and right of this segment */
	    xleft = TIME_TO_XPOS_REL (CPTR_TIME(cptr),printtime);
	    xleft = MIN (xleft, xend);
	    xleft = MAX (xleft, xright);

	    xright = TIME_TO_XPOS_REL (CPTR_TIME(nptr),printtime);
	    xright = MIN (xright, xend);

	    if (cptr->siglw.stbits.allhigh) dr_mask |= DR_HIGHHIGH;
	    switch (cptr->siglw.stbits.state) {
	    case STATE_0:	dr_mask |= DR_LOW; break;
	    case STATE_1:	dr_mask |= DR_HIGHHIGH; break;
	    case STATE_U:	dr_mask |= DR_U; break;
	    case STATE_F32:	dr_mask |= DR_U; break;
	    case STATE_F128:	dr_mask |= DR_U; break;
	    case STATE_Z:	dr_mask |= DR_Z; break;
	    default:		dr_mask |= DR_LOW | DR_HIGH; break;
	    }

	    /*printf ("cptr %s t %d x %d xl %d xr %d xe %d\n",
	      sig_ptr->signame, (int)CPTR_TIME(cptr), (int)TIME_TO_XPOS(CPTR_TIME(cptr)),
	      xleft, xright, xend );
	      printf (" nptr    t %d x %d xl %d xr %d xe %d\n",
	      (int)CPTR_TIME(nptr), (int)TIME_TO_XPOS(CPTR_TIME(nptr)),
	      xleft, xright, xend ); */

	    /* Compress invisible transitions into a glitch */
	    if ((xright - xleft) <= (xsigrf*2)) {
		int xleft_ok_next = MIN(xend, xleft + xsigrf*2);	/* May pass xend.. looks better then truncating */
		const Value_t *nnptr;
		xright = xleft + xsigrf*2;
		/*printf (" glitch %d\n", xleft_ok_next);*/
		/* Too tight for drawing, compress transitions in this space */
		/* Scan forward nptrs till find end of compression point */
		while (1) {
		    nnptr = CPTR_NEXT(nptr);
		    if ((CPTR_TIME(nnptr) == EOT
			 || (TIME_TO_XPOS (CPTR_TIME(nnptr)) > xleft_ok_next))) {
			break;
		    }
		    /*printf ("nnptr    t %d %s x %d xl %d xr %d xe %d  *hunt %x\n",
		      (int)CPTR_TIME(nnptr), (int)TIME_TO_XPOS(CPTR_TIME(nnptr)),
		      xleft, xright, xend, xleft_ok_next );*/
		    /* Build combination image, which is overlay of all values in space */
		    switch (nptr->siglw.stbits.state) {
		    case STATE_0:	dr_mask |= DR_LOW; break;
		    case STATE_1:	dr_mask |= DR_HIGHHIGH; break;
		    case STATE_U:	dr_mask |= DR_U; break;
		    case STATE_F32:	dr_mask |= DR_U; break;
		    case STATE_F128:	dr_mask |= DR_U; break;
		    case STATE_Z:	dr_mask |= DR_Z; break;
		    default:		dr_mask |= DR_LOW | DR_HIGH; break;
		    }
		    xright = MAX(xright, TIME_TO_XPOS (CPTR_TIME(nnptr)));
		    nptr = nnptr;
		}
		xright = MIN (xright, xend);
	    }
	    
	    /* Draw the transition lines */
	    if (dr_mask) {
		char cmd;
		if (dr_mask & DR_U) {
		    cmd='U';	/* SU  unknown fill */
		} else if (dr_mask == DR_LOW) {
		    cmd='0';	/* S0  low */
		} else if (dr_mask == DR_HIGHHIGH) {
		    cmd='1';	/* S1  high */
		} else if (dr_mask & DR_HIGHHIGH) {
		    cmd='H';	/* SH  bus all high */
		} else if (dr_mask == DR_Z) {
		    cmd='Z';	/* SZ  tristate dash */
		} else cmd='B';	/* SB  Bus, if nothing better chosen */
		fprintf (psfile,"%d S%c",xright, cmd);
	    }

	    /* Plot value */
	    if (sig_ptr->bits>1
		&& cptr->siglw.stbits.state != STATE_U
		&& cptr->siglw.stbits.state != STATE_Z
		) {
		char *vsname = NULL;
		char *vname = "";
		uint_t num0 = 0;
		if (cptr->siglw.stbits.state != STATE_0) num0 = cptr->number[0];
		/* Below evaluation left to right important to prevent error */
		if (cptr->siglw.stbits.state != STATE_0) {
		    val_to_string (sig_ptr->radix, strg, cptr, TRUE);
		    vname = strg;
		}
		if ((cptr->siglw.stbits.state == STATE_B32
		     || cptr->siglw.stbits.state == STATE_0) &&
		    (sig_ptr->decode != NULL) &&
		    (num0 < sig_ptr->decode->numstates) &&
		    (sig_ptr->decode->statename[num0][0] != '\0')) {
		    vsname = sig_ptr->decode->statename[num0];
		    fprintf (psfile," (%s) (%s) SN",vsname,vname);
		}
		else if (vname[0]) {
		    fprintf (psfile," (%s) SV",vname);
		}
	    } /* if bus */
	    fprintf (psfile,"\n");

	    if (unstroked++ > 400) {
		/* Must stroke every so often to avoid overflowing printer stack */
		fprintf (psfile," STROKE\n");
		unstroked=0;
	    }
	} /* for cptr */
	fprintf (psfile,"stroke\n");
    } /* for sig_ptr */
} /* End of DRAW */


static void print_draw_sig (
    Trace	*trace,
    FILE	*psfile,
    Signal	*sig_ptr,	/* Vertical signal to start on */
    Signal	*sig_end_ptr)	/* Last signal to print */
{
    int		numprt=0;
    
    if (DTPRINT_ENTRY) printf ("In print_drawsig - filename=%s\n",trace->filename);
    
    /* don't draw anything if there is no file is loaded */
    if (!trace->loaded) return;
    
    /* loop thru all the visible signals */
    for (; sig_ptr && sig_ptr!=sig_end_ptr; sig_ptr = sig_ptr->forward) {
	int yhigh, ylow, ymdpt;

	/* calculate the location to start drawing the signal name */
	/* printf ("xstart=%d, %s\n",global->xstart, sig_ptr->signame); */
	
	/* calculate the y location to draw the signal name and draw it */
	yhigh = trace->ystart + numprt * global->sighgt;
	ylow = yhigh + global->sighgt - Y_SIG_SPACE;
	ymdpt = (yhigh + ylow)/2;

	fprintf (psfile,"%d ", ymdpt);
	print_signame_scrolled (trace, psfile, sig_ptr);
	fprintf (psfile," SIGNAME\n");

	numprt++;
    }
}

static void print_draw_cursors (
    Trace	*trace,
    FILE	*psfile,
    DTime	printtime)	/* Time to start on */
{
    int		end_time;
    int		last_drawn_xright;
    char 	strg[MAXTIMELEN];		/* String value to print out */
    DCursor 	*csr_ptr;			/* Current cursor being printed */
    Position	x1;
    Position	ytop,ybot,ydelta;
    Dimension m_time_height = global->time_font->ascent;
    
    if (!global->cursor_vis) return;

    /* initial the y values for drawing */
    ytop = trace->ystart - Y_CURSOR_TOP;
    ybot = trace->ycursortimeabs - m_time_height - Y_TEXT_SPACE;
    ydelta = trace->ycursortimerel - m_time_height/2;
    last_drawn_xright = -1;
    end_time = printtime + TIME_WIDTH (trace);

    /* Start cursor, for now == start grid */
    fprintf (psfile,"\n%d %d %d START_GRID\n", ytop, ybot, trace->ycursortimeabs);

    for (csr_ptr = global->cursor_head; csr_ptr; csr_ptr = csr_ptr->next) {
	/* check if cursor is on the screen */
	if (csr_ptr->time > end_time) break;
	if (csr_ptr->time >= printtime) {

	    /* draw the vertical cursor line */
	    x1 = TIME_TO_XPOS_REL (csr_ptr->time, printtime);

	    /* draw the cursor value */
	    time_to_string (trace, strg, csr_ptr->time, FALSE);
	    fprintf (psfile,"%d (%s) CSR%c\n", x1, strg,
		     ((csr_ptr->type==USER)?'U':'A')
		     );
		
	    /* if there is a previous visible cursor, draw delta line */
	    if ( csr_ptr->prev && (csr_ptr->prev->time > printtime) ) {
		Position x2;

		x2 = TIME_TO_XPOS_REL (csr_ptr->prev->time, printtime);
		time_to_string (trace, strg, csr_ptr->time - csr_ptr->prev->time, TRUE);
		fprintf (psfile,"%d %d %d (%s) CURSOR_DELTA\n", x1, x2, ydelta, strg);
	    }
	}
    }
}

void    print_internal (Trace *trace)
{
    FILE	*psfile=NULL;
    int		sigs_per_page;
    DTime	time_per_page;
    int		horiz_pages;		/* Number of pages to print the time on (horizontal) */
    int		vert_pages;		/* Number of pages to print signal names on (vertical) */
    int		horiz_page, vert_page;
    char	*timeunits;
    int		encapsulated;
    Signal	*sig_ptr, *sig_end_ptr=NULL;
    uint_t	numprt;
    DTime	printtime;	/* Time current page starts on */
    char	pagenum[20];
    char	sstrg[MAXTIMELEN];
    char	estrg[MAXTIMELEN];
    
    if (DTPRINT_ENTRY) printf ("In print_internal - trace=%p\n",trace);
    if (!trace->loaded) return;
    
    if (trace->printname) {
	/* Open the file */
	if (DTPRINT_ENTRY) printf ("Filename=%s\n", trace->printname);
	psfile = fopen (trace->printname,"w");
    }
    else {
	if (DTPRINT_ENTRY) printf ("Null filename\n");
	psfile = NULL;
    }

    if (psfile == NULL) {
	sprintf (message,"Bad Filename: %s\n", trace->printname);
	dino_error_ack (trace,message);
	return;
    }
    
    draw_update ();

    set_cursor (DC_BUSY);
    XSync (global->display,0);
    
    /* calculate time per page */
    time_per_page = TIME_WIDTH (trace);
    sigs_per_page = trace->numsigvis;
    
    /* Reset stuff if doing all times */
    /* calculate number of pages needed to draw the entire trace */
    horiz_pages = (global->print_end_time - global->print_begin_time)/time_per_page;
    if ( (global->print_end_time - global->print_begin_time) % time_per_page )
	horiz_pages++;

    /* Reset stuff if doing all signals */
    vert_pages = 1;
    if (global->print_all_signals) {
	/* calculate number of pages needed to draw the entire trace */
	vert_pages = (int)((trace->numsig) / sigs_per_page);
	if ( (trace->numsig) % sigs_per_page )
	    vert_pages++;
    }
    
    /* encapsulated? */
    encapsulated = (global->print_size==PRINTSIZE_EPSPORT)
	|| (global->print_size==PRINTSIZE_EPSLAND);
    
    /* File header information */
    fprintf (psfile, "%%!PS-Adobe-1.0\n");
    fprintf (psfile, "%%%%Title: %s\n", trace->printname);
    fprintf (psfile, "%%%%Creator: %s %sPostscript\n", DTVERSION,
	     encapsulated ? "Encapsulated ":"");
    fprintf (psfile, "%%%%CreationDate: %s\n", date_string(0));
    fprintf (psfile, "%%%%Pages: %d\n", encapsulated ? 0 : horiz_pages * vert_pages );
    /* Took page size, subtracted 50 to loose title information */
    if (encapsulated) {
	if (global->print_size==PRINTSIZE_EPSLAND)
	    fprintf (psfile, "%%%%BoundingBox: 0 0 569 792\n");
	else fprintf (psfile, "%%%%BoundingBox: 0 0 792 569\n");
    }
    fprintf (psfile, "%%%%EndComments\n");
    fprintf (psfile, "\n");
    if (encapsulated) fprintf (psfile,"save\n");
    
    /* include the postscript macro information */
    fputs (dt_post, psfile);
    
    /* Grab units */
    timeunits = time_units_to_string (global->timerep, FALSE);

    /* output the page scaling and rf time */
    fprintf (psfile,"\n%d %d %d %d %d %d PAGESCALE\n",
	     trace->height, trace->width, global->xstart, global->sigrf,
	     (int) ( ( (global->print_size==PRINTSIZE_B) ? 11.0 :  8.5) * 72.0),
	     (int) ( ( (global->print_size==PRINTSIZE_B) ? 16.8 : 10.8) * 72.0)
	     );
	
    /* Signal to start on */
    if (vert_pages > 1) {
	sig_ptr = trace->firstsig;
    }
    else {
	sig_ptr = trace->dispsig;
    }

    for (numprt = 0; sig_ptr && ( numprt<trace->numsigvis || (vert_pages>1));
	 sig_ptr = sig_ptr->forward, numprt++) {
	print_signame_scrolled (trace, psfile, sig_ptr);
	fprintf (psfile," SIGMARGIN\n");
    }
    fprintf (psfile,"XSCALESET\n\n");

    /* print out each page */
    for (horiz_page=0; horiz_page<horiz_pages; horiz_page++) {
	
	/* Time at left edge of printout */
	printtime = time_per_page * horiz_page + global->print_begin_time;

	/* Signal to start on */
	if (vert_pages > 1) {
	    sig_ptr = trace->firstsig;
	}
	else {
	    sig_ptr = trace->dispsig;
	}

	for (vert_page=0; vert_page<vert_pages; vert_page++) {
	    fprintf (psfile,"%% Beginning of page %d - %d\n", horiz_page, vert_page);

	    if (vert_page > 0) {
		/* move pointer forward to next signal set */
		sig_ptr = sig_end_ptr;
	    }

	    /* Find last signal to print on this page */
	    if (! sig_ptr) break;
	    for (sig_end_ptr = sig_ptr, numprt = 0; sig_end_ptr && numprt<trace->numsigvis;
		 sig_end_ptr = sig_end_ptr->forward, numprt++) ;

	    /******* NEW PAGE */
	    if (vert_pages == 1) {
		if (horiz_pages == 1) 	pagenum[0]='\0';	/* Just 1 page */
		else			sprintf (pagenum, "Page %d", horiz_page+1);
	    }
	    else {
		if (horiz_pages == 1) 	sprintf (pagenum, "Page %d", vert_page+1);
		else			sprintf (pagenum, "Page %d-%d", horiz_page+1, vert_page+1);
	    }
	    
	    /* decode times */
	    time_to_string (trace, sstrg, printtime, FALSE);
	    time_to_string (trace, estrg, printtime + time_per_page, FALSE);

	    /* output the page header macro */
	    fprintf (psfile, "(%s-%s %s) (%d %s/page) (%s) (%s) (%s) (%s) (%s) %s\n",
		     sstrg,			/* start time */
		     estrg,			/* end time */
		     timeunits,
		     time_per_page,		/* resolution */
		     timeunits,
		     date_string(0),		/* time & date */
		     global->printnote,		/* filenote */
		     trace->filename,		/* filename */
		     pagenum,			/* page number */
		     DTVERSION,			/* version (for title) */
		     ( (global->print_size==PRINTSIZE_EPSPORT)
		      ? "EPSPHDR" :
		      ( (global->print_size==PRINTSIZE_EPSLAND)
		       ? "EPSLHDR" : "PAGEHDR"))	/* which macro to use */
		     );
	    
	    /* draw the signal names and the traces */
	    print_draw_sig (trace, psfile, sig_ptr, sig_end_ptr);
	    print_draw_val (trace, psfile, sig_ptr, sig_end_ptr, printtime);
	    print_draw_grids (trace, psfile, printtime);
	    print_draw_cursors (trace, psfile, printtime);
	    
	    /* print the page */
	    if (encapsulated)
		fprintf (psfile,"stroke\nrestore\n");
	    else fprintf (psfile,"stroke\nshowpage\n");

	} /* vert page */
    } /* horiz page */
    
    if (!encapsulated) {
	fprintf (psfile,"\n%%Trailer\n");
	fprintf (psfile,"%%EOF\n");
    }

    /* close output file */
    fclose (psfile);
    new_time (trace);
    set_cursor (DC_NORMAL);
}

/****************************** MENU OPTIONS ******************************/

void    print_range_sensitives_cb (
    Widget		w,
    RangeWidgets_t	*range_ptr,	/* <<<< NOTE not Trace!!! */
    XmSelectionBoxCallbackStruct *cb)
{
    int		opt;
    int		active;
    char	strg[MAXTIMELEN];
    Trace	*trace;

    if (DTPRINT_ENTRY) printf ("In print_range_sensitives_cb\n");

    trace = range_ptr->trace;	/* Snarf from range, as the callback doesn't have it */

    opt = option_to_number(range_ptr->time_option, range_ptr->time_pulldownbutton, 3);
    
    switch (opt) {
    case 3:	/* Window */
	active = FALSE;
	range_ptr->dastime = (range_ptr->type == BEGIN) ? global->time 
	    :   (global->time + TIME_WIDTH (trace));
	break;
    case 2:	/* Trace */ 
	active = FALSE;
	range_ptr->dastime = (range_ptr->type == BEGIN) ? trace->start_time : trace->end_time;
	break;
    case 1:	/* Cursor */
	active = FALSE;
	range_ptr->dastime = (range_ptr->type == BEGIN) ? cur_time_first(trace) : cur_time_last(trace);
	break;
    default:	/* Manual */
	active = TRUE;
	break;
    }

    XtSetArg (arglist[0], XmNsensitive, active);
    XtSetValues (range_ptr->time_text, arglist, 1);
    if (!active) {
	time_to_string (trace, strg, range_ptr->dastime, TRUE);
	XmTextSetString (range_ptr->time_text, strg);
    }
    else {
	range_ptr->dastime = string_to_time (range_ptr->trace, XmTextGetString (range_ptr->time_text));
    }
}


static void    print_range_create (
    Trace		*trace,
    RangeWidgets_t	*range_ptr,
    Widget		above,		/* Upper widget for form attachment */
    char		*descrip,	/* Description of selection */
    Boolean_t		type)		/* True if END, else beginning */
{
    if (DTPRINT_ENTRY) printf ("In print_create_range - trace=%p\n",trace);

    if (!range_ptr->trace) {
	range_ptr->trace = trace;
	range_ptr->type = type;

	/* Label */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple (descrip) );
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopOffset, 10);
	XtSetArg (arglist[4], XmNtopWidget, above);
	range_ptr->time_label = XmCreateLabel (trace->print.dialog,"",arglist,5);
	DManageChild (range_ptr->time_label, trace, MC_NOKEYS);
	
	/* Begin pulldown */
	range_ptr->time_pulldown = XmCreatePulldownMenu (trace->print.dialog,"time_pulldown",arglist,0);

	if (range_ptr->type == BEGIN)
	    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Window Left Edge") );
	else XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Window Right Edge") );
	range_ptr->time_pulldownbutton[3] =
	    XmCreatePushButtonGadget (range_ptr->time_pulldown,"pdbutton0",arglist,1);
	DAddCallback (range_ptr->time_pulldownbutton[3], XmNactivateCallback, (XtCallbackProc)print_range_sensitives_cb, range_ptr);
	DManageChild (range_ptr->time_pulldownbutton[3], trace, MC_NOKEYS);

	if (range_ptr->type == BEGIN)
	    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Trace Beginning") );
	else XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Trace End") );
	range_ptr->time_pulldownbutton[2] =
	    XmCreatePushButtonGadget (range_ptr->time_pulldown,"pdbutton0",arglist,1);
	DAddCallback (range_ptr->time_pulldownbutton[2], XmNactivateCallback, (XtCallbackProc)print_range_sensitives_cb, range_ptr);
	DManageChild (range_ptr->time_pulldownbutton[2], trace, MC_NOKEYS);

	if (range_ptr->type == BEGIN)
	    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("First Cursor") );
	else XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Last Cursor") );
	range_ptr->time_pulldownbutton[1] =
	    XmCreatePushButtonGadget (range_ptr->time_pulldown,"pdbutton0",arglist,1);
	DAddCallback (range_ptr->time_pulldownbutton[1], XmNactivateCallback, (XtCallbackProc)print_range_sensitives_cb, range_ptr);
	DManageChild (range_ptr->time_pulldownbutton[1], trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Entered Time") );
	range_ptr->time_pulldownbutton[0] =
	    XmCreatePushButtonGadget (range_ptr->time_pulldown,"pdbutton0",arglist,1);
	DAddCallback (range_ptr->time_pulldownbutton[0], XmNactivateCallback, (XtCallbackProc)print_range_sensitives_cb, range_ptr);
	DManageChild (range_ptr->time_pulldownbutton[0], trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNsubMenuId, range_ptr->time_pulldown);
	XtSetArg (arglist[1], XmNx, 20);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopOffset, 0);
	XtSetArg (arglist[4], XmNtopWidget, range_ptr->time_label);
	range_ptr->time_option = XmCreateOptionMenu (trace->print.dialog,"options",arglist,5);
	DManageChild (range_ptr->time_option, trace, MC_NOKEYS);

	/* Default */
	XtSetArg (arglist[0], XmNmenuHistory, range_ptr->time_pulldownbutton[3]);
	XtSetValues (range_ptr->time_option, arglist, 1);

	/* Begin Text */
	XtSetArg (arglist[0], XmNrows, 1);
	XtSetArg (arglist[1], XmNcolumns, 10);
	XtSetArg (arglist[2], XmNleftAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNleftWidget, range_ptr->time_option );
	XtSetArg (arglist[4], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[5], XmNtopOffset, 0);
	XtSetArg (arglist[6], XmNtopWidget, range_ptr->time_label);
	XtSetArg (arglist[7], XmNresizeHeight, FALSE);
	XtSetArg (arglist[8], XmNeditMode, XmSINGLE_LINE_EDIT);
	range_ptr->time_text = XmCreateText (trace->print.dialog,"textn",arglist,9);
	DManageChild (range_ptr->time_text, trace, MC_NOKEYS);
    }

    /* Get initial values correct */
    print_range_sensitives_cb (NULL, range_ptr, NULL);
}


DTime	print_range_value (
    RangeWidgets_t	*range_ptr)
    /* Read the range value */
{
    /* Make sure have latest cursor, etc */
    print_range_sensitives_cb (NULL, range_ptr, NULL);

    return (range_ptr->dastime);
}


void    print_dialog_cb (
    Widget		w)
{
    Trace *trace = widget_to_trace(w);
    if (DTPRINT_ENTRY) printf ("In print_screen - trace=%p\n",trace);
    
    if (!trace->print.dialog) {
	XtSetArg (arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg (arglist[1], XmNdialogTitle, XmStringCreateSimple ("Print Screen Menu"));
	XtSetArg (arglist[2], XmNverticalSpacing, 10);
	XtSetArg (arglist[3], XmNhorizontalSpacing, 10);
	trace->print.dialog = XmCreateFormDialog (trace->work, "print",arglist,3);
	
	/* create label widget for text widget */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("File Name") );
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_FORM );
	XtSetArg (arglist[3], XmNtopOffset, 5);
	trace->print.label = XmCreateLabel (trace->print.dialog,"",arglist,4);
	DManageChild (trace->print.label, trace, MC_NOKEYS);
	
	/* create the file name text widget */
	XtSetArg (arglist[0], XmNrows, 1);
	XtSetArg (arglist[1], XmNcolumns, 30);
	XtSetArg (arglist[2], XmNx, 10);
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNtopWidget, trace->print.label);
	XtSetArg (arglist[5], XmNtopOffset, 0);
	XtSetArg (arglist[6], XmNresizeHeight, FALSE);
	XtSetArg (arglist[7], XmNeditMode, XmSINGLE_LINE_EDIT);
	trace->print.text = XmCreateText (trace->print.dialog,"",arglist,8);
	DManageChild (trace->print.text, trace, MC_NOKEYS);
	DAddCallback (trace->print.text, XmNactivateCallback, print_req_cb, trace);
	
	/* create label widget for notetext widget */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Note") );
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopOffset, 10);
	XtSetArg (arglist[4], XmNtopWidget, trace->print.text);
	trace->print.label = XmCreateLabel (trace->print.dialog,"",arglist,5);
	DManageChild (trace->print.label, trace, MC_NOKEYS);
	
	/* create the print note text widget */
	XtSetArg (arglist[0], XmNrows, 1);
	XtSetArg (arglist[1], XmNcolumns, 30);
	XtSetArg (arglist[2], XmNx, 10);
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNtopWidget, trace->print.label);
	XtSetArg (arglist[5], XmNtopOffset, 0);
	XtSetArg (arglist[6], XmNresizeHeight, FALSE);
	XtSetArg (arglist[7], XmNeditMode, XmSINGLE_LINE_EDIT);
	trace->print.notetext = XmCreateText (trace->print.dialog,"notetext",arglist,8);
	DAddCallback (trace->print.notetext, XmNactivateCallback, print_req_cb, trace);
	DManageChild (trace->print.notetext, trace, MC_NOKEYS);
	
	/* Create radio box for page size */
	trace->print.size_menu = XmCreatePulldownMenu (trace->print.dialog,"size",arglist,0);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("A-Sized"));
	trace->print.sizea = XmCreatePushButtonGadget (trace->print.size_menu,"sizea",arglist,1);
	DManageChild (trace->print.sizea, trace, MC_NOKEYS);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("B-Sized"));
	trace->print.sizeb = XmCreatePushButtonGadget (trace->print.size_menu,"sizeb",arglist,1);
	DManageChild (trace->print.sizeb, trace, MC_NOKEYS);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("EPS Portrait"));
	trace->print.sizeep = XmCreatePushButtonGadget (trace->print.size_menu,"sizeep",arglist,1);
	DManageChild (trace->print.sizeep, trace, MC_NOKEYS);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("EPS Landscape"));
	trace->print.sizeel = XmCreatePushButtonGadget (trace->print.size_menu,"sizeel",arglist,1);
	DManageChild (trace->print.sizeel, trace, MC_NOKEYS);
	
	XtSetArg (arglist[0], XmNx, 10);
	XtSetArg (arglist[1], XmNlabelString, XmStringCreateSimple ("Layout"));
	XtSetArg (arglist[2], XmNsubMenuId, trace->print.size_menu);
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNtopOffset, 10);
	XtSetArg (arglist[5], XmNtopWidget, trace->print.notetext);
	trace->print.size_option = XmCreateOptionMenu (trace->print.dialog,"sizeo",arglist,6);
	DManageChild (trace->print.size_option, trace, MC_NOKEYS);

	/* Create all_signals button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Include off-screen signals"));
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNshadowThickness, 1);
	XtSetArg (arglist[3], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[4], XmNtopOffset, 10);
	XtSetArg (arglist[5], XmNtopWidget, trace->print.size_option);
	trace->print.all_signals = XmCreateToggleButton (trace->print.dialog,
							   "all_signals",arglist,6);
	DManageChild (trace->print.all_signals, trace, MC_NOKEYS);

	print_range_create (trace, &(trace->print.begin_range),
			 trace->print.all_signals, "Begin Printing at:", 0);

	print_range_create (trace, &(trace->print.end_range),
			 trace->print.begin_range.time_text, "End Printing at:", 1);

	/* Ok/apply/cancel */
	ok_apply_cancel (&trace->print.okapply, trace->print.dialog,
			 dmanage_last,
			 (XtCallbackProc)print_req_cb, trace,
			 NULL, NULL,
			 (XtCallbackProc)print_reset_cb, trace,
			 (XtCallbackProc)unmanage_cb, (Trace*)trace->print.dialog);
	}
    
    /* reset page size */
    switch (global->print_size) {
      default:
      case PRINTSIZE_A:
	XtSetArg (arglist[0], XmNmenuHistory, trace->print.sizea);
	break;
      case PRINTSIZE_B:
	XtSetArg (arglist[0], XmNmenuHistory, trace->print.sizeb);
	break;
      case PRINTSIZE_EPSPORT:
	XtSetArg (arglist[0], XmNmenuHistory, trace->print.sizeep);
	break;
      case PRINTSIZE_EPSLAND:
	XtSetArg (arglist[0], XmNmenuHistory, trace->print.sizeel);
	break;
    }
    XtSetValues (trace->print.size_option, arglist, 1);

    /* reset flags */
    XtSetArg (arglist[0], XmNset, global->print_all_signals ? 1:0);
    XtSetValues (trace->print.all_signals,arglist,1);

    /* reset file name */
    XtSetArg (arglist[0], XmNvalue, trace->printname);
    XtSetValues (trace->print.text,arglist,1);
    
    /* reset file note */
    XtSetArg (arglist[0], XmNvalue, global->printnote);
    XtSetValues (trace->print.notetext,arglist,1);
    
    /* if a file has been read in, make printscreen buttons active */
    XtSetArg (arglist[0],XmNsensitive, (trace->loaded)?TRUE:FALSE);
    XtSetValues (trace->print.okapply.ok,arglist,1);
    
    /* manage the popup on the screen */
    DManageChild (trace->print.dialog, trace, MC_NOKEYS);
}

void    print_reset_cb (
    Widget		w)
{
    Trace *trace = widget_to_trace(w);
    print_reset (trace);
    XtUnmanageChild (trace->print.dialog);
    print_dialog_cb (w);
}

void    print_direct_cb (
    Widget		w)
{
    Trace *trace = widget_to_trace(w);
    if (!trace->print.dialog) {
	print_dialog_cb (w);
    } else {
	print_internal (trace);
    }
}

void    print_req_cb (
    Widget		w)
{
    Trace *trace = widget_to_trace(w);
    Widget	clicked;
    
    if (DTPRINT_ENTRY) printf ("In print_req_cb - trace=%p\n",trace);
    
    XtSetArg (arglist[0], XmNmenuHistory, &clicked);
    XtGetValues (trace->print.size_option, arglist, 1);
    if (clicked == trace->print.sizeep)
	global->print_size = PRINTSIZE_EPSPORT;
    else if (clicked == trace->print.sizeel)
	global->print_size = PRINTSIZE_EPSLAND;
    else if (clicked == trace->print.sizeb)
	global->print_size = PRINTSIZE_B;
    else global->print_size = PRINTSIZE_A;

    global->print_all_signals = XmToggleButtonGetState (trace->print.all_signals);

    /* ranges */
    global->print_begin_time = print_range_value ( &(trace->print.begin_range) );
    global->print_end_time = print_range_value ( &(trace->print.end_range) );

    /* get note */
    strcpy (global->printnote, XmTextGetString (trace->print.notetext));

    /* open output file */
    strcpy (trace->printname, XmTextGetString (trace->print.text));

    /* hide the print screen window */
    XtUnmanageChild (trace->print.dialog);
    
    print_internal (trace);
}
    

