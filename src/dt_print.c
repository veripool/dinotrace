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
static char rcsid[] = "$Id$";

#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>

#ifdef VMS
# include <math.h>
# include <descrip.h>
#endif

#include <X11/Xlib.h>
#include <Xm/Xm.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include <Xm/Text.h>
#include <Xm/BulletinB.h>
#include <Xm/Scale.h>
#include <Xm/Label.h>

#include "dinotrace.h"
#include "callbacks.h"
#include "dinopost.h"

extern void	ps_drawsig(), ps_draw(), ps_draw_grid(), ps_draw_cursors();



void    ps_dialog (w,trace,cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
    
{
    if (DTPRINT_ENTRY) printf ("In print_screen - trace=%d\n",trace);
    
    if (!trace->prntscr.customize)
	{
	XtSetArg (arglist[0],XmNdefaultPosition, TRUE);
	XtSetArg (arglist[1],XmNdialogTitle, XmStringCreateSimple ("Print Screen Menu"));
	/* XtSetArg (arglist[2],XmNwidth, 300);
	   XtSetArg (arglist[3],XmNheight, 225); */
	trace->prntscr.customize = XmCreateBulletinBoardDialog (trace->work, "print",arglist,2);
	
	/* create label widget for text widget */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("File Name") );
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNy, 3);
	trace->prntscr.label = XmCreateLabel (trace->prntscr.customize,"",arglist,3);
	XtManageChild (trace->prntscr.label);
	
	/* create the file name text widget */
	XtSetArg (arglist[0], XmNrows, 1);
	XtSetArg (arglist[1], XmNcolumns, 30);
	XtSetArg (arglist[2], XmNx, 10);
	XtSetArg (arglist[3], XmNy, 35);
	XtSetArg (arglist[4], XmNresizeHeight, FALSE);
	XtSetArg (arglist[5], XmNeditMode, XmSINGLE_LINE_EDIT);
	trace->prntscr.text = XmCreateText (trace->prntscr.customize,"",arglist,6);
	XtManageChild (trace->prntscr.text);
	XtAddCallback (trace->prntscr.text, XmNactivateCallback, ps_print_req_cb, trace);
	
	/* create label widget for notetext widget */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Note") );
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNy, 75);
	trace->prntscr.label = XmCreateLabel (trace->prntscr.customize,"",arglist,3);
	XtManageChild (trace->prntscr.label);
	
	/* create the print note text widget */
	XtSetArg (arglist[0], XmNrows, 1);
	XtSetArg (arglist[1], XmNcolumns, 30);
	XtSetArg (arglist[2], XmNx, 10);
	XtSetArg (arglist[3], XmNy, 110);
	XtSetArg (arglist[4], XmNresizeHeight, FALSE);
	XtSetArg (arglist[5], XmNeditMode, XmSINGLE_LINE_EDIT);
	trace->prntscr.notetext = XmCreateText (trace->prntscr.customize,"notetext",arglist,6);
	XtManageChild (trace->prntscr.notetext);
	XtAddCallback (trace->prntscr.notetext, XmNactivateCallback, ps_print_req_cb, trace);
	
	/* Create number of pages label */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Number of Pages") );
	XtSetArg (arglist[1], XmNx, 20);
	XtSetArg (arglist[2], XmNy, 150);
	trace->prntscr.pagelabel = XmCreateLabel (trace->prntscr.customize,"",arglist,3);
	XtManageChild (trace->prntscr.pagelabel);
	
	/* Create number of pages slider */
	XtSetArg (arglist[0], XmNshowValue, 1);
	XtSetArg (arglist[1], XmNx, 20);
	XtSetArg (arglist[2], XmNy, 170);
	XtSetArg (arglist[3], XmNwidth, 120);
	XtSetArg (arglist[4], XmNvalue, trace->numpag);
	XtSetArg (arglist[5], XmNminimum, 1);
	XtSetArg (arglist[6], XmNmaximum, 50);
	XtSetArg (arglist[7], XmNorientation, XmHORIZONTAL);
	XtSetArg (arglist[8], XmNprocessingDirection, XmMAX_ON_RIGHT);
	trace->prntscr.s1 = XmCreateScale (trace->prntscr.customize,"numpag",arglist,9);
	XtAddCallback (trace->prntscr.s1, XmNvalueChangedCallback, ps_numpag_cb, trace);
	XtManageChild (trace->prntscr.s1);
	
	/* Create radio box for page size */
	trace->prntscr.size_menu = XmCreatePulldownMenu (trace->prntscr.customize,"size",arglist,0);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("A-Sized"));
	trace->prntscr.sizea = XmCreatePushButtonGadget (trace->prntscr.size_menu,"sizea",arglist,1);
	XtManageChild (trace->prntscr.sizea);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("B-Sized"));
	trace->prntscr.sizeb = XmCreatePushButtonGadget (trace->prntscr.size_menu,"sizeb",arglist,1);
	XtManageChild (trace->prntscr.sizeb);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("EPS Portrait"));
	trace->prntscr.sizeep = XmCreatePushButtonGadget (trace->prntscr.size_menu,"sizeep",arglist,1);
	XtManageChild (trace->prntscr.sizeep);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("EPS Landscape"));
	trace->prntscr.sizeel = XmCreatePushButtonGadget (trace->prntscr.size_menu,"sizeel",arglist,1);
	XtManageChild (trace->prntscr.sizeel);
	
	XtSetArg (arglist[0], XmNx, 160);
	XtSetArg (arglist[1], XmNy, 180);
	XtSetArg (arglist[2], XmNlabelString, XmStringCreateSimple ("Layout"));
	XtSetArg (arglist[3], XmNsubMenuId, trace->prntscr.size_menu);
	trace->prntscr.size_option = XmCreateOptionMenu (trace->prntscr.customize,"sizeo",arglist,4);
	XtManageChild (trace->prntscr.size_option);

	/* Create all_times button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Include off-screen times"));
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNy, 220);
	XtSetArg (arglist[3], XmNshadowThickness, 1);
	trace->prntscr.all_times = XmCreateToggleButton (trace->prntscr.customize,
							 "all_times",arglist,4);
	XtManageChild (trace->prntscr.all_times);

	/* Create all_signals button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Include off-screen signals"));
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNy, 255);
	XtSetArg (arglist[3], XmNshadowThickness, 1);
	trace->prntscr.all_signals = XmCreateToggleButton (trace->prntscr.customize,
							   "all_signals",arglist,4);
	XtManageChild (trace->prntscr.all_signals);

	/* Create Print button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Print") );
	XtSetArg (arglist[1], XmNx, 10 );
	XtSetArg (arglist[2], XmNy, 300 );
	trace->prntscr.b1 = XmCreatePushButton (trace->prntscr.customize, "print",arglist,3);
	XtAddCallback (trace->prntscr.b1, XmNactivateCallback, ps_print_req_cb, trace);
	XtManageChild (trace->prntscr.b1);
	
	/* create cancel button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Cancel") );
	XtSetArg (arglist[1], XmNx, 170 );
	XtSetArg (arglist[2], XmNy, 300 );
	trace->prntscr.b3 = XmCreatePushButton (trace->prntscr.customize,"cancel",arglist,3);
	XtAddCallback (trace->prntscr.b3, XmNactivateCallback, unmanage_cb, trace->prntscr.customize);
	XtManageChild (trace->prntscr.b3);
	}
    
    /* reset number of pages to one */
    trace->numpag = 1;
    XtSetArg (arglist[0], XmNvalue, trace->numpag);
    XtSetValues (trace->prntscr.s1,arglist,1);
    
    /* reset page size */
    switch (global->print_size) {
      default:
      case PRINTSIZE_A:
	XtSetArg (arglist[0], XmNmenuHistory, trace->prntscr.sizea);
	break;
      case PRINTSIZE_B:
	XtSetArg (arglist[0], XmNmenuHistory, trace->prntscr.sizeb);
	break;
      case PRINTSIZE_EPSPORT:
	XtSetArg (arglist[0], XmNmenuHistory, trace->prntscr.sizeep);
	break;
      case PRINTSIZE_EPSLAND:
	XtSetArg (arglist[0], XmNmenuHistory, trace->prntscr.sizeel);
	break;
	}
    XtSetValues (trace->prntscr.size_option, arglist, 1);

    /* reset flags */
    XtSetArg (arglist[0], XmNset, global->print_all_times ? 1:0);
    XtSetValues (trace->prntscr.all_times,arglist,1);
    XtSetArg (arglist[0], XmNset, global->print_all_signals ? 1:0);
    XtSetValues (trace->prntscr.all_signals,arglist,1);

    /* reset file name */
    XtSetArg (arglist[0], XmNvalue, trace->printname);
    XtSetValues (trace->prntscr.text,arglist,1);
    
    /* reset file note */
    XtSetArg (arglist[0], XmNvalue, global->printnote);
    XtSetValues (trace->prntscr.notetext,arglist,1);
    
    /* if a file has been read in, make printscreen buttons active */
    XtSetArg (arglist[0],XmNsensitive, (trace->loaded)?TRUE:FALSE);
    XtSetValues (trace->prntscr.b1,arglist,1);
    
    /* manage the popup on the screen */
    XtManageChild (trace->prntscr.customize);
    }

void    ps_numpag_cb (w,trace,cb)
    Widget		w;
    TRACE		*trace;
    XmScaleCallbackStruct *cb;
{
    int		max;
    DTime	pagetime;
    
    if (DTPRINT_ENTRY) printf ("In ps_numpag_cb - trace=%d value passed=%d\n",trace,cb->value);
    
    /* calculate ns per page */
    pagetime = (int)((trace->width - global->xstart)/global->res);
    
    /* calculate max number of pages from current time to end */
    max = (trace->end_time - global->time)/pagetime;
    if ( (trace->end_time - global->time) % pagetime )
	max++;;
    
    /* update num pages making sure user didn't select too many */
    trace->numpag = MIN ((int)cb->value,max);
    }

void    ps_print_direct_cb (w,trace,cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (!trace->loaded) return;
    ps_print_internal (trace);
    }

void    ps_print_req_cb (w,trace,cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    Widget	clicked;
    
    if (DTPRINT_ENTRY) printf ("In ps_print_req_cb - trace=%d\n",trace);
    
    XtSetArg (arglist[0], XmNmenuHistory, &clicked);
    XtGetValues (trace->prntscr.size_option, arglist, 1);
    if (clicked == trace->prntscr.sizeep)
	global->print_size = PRINTSIZE_EPSPORT;
    else if (clicked == trace->prntscr.sizeel)
	global->print_size = PRINTSIZE_EPSLAND;
    else if (clicked == trace->prntscr.sizeb)
	global->print_size = PRINTSIZE_B;
    else global->print_size = PRINTSIZE_A;

    global->print_all_signals = XmToggleButtonGetState (trace->prntscr.all_signals);
    global->print_all_times = XmToggleButtonGetState (trace->prntscr.all_times);

    /* get note */
    strcpy (global->printnote, XmTextGetString (trace->prntscr.notetext));

    /* open output file */
    strcpy (trace->printname, XmTextGetString (trace->prntscr.text));

    /* hide the print screen window */
    XtUnmanageChild (trace->prntscr.customize);
    
    ps_print_internal (trace);
    }
    
void    ps_print_internal (trace)
    TRACE		*trace;
{
    FILE	*psfile=NULL;
    int		sigs_per_page;
    DTime	time_per_page;
    int		horiz_pages;		/* Number of pages to print the time on (horizontal) */
    int		vert_pages;		/* Number of pages to print signal names on (vertical) */
    int		horiz_page, vert_page;
    char	*timeunits;
    int		encapsulated;
    SIGNAL	*sig_ptr, *sig_end_ptr;
    int		numprt;
    DTime	printtime;	/* Time to start on */
    char	pagenum[20];
    
    if (DTPRINT_ENTRY) printf ("In ps_print_internal - trace=%d\n",trace);
    
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
    
    set_cursor (trace, DC_BUSY);
    XSync (global->display,0);
    
    /* calculate time per page */
    time_per_page = (int)((trace->width - global->xstart)/global->res);
    sigs_per_page = trace->numsigvis;
    
    /* Reset stuff if doing all times */
    horiz_pages = trace->numpag;
    if (global->print_all_times) {
	/* calculate number of pages needed to draw the entire trace */
	horiz_pages = (trace->end_time - trace->start_time)/time_per_page;
	if ( (trace->end_time - trace->start_time) % time_per_page )
	    horiz_pages++;
	}

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
    if (encapsulated) fprintf (psfile,"save\n");
    
    /* include the postscript macro information */
    fputs (dinopost,psfile);
    
    /* Grab units */
    timeunits = time_units_to_string (trace->timerep, FALSE);

    /* output the page scaling and rf time */
    fprintf (psfile,"\n%d %d %d %d %d %d PAGESCALE\n",
	     trace->height, trace->width, global->xstart, trace->sigrf,
	     (int) ( ( (global->print_size==PRINTSIZE_B) ? 11.0 :  8.5) * 72.0),
	     (int) ( ( (global->print_size==PRINTSIZE_B) ? 16.8 : 10.8) * 72.0)
	     );
	
    /* output signal name width scalling */
    fprintf (psfile,"/sigwidth 0 def\nstroke /Times-Roman findfont 8 scalefont setfont\n");
    /* Signal to start on */
    if (vert_pages > 1) {
	sig_ptr = trace->firstsig;
	}
    else {
	sig_ptr = trace->dispsig;
	}
    for (numprt = 0; sig_ptr && ( numprt<trace->numsigvis || (vert_pages>1));
	 sig_ptr = sig_ptr->forward, numprt++) {
	fprintf (psfile,"(%s) SIGMARGIN\n", sig_ptr->signame);
	}
    fprintf (psfile,"XSCALESET\n\n");

    /* print out each page */
    for (horiz_page=0; horiz_page<horiz_pages; horiz_page++) {
	
	/* Start time */
	if (horiz_pages > 1) {
	    printtime = time_per_page * horiz_page + trace->start_time;
	    }
	else {
	    printtime = global->time;
	    }

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
		if (horiz_pages == 1) 	sprintf (pagenum, "");	/* Just 1 page */
		else			sprintf (pagenum, "Page %d", horiz_page+1);
		}
	    else {
		if (horiz_pages == 1) 	sprintf (pagenum, "Page %d", vert_page+1);
		else			sprintf (pagenum, "Page %d-%d", horiz_page+1, vert_page+1);
		}
	    
	    /* output the page header macro */
	    fprintf (psfile, "(%d-%d %s) (%d %s/page) (%s) (%s) (%s) (%s) (%s) %s\n",
		     printtime,			/* start time */
		     printtime + time_per_page,	/* end time */
		     timeunits,
		     time_per_page,			/* resolution */
		     timeunits,
		     date_string(0),			/* time & date */
		     global->printnote,		/* filenote */
		     trace->filename,		/* filename */
		     pagenum,				/* page number */
		     DTVERSION,			/* version (for title) */
		     ( (global->print_size==PRINTSIZE_EPSPORT)
		      ? "EPSPHDR" :
		      ( (global->print_size==PRINTSIZE_EPSLAND)
		       ? "EPSLHDR" : "PAGEHDR"))	/* which macro to use */
		     );
	    
	    /* draw the signal names and the traces */
	    ps_drawsig (trace, psfile, sig_ptr, sig_end_ptr);
	    ps_draw (trace, psfile, sig_ptr, sig_end_ptr, printtime);
	    
	    /* print the page */
	    if (encapsulated)
		fprintf (psfile,"stroke\nrestore\n");
	    else fprintf (psfile,"stroke\nshowpage\n");

	    } /* vert page */
	} /* horiz page */
    
    /* close output file */
    fclose (psfile);
    new_time (trace);
    set_cursor (trace, DC_NORMAL);
    }

void    ps_reset (w,trace,cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    char 		*pchar;
    if (DTPRINT_ENTRY) printf ("In ps_reset - trace=%d",trace);
    
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

void ps_draw (trace, psfile, sig_ptr, sig_end_ptr, printtime)
    TRACE	*trace;
    FILE	*psfile;
    SIGNAL	*sig_ptr;	/* Vertical signal to start on */
    SIGNAL	*sig_end_ptr;	/* Last signal to print */
    DTime	printtime;	/* Time to start on */
{
    int c=0,i,adj,ymdpt,yt,xloc,xend,len,mid,xstart,ystart;
    int x1,y1,x2,y2;
    float iff,xlocf,xtimf;
    SIGNAL_LW *cptr,*nptr;
    char strg[32];
    unsigned int value;
    int unstroked=0;		/* Number commands not stroked */
    int		grid_num;
    
    if (DTPRINT_ENTRY) printf ("In ps_draw - filename=%s, printtime=%d sig=%s\n",trace->filename, printtime, sig_ptr->signame);
    
    xend = trace->width - XMARGIN;
    adj = printtime * global->res - global->xstart;
    
    if (DTPRINT_PRINT) printf ("global->res=%f adj=%d\n",global->res,adj);
    
    /* Loop and draw each signal individually */
    for (; sig_ptr && sig_ptr!=sig_end_ptr; sig_ptr = sig_ptr->forward) {

	y1 = trace->height - trace->ystart - c * trace->sighgt - SIG_SPACE;
	ymdpt = y1 - (int)(trace->sighgt/2) + SIG_SPACE;
	y2 = y1 - trace->sighgt + 2*SIG_SPACE;
	c++;
	xloc = 0;
	
	/* find data start */
	cptr = (SIGNAL_LW *)sig_ptr->bptr;
	if (printtime >= (*cptr).sttime.time ) {
	    while (((*cptr).sttime.time != EOT) &&
		   (printtime > (* (SIGNAL_LW *)((cptr) + sig_ptr->lws)).sttime.time)) {
		cptr += sig_ptr->lws;
		}
	    }

	/* output y information - note reverse from draw() due to y-axis */
	fprintf (psfile,"/y1 %d YADJ def /ym %d YADJ def /y2 %d YADJ def\n",
		y2,ymdpt,y1);
	
	/* Compute starting points for signal */
	xstart = global->xstart;
	switch ( cptr->sttime.state )
	    {
	  case STATE_0: ystart = y2; break;
	  case STATE_1: ystart = y1; break;
	  case STATE_U: ystart = ymdpt; break;
	  case STATE_Z: ystart = ymdpt; break;
	  case STATE_B32: ystart = ymdpt; break;
	  case STATE_B64: ystart = ymdpt; break;
	  case STATE_B96: ystart = ymdpt; break;
	  default: printf ("Error: State=%d\n",cptr->sttime.state); break;
	    }
	
	/* output starting positional information */
	fprintf (psfile,"%d %d START\n",xstart,ystart);
	
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
	    switch ( cptr->sttime.state ) {
	      case STATE_0: if ( xloc > xend ) xloc = xend;
		fprintf (psfile,"%d STATE_0\n",xloc);
		break;
		
	      case STATE_1: if ( xloc > xend ) xloc = xend;
		fprintf (psfile,"%d STATE_1\n",xloc);
		break;
		
	      case STATE_U: if ( xloc > xend ) xloc = xend;
		fprintf (psfile,"%d STATE_U\n",xloc);
		break;
		
	      case STATE_Z: if ( xloc > xend ) xloc = xend;
		fprintf (psfile,"%d STATE_Z\n",xloc);
		break;
		
	      case STATE_B32: if ( xloc > xend ) xloc = xend;
		value = *((unsigned int *)cptr+1);
		/* Below evaluation left to right important to prevent error */
		if ( (sig_ptr->decode != NULL) &&
		    (value < sig_ptr->decode->numstates) &&
		    (sig_ptr->decode->statename[value][0] != '\0')) {
		    strcpy (strg, sig_ptr->decode->statename[value]);
		    }
		else {
		    if (trace->busrep == HBUS)
			sprintf (strg,"%x", value);
		    else if (trace->busrep == OBUS)
			sprintf (strg,"%o", value);
		    }
		
		fprintf (psfile,"%d (%s) STATE_B32\n",xloc,strg);
		break;
		
	      case STATE_B64: if ( xloc > xend ) xloc = xend;
		if (trace->busrep == HBUS)
		    sprintf (strg,"%x %08x",*((unsigned int *)cptr+2),
			    *((unsigned int *)cptr+1));
		else if (trace->busrep == OBUS)
		    sprintf (strg,"%o %o",*((unsigned int *)cptr+2),
			    *((unsigned int *)cptr+1));
		
		fprintf (psfile,"%d (%s) STATE_B32\n",xloc,strg);
		break;
		
	      case STATE_B96: if ( xloc > xend ) xloc = xend;
		if (trace->busrep == HBUS)
		    sprintf (strg,"%x %08x %08x",*((unsigned int *)cptr+3),
			    *((unsigned int *)cptr+2),
			    *((unsigned int *)cptr+1));
		else if (trace->busrep == OBUS)
		    sprintf (strg,"%o %o %o",*((unsigned int *)cptr+3),
			    *((unsigned int *)cptr+2),
			    *((unsigned int *)cptr+1));
		fprintf (psfile,"%d (%s) STATE_B32\n",xloc,strg);
		break;
		
	      default: printf ("Error: State=%d\n",cptr->sttime.state); break;
		} /* end switch */
	    
	    if (unstroked++ > 400) {
		/* Must stroke every so often to avoid overflowing printer stack */
		fprintf (psfile,"currentpoint stroke MT\n");
		unstroked=0;
		}

	    cptr += sig_ptr->lws;
	    }
	} /* end of FOR */
    
    /* Draw grids */
    for (grid_num=0; grid_num<MAXGRIDS; grid_num++) {
	ps_draw_grid (trace, psfile, printtime, &(trace->grid[grid_num]));
    }

    /* draw the cursors if they are visible */
    ps_draw_cursors (trace, psfile, printtime);
} /* End of DRAW */


void ps_drawsig (trace, psfile, sig_ptr, sig_end_ptr)
    TRACE	*trace;
    FILE	*psfile;
    SIGNAL	*sig_ptr;	/* Vertical signal to start on */
    SIGNAL	*sig_end_ptr;	/* Last signal to print */
{
    int		c=0,ymdpt;
    int		x1,y1;
    
    if (DTPRINT_ENTRY) printf ("In ps_drawsig - filename=%s\n",trace->filename);
    
    /* don't draw anything if there is no file is loaded */
    if (!trace->loaded) return;
    
    /* loop thru all the visible signals */
    for (; sig_ptr && sig_ptr!=sig_end_ptr; sig_ptr = sig_ptr->forward) {

	/* calculate the location to start drawing the signal name */
	x1 = global->xstart;
	/* printf ("x1=%d, xstart=%d, %s\n",x1,global->xstart, sig_ptr->signame); */
	
	/* calculate the y location to draw the signal name and draw it */
	y1 = trace->height - trace->ystart - c * trace->sighgt - SIG_SPACE;
	ymdpt = y1 - (int)(trace->sighgt/2) + SIG_SPACE;
	
	fprintf (psfile,"sigwidth %d YADJ 3 sub MT (%s) RIGHTSHOW\n",
		 ymdpt, sig_ptr->signame);
	c++;
	}
    }

void ps_draw_cursors (trace, psfile, printtime)
    TRACE	*trace;
    FILE	*psfile;
    DTime	printtime;	/* Time to start on */
{
    Position	x1,x2,y1,y2;
    char 	strg[32];
    int		len,mid;
    int		adj,xend;
    CURSOR	*csr_ptr;
    
    /* reset the line attributes */
    fprintf (psfile,"stroke [] 0 setdash\n");
    
    /* draw the cursors if they are visible */
    if ( trace->cursor_vis ) {
	/* initial the y values for drawing */
	y1 = trace->height - 25 - 10;
	y2 = trace->height - ( (int)((trace->height-trace->ystart)/trace->sighgt)-1) *
	    trace->sighgt - trace->sighgt/2 - trace->ystart - 2;

	for (csr_ptr = global->cursor_head; csr_ptr; csr_ptr = csr_ptr->next) {

	    /* check if cursor is on the screen */
	    if (csr_ptr->time > printtime) {
		
		/* draw the vertical cursor line */
		x1 = csr_ptr->time * global->res - adj;
		if (x1 > xend) break;	/* past end of screen, since sorted list no more to do */
		fprintf (psfile,"%d XADJ %d YADJ MT %d XADJ %d YADJ LT\n",
			 x1,y1,x1,y2);
		
		/* draw the cursor value */
		time_to_string (trace, strg, csr_ptr->time, FALSE);
 		len = XTextWidth (global->time_font,strg,strlen (strg));
		fprintf (psfile,"%d XADJ %d YADJ MT (%s) show\n",
			 x1-len/2,y2-8,strg);
		
		/* if there is a previous visible cursor, draw delta line */
		if ( csr_ptr->prev && csr_ptr->prev->time > printtime ) {

		    x2 = csr_ptr->prev->time * global->res - adj;
		    time_to_string (trace, strg, csr_ptr->time - csr_ptr->prev->time, TRUE);
 		    len = XTextWidth (global->time_font,strg,strlen (strg));
		    
		    /* write the delta value if it fits */
 		    if ( x1 - x2 >= len + 6 ) {
			/* calculate the mid pt of the segment */
			mid = x2 + (x1 - x2)/2;
			fprintf (psfile,"%d XADJ %d YADJ MT %d XADJ %d YADJ LT\n",
				 x2,y2+5,mid-len/2-2,y2+5);
			fprintf (psfile,"%d XADJ %d YADJ MT %d XADJ %d YADJ LT\n",
				 mid+len/2+2,y2+5,x1,y2+5);
			
			fprintf (psfile,"%d XADJ %d YADJ MT (%s) show\n",
				 mid-len/2,y2+2,strg);
		    }
 		    /* or just draw the delta line */
 		    else {
			fprintf (psfile,"%d XADJ %d YADJ MT %d XADJ %d YADJ LT\n",
				 x1,y2+5,x2,y2+5);
		    }
		}
	    }
	}
    }
}

void ps_draw_grid (trace, psfile, printtime, grid_ptr)
    TRACE	*trace;
    FILE	*psfile;
    DTime	printtime;	/* Time to start on */
    GRID	*grid_ptr;		/* Grid information */
{
    char 	strg[MAXSTATELEN+16];	/* String value to print out */
    char 	primary_dash[4];	/* Dash pattern */
    int		end_time;
    int		time_width;		/* Width of time printing */
    DTime	xtime;
    int		x_last_time;		/* Last x coordinate time was printed at */
    Position	x2;			/* Coordinate of current time */
    Position	y1,y2,yt;		/* Starting and ending points */

    /*
    int c=0,i,adj,ymdpt,yt,xloc,xend,len,mid,xstart,ystart;
    int x1,y1,x2,y2;
    float iff,xlocf,xtimf;
    SIGNAL_LW *cptr,*nptr;
    char strg[32];
    unsigned int value;
    */
    
    if (grid_ptr->period < 1) grid_ptr->period = 1;	/* Prevents round-down to 0 causing infinite loop */

    /* Is the grid too small or invisible?  If so, skip it */
    if ((! grid_ptr->visible) || ((grid_ptr->period * global->res) < MIN_GRID_RES)) {
	return;
    }

    /* set the line attributes as the specified dash pattern */
    fprintf (psfile,"stroke\n[%d YTRN %d YTRN] 0 setdash\n",SIG_SPACE/2, trace->sighgt - SIG_SPACE/2);
    
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
    yt = trace->height - 20;
    y1 = trace->height - trace->ystart + SIG_SPACE/4;
    y2 = trace->sighgt;

    /* Loop through grid times */
    for ( ; xtime <= end_time; xtime += grid_ptr->period) {
	x2 = TIME_TO_XPOS (xtime);

	/* compute the time value and draw it if it fits */
	time_to_string (trace, strg, xtime, FALSE);
	time_width = XTextWidth (global->time_font,strg,strlen (strg));
	if ( (x2 - x_last_time) >= (time_width+5) ) {
	    /* Draw if space, centered on grid line */
	    fprintf (psfile,"%d XADJ %d YADJ MT (%s) show\n", x2, yt-10, strg);
	    x_last_time = x2;
	}
	    
	/* if the grid is visible, draw a vertical dashed line */
	fprintf (psfile,"%d XADJ %d YADJ MT %d XADJ %d YADJ LT\n", x2, y1, x2, y2);
    }
}



