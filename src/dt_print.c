/******************************************************************************
 *
 * Filename:
 *     dt_printscreen.c
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
 *
 * Modification History:
 *     AAG	28-Jul-89	Original Version
 *     AAG	22-Aug-90	Base Level V4.1
 *     AAG	29-Apr-91	Use X11 for Ultrix support
 *     WPS	11-Jan-93	Changed signal margin, scrunched bottom
 *     WPS	19-Mar-93	Conversion to Motif
 *
 */


#include <stdio.h>
#include <stdlib.h>

#ifdef VMS
#include <math.h>
#include <descrip.h>
#endif VMS

#include <X11/Xlib.h>
#include <Xm/Xm.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include <Xm/Text.h>
#include <Xm/BulletinB.h>
#include <Xm/Scale.h>

#include "dinotrace.h"
#include "callbacks.h"
#include "dinopost.h"



void    ps_dialog(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
    
{
    if (DTPRINT) printf("In print_screen - trace=%d\n",trace);
    
    if (!trace->prntscr.customize)
	{
	XtSetArg(arglist[0],XmNdefaultPosition, TRUE);
	XtSetArg(arglist[1],XmNwidth, 300);
	XtSetArg(arglist[2],XmNheight, 150);
	XtSetArg(arglist[3],XmNdialogTitle, XmStringCreateSimple("Print Screen Menu"));
	trace->prntscr.customize = XmCreateBulletinBoardDialog(trace->work, "print",arglist,4);
	
	/* create label widget for text widget */
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("File Name") );
	XtSetArg(arglist[1], XmNx, 10);
	XtSetArg(arglist[2], XmNy, 3);
	trace->prntscr.label = XmCreateLabel(trace->prntscr.customize,"",arglist,3);
	XtManageChild(trace->prntscr.label);
	
	/* create the file name text widget */
	XtSetArg(arglist[0], XmNrows, 1);
	XtSetArg(arglist[1], XmNcolumns, 30);
	XtSetArg(arglist[2], XmNx, 10);
	XtSetArg(arglist[3], XmNy, 35);
	XtSetArg(arglist[4], XmNresizeHeight, FALSE);
#ifdef VMS
	XtSetArg(arglist[5], XmNvalue, "sys$login:dinotrace.ps");
#else
	{
	char homestrg[200],*pchar;

	homestrg[0] = '\0';
	if (NULL != (pchar = getenv ("HOME"))) strcpy (homestrg, pchar);
	if (homestrg[0]) strcat (homestrg, "/");
	strcat (homestrg, "dinotrace.ps");
	XtSetArg(arglist[5], XmNvalue, homestrg);
	}
#endif
	XtSetArg(arglist[6], XmNeditMode, XmSINGLE_LINE_EDIT);
	trace->prntscr.text = XmCreateText(trace->prntscr.customize,"",arglist,7);
	XtManageChild(trace->prntscr.text);
	
	/* Create number of pages label */
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("Number of Pages") );
	XtSetArg(arglist[1], XmNx, 20);
	XtSetArg(arglist[2], XmNy, 75);
	trace->prntscr.pagelabel = XmCreateLabel(trace->prntscr.customize,"",arglist,3);
	XtManageChild(trace->prntscr.pagelabel);
	
	/* Create number of pages slider */
	XtSetArg(arglist[0], XmNshowValue, 1);
	XtSetArg(arglist[1], XmNx, 20);
	XtSetArg(arglist[2], XmNy, 95);
	XtSetArg(arglist[3], XmNwidth, 150);
	XtSetArg(arglist[4], XmNvalue, trace->numpag);
	XtSetArg(arglist[5], XmNminimum, 1);
	XtSetArg(arglist[6], XmNmaximum, 50);
	XtSetArg(arglist[7], XmNorientation, XmHORIZONTAL);
	XtSetArg(arglist[8], XmNprocessingDirection, XmMAX_ON_RIGHT);
	trace->prntscr.s1 = XmCreateScale(trace->prntscr.customize,"numpag",arglist,9);
	XtAddCallback(trace->prntscr.s1, XmNvalueChangedCallback, ps_numpag, trace);
	XtManageChild(trace->prntscr.s1);
	
	/* Create radio box for page size */
	XtSetArg(arglist[0], XmNx, 200);
	XtSetArg(arglist[1], XmNy, 75);
	XtSetArg(arglist[2], XmNspacing, 2);
	trace->prntscr.rsize = XmCreateRadioBox(trace->prntscr.customize,"rsize",arglist,3);
	
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("A-Sized"));
	trace->prntscr.rsizea = XmCreateToggleButton(trace->prntscr.rsize,"rsizea",arglist,1);
	XtManageChild(trace->prntscr.rsizea);
	
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("B-Sized"));
	trace->prntscr.rsizeb = XmCreateToggleButton(trace->prntscr.rsize,"tsizeb",arglist,1);
	XtManageChild(trace->prntscr.rsizeb);
	
	XtManageChild(trace->prntscr.rsize);

	/* Create Print button */
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("Print") );
	XtSetArg(arglist[1], XmNx, 10 );
	XtSetArg(arglist[2], XmNy, 145 );
	trace->prntscr.b1 = XmCreatePushButton(trace->prntscr.customize, "print",arglist,3);
	XtAddCallback(trace->prntscr.b1, XmNactivateCallback, ps_print, trace);
	XtManageChild(trace->prntscr.b1);
	
	/* Create Print All button */
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("Print All") );
	XtSetArg(arglist[1], XmNx, 60 );
	XtSetArg(arglist[2], XmNy, 145 );
	trace->prntscr.b2 = XmCreatePushButton(trace->prntscr.customize, "printall",arglist,3);
	XtAddCallback(trace->prntscr.b2, XmNactivateCallback, ps_print_all, trace);
	XtManageChild(trace->prntscr.b2);
	
	/* create cancel button */
	XtSetArg(arglist[0], XmNlabelString, XmStringCreateSimple("Cancel") );
	XtSetArg(arglist[1], XmNx, 135 );
	XtSetArg(arglist[2], XmNy, 145 );
	trace->prntscr.b3 = XmCreatePushButton(trace->prntscr.customize,"cancel",arglist,3);
	XtAddCallback(trace->prntscr.b3, XmNactivateCallback, ps_cancel, trace);
	XtManageChild(trace->prntscr.b3);
	}
    
    /* reset number of pages to one */
    trace->numpag = 1;
    XtSetArg(arglist[0], XmNvalue, trace->numpag);
    XtSetValues(trace->prntscr.s1,arglist,1);
    
    /* reset page size */
    XtSetArg(arglist[0], XmNset, !(global->bsized));
    XtSetValues(trace->prntscr.rsizea,arglist,1);
    XtSetArg(arglist[0], XmNset, (global->bsized));
    XtSetValues(trace->prntscr.rsizeb,arglist,1);

    /* if a file has been read in, make printscreen buttons active */
    XtSetArg(arglist[0],XmNsensitive, (trace->loaded==NULL)?FALSE:TRUE);
    XtSetValues(trace->prntscr.b1,arglist,1);
    XtSetValues(trace->prntscr.b2,arglist,1);
    
    /* manage the popup on the screen */
    XtManageChild(trace->prntscr.customize);
    }

void    ps_numpag(w,trace,cb)
    Widget		w;
    TRACE	*trace;
    XmScaleCallbackStruct *cb;
{
    int		ns,max;
    
    if (DTPRINT) printf("In ps_numpag - trace=%d value passed=%d\n",trace,cb->value);
    
    /* calculate ns per page */
    ns = (int)((trace->width - global->xstart)/global->res);
    
    /* calculate max number of pages from current time to end */
    max = (trace->end_time - global->time)/ns;
    if ( (trace->end_time - global->time) % ns )
	max++;;
    
    /* update num pages making sure user didn't select too many */
    trace->numpag = MIN((int)cb->value,max);
    }

void    ps_print(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    FILE	*psfile=NULL;
    int		i,ns;
    char	*psfilename;
    
    if (DTPRINT) printf("In ps_print - trace=%d\n",trace);
    
    /* get page size */
    global->bsized = XmToggleButtonGetState (trace->prntscr.rsizeb);
    
    /* open output file */
    psfilename = XmTextGetString (trace->prntscr.text);

    if (psfilename) {
	/* Open the file */
	if (DTPRINT) printf("Filename=%s\n", psfilename);
	psfile = fopen(psfilename,"w");
	}
    else {
	if (DTPRINT) printf ("Null filename\n");
	psfilename = "NULL";
	}

    /* hide the print screen window */
    XtUnmanageChild(trace->prntscr.customize);
    
    if (psfile == NULL) {
	sprintf(message,"Bad Filename: %s\n",psfilename);
	dino_error_ack(trace,message);
	return;
	}
    
    set_cursor (trace, DC_BUSY);
    XSync(global->display,0);
    
    /* include the postscript macro information */
    fputs(dinopost,psfile);
    
    /* print out each page */
    for (i=0;i<trace->numpag && trace->filename[0] != '\0';i++) {
	
	ns = (int)((trace->width - global->xstart)/global->res);
	
	/* output the page scaling and rf time */
	fprintf(psfile,"%d %d %d %d %d PAGESCALE\n",
		trace->height, trace->width, trace->sigrf,
		(int) ( ( global->bsized ? 11.0 :  8.5) * 72.0),
		(int) ( ( global->bsized ? 17.0 : 11.0) * 72.0)
		);
	
	/* output the page header macro */
	fprintf(psfile,"(%d-%d) %d (%s) (%s) %d (%s) PAGEHDR\n",
		global->time + ns,		/* end time */
		global->time,			/* start time */
		ns,				/* resolution */
		date_string(),			/* time & date */
		trace->filename,		/* filename */
		i+1,				/* page number */
		DTVERSION			/* version (for title) */
		);
	
	/* draw the signal names and the traces */
	ps_drawsig(trace,psfile);
	ps_draw(trace,psfile);
	
	/* print the page */
	fprintf(psfile,"stroke\nshowpage\n");
	
	/* if not the last page, draw the next page */
	if (i < trace->numpag-1) {
	    /* increment to next page */
	    global->time += ns;
	    
	    /* redraw the display */
	    get_geometry(trace);
	    new_time(trace);
	    }
	}
    
    /* free the memory from getting the filename */
    XtFree(psfilename);
    
    /* close output file */
    fclose(psfile);
    set_cursor (trace, DC_NORMAL);
    }

void    ps_print_all(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    int		ns;
    
    if (DTPRINT) printf("In ps_print_all - trace=%d\n",trace);
    
    /* reset the drawing back to the beginning */
    global->time = trace->start_time;
    
    /* redraw the display */
    get_geometry(trace);
    new_time(trace);
    
    /* calculate ns per page */
    ns = (int)((trace->width - global->xstart)/global->res);
    
    /* calculate number of pages needed to draw the entire trace */
    trace->numpag = (trace->end_time - trace->start_time)/ns;
    if ( (trace->end_time - trace->start_time) % ns )
	trace->numpag++;;
    
    /* draw the entire trace */
    ps_print(NULL,trace,NULL);
    }

void    ps_cancel(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In ps_cancel - trace=%d\n",trace);
    
    /* hide the window */
    XtUnmanageChild(trace->prntscr.customize);
    }

void    ps_reset(w,trace,cb)
    Widget			w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In ps_reset - trace=%d",trace);
    
    /* ADD RESET CODE !!! */
    }

ps_draw(trace,psfile)
    TRACE	*trace;
    FILE		*psfile;
{
    int c=0,numprt,i,adj,ymdpt,yt,xloc,xend,len,mid,xstart,ystart;
    int x1,y1,x2,y2;
    float iff,xlocf,xtimf;
    SIGNAL_LW *cptr,*nptr;
    SIGNAL *sig_ptr;
    char strg[32];
    unsigned int value;
    CURSOR *csr_ptr;			/* Current cursor being printed */
    
    if (DTPRINT) printf("In ps_draw - filename=%s\n",trace->filename);
    
    xend = trace->width - XMARGIN;
    adj = global->time * global->res - global->xstart;
    
    if (DTPRINT) printf("global->res=%f adj=%d\n",global->res,adj);
    
    /* Loop and draw each signal individually */
    for (sig_ptr = trace->dispsig, numprt = 0; sig_ptr && numprt<trace->numsigvis;
	 sig_ptr = sig_ptr->forward, numprt++) {

	y1 = trace->height - trace->ystart - c * trace->sighgt - SIG_SPACE;
	ymdpt = y1 - (int)(trace->sighgt/2) + SIG_SPACE;
	y2 = y1 - trace->sighgt + 2*SIG_SPACE;
	cptr = (SIGNAL_LW *)sig_ptr->cptr;
	c++;
	xloc = 0;
	
	/* output y information - note reverse from draw() due to y-axis */
	fprintf(psfile,"/y1 %d YADJ def /ym %d YADJ def /y2 %d YADJ def\n",
		y2,ymdpt,y1);
	
	/* Compute starting points for signal */
	xstart = global->xstart;
	switch( cptr->sttime.state )
	    {
	  case STATE_0: ystart = y2; break;
	  case STATE_1: ystart = y1; break;
	  case STATE_U: ystart = ymdpt; break;
	  case STATE_Z: ystart = ymdpt; break;
	  case STATE_B32: ystart = ymdpt; break;
	  case STATE_B64: ystart = ymdpt; break;
	  case STATE_B96: ystart = ymdpt; break;
	  default: printf("Error: State=%d\n",cptr->sttime.state); break;
	    }
	
	/* output starting positional information */
	fprintf(psfile,"%d %d START\n",xstart,ystart);
	
	/* Loop as long as the time and end of trace are in current screen */
	while ( cptr->sttime.time != EOT && xloc < xend )
	    {
	    /* find the next transition */
	    nptr = cptr + sig_ptr->lws;
	    
	    /* if next transition is the end, don't draw */
	    if (nptr->sttime.time == EOT) break;
	    
	    /* find the x location for the end of this segment */
	    xloc = nptr->sttime.time * global->res - adj;
	    
	    /* Determine what the state of the signal is and build transition */
	    switch( cptr->sttime.state ) {
	      case STATE_0: if ( xloc > xend ) xloc = xend;
		fprintf(psfile,"%d STATE_0\n",xloc);
		break;
		
	      case STATE_1: if ( xloc > xend ) xloc = xend;
		fprintf(psfile,"%d STATE_1\n",xloc);
		break;
		
	      case STATE_U: if ( xloc > xend ) xloc = xend;
		fprintf(psfile,"%d STATE_U\n",xloc);
		break;
		
	      case STATE_Z: if ( xloc > xend ) xloc = xend;
		fprintf(psfile,"%d STATE_Z\n",xloc);
		break;
		
	      case STATE_B32: if ( xloc > xend ) xloc = xend;
		value = *((unsigned int *)cptr+1);
		/* Below evaluation left to right important to prevent error */
		if ( (sig_ptr->decode != NULL) &&
		    (value<MAXSTATENAMES) &&
		    (sig_ptr->decode->statename[value][0] != '\0')) {
		    strcpy (strg, sig_ptr->decode->statename[value]);
		    }
		else {
		    if (trace->busrep == HBUS)
			sprintf(strg,"%X", value);
		    else if (trace->busrep == OBUS)
			sprintf(strg,"%o", value);
		    }
		
		fprintf(psfile,"%d (%s) STATE_B32\n",xloc,strg);
		break;
		
	      case STATE_B64: if ( xloc > xend ) xloc = xend;
		if (trace->busrep == HBUS)
		    sprintf(strg,"%X %08X",*((unsigned int *)cptr+2),
			    *((unsigned int *)cptr+1));
		else if (trace->busrep == OBUS)
		    sprintf(strg,"%o %o",*((unsigned int *)cptr+2),
			    *((unsigned int *)cptr+1));
		
		fprintf(psfile,"%d (%s) STATE_B32\n",xloc,strg);
		break;
		
	      case STATE_B96: if ( xloc > xend ) xloc = xend;
		if (trace->busrep == HBUS)
		    sprintf(strg,"%X %08X %08X",*((unsigned int *)cptr+3),
			    *((unsigned int *)cptr+2),
			    *((unsigned int *)cptr+1));
		else if (trace->busrep == OBUS)
		    sprintf(strg,"%o %o %o",*((unsigned int *)cptr+3),
			    *((unsigned int *)cptr+2),
			    *((unsigned int *)cptr+1));
		fprintf(psfile,"%d (%s) STATE_B32\n",xloc,strg);
		break;
		
	      default: printf("Error: State=%d\n",cptr->sttime.state); break;
		} /* end switch */
	    
	    cptr += sig_ptr->lws;
	    }
	} /* end of FOR */
    
    /*** draw the time line and the grid if its visible ***/
    
    /* calculate the starting window pixel location of the first time */
    xlocf = (trace->grid_align + (global->time-trace->grid_align)
	     /trace->grid_res*trace->grid_res)*global->res - adj;
    
    /* calculate the starting time */
    xtimf = (xlocf + (float)adj)/global->res + .001;
    
    /* initialize some parameters */
    i = 0;
    x1 = (int)xtimf;
    yt = trace->height - 20;
    y1 = trace->height - trace->ystart + SIG_SPACE/4;
    y2 = trace->sighgt;
    
    /* create the dash pattern for the vertical grid lines */
    strg[0] = SIG_SPACE/2;
    strg[1] = trace->sighgt - strg[0];
    
    /* set the line attributes as the specified dash pattern */
    fprintf(psfile,"stroke\n[%d YTRN %d YTRN] 0 setdash\n",strg[0],strg[1]);
    
    /* check if there is a reasonable amount of increments to draw the time grid */
    if ( ((float)xend - xlocf)/(trace->grid_res*global->res) < MIN_GRID_RES )
	{
        for (iff=xlocf;iff<(float)xend; iff+=trace->grid_res*global->res)
	    {
	    /* compute the time value and draw it if it fits */
	    time_to_string (trace, strg, x1, FALSE);
	    if ( (int)iff - i >= XTextWidth(trace->text_font,strg,strlen(strg)) + 5 )
		{
		fprintf(psfile,"%d XADJ %d YADJ MT (%s) show\n",
			(int)iff,yt-10,strg);
	        i = (int)iff;
		}
	    
	    /* if the grid is visible, draw a vertical dashed line */
	    if (trace->grid_vis)
		{
		fprintf(psfile,"%d XADJ %d YADJ MT %d XADJ %d YADJ LT\n",
			(int)iff,y1,(int)iff,y2);
		}
	    x1 += trace->grid_res;
	    }
	}
    else
	{
	/* grid res is useless - must increase the spacing */
	/*	dino_warning_ack(trace,"Grid Spacing Too Small - Increase Res"); */
	}
    
    /* reset the line attributes */
    fprintf(psfile,"stroke [] 0 setdash\n");
    
    /* draw the cursors if they are visible */
    if ( trace->cursor_vis )
	{
	/* initial the y values for drawing */
	y1 = trace->height - 25 - 10;
	y2 = trace->height - ( (int)((trace->height-trace->ystart)/trace->sighgt)-1) *
	    trace->sighgt - trace->sighgt/2 - trace->ystart - 2;

	for (csr_ptr = global->cursor_head; csr_ptr; csr_ptr = csr_ptr->next) {

	    /* check if cursor is on the screen */
	    if (csr_ptr->time > global->time) {

		/* draw the vertical cursor line */
		x1 = csr_ptr->time * global->res - adj;
		fprintf(psfile,"%d XADJ %d YADJ MT %d XADJ %d YADJ LT\n",
			x1,y1,x1,y2);
		
		/* draw the cursor value */
		time_to_string (trace, strg, csr_ptr->time, FALSE);
 		len = XTextWidth(trace->text_font,strg,strlen(strg));
		fprintf(psfile,"%d XADJ %d YADJ MT (%s) show\n",
			x1-len/2,y2-8,strg);
		
		/* if there is a previous visible cursor, draw delta line */
		if ( csr_ptr->prev && csr_ptr->prev->time > global->time ) {

		    x2 = csr_ptr->prev->time * global->res - adj;
		    time_to_string (trace, strg, csr_ptr->time - csr_ptr->prev->time, TRUE);
 		    len = XTextWidth(trace->text_font,strg,strlen(strg));
		    
		    /* write the delta value if it fits */
 		    if ( x1 - x2 >= len + 6 )
			{
			/* calculate the mid pt of the segment */
			mid = x2 + (x1 - x2)/2;
			fprintf(psfile,"%d XADJ %d YADJ MT %d XADJ %d YADJ LT\n",
				x2,y2+5,mid-len/2-2,y2+5);
			fprintf(psfile,"%d XADJ %d YADJ MT %d XADJ %d YADJ LT\n",
				mid+len/2+2,y2+5,x1,y2+5);
			
			fprintf(psfile,"%d XADJ %d YADJ MT (%s) show\n",
				mid-len/2,y2+2,strg);
			}
 		    /* or just draw the delta line */
 		    else
			{
			fprintf(psfile,"%d XADJ %d YADJ MT %d XADJ %d YADJ LT\n",
				x1,y2+5,x2,y2+5);
			}
		    }
		}
	    }
	}
    } /* End of DRAW */

int    ps_drawsig(trace,psfile)
    TRACE	*trace;
    FILE		*psfile;
{
    SIGNAL *sig_ptr;
    int c=0,numprt,ymdpt;
    int		x1,y1;
    
    if (DTPRINT) printf("In ps_drawsig - filename=%s\n",trace->filename);
    
    /* don't draw anything if there is no file is loaded */
    if (!trace->loaded) return;
    
    /* loop thru all the visible signals */
    for (sig_ptr = trace->dispsig, numprt = 0; sig_ptr && numprt<trace->numsigvis;
	 sig_ptr = sig_ptr->forward, numprt++) {

	/* calculate the location to start drawing the signal name */
	x1 = global->xstart - 105;
	/* printf ("x1=%d, xstart=%d, %s\n",x1,global->xstart, sig_ptr->signame); */
	
	/* calculate the y location to draw the signal name and draw it */
	y1 = trace->height - trace->ystart - c * trace->sighgt - SIG_SPACE;
	ymdpt = y1 - (int)(trace->sighgt/2) + SIG_SPACE;
	
	fprintf(psfile,"%d XADJ %d YADJ 3 sub MT (%s) RIGHTSHOW\n",x1,ymdpt,
		sig_ptr->signame);
	c++;
	}
    }

