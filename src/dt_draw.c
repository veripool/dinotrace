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
 */


#include <X11/DECwDwtApplProg.h>
#include <X11/Xlib.h>

#include "dinotrace.h"

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
 *                   |   (Pts[cnt+1].x+ptr->sigrf,y2)             (xloc,y2)
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
 *           ptr->sighgt                  \
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
                                           
draw(ptr)                                 
DISPLAY_SB	*ptr;                        
{         
    int c=0,i,j,k=2,d,cnt,adj,ymdpt,inc,xt,yt,xloc,xend,du,len,mid,yfntloc,max_y;
    XPoint Pts[5000];
    float iff,xlocf,xtimf;
    char tmp[MAXSTATELEN+16];
    register SIGNAL_LW *cptr,*nptr;
    SIGNAL_SB *tmp_sig_ptr;
    unsigned int value;

    if (DTPRINT) printf("In draw - filename=%s\n",ptr->filename);

    /* don't draw anything if no file is loaded */
    if (ptr->filename[0] == '\0') return;

    /* calculate the font y location */
    yfntloc = ptr->text_font->max_bounds.ascent + ptr->text_font->max_bounds.descent;
    yfntloc = (ptr->sighgt - yfntloc - SIG_SPACE)/2;

    xend = ptr->width - XMARGIN;
    adj = ptr->time * ptr->res - ptr->xstart;

    if (DTPRINT) printf("ptr->res=%f adj=%d\n",ptr->res,adj);

    /* Loop and draw each signal individually */
    tmp_sig_ptr = (SIGNAL_SB *)ptr->startsig;
    for (i=0;i<ptr->numsigvis;i++)
    {
	/* check for all signals being deleted */
	if (ptr->startsig == NULL) break;

	y1 = ptr->ystart + c * ptr->sighgt + SIG_SPACE;
	ymdpt = y1 + (int)(ptr->sighgt/2) - SIG_SPACE;
	y2 = y1 + ptr->sighgt - 2*SIG_SPACE;
	cptr = (SIGNAL_LW *)tmp_sig_ptr->cptr;
	cnt = 0;
	c++;

	/* Compute starting points for signal */
	Pts[cnt].x = ptr->xstart - ptr->sigrf;
	switch( cptr->state )
	{
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
	while ( cptr->time != EOT && Pts[cnt].x < xend && cnt < 5000)
	{
	    /* find the next transition */
	    nptr = cptr + tmp_sig_ptr->inc;

	    /* if next transition is the end, don't draw */
	    if (nptr->time == EOT) break;

	    /* find the x location for the end of this segment */
	    xloc = nptr->time * ptr->res - adj;

	    /* Determine what the state of the signal is and build transition */
	    switch( cptr->state ) {
	      case STATE_0:
		if ( xloc > xend ) xloc = xend;
		Pts[cnt+1].x = Pts[cnt].x+ptr->sigrf; Pts[cnt+1].y = y2;
		Pts[cnt+2].x = xloc;                  Pts[cnt+2].y = y2;
		break;
		
	      case STATE_1:
		if ( xloc > xend ) xloc = xend;
		Pts[cnt+1].x = Pts[cnt].x+ptr->sigrf; Pts[cnt+1].y = y1;
		Pts[cnt+2].x = xloc;                  Pts[cnt+2].y = y1;
		break;
		
	      case STATE_U:
		Pts[cnt+1].x=Pts[cnt].x+ptr->sigrf; Pts[cnt+1].y=ymdpt;
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
		Pts[cnt+1].x = Pts[cnt].x+ptr->sigrf; Pts[cnt+1].y = ymdpt;
		Pts[cnt+2].x = xloc;                  Pts[cnt+2].y = ymdpt;
		break;
		
	      case STATE_B32:
		if ( xloc > xend ) xloc = xend;
		Pts[cnt+1].x=Pts[cnt].x+ptr->sigrf; Pts[cnt+1].y=y2;
		Pts[cnt+2].x=xloc-ptr->sigrf;       Pts[cnt+2].y=y2;
		Pts[cnt+3].x=xloc;                  Pts[cnt+3].y=ymdpt;
		Pts[cnt+4].x=xloc-ptr->sigrf;       Pts[cnt+4].y=y1;
		Pts[cnt+5].x=Pts[cnt].x+ptr->sigrf; Pts[cnt+5].y=y1;
		Pts[cnt+6].x=Pts[cnt].x;            Pts[cnt+6].y=ymdpt;
		Pts[cnt+7].x=Pts[cnt].x+ptr->sigrf; Pts[cnt+7].y=y1;
		Pts[cnt+8].x=xloc-ptr->sigrf;       Pts[cnt+8].y=y1;
		Pts[cnt+9].x=xloc;                  Pts[cnt+9].y=ymdpt;
		
		value = *((unsigned int *)cptr+1);
		
		/* Below evaluation left to right important to prevent error */
		if ( (tmp_sig_ptr->decode != NULL) &&
		    (value>=0 && value<MAXSTATENAMES) &&
		    (tmp_sig_ptr->decode->statename[value][0] != '\0')) {
		    strcpy (tmp, tmp_sig_ptr->decode->statename[value]);
		    }
		else {
		    if (ptr->busrep == HBUS)
			sprintf(tmp,"%X", value);
		    else if (ptr->busrep == OBUS)
			sprintf(tmp,"%o", value);
		    }
		
		/* calculate positional parameters */
		mid = Pts[cnt].x + (int)( (xloc-Pts[cnt].x)/2 );
		len = XTextWidth(ptr->text_font,tmp,strlen(tmp));
		
		/* write the bus value if it fits */
		if ( xloc-Pts[cnt].x >= len + 2 )
		    {
		    XDrawString(XtDisplay(toplevel), XtWindow( ptr->work),
				ptr->gc, mid-len/2, y2-yfntloc, tmp, strlen(tmp) );
		    }
		cnt += 7;
		break;
		
	      case STATE_B64:
		if ( xloc > xend ) xloc = xend;
		Pts[cnt+1].x=Pts[cnt].x+ptr->sigrf; Pts[cnt+1].y=y2;
		Pts[cnt+2].x=xloc-ptr->sigrf;       Pts[cnt+2].y=y2;
		Pts[cnt+3].x=xloc;                  Pts[cnt+3].y=ymdpt;
		Pts[cnt+4].x=xloc-ptr->sigrf;       Pts[cnt+4].y=y1;
		Pts[cnt+5].x=Pts[cnt].x+ptr->sigrf; Pts[cnt+5].y=y1;
		Pts[cnt+6].x=Pts[cnt].x;            Pts[cnt+6].y=ymdpt;
		Pts[cnt+7].x=Pts[cnt].x+ptr->sigrf; Pts[cnt+7].y=y1;
		Pts[cnt+8].x=xloc-ptr->sigrf;       Pts[cnt+8].y=y1;
		Pts[cnt+9].x=xloc;                  Pts[cnt+9].y=ymdpt;
		if (ptr->busrep == HBUS)
		    sprintf(tmp,"%X %08X",*((unsigned int *)cptr+1),
			    *((unsigned int *)cptr+2));
		else if (ptr->busrep == OBUS)
		    sprintf(tmp,"%o %o",*((unsigned int *)cptr+1),
			    *((unsigned int *)cptr+2));
		
		/* calculate positional parameters */
		mid = Pts[cnt].x + (int)( (xloc-Pts[cnt].x)/2 );
		len = XTextWidth(ptr->text_font,tmp,strlen(tmp));
		
		/* write the bus value if it fits */
		if ( xloc-Pts[cnt].x >= len + 2 )
		    {
		    XDrawString(XtDisplay(toplevel), XtWindow( ptr->work),
				ptr->gc, mid-len/2, y2-yfntloc, tmp, strlen(tmp) );
		    }
		cnt += 7;
		break;
		
	      case STATE_B96:
		if ( xloc > xend ) xloc = xend;
		Pts[cnt+1].x=Pts[cnt].x+ptr->sigrf; Pts[cnt+1].y=y2;
		Pts[cnt+2].x=xloc-ptr->sigrf;       Pts[cnt+2].y=y2;
		Pts[cnt+3].x=xloc;                  Pts[cnt+3].y=ymdpt;
		Pts[cnt+4].x=xloc-ptr->sigrf;       Pts[cnt+4].y=y1;
		Pts[cnt+5].x=Pts[cnt].x+ptr->sigrf; Pts[cnt+5].y=y1;
		Pts[cnt+6].x=Pts[cnt].x;            Pts[cnt+6].y=ymdpt;
		Pts[cnt+7].x=Pts[cnt].x+ptr->sigrf; Pts[cnt+7].y=y1;
		Pts[cnt+8].x=xloc-ptr->sigrf;       Pts[cnt+8].y=y1;
		Pts[cnt+9].x=xloc;                  Pts[cnt+9].y=ymdpt;
		if (ptr->busrep == HBUS)
		    sprintf(tmp,"%X %08X %08X",*((unsigned int *)cptr+1),
			    *((unsigned int *)cptr+2),
			    *((unsigned int *)cptr+2));
		else if (ptr->busrep == OBUS)
		    sprintf(tmp,"%o %o %o",*((unsigned int *)cptr+1),
			    *((unsigned int *)cptr+2),
			    *((unsigned int *)cptr+2));
		
		/* calculate positional parameters */
		mid = Pts[cnt].x + (int)( (xloc-Pts[cnt].x)/2 );
		len = XTextWidth(ptr->text_font,tmp,strlen(tmp));
		
		/* write the bus value if it fits */
		if ( xloc-Pts[cnt].x >= len + 2 )
		    {
		    XDrawString(XtDisplay(toplevel), XtWindow( ptr->work),
				ptr->gc, mid-len/2, y2-yfntloc, tmp, strlen(tmp) );
		    }
		cnt += 7;
		break;
		
	      default:
		printf("Error: State=%d\n",cptr->state); break;
		} /* end switch */

	    cnt += 2;
	    cptr += tmp_sig_ptr->inc;
	}
        cnt++;

	/* draw the lines */
	XDrawLines(ptr->disp, ptr->wind, ptr->gc, Pts, cnt, CoordModeOrigin);

	/* get out of loop if ptr->numsigvis > signals left */
	if (tmp_sig_ptr->forward == NULL) break;

	tmp_sig_ptr = (SIGNAL_SB *)tmp_sig_ptr->forward;

    } /* end of FOR */

    /*** draw the time line and the grid if its visible ***/

    /* calculate the starting window pixel location of the first time */
    xlocf = (ptr->grid_align + (ptr->time-ptr->grid_align)
	/ptr->grid_res*ptr->grid_res)*ptr->res - adj;

    /* calculate the starting time */
    xtimf = (xlocf + (float)adj)/ptr->res + .001;

    /* initialize some parameters */
    i = 0;
    x1 = (int)xtimf;
    yt = 20;
    y1 = ptr->ystart - SIG_SPACE/4;
    y2 = ptr->height - ptr->sighgt;

    /* create the dash pattern for the vertical grid lines */
    tmp[0] = SIG_SPACE/2;
    tmp[1] = ptr->sighgt - tmp[0];

    /* set the line attributes as the specified dash pattern */
    XSetLineAttributes(ptr->disp,ptr->gc,0,LineOnOffDash,0,0);
    XSetDashes(ptr->disp,ptr->gc,0,tmp,2);

    /* check if there is a reasonable amount of increments to draw the time grid */
    if ( ((float)xend - xlocf)/(ptr->grid_res*ptr->res) < MIN_GRID_RES )
    {
        for (iff=xlocf;iff<(float)xend; iff+=ptr->grid_res*ptr->res)
        {
	    /* compute the time value and draw it if it fits */
	    sprintf(tmp,"%d",x1);
	    if ( (int)iff - i >= XTextWidth(ptr->text_font,tmp,strlen(tmp)) + 5 )
	    {
	        XDrawString(ptr->disp,ptr->wind,
	            ptr->gc, (int)iff, yt, tmp, strlen(tmp));
	        i = (int)iff;
	    }

	    /* if the grid is visible, draw a vertical dashed line */
	    if (ptr->grid_vis)
	    {
	        XDrawLine(ptr->disp, ptr->wind,ptr->gc,(int)iff,y1,(int)iff,y2);
	    }
	    x1 += ptr->grid_res;
        }
    }
    else
    {
	/* grid res is useless - must increase the spacing */
/*	dino_message_ack(ptr,"Grid Spacing Too Small - Increase Res"); */
    }

    /* reset the line attributes */
    XSetLineAttributes(ptr->disp,ptr->gc,0,LineSolid,0,0);

    /* draw the cursors if they are visible */
    if ( ptr->cursor_vis )
    {
	/* initial the y values for drawing */
	y1 = 25;
	y2 = ( (int)((ptr->height-ptr->ystart)/ptr->sighgt) - 1 ) *
			ptr->sighgt + ptr->sighgt/2 + ptr->ystart + 2;
	for (i=0; i < ptr->numcursors; i++)
	{
	    /* check if cursor is on the screen */
	    if (ptr->cursors[i] > ptr->time)
	    {
		/* draw the cursor */
		x1 = ptr->cursors[i] * ptr->res - adj;
		XDrawLine(ptr->disp,ptr->wind,ptr->gc,x1,y1,x1,y2);

		/* draw the cursor value */
		sprintf(tmp,"%d",ptr->cursors[i]);
 		len = XTextWidth(ptr->text_font,tmp,strlen(tmp));
		XDrawString(ptr->disp,ptr->wind,
		    ptr->gc, x1-len/2, y2+10, tmp, strlen(tmp));

		/* if there is a previous visible cursor, draw delta line */
		if ( i != 0 && ptr->cursors[i-1] > ptr->time )
		{
		    x2 = ptr->cursors[i-1] * ptr->res - adj;
		    sprintf(tmp,"%d",ptr->cursors[i] - ptr->cursors[i-1]);
 		    len = XTextWidth(ptr->text_font,tmp,strlen(tmp));

		    /* write the delta value if it fits */
 		    if ( x1 - x2 >= len + 6 )
		    {
			/* calculate the mid pt of the segment */
			mid = x2 + (x1 - x2)/2;
			XDrawLine(ptr->disp, ptr->wind, ptr->gc,
			    x2, y2-5, mid-len/2-2, y2-5);
			XDrawLine(ptr->disp, ptr->wind, ptr->gc,
			    mid+len/2+2, y2-5, x1, y2-5);

			XDrawString(ptr->disp,ptr->wind,
			    ptr->gc, mid-len/2, y2, tmp, strlen(tmp));
		    }
 		    /* or just draw the delta line */
 		    else
		    {
 		        XDrawLine(ptr->disp, ptr->wind,
			    ptr->gc, x1, y2-5, x2, y2-5);
		    }
		}
	    }
	}
    }

} /* End of DRAW */

void
drawsig( ptr )
DISPLAY_SB	*ptr;
{
    SIGNAL_SB *tmp_sig_ptr;
    int c=1,i,yfntloc;

/* don't draw anything if there is no file is loaded */

    if (ptr->filename[0] == '\0') return;

/* check for all signals being deleted */
    if (ptr->startsig == NULL) return;

/* initialize the signal pointer to the first visible one */

    tmp_sig_ptr = (SIGNAL_SB *)ptr->startsig;

/* calculate the fnt location within the envelope */

    yfntloc = ptr->text_font->max_bounds.ascent + ptr->text_font->max_bounds.descent;
    yfntloc = (ptr->sighgt - yfntloc - SIG_SPACE)/2;

/* loop thru all the visible signals */

    for (i=0;i<ptr->numsigvis;i++)
    {
	/* calculate the location to start drawing the signal name */
	x1 = ptr->xstart - XTextWidth(ptr->text_font,tmp_sig_ptr->signame,
		strlen(tmp_sig_ptr->signame)) - 10;

	/* calculate the y location to draw the signal name and draw it */
	y1 = ptr->ystart+c*ptr->sighgt - SIG_SPACE - yfntloc;

	XDrawString(ptr->disp, ptr->wind, ptr->gc, x1, y1,
	    tmp_sig_ptr->signame, strlen(tmp_sig_ptr->signame) );
	c++;

	/* get out of loop if ptr->numsigvis > signals left */
	if (tmp_sig_ptr->forward == NULL) break;

	/* get next signal pointer */
	tmp_sig_ptr = (SIGNAL_SB *)tmp_sig_ptr->forward;
    }
}

