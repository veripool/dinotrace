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


#include <X11/Xlib.h>
#include <Xm/Xm.h>

#include "dinotrace.h"
#include "callbacks.h"

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


void draw(trace)                                 
    TRACE	*trace;                        
{         
    int c=0,i,cnt,adj,ymdpt,yt,xloc,xend,du,len,mid,yfntloc;
    int	x1,x2,y1,y2;
    int numprt;				/* Number of signals printed out on screen */
    int srch_this_color;		/* Color to print signal if matches search value */
    XPoint Pts[6000];			/* Array of points to plot */
    float iff,xlocf,xtimf;
    char strg[MAXSTATELEN+16];		/* String value to print out */
    register SIGNAL_LW *cptr,*nptr;	/* Current value pointer and next value pointer */
    SIGNAL_SB *sig_ptr;			/* Current signal being printed */
    unsigned int value;
    int temp_color=0;
    char temp_strg[20];
    
    if (DTPRINT) printf("In draw - filename=%s\n",trace->filename);
    
    /* don't draw anything if no file is loaded */
    if (!trace->loaded) return;
    /* check for all signals being deleted */
    if (trace->dispsig == NULL) return;
    
    /* calculate the font y location */
    yfntloc = trace->text_font->max_bounds.ascent + trace->text_font->max_bounds.descent;
    yfntloc = (trace->sighgt - yfntloc - SIG_SPACE)/2;
    
    xend = trace->width - XMARGIN;
    adj = global->time * global->res - global->xstart;
    
    if (DTPRINT) printf("global->res=%f adj=%d\n",global->res,adj);
    
    /* Loop and draw each signal individually */
    for (sig_ptr = trace->dispsig, numprt = 0; sig_ptr && numprt<trace->numsigvis;
	 sig_ptr = sig_ptr->forward, numprt++) {

	/* Grab the color we want */
	XSetForeground (global->display, trace->gc, trace->xcolornums[sig_ptr->color]);

	/*
	if (temp_color > 9) temp_color = 0;
	XSetForeground (global->display, trace->gc, trace->xcolornums[temp_color++]);
	sprintf (temp_strg, "%d", temp_color);
	XSetForeground (global->display, trace->gc, temp_color++);
	*/

	/* calculate the location to draw the signal name and draw it */
	x1 = global->xstart - XTextWidth(trace->text_font,sig_ptr->signame,
				       strlen(sig_ptr->signame)) - 10;
	y1 = trace->ystart + (c+1) * trace->sighgt - SIG_SPACE - yfntloc;
	XDrawString(global->display, trace->wind, trace->gc, x1, y1,
		    sig_ptr->signame, strlen(sig_ptr->signame) );

	/*
	XDrawString(global->display, trace->wind, trace->gc, x1, y1,
		    temp_strg, strlen(temp_strg) );
	*/

	/* Calculate line position */
	y1 = trace->ystart + c * trace->sighgt + SIG_SPACE;
	ymdpt = y1 + (int)(trace->sighgt/2) - SIG_SPACE;
	y2 = y1 + trace->sighgt - 2*SIG_SPACE;
	cptr = (SIGNAL_LW *)sig_ptr->cptr;
	cnt = 0;
	c++;
	
	/* Compute starting points for signal */
	Pts[cnt].x = global->xstart - trace->sigrf;
	switch( cptr->state ) {
	  case STATE_0: Pts[cnt].y = y2; break;
	  case STATE_1: Pts[cnt].y = y1; break;
	  case STATE_U: Pts[cnt].y = ymdpt; break;
	  case STATE_Z: Pts[cnt].y = ymdpt; break;
	  case STATE_B32: Pts[cnt].y = ymdpt; break;
	  case STATE_B64: Pts[cnt].y = ymdpt; break;
	  case STATE_B96: Pts[cnt].y = ymdpt; break;
	  default: printf("Error: State=%d\n",cptr->state); break;
	    }
	
	/* Loop as long as the time and end of trace are in current screen */
	while ( cptr->time != EOT && Pts[cnt].x < xend && cnt < 5000) {

	    /* find the next transition */
	    nptr = cptr + sig_ptr->inc;
	    
	    /* if next transition is the end, don't draw */
	    if (nptr->time == EOT) break;
	    
	    /* find the x location for the end of this segment */
	    xloc = nptr->time * global->res - adj;
	    
	    /* Determine what the state of the signal is and build transition */
	    switch( cptr->state ) {
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
		
		value = *((unsigned int *)cptr+1);
		
		/* Below evaluation left to right important to prevent error */
		if ( (sig_ptr->decode != NULL) &&
		    (value<MAXSTATENAMES) &&
		    (sig_ptr->decode->statename[value][0] != '\0')) {
		    strcpy (strg, sig_ptr->decode->statename[value]);
		    len = XTextWidth(trace->text_font,strg,strlen(strg));
		    if ( xloc-Pts[cnt].x < len + 2 ) {
			/* doesn't fit, try number */
			goto value_rep;
			}
		    }
		else {
		  value_rep:	    if (trace->busrep == HBUS)
		      sprintf(strg,"%X", value);
		  else if (trace->busrep == OBUS)
		      sprintf(strg,"%o", value);
		    }
		
		srch_this_color = 0;
		if (sig_ptr->srch_ena) {
		    for (i=0; i<MAX_SRCH; i++) {
			if ( ( global->srch_value[i][2]==value ) ) {
			    srch_this_color = global->srch_color[i];
			    break;
			    }
			}
		    }

		goto state_plot;

	      case STATE_B64:
		if ( xloc > xend ) xloc = xend;

		if (trace->busrep == HBUS)
		    sprintf(strg,"%X %08X",*((unsigned int *)cptr+1),
			    *((unsigned int *)cptr+2));
		else if (trace->busrep == OBUS)
		    sprintf(strg,"%o %o",*((unsigned int *)cptr+1),
			    *((unsigned int *)cptr+2));
		
		srch_this_color = 0;
		if (sig_ptr->srch_ena) {
		    for (i=0; i<MAX_SRCH; i++) {
			if ( ( global->srch_value[i][2]== *((unsigned int *)cptr+2) )
			    && ( global->srch_value[i][1]== *((unsigned int *)cptr+1) ) ) {
			    srch_this_color = global->srch_color[i];
			    break;
			    }
			}
		    }

		goto state_plot;
		
	      case STATE_B96:
		if ( xloc > xend ) xloc = xend;

		if (trace->busrep == HBUS)
		    sprintf(strg,"%X %08X %08X",*((unsigned int *)cptr+1),
			    *((unsigned int *)cptr+2),
			    *((unsigned int *)cptr+3));
		else if (trace->busrep == OBUS)
		    sprintf(strg,"%o %o %o",*((unsigned int *)cptr+1),
			    *((unsigned int *)cptr+2),
			    *((unsigned int *)cptr+3));
		
		srch_this_color = 0;
		if (sig_ptr->srch_ena) {
		    for (i=0; i<MAX_SRCH; i++) {
			if ( ( global->srch_value[i][2]== *((unsigned int *)cptr+3) )
			    && ( global->srch_value[i][1]== *((unsigned int *)cptr+2) )
			    && ( global->srch_value[i][0]== *((unsigned int *)cptr+1) ) ) {
			    srch_this_color = global->srch_color[i];
			    break;
			    }
			}
		    }

		goto state_plot;
		

		/***** COMMON section of all 3 STATE_* encodings *****/
	      state_plot:

		/* calculate positional parameters */
		len = XTextWidth(trace->text_font,strg,strlen(strg));
		
		/* write the bus value if it fits */
		if ( xloc-Pts[cnt].x >= len + 2 ) {
		    mid = Pts[cnt].x + (int)( (xloc-Pts[cnt].x)/2 );
		    if (srch_this_color) {
			/* Grab the color we want */
			XSetForeground (global->display, trace->gc, trace->xcolornums[srch_this_color]);
			XDrawString(XtDisplay(trace->toplevel), XtWindow( trace->work),
				    trace->gc, mid-len/2, y2-yfntloc, strg, strlen(strg) );
			XSetForeground (global->display, trace->gc, trace->xcolornums[sig_ptr->color]);
			}
		    else {
			XDrawString(XtDisplay(trace->toplevel), XtWindow( trace->work),
				    trace->gc, mid-len/2, y2-yfntloc, strg, strlen(strg) );
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
		printf("Error: State=%d\n",cptr->state); break;
		} /* end switch */
	    
	    cnt += 2;
	    cptr += sig_ptr->inc;
	    }
        cnt++;
	
	/* draw the lines */
	XDrawLines(global->display, trace->wind, trace->gc, Pts, cnt, CoordModeOrigin);

	} /* end of FOR */
    
    /* Back to default color */
    XSetForeground (global->display, trace->gc, trace->xcolornums[0]);

    /*** draw the time line and the grid if its visible ***/
    
    /* calculate the starting window pixel location of the first time */
    xlocf = (trace->grid_align + (global->time-trace->grid_align)
	     /trace->grid_res*trace->grid_res)*global->res - adj;
    
    /* calculate the starting time */
    xtimf = (xlocf + (float)adj)/global->res + .001;
    
    /* initialize some parameters */
    i = 0;
    x1 = (int)xtimf;
    yt = 20;
    y1 = trace->ystart - SIG_SPACE/4;
    y2 = trace->height - trace->sighgt;
    
    /* create the dash pattern for the vertical grid lines */
    strg[0] = SIG_SPACE/2;
    strg[1] = trace->sighgt - strg[0];
    
    /* set the line attributes as the specified dash pattern */
    XSetLineAttributes(global->display,trace->gc,0,LineOnOffDash,0,0);
    XSetDashes(global->display,trace->gc,0,strg,2);
    
    /* check if there is a reasonable amount of increments to draw the time grid */
    if ( ((float)xend - xlocf)/(trace->grid_res*global->res) < MIN_GRID_RES )
	{
        for (iff=xlocf;iff<(float)xend; iff+=trace->grid_res*global->res)
	    {
	    /* compute the time value and draw it if it fits */
	    sprintf(strg,"%d",x1);
	    if ( (int)iff - i >= XTextWidth(trace->text_font,strg,strlen(strg)) + 5 )
		{
	        XDrawString(global->display,trace->wind,
			    trace->gc, (int)iff, yt, strg, strlen(strg));
	        i = (int)iff;
		}
	    
	    /* if the grid is visible, draw a vertical dashed line */
	    if (trace->grid_vis)
		{
	        XDrawLine(global->display, trace->wind,trace->gc,(int)iff,y1,(int)iff,y2);
		}
	    x1 += trace->grid_res;
	    }
	}
    else
	{
	/* grid res is useless - must increase the spacing */
	/*	dino_warning_ack(trace, "Grid Spacing Too Small - Increase Res"); */
	}
    
    /* reset the line attributes */
    XSetLineAttributes(global->display,trace->gc,0,LineSolid,0,0);
    
    /* draw the cursors if they are visible */
    if ( trace->cursor_vis )
	{
	/* initial the y colors for drawing */
	y1 = 25;
	y2 = ( (int)((trace->height-trace->ystart)/trace->sighgt) - 1 ) *
	    trace->sighgt + trace->sighgt/2 + trace->ystart + 2;

	for (i=0; i < global->numcursors; i++) {
	    /* check if cursor is on the screen */
	    if (global->cursors[i] > global->time) {

		/* Change color */
		XSetForeground (global->display, trace->gc,
				trace->xcolornums[global->cursor_color[i]]);

		/* draw the cursor */
		x1 = global->cursors[i] * global->res - adj;
		XDrawLine(global->display,trace->wind,trace->gc,x1,y1,x1,y2);
		
		/* draw the cursor value */
		sprintf(strg,"%d",global->cursors[i]);
 		len = XTextWidth(trace->text_font,strg,strlen(strg));
		XDrawString(global->display,trace->wind,
			    trace->gc, x1-len/2, y2+10, strg, strlen(strg));
		
		/* if there is a previous visible cursor, draw delta line */
		if ( i != 0 && global->cursors[i-1] > global->time )
		    {
		    x2 = global->cursors[i-1] * global->res - adj;
		    sprintf(strg,"%d",global->cursors[i] - global->cursors[i-1]);
 		    len = XTextWidth(trace->text_font,strg,strlen(strg));
		    
		    /* write the delta value if it fits */
 		    if ( x1 - x2 >= len + 6 )
			{
			/* calculate the mid pt of the segment */
			mid = x2 + (x1 - x2)/2;
			XDrawLine(global->display, trace->wind, trace->gc,
				  x2, y2-5, mid-len/2-2, y2-5);
			XDrawLine(global->display, trace->wind, trace->gc,
				  mid+len/2+2, y2-5, x1, y2-5);
			
			XDrawString(global->display,trace->wind,
				    trace->gc, mid-len/2, y2, strg, strlen(strg));
			}
 		    /* or just draw the delta line */
 		    else
			{
 		        XDrawLine(global->display, trace->wind,
				  trace->gc, x1, y2-5, x2, y2-5);
			}
		    }
		}
	    }
	}
    
    /* Back to default color */
    XSetForeground (global->display, trace->gc, trace->xcolornums[0]);

    } /* End of DRAW */


void	update_globals ()
{
    TRACE *trace;
    SIGNAL_SB *sig_ptr;
    int xstarttemp;
    char *t1;

    /* Calculate xstart from longest signal name */
    xstarttemp=XSTART_MIN;
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	if (trace->loaded) {
	    for (sig_ptr = (SIGNAL_SB *)trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
		for (t1=sig_ptr->signame; *t1==' '; t1++);
		if (strncmp (t1, "%NET.",5)==0) t1+=5;
		/* if (DTPRINT) printf("Signal = '%s'  xstart=%d\n",t1,xstarttemp); */
		if (xstarttemp < XTextWidth(trace->text_font,t1,strlen(t1)))
		    xstarttemp = XTextWidth(trace->text_font,t1,strlen(t1));
		}
	    }
	}
    global->xstart = xstarttemp + XSTART_MARGIN;
    }


void redraw_all (trace)
    TRACE		*trace;
{
    if (DTPRINT) printf("In new_grid - trace=%d\n",trace);

    /* do not unroll this loop, as it will make the refresh across windows */
    /* appear out of sync */
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	get_geometry (trace);
	}
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	XClearWindow (global->display,trace->wind);
	}
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	draw (trace);
	}
    }


/**********************************************************************
 *	update_search
 **********************************************************************/

void	update_search ()
{
    TRACE	*trace;
    SIGNAL_SB	*sig_ptr;
    SIGNAL_LW	*cptr;
    int		found;
    int		i;

    if (DTPRINT) printf("In update_search\n");

    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	/* don't do anything if no file is loaded */
	if (!trace->loaded) continue;
	
	for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	    if (sig_ptr->inc == 1) {
		/* Single bit signal, don't search for values */
		continue;
		}
	    
	    found=0;
	    cptr = (SIGNAL_LW *)(sig_ptr)->bptr;
	    
	    for (; !found && (cptr->time != EOT); cptr += sig_ptr->inc) {
		switch (cptr->state) {
		  case STATE_B32:
		    for (i=0; i<MAX_SRCH; i++) {
			if ( ( global->srch_value[i][2]== *((unsigned int *)cptr+1) )
			    && ( global->srch_value[i][1] == 0) 
			    && ( global->srch_value[i][0] == 0)
			    && ( global->srch_color[i] != 0) ) {
			    found=1;
			    break;
			    }
			}
		    break;
		    
		  case STATE_B64:
		    for (i=0; i<MAX_SRCH; i++) {
			if ( ( global->srch_value[i][2]== *((unsigned int *)cptr+2) )
			    && ( global->srch_value[i][1]== *((unsigned int *)cptr+1) )
			    && ( global->srch_value[i][0] == 0) 
			    && ( global->srch_color[i] != 0) ) {
			    found=1;
			    break;
			    }
			}
		    break;
		    
		  case STATE_B96:
		    for (i=0; i<MAX_SRCH; i++) {
			if ( ( global->srch_value[i][2]== *((unsigned int *)cptr+3) )
			    && ( global->srch_value[i][1]== *((unsigned int *)cptr+2) )
			    && ( global->srch_value[i][0]== *((unsigned int *)cptr+1) )
			    && ( global->srch_color[i] != 0) ) {
			    found=1;
			    break;
			    }
			}
		    break;
		    } /* switch */
		} /* for cptr */
	    
	    sig_ptr->srch_ena = found;
	    if (found && DTPRINT) printf ("Signal %s matches search string.\n", sig_ptr->signame);
	    
	    } /* for sig */
	} /* for trace */
    }

