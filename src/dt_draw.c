/******************************************************************************
 *
 * Filename:
 *     Draw.c
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
 *     This module draws the data on the screen
 *
 * Modification History:
 *     AAG	 5-Jul-89	Original Version
 *     AAG	22-Aug-90	Base Level V4.1
 *     AAG	 6-Nov-90	Upped Pts[] to 5000 and added check in case
 *				that limit is exceeded
 *     AAG	29-Apr-91	Use X11, fixed casts for Ultrix support
 *     WPS	01-Jan-93	V5.0 signal_states support
 *     WPS	15-Feb-93	V5.2 print number if state doesn't fit
 */
static char rcsid[] = "$Id$";

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <X11/Xlib.h>
#include <Xm/Xm.h>
#include <Xm/ScrollBar.h>
#include <Xm/ScrollBarP.h>

#include "dinotrace.h"
#include "callbacks.h"

#define MAXCNT	4000		/* Maximum number of segment pairs to draw before stopping this signal */
#define OVERLAPSPACE 2		/* Number of pixels within which two signal transitions are visualized as one */
/* Don't make this above 4000, as some servers choke with that many points. */

extern void draw_hscroll (),
    draw_vscroll ();

void draw_grid (trace, grid_ptr)
    TRACE	*trace;           
    GRID	*grid_ptr;		/* Grid information */
{         
    char 	strg[MAXSTATELEN+16];	/* String value to print out */
    char 	primary_dash[4];	/* Dash pattern */
    int		end_time;
    int		time_width;		/* Width of time printing */
    DTime	xtime;
    int		x_last_time;		/* Last x coordinate time was printed at */
    Position	x2;			/* Coordinate of current time */
    Position	y1,y2;			/* Starting and ending points */

    if (grid_ptr->period < 1) grid_ptr->period = 1;	/* Prevents round-down to 0 causing infinite loop */

    /* Is the grid too small or invisible?  If so, skip it */
    if ((! grid_ptr->visible) || ((grid_ptr->period * global->res) < MIN_GRID_RES)) {
	return;
    }

    /* create the dash pattern for the vertical grid lines */
    primary_dash[0] = PDASH_HEIGHT;
    primary_dash[1] = trace->sighgt - primary_dash[0];
    
    /* set the line attributes as the specified dash pattern */
    XSetLineAttributes (global->display, trace->gc, 0, LineOnOffDash, 0, 0);
    XSetDashes (global->display, trace->gc, 0, primary_dash, 2);
    XSetFont (global->display, trace->gc, global->time_font->fid);

    /* Start to left of right edge */
    xtime = global->time;
    end_time = global->time + (( trace->width - XMARGIN - global->xstart ) / global->res);

    /* Move starting point to the right to hit where a grid line is aligned */
    xtime = ((xtime / grid_ptr->period) * grid_ptr->period) + (grid_ptr->alignment % grid_ptr->period);

    /* If possible, put one grid line inside the signal names */
    if (((grid_ptr->period * global->res) < global->xstart)
	&& (xtime >= grid_ptr->period)) {
	xtime -= grid_ptr->period;
    }

    /* Other coordinates */
    y1 = trace->ystart - SIG_SPACE/4;
    y2 = trace->height - trace->sighgt;

    /* Loop through grid times */
    for ( ; xtime <= end_time; xtime += grid_ptr->period) {
	x2 = TIME_TO_XPOS (xtime);
	XSetForeground (global->display, trace->gc, trace->xcolornums[grid_ptr->color]);

	/* compute the time value and draw it if it fits */
	time_to_string (trace, strg, xtime, FALSE);
	time_width = XTextWidth (global->time_font,strg,strlen (strg));
	if ( (x2 - x_last_time) >= (time_width+5) ) {
	    /* Draw if space, centered on grid line */
	    XDrawString (global->display,trace->wind, trace->gc, x2 - time_width/2, GRID_TIME_Y, strg, strlen (strg));
	    x_last_time = x2;
	}
	    
	/* if the grid is visible, draw a vertical dashed line */
	XSetLineAttributes (global->display, trace->gc, 0, LineOnOffDash, 0, 0);
	XDrawLine     (global->display, trace->wind,trace->gc,   x2, y1,   x2, y2);
	if (grid_ptr->wide_line) {
	    XDrawLine (global->display, trace->wind,trace->gc, 1+x2, y1, 1+x2, y2);
	}

	XSetLineAttributes (global->display,trace->gc,0,LineSolid,0,0);
	XDrawLine     (global->display, trace->wind,trace->gc, x2,   y1,   x2, GRID_TIME_Y+2);
	if (grid_ptr->wide_line) {
	    XDrawLine (global->display, trace->wind,trace->gc, 1+x2, y1, 1+x2, GRID_TIME_Y+2);
	}
    }

    /* Back to default color */
    XSetForeground (global->display, trace->gc, trace->xcolornums[0]);

    /* reset the line attributes */
    XSetLineAttributes (global->display,trace->gc,0,LineSolid,0,0);
    }

void draw_cursors (trace)
    TRACE	*trace;                        
{         
    int		len,end_time;
    char 	strg[MAXSTATELEN+16];		/* String value to print out */
    CURSOR 	*csr_ptr;			/* Current cursor being printed */
    Position	x1,mid,x2,y2;
    char 	nonuser_dash[2];		/* Dashed line for nonuser cursors */

    nonuser_dash[0]=2;	nonuser_dash[1]=2;	/* Can't auto-init in ultrix compiler */

    XSetFont (global->display, trace->gc, global->time_font->fid);

    end_time = global->time + (( trace->width - XMARGIN - global->xstart ) / global->res);

    /* initial the y colors for drawing */
    y2 = ( (int)((trace->height-trace->ystart)/trace->sighgt) - 1 ) *
	trace->sighgt + trace->sighgt/2 + trace->ystart + 2;
    
    for (csr_ptr = global->cursor_head; csr_ptr; csr_ptr = csr_ptr->next) {
	
	/* check if cursor is on the screen */
	if ((csr_ptr->time >= global->time) && (csr_ptr->time <= end_time)) {
	    
	    /* Change color */
	    XSetForeground (global->display, trace->gc,
			    trace->xcolornums[csr_ptr->color]);
	    if (csr_ptr->type==USER) {
		XSetLineAttributes (global->display,trace->gc,0,LineSolid,0,0);
		}
	    else {
		XSetLineAttributes (global->display, trace->gc, 0, LineOnOffDash, 0,0);
		XSetDashes (global->display, trace->gc, 0, nonuser_dash, 2);
		}
	    
	    /* draw the cursor */
	    x1 = TIME_TO_XPOS (csr_ptr->time);
	    XDrawLine (global->display,trace->wind,trace->gc,x1,25,x1,y2);
	    
	    /* draw the cursor value */
	    time_to_string (trace, strg, csr_ptr->time, FALSE);
	    len = XTextWidth (global->time_font,strg,strlen (strg));
	    XDrawString (global->display,trace->wind,
			 trace->gc, x1-len/2, y2+10, strg, strlen (strg));
	    
	    /* if there is a previous visible cursor, draw delta line */
	    if ( csr_ptr->prev && (csr_ptr->prev->time > global->time) ) {
		
		x2 = TIME_TO_XPOS (csr_ptr->prev->time);
		time_to_string (trace, strg, csr_ptr->time - csr_ptr->prev->time, TRUE);
		len = XTextWidth (global->time_font,strg,strlen (strg));
		
		/* write the delta value if it fits */
		XSetLineAttributes (global->display,trace->gc,0,LineSolid,0,0);
		if ( x1 - x2 >= len + 6 ) {
		    /* calculate the mid pt of the segment */
		    mid = x2 + (x1 - x2)/2;
		    XDrawLine (global->display, trace->wind, trace->gc,
			       x2, y2-5, mid-len/2-2, y2-5);
		    XDrawLine (global->display, trace->wind, trace->gc,
			       mid+len/2+2, y2-5, x1, y2-5);
		    
		    XDrawString (global->display,trace->wind,
				 trace->gc, mid-len/2, y2, strg, strlen (strg));
		    }
		/* or just draw the delta line */
		else {
		    XDrawLine (global->display, trace->wind,
			       trace->gc, x2, y2-5, x1, y2-5);
		    }
		}
	    }
	}
    /* Reset */
    XSetLineAttributes (global->display,trace->gc,0,LineSolid,0,0);
    }

/*
 *
 * Basic DRAW Algorithm:
 *
 * 1) Select Start Point
 * 2) WHILE next_state != EOS and current_location < end_of_screen DO
 *     2a) Draw Transition
 *     2b) Draw State to next_state
 *
 *****************************************************************************
 *
 *
 *
 *      _____________A
 *                   ^\  1) draw from previous pt A to
 *                   | \    pt B at new 'y' for 'transition'
 *                   |  \
 *                   |   \
 *                   |    \               2) draw to new pt C keeping
 *                   |     \                 'y' level constant
 *                   |      B_________________________________________C
 *                   |      ^                                         ^
 *                   |      |                                         |
 *                   |   (Pts[cnt+1].x+trace->sigrf,y2)             (xloc,y2)
 *                   |
 *        (Pts[cnt].x,Pts[cnt].y)
 *
 *
 *                                              /
 *                                             /
 *                   _________________________/
 *                ^         ^
 *                |         |
 *                |     SIG_SPACE
 *                |         |
 *                |         v
 *                |   _________________\
 *                |                     \
 *                |                      \
 *           trace->sighgt                \
 *                |                        \
 *                |                         \
 *                |                          \
 *                |                           \_____________________
 *                |          ^
 *                |          |
 *                |       SIG_SPACE
 *                |          |
 *                v          v
 *              _______________________\
 *                                      \
 *                                       \
 *
 *
 */                          


void draw_trace (trace)                                 
    TRACE	*trace;                        
{         
    int c=0,i,cnt,adj,ymdpt,xloc,xend,du,len,mid,yfntloc;
    int last_drawn_xloc;
    unsigned int last_drawn_state=EOT;
    int	x1,y1,y2;
    int numprt;				/* Number of signals printed out on screen */
    int srch_this_color;		/* Color to print signal if matches search value */
    XPoint Pts[MAXCNT+100];			/* Array of points to plot */
    char strg[MAXSTATELEN+16];		/* String value to print out */
    register SIGNAL_LW *cptr,*nptr;	/* Current value pointer and next value pointer */
    SIGNAL *sig_ptr;			/* Current signal being printed */
    unsigned int value;
    /* int temp_color=0; */
    /* char temp_strg[20]; */
    int end_time;
    int question_width;			/* Width of '?' character */
    int	grid_num;
    
    if (DTPRINT_ENTRY) printf ("In draw_trace\n");
    
    /* don't draw anything if no file is loaded */
    if (!trace->loaded) return;
    /* check for all signals being deleted */
    if (trace->dispsig == NULL) return;
    
    question_width = XTextWidth (global->value_font,"?",1);

    /* calculate the font y location */
    yfntloc = global->value_font->max_bounds.ascent + global->value_font->max_bounds.descent;
    yfntloc = (trace->sighgt - yfntloc - SIG_SPACE)/2;
    
    xend = trace->width - XMARGIN;
    adj = global->time * global->res - global->xstart;
    end_time = global->time + (( trace->width - XMARGIN - global->xstart ) / global->res);
    
    if (DTPRINT_DRAW) printf ("global->res=%f adj=%d time=%d-%d\n",global->res,adj,
			     global->time, end_time);
    
    /* Loop and draw each signal individually */
    for (sig_ptr = trace->dispsig, numprt = 0; sig_ptr && numprt<trace->numsigvis;
	 sig_ptr = sig_ptr->forward, numprt++) {

	/* Print the green bars */
	if ( (c & 1) ^ (trace->numsigstart & 1) ) {
	    y1 = trace->ystart + c * trace->sighgt + SIG_SPACE;
	    y2 = y1 + trace->sighgt - 2*SIG_SPACE;

	    XSetForeground (global->display, trace->gc, trace->barcolornum);
	    XFillRectangle (global->display, trace->wind, trace->gc,
			    0, y1, trace->width - XMARGIN, y2-y1);
	    }
	
	/* Grab the color we want */
	XSetFont (global->display, trace->gc, global->signal_font->fid);
	XSetForeground (global->display, trace->gc, trace->xcolornums[sig_ptr->color]);

	/*
	if (temp_color > 9) temp_color = 0;
	XSetForeground (global->display, trace->gc, trace->xcolornums[temp_color++]);
	sprintf (temp_strg, "%d", temp_color);
	XSetForeground (global->display, trace->gc, temp_color++);
	*/

	/* calculate the location to draw the signal name and draw it */
	x1 = global->xstart - XTextWidth (global->signal_font,sig_ptr->signame,
				       strlen (sig_ptr->signame)) - 10;
	y1 = trace->ystart + (c+1) * trace->sighgt - SIG_SPACE - yfntloc;
	XDrawString (global->display, trace->wind, trace->gc, x1, y1,
		    sig_ptr->signame, strlen (sig_ptr->signame) );

	/*
	XDrawString (global->display, trace->wind, trace->gc, x1, y1,
		    temp_strg, strlen (temp_strg) );
	*/

	XSetFont (global->display, trace->gc, global->value_font->fid);

	/* Calculate line position */
	y1 = trace->ystart + c * trace->sighgt + SIG_SPACE;
	ymdpt = y1 + (int)(trace->sighgt/2) - SIG_SPACE;
	y2 = y1 + trace->sighgt - 2*SIG_SPACE;
	cptr = (SIGNAL_LW *)sig_ptr->cptr;
	cnt = 0;
	c++;
	
	/* Compute starting points for signal */
	Pts[cnt].x = global->xstart - trace->sigrf;
	last_drawn_xloc = -1;
	switch ( cptr->sttime.state ) {
	  case STATE_0: Pts[cnt].y = y2; break;
	  case STATE_1: Pts[cnt].y = y1; break;
	  case STATE_U: Pts[cnt].y = ymdpt; break;
	  case STATE_Z: Pts[cnt].y = ymdpt; break;
	  case STATE_B32: Pts[cnt].y = ymdpt; break;
	  case STATE_B64: Pts[cnt].y = ymdpt; break;
	  case STATE_B96: Pts[cnt].y = ymdpt; break;
	  default: printf ("Error: State=%d\n",cptr->sttime.state); break;
	    }
	
	/* Loop as long as the time and end of trace are in current screen */
	while ( cptr->sttime.time != EOT && Pts[cnt].x < xend && cnt < MAXCNT) {

	    /* find the next transition */
	    nptr = cptr + sig_ptr->lws;
	    
	    /* if next transition is the end, don't draw */
	    if (nptr->sttime.time == EOT) break;
	    
	    /* find the x location for the end of this segment */
	    xloc = TIME_TO_XPOS (nptr->sttime.time);
	    
	    /* printf ("L %07x\t%d < %d < %d\n", cptr, last_drawn_xloc+OVERLAPSPACE, xloc, xend); */
	    if ( (last_drawn_state==(cptr->sttime.state)) && ((last_drawn_xloc+OVERLAPSPACE) > xloc)
		&& (xloc < xend-OVERLAPSPACE) ) {
		/* Too close to previously drawn vector.  User won't see the difference */
		/* printf ("\tskip\n"); */
		goto next_state;
		}
	    else {
		last_drawn_xloc = xloc;
		last_drawn_state = cptr->sttime.state;
		}
	    
	    /* Determine what the state of the signal is and build transition */
	    switch ( cptr->sttime.state ) {
	      case STATE_0:
		if ( xloc > xend ) xloc = xend;
		Pts[cnt+1].x = Pts[cnt].x+trace->sigrf; Pts[cnt+1].y = y2;
		Pts[cnt+2].x = xloc;                  Pts[cnt+2].y = y2;
		break;
		
	      case STATE_1:
		if ( xloc > xend ) xloc = xend;
		Pts[cnt+1].x = Pts[cnt].x+trace->sigrf; Pts[cnt+1].y = y1;
		Pts[cnt+2].x = xloc;                  Pts[cnt+2].y = y1;
		break;
		
	      case STATE_U:
		Pts[cnt+1].x=Pts[cnt].x+trace->sigrf; Pts[cnt+1].y=ymdpt;
		cnt++;
		if ( xloc > xend ) xloc = xend;
		if ( xloc - Pts[cnt].x < DELU2)
		    {
		    du = xloc - Pts[cnt].x;
		    Pts[cnt+1].x=Pts[cnt].x+du/2;  Pts[cnt+1].y = y2;
		    Pts[cnt+2].x=Pts[cnt].x+du; Pts[cnt+2].y = ymdpt;
		    Pts[cnt+3].x=Pts[cnt].x+du/2;  Pts[cnt+3].y = y1;
		    Pts[cnt+4].x=Pts[cnt].x;       Pts[cnt+4].y = ymdpt;
		    Pts[cnt+5].x=Pts[cnt].x+du/2;  Pts[cnt+5].y = y1;
		    Pts[cnt+6].x=Pts[cnt].x+du; Pts[cnt+6].y = ymdpt;
		    cnt += 6;
		    }
		else
		    {
		    while ( Pts[cnt].x < xloc - DELU )
			{
                        Pts[cnt+1].x=Pts[cnt].x+DELU;  Pts[cnt+1].y = y2;
                        Pts[cnt+2].x=Pts[cnt].x+DELU2; Pts[cnt+2].y = ymdpt;
                        Pts[cnt+3].x=Pts[cnt].x+DELU;  Pts[cnt+3].y = y1;
                        Pts[cnt+4].x=Pts[cnt].x;       Pts[cnt+4].y = ymdpt;
                        Pts[cnt+5].x=Pts[cnt].x+DELU;  Pts[cnt+5].y = y1;
                        Pts[cnt+6].x=Pts[cnt].x+DELU2; Pts[cnt+6].y = ymdpt;
                        cnt += 6;
			}
		    }
		Pts[cnt].x = Pts[cnt-4].x = xloc;
		cnt -= 2;
		break;
		
	      case STATE_Z:
		if ( xloc > xend ) xloc = xend;
		Pts[cnt+1].x = Pts[cnt].x+trace->sigrf; Pts[cnt+1].y = ymdpt;
		Pts[cnt+2].x = xloc;                  Pts[cnt+2].y = ymdpt;
		break;
		
	      case STATE_B32:
		if ( xloc > xend ) xloc = xend;
		
		value = * ((unsigned int *)cptr+1);
		
		/* Below evaluation left to right important to prevent error */
		if ( (sig_ptr->decode != NULL) &&
		    (value < sig_ptr->decode->numstates) &&
		    (sig_ptr->decode->statename[value][0] != '\0')) {
		    strcpy (strg, sig_ptr->decode->statename[value]);
		    len = XTextWidth (global->value_font,strg,strlen (strg));
		    if ( xloc-Pts[cnt].x < len + 2 ) {
			/* doesn't fit, try number */
			goto value_rep;
			}
		    }
		else {
		  value_rep:	    if (trace->busrep == HBUS)
		      sprintf (strg,"%x", value);
		  else if (trace->busrep == OBUS)
		      sprintf (strg,"%o", value);
		    }
		
		srch_this_color = 0;
		if (sig_ptr->srch_ena) {
		    for (i=0; i<MAX_SRCH; i++) {
			if ( global->val_srch[i].value[0]==value
			    && global->val_srch[i].color ) {
			    srch_this_color = global->val_srch[i].color;
			    break;
			    }
			}
		    }

		goto state_plot;

	      case STATE_B64:
		if ( xloc > xend ) xloc = xend;

		if (trace->busrep == HBUS)
		    sprintf (strg,"%x %08x",*((unsigned int *)cptr+2),
			    *((unsigned int *)cptr+1));
		else if (trace->busrep == OBUS)
		    sprintf (strg,"%o %o",*((unsigned int *)cptr+2),
			    *((unsigned int *)cptr+1));
		
		srch_this_color = 0;
		if (sig_ptr->srch_ena) {
		    for (i=0; i<MAX_SRCH; i++) {
			if ( ( global->val_srch[i].value[0]== *((unsigned int *)cptr+1) )
			    && ( global->val_srch[i].value[1]== *((unsigned int *)cptr+2) )
			    && global->val_srch[i].color ) {
			    srch_this_color = global->val_srch[i].color;
			    break;
			    }
			}
		    }

		goto state_plot;
		
	      case STATE_B96:
		if ( xloc > xend ) xloc = xend;

		if (trace->busrep == HBUS)
		    sprintf (strg,"%x %08x %08x",*((unsigned int *)cptr+3),
			    *((unsigned int *)cptr+2),
			    *((unsigned int *)cptr+1));
		else if (trace->busrep == OBUS)
		    sprintf (strg,"%o %o %o",*((unsigned int *)cptr+3),
			    *((unsigned int *)cptr+2),
			    *((unsigned int *)cptr+1));
		
		srch_this_color = 0;
		if (sig_ptr->srch_ena) {
		    for (i=0; i<MAX_SRCH; i++) {
			if ( ( global->val_srch[i].value[2]== *((unsigned int *)cptr+3) )
			    && ( global->val_srch[i].value[1]== *((unsigned int *)cptr+2) )
			    && ( global->val_srch[i].value[0]== *((unsigned int *)cptr+1) )
			    && global->val_srch[i].color ) {
			    srch_this_color = global->val_srch[i].color;
			    break;
			    }
			}
		    }

		goto state_plot;
		

		/***** COMMON section of all 3 STATE_* encodings *****/
	      state_plot:

		/* calculate positional parameters */

		len = XTextWidth (global->value_font,strg,strlen (strg));
		
		if (xloc-Pts[cnt].x-2 < len) {
		    /* Value won't fit, try question mark */
		    len = question_width;
		    strg[0] = '?';  strg[1] = '\0';
		    }

		/* write the bus value if it fits */
		if ( xloc-Pts[cnt].x-2 >= len ) {
		    mid = Pts[cnt].x + (int)( (xloc-Pts[cnt].x)/2 );
		    if (srch_this_color) {
			/* Grab the color we want */
			XSetForeground (global->display, trace->gc, trace->xcolornums[srch_this_color]);
			XDrawString (global->display, XtWindow ( trace->work),
				    trace->gc, mid-len/2, y2-yfntloc, strg, strlen (strg) );
			XSetForeground (global->display, trace->gc, trace->xcolornums[sig_ptr->color]);
			}
		    else {
			XDrawString (global->display, XtWindow ( trace->work),
				    trace->gc, mid-len/2, y2-yfntloc, strg, strlen (strg) );
			}
		    }

		/* Plot points */
		Pts[cnt+1].x=Pts[cnt].x+trace->sigrf; Pts[cnt+1].y=y2;
		Pts[cnt+2].x=xloc-trace->sigrf;       Pts[cnt+2].y=y2;
		Pts[cnt+3].x=xloc;                  Pts[cnt+3].y=ymdpt;
		Pts[cnt+4].x=xloc-trace->sigrf;       Pts[cnt+4].y=y1;
		Pts[cnt+5].x=Pts[cnt].x+trace->sigrf; Pts[cnt+5].y=y1;
		Pts[cnt+6].x=Pts[cnt].x;            Pts[cnt+6].y=ymdpt;
		Pts[cnt+7].x=Pts[cnt].x+trace->sigrf; Pts[cnt+7].y=y1;
		Pts[cnt+8].x=xloc-trace->sigrf;       Pts[cnt+8].y=y1;
		Pts[cnt+9].x=xloc;                  Pts[cnt+9].y=ymdpt;
		cnt += 7;
		break;
		
	      default:
		printf ("Error: State=%d\n",cptr->sttime.state); break;
		} /* end switch */
	    
	    cnt += 2;

	  next_state:

	    cptr += sig_ptr->lws;
	    }
        cnt++;
	
	/* draw the lines */
	XDrawLines (global->display, trace->wind, trace->gc, Pts, cnt, CoordModeOrigin);
	/*
	for (cnt--; cnt>0; cnt--) {
	    printf ("C%d\tx%d\ty%d\n", cnt, Pts[cnt].x, Pts[cnt].y);
	    }
	    */
	} /* end of FOR */
    
    /* Back to default color */
    XSetForeground (global->display, trace->gc, trace->xcolornums[0]);

    for (grid_num=0; grid_num<MAXGRIDS; grid_num++) {
	draw_grid (trace, &(trace->grid[grid_num]));
    }

    /* draw the cursors if they are visible */
    if ( trace->cursor_vis ) draw_cursors (trace);

    /* Draw the scroll bar */
    draw_hscroll (trace);
    draw_vscroll (trace);
    
    /* Back to default color */
    XSetForeground (global->display, trace->gc, trace->xcolornums[0]);
    } /* End of DRAW */


void	update_globals ()
{
    TRACE *trace;
    SIGNAL *sig_ptr;
    int xstarttemp;
    char *t1;

    if (DTPRINT_ENTRY) printf ("In update_globals\n");

    /* Calculate xstart from longest signal name */
    xstarttemp=XSTART_MIN;
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	if (trace->loaded) {
	    for (sig_ptr = (SIGNAL *)trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
		t1=sig_ptr->signame;
		if (strncmp (t1, "%NET.",5)==0) t1+=5;
		/* if (DTPRINT) printf ("Signal = '%s'  xstart=%d\n",t1,xstarttemp); */
		if (xstarttemp < XTextWidth (global->signal_font,t1,strlen (t1)))
		    xstarttemp = XTextWidth (global->signal_font,t1,strlen (t1));
		}
	    }
	}
    global->xstart = xstarttemp + XSTART_MARGIN;
    }


void draw_perform ()
{
    TRACE		*trace;

    if (DTPRINT_ENTRY) printf ("In draw_perform %d %d\n", global->redraw_needed, global->trace_head->redraw_needed);

    /* do not unroll this loop, as it will make the refresh across windows */
    /* appear out of sync */
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	if ((global->redraw_needed & 2) || (trace->redraw_needed & 1)) {
	    get_geometry (trace);
	    }
	}
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	if ((global->redraw_needed & 2) || (trace->redraw_needed & 1)) {
	    XClearWindow (global->display,trace->wind);
	    }
	}
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	if ((global->redraw_needed & 2) || trace->redraw_needed) {
	    draw_trace (trace);
	    trace->redraw_needed = FALSE;
	    }
	}
    global->redraw_needed = FALSE;
    }


/**********************************************************************
 *	Scroll bar hacks (dependent on Motif internals)
 **********************************************************************/

void	draw_hscroll (trace)
    TRACE *trace;
    /* Draw on the horizontal scroll bar - needs guts of the ScrollBarWidget */
{
    int ymin,ymax,x1,xmin,xmax,slider_xmin,slider_xmax;
    float xscale;
    CURSOR *csr_ptr;			/* Current cursor being printed */
    char 	nonuser_dash[2];		/* Dashed line for nonuser cursors */

    nonuser_dash[0]=2;	nonuser_dash[1]=2;	/* Can't auto-init in ultrix compiler */

    if (!trace->loaded || (trace->end_time == trace->start_time)) return;

    /* initial the y colors for drawing */
    xmin = ((XmScrollBarRec *)trace->hscroll)->scrollBar.slider_area_x;
    xmax = xmin + ((XmScrollBarRec *)trace->hscroll)->scrollBar.slider_area_width;
    ymin = ((XmScrollBarRec *)trace->hscroll)->scrollBar.slider_area_y;
    ymax = ymin + ((XmScrollBarRec *)trace->hscroll)->scrollBar.slider_area_height;
    slider_xmin = ((XmScrollBarRec *)trace->hscroll)->scrollBar.slider_x;
    slider_xmax = slider_xmin + ((XmScrollBarRec *)trace->hscroll)->scrollBar.slider_width;
    xscale =
	(float)((XmScrollBarRec *)trace->hscroll)->scrollBar.slider_area_width	/* sc width */
	/ (float)(trace->end_time - trace->start_time);		/* range of times */

    /* Blank area to either side of slider */
    XSetForeground (global->display, trace->gc,
		    ((XmScrollBarRec *)trace->hscroll)->scrollBar.trough_color );
    XFillRectangle (global->display, XtWindow (trace->hscroll), trace->gc,
		   xmin, ymin, slider_xmin - xmin, ymax-ymin);
    XFillRectangle (global->display, XtWindow (trace->hscroll), trace->gc,
		   slider_xmax, ymin, xmax - slider_xmax , ymax-ymin);

    if ( trace->cursor_vis ) {
	for (csr_ptr = global->cursor_head; csr_ptr; csr_ptr = csr_ptr->next) {
	    /* draw the cursor */
	    x1 = xmin + xscale * (csr_ptr->time - trace->start_time);

	    /* Don't plot if it would overwrite the slider */
	    if ((x1 < slider_xmin) || (x1 > slider_xmax)) {
		/* Change color */
		XSetForeground (global->display, trace->gc, trace->xcolornums[csr_ptr->color]);

		if (csr_ptr->type==USER) {
		    XSetLineAttributes (global->display,trace->gc,0,LineSolid,0,0);
		    }
		else {
		    XSetLineAttributes (global->display, trace->gc, 0, LineOnOffDash, 0,0);
		    XSetDashes (global->display, trace->gc, 0, nonuser_dash, 2);
		    }

		XDrawLine (global->display, XtWindow (trace->hscroll),
			  trace->gc, x1,ymin,x1,ymax);
		}
	    }
	/* Reset */
	XSetLineAttributes (global->display,trace->gc,0,LineSolid,0,0);
	}
    }


void	draw_vscroll (trace)
    TRACE *trace;
    /* Draw on the vertical scroll bar - needs guts of the ScrollBarWidget */
{
    int ymin,ymax,y1,xmin,xmax,slider_ymin,slider_ymax;
    int i;
    float yscale;
    SIGNAL	*sig_ptr;

    if (DTPRINT_ENTRY) printf ("In draw_vscroll\n");

    /* initial the y colors for drawing */
    xmin = ((XmScrollBarRec *)trace->vscroll)->scrollBar.slider_area_x;
    xmax = xmin + ((XmScrollBarRec *)trace->vscroll)->scrollBar.slider_area_width;
    ymin = ((XmScrollBarRec *)trace->vscroll)->scrollBar.slider_area_y;
    ymax = ymin + ((XmScrollBarRec *)trace->vscroll)->scrollBar.slider_area_height;
    slider_ymin = ((XmScrollBarRec *)trace->vscroll)->scrollBar.slider_y;
    slider_ymax = slider_ymin + ((XmScrollBarRec *)trace->vscroll)->scrollBar.slider_height;

    /*
    if (DTPRINT_DRAW) printf (">>X %d - %d   Y %d - %d  Sli %d - %d\n", 
			xmin,xmax,ymin,ymax,slider_ymin,slider_ymax);
			*/

    /* Blank area to either side of slider */
    XSetForeground (global->display, trace->gc,
		    ((XmScrollBarRec *)trace->vscroll)->scrollBar.trough_color );
    XFillRectangle (global->display, XtWindow (trace->vscroll), trace->gc,
		   xmin, ymin, xmax - xmin, slider_ymin - ymin);
    XFillRectangle (global->display, XtWindow (trace->vscroll), trace->gc,
		   xmin, slider_ymax, xmax - xmin, ymax - slider_ymax);

    if (!trace->loaded || !trace->numsig) return;

    yscale =
	(float)((XmScrollBarRec *)trace->vscroll)->scrollBar.slider_area_height	/* sc height */
	    / (float)(trace->numsig);		/* number of signals */

    for (sig_ptr = trace->firstsig, i=0; sig_ptr; sig_ptr = sig_ptr->forward, i++) {
	if (sig_ptr->color && ((i < trace->numsigstart) ||
			       (i >= ( trace->numsigstart + trace->numsigvis)))) {
	    y1 = ymin + yscale * i;

	    /* If plotting it would overwrite the slider move it to one side or the other */
	    if (y1 < slider_ymax && y1 > slider_ymin) {
		if ((y1 - slider_ymin) < ((slider_ymax - slider_ymin ) / 2))
		    y1 = slider_ymin - 1;	/* Adjust up */
		else y1 = slider_ymax + 1;	/* Adjust down */
		}
	    /* Change color */
	    XSetForeground (global->display, trace->gc, trace->xcolornums[sig_ptr->color]);
	    XDrawLine (global->display, XtWindow (trace->vscroll),
		      trace->gc, xmin,y1,xmax,y1);
	    }
	}
    }

