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

/* If XTextWidth is called in this file, then something's wrong, as X widths != print widths */


void    ps_range_sensitives_cb (w, range_ptr, cb)
    Widget		w;
    RANGE_WDGTS		*range_ptr;	/* <<<< NOTE not TRACE!!! */
    XmSelectionBoxCallbackStruct *cb;
{
    int		opt;
    int		active;
    char	strg[MAXTIMELEN];
    TRACE	*trace;

    if (DTPRINT_ENTRY) printf ("In ps_range_sensitives_cb\n");

    trace = range_ptr->trace;	/* Snarf from range, as the callback doesn't have it */

    opt = option_to_number(range_ptr->time_option, range_ptr->time_pulldownbutton, 3);
    
    switch (opt) {
      case 3:	/* Window */
	active = FALSE;
	range_ptr->dastime = (range_ptr->type == BEGIN) ? global->time 
	    :   (global->time + (( trace->width - XMARGIN - global->xstart ) / global->res));
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


void    ps_range_create (trace, range_ptr, popup, x_ptr, y_ptr, descrip, type)
    TRACE		*trace;
    RANGE_WDGTS		*range_ptr;
    Widget		popup;		/* What the root widget is */
    Position		*x_ptr,*y_ptr;	/* Pointer to coords, x is modified */
    char		*descrip;	/* Description of selection */
    Boolean		type;		/* True if END, else beginning */
{
    int x,y;

    if (DTPRINT_ENTRY) printf ("In ps_create_range - trace=%d\n",trace);

    x = *x_ptr;
    y = *y_ptr;
    
    if (!range_ptr->trace) {
	range_ptr->trace = trace;
	range_ptr->type = type;

	/* Label */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple (descrip) );
	XtSetArg (arglist[1], XmNx, x);
	XtSetArg (arglist[2], XmNy, y);
	trace->prntscr.label = XmCreateLabel (trace->prntscr.customize,"",arglist,3);
	XtManageChild (trace->prntscr.label);
	y+=20;
	
	/* Begin pulldown */
	range_ptr->time_pulldown = XmCreatePulldownMenu (popup,"time_pulldown",arglist,0);

	if (range_ptr->type == BEGIN)
	    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Window Left Edge") );
	else XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Window Right Edge") );
	range_ptr->time_pulldownbutton[3] =
	    XmCreatePushButtonGadget (range_ptr->time_pulldown,"pdbutton0",arglist,1);
	XtAddCallback (range_ptr->time_pulldownbutton[3], XmNactivateCallback, ps_range_sensitives_cb, range_ptr);
	XtManageChild (range_ptr->time_pulldownbutton[3]);

	if (range_ptr->type == BEGIN)
	    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Trace Beginning") );
	else XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Trace End") );
	range_ptr->time_pulldownbutton[2] =
	    XmCreatePushButtonGadget (range_ptr->time_pulldown,"pdbutton0",arglist,1);
	XtAddCallback (range_ptr->time_pulldownbutton[2], XmNactivateCallback, ps_range_sensitives_cb, range_ptr);
	XtManageChild (range_ptr->time_pulldownbutton[2]);

	if (range_ptr->type == BEGIN)
	    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("First Cursor") );
	else XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Last Cursor") );
	range_ptr->time_pulldownbutton[1] =
	    XmCreatePushButtonGadget (range_ptr->time_pulldown,"pdbutton0",arglist,1);
	XtAddCallback (range_ptr->time_pulldownbutton[1], XmNactivateCallback, ps_range_sensitives_cb, range_ptr);
	XtManageChild (range_ptr->time_pulldownbutton[1]);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Entered Time") );
	range_ptr->time_pulldownbutton[0] =
	    XmCreatePushButtonGadget (range_ptr->time_pulldown,"pdbutton0",arglist,1);
	XtAddCallback (range_ptr->time_pulldownbutton[0], XmNactivateCallback, ps_range_sensitives_cb, range_ptr);
	XtManageChild (range_ptr->time_pulldownbutton[0]);

	XtSetArg (arglist[0], XmNsubMenuId, range_ptr->time_pulldown);
	XtSetArg (arglist[1], XmNx, x+10);
	XtSetArg (arglist[2], XmNy, y);
	range_ptr->time_option = XmCreateOptionMenu (popup,"options",arglist,3);
	XtManageChild (range_ptr->time_option);

	/* Default */
	XtSetArg (arglist[0], XmNmenuHistory, range_ptr->time_pulldownbutton[3]);
	XtSetValues (range_ptr->time_option, arglist, 1);

	/* Begin Text */
	XtSetArg (arglist[0], XmNrows, 1);
	XtSetArg (arglist[1], XmNcolumns, 10);
	XtSetArg (arglist[2], XmNx, x+190);
	XtSetArg (arglist[3], XmNy, y+3);
	XtSetArg (arglist[4], XmNresizeHeight, FALSE);
	XtSetArg (arglist[5], XmNeditMode, XmSINGLE_LINE_EDIT);
	range_ptr->time_text = XmCreateText (popup,"textn",arglist,6);
	XtManageChild (range_ptr->time_text);

	y += 50;
    }

    /* Get initial values correct */
    ps_range_sensitives_cb (NULL, range_ptr, NULL);

    *x_ptr = x;
    *y_ptr = y;
}


DTime	ps_range_value (range_ptr)
    RANGE_WDGTS		*range_ptr;
    /* Read the range value */
{
    /* Make sure have latest cursor, etc */
    ps_range_sensitives_cb (NULL, range_ptr, NULL);

    return (range_ptr->dastime);
}


void    ps_dialog (w,trace,cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
    
{
    int			x=10,y=3;

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
	XtSetArg (arglist[1], XmNx, x);
	XtSetArg (arglist[2], XmNy, y);
	trace->prntscr.label = XmCreateLabel (trace->prntscr.customize,"",arglist,3);
	XtManageChild (trace->prntscr.label);
	y+=30;
	
	/* create the file name text widget */
	XtSetArg (arglist[0], XmNrows, 1);
	XtSetArg (arglist[1], XmNcolumns, 30);
	XtSetArg (arglist[2], XmNx, x);
	XtSetArg (arglist[3], XmNy, y);
	XtSetArg (arglist[4], XmNresizeHeight, FALSE);
	XtSetArg (arglist[5], XmNeditMode, XmSINGLE_LINE_EDIT);
	trace->prntscr.text = XmCreateText (trace->prntscr.customize,"",arglist,6);
	XtManageChild (trace->prntscr.text);
	XtAddCallback (trace->prntscr.text, XmNactivateCallback, ps_print_req_cb, trace);
	y+=45;
	
	/* create label widget for notetext widget */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Note") );
	XtSetArg (arglist[1], XmNx, x);
	XtSetArg (arglist[2], XmNy, y);
	trace->prntscr.label = XmCreateLabel (trace->prntscr.customize,"",arglist,3);
	XtManageChild (trace->prntscr.label);
	y+=22;
	
	/* create the print note text widget */
	XtSetArg (arglist[0], XmNrows, 1);
	XtSetArg (arglist[1], XmNcolumns, 30);
	XtSetArg (arglist[2], XmNx, x);
	XtSetArg (arglist[3], XmNy, y);
	XtSetArg (arglist[4], XmNresizeHeight, FALSE);
	XtSetArg (arglist[5], XmNeditMode, XmSINGLE_LINE_EDIT);
	trace->prntscr.notetext = XmCreateText (trace->prntscr.customize,"notetext",arglist,6);
	XtManageChild (trace->prntscr.notetext);
	XtAddCallback (trace->prntscr.notetext, XmNactivateCallback, ps_print_req_cb, trace);
	y+=55;
	
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
	
	XtSetArg (arglist[0], XmNx, x);
	XtSetArg (arglist[1], XmNy, y);
	XtSetArg (arglist[2], XmNlabelString, XmStringCreateSimple ("Layout"));
	XtSetArg (arglist[3], XmNsubMenuId, trace->prntscr.size_menu);
	trace->prntscr.size_option = XmCreateOptionMenu (trace->prntscr.customize,"sizeo",arglist,4);
	XtManageChild (trace->prntscr.size_option);
	y+=45;

	/* Create all_signals button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Include off-screen signals"));
	XtSetArg (arglist[1], XmNx, x);
	XtSetArg (arglist[2], XmNy, y);
	XtSetArg (arglist[3], XmNshadowThickness, 1);
	trace->prntscr.all_signals = XmCreateToggleButton (trace->prntscr.customize,
							   "all_signals",arglist,4);
	XtManageChild (trace->prntscr.all_signals);
	y+=45;

	ps_range_create (trace, &(trace->prntscr.begin_range), trace->prntscr.customize,
			 &x, &y, "Begin Printing at:", 0);

	ps_range_create (trace, &(trace->prntscr.end_range), trace->prntscr.customize,
			 &x, &y, "End Printing at:", 1);

	/* Create Print button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Print") );
	XtSetArg (arglist[1], XmNx, x );
	XtSetArg (arglist[2], XmNy, y );
	trace->prntscr.print = XmCreatePushButton (trace->prntscr.customize, "print",arglist,3);
	XtAddCallback (trace->prntscr.print, XmNactivateCallback, ps_print_req_cb, trace);
	XtManageChild (trace->prntscr.print);
	
	/* create cancel button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Cancel") );
	XtSetArg (arglist[1], XmNx, x+160 );
	XtSetArg (arglist[2], XmNy, y );
	trace->prntscr.cancel = XmCreatePushButton (trace->prntscr.customize,"cancel",arglist,3);
	XtAddCallback (trace->prntscr.cancel, XmNactivateCallback, unmanage_cb, trace->prntscr.customize);
	XtManageChild (trace->prntscr.cancel);
	}
    
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
    XtSetValues (trace->prntscr.print,arglist,1);
    
    /* manage the popup on the screen */
    XtManageChild (trace->prntscr.customize);
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

    /* ranges */
    global->print_begin_time = ps_range_value ( &(trace->prntscr.begin_range) );
    global->print_end_time = ps_range_value ( &(trace->prntscr.end_range) );

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
    DTime	printtime;	/* Time current page starts on */
    char	pagenum[20];
    char	sstrg[MAXTIMELEN];
    char	estrg[MAXTIMELEN];
    
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
		if (horiz_pages == 1) 	sprintf (pagenum, "");	/* Just 1 page */
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

void ps_draw_grid (trace, psfile, printtime, grid_ptr, draw_numbers)
    TRACE	*trace;
    FILE	*psfile;
    DTime	printtime;	/* Time to start on */
    GRID	*grid_ptr;		/* Grid information */
    Boolean	draw_numbers;		/* Whether to print the times or not */
{ 
    char 	strg[MAXTIMELEN];	/* String value to print out */
    int		end_time;
    DTime	xtime;
    Position	x2;			/* Coordinate of current time */
    Position	yl,yh,yt;		/* Starting and ending points */

    if (grid_ptr->period < 1) grid_ptr->period = 1;	/* Prevents round-down to 0 causing infinite loop */

    /* Is the grid too small or invisible?  If so, skip it */
    if ((! grid_ptr->visible) || ((grid_ptr->period * global->res) < MIN_GRID_RES)) {
	return;
    }

    /* set the line attributes as the specified dash pattern */
    fprintf (psfile,"stroke\n[%d YTRN %d YTRN] 0 setdash\n",SIG_SPACE/2, trace->sighgt - SIG_SPACE/2);
    
    /* Start to left of right edge */
    xtime = printtime;
    end_time = printtime + (( trace->width - XMARGIN - global->xstart) / global->res);

    /* Move starting point to the right to hit where a grid line is aligned */
    xtime = ((xtime / grid_ptr->period) * grid_ptr->period) + (grid_ptr->alignment % grid_ptr->period);

    /* If possible, put one grid line inside the signal names */
    if (((grid_ptr->period * global->res) < 0)
	&& (xtime >= grid_ptr->period)) {
	xtime -= grid_ptr->period;
    }

    /* Other coordinates */
    yt = trace->height - 20;
    yl = trace->sighgt;
    yh = trace->height - trace->ystart + SIG_SPACE/4;

    /* Start grid */
    fprintf (psfile,"%d %d %d START_GRID\n", yh, yl, yt-10);

    /* Loop through grid times */
    for ( ; xtime <= end_time; xtime += grid_ptr->period) {
	x2 = ( ((xtime) - printtime) * global->res + global->xstart );	/* Similar to TIME_TO_XPOS (xtime) */

	/* compute the time value and draw it if it fits */
	if (draw_numbers) {
	    time_to_string (trace, strg, xtime, FALSE);
	}
	else {
	    strg[0] = '\0';
	}

	/* Draw if space, centered on grid line */
	fprintf (psfile,"%d (%s) GRID\n", x2, strg);
    }
}

void ps_draw_grids (trace, psfile, printtime)
    TRACE	*trace;
    FILE	*psfile;
    DTime	printtime;
{           
    int		grid_num;
    GRID	*grid_ptr;
    GRID	*grid_smallest_ptr;

    /* WARNING, major weedyness follows: */
    /* Determine which grid has the smallest period and only plot it's lines. */
    /* If we don't do this, labels may overlap */

    grid_smallest_ptr = &(trace->grid[0]);
    for (grid_num=0; grid_num<MAXGRIDS; grid_num++) {
	grid_ptr = &(trace->grid[grid_num]);
	if ( grid_ptr->visible
	    && ((grid_ptr->period * global->res) >= MIN_GRID_RES)	/* not too small */
	    && (grid_ptr->period < grid_smallest_ptr->period) ) {
	    grid_smallest_ptr = grid_ptr;
	}
    }


    /* Draw each grid */
    for (grid_num=0; grid_num<MAXGRIDS; grid_num++) {
	grid_ptr = &(trace->grid[grid_num]);
	ps_draw_grid (trace, psfile, printtime, grid_ptr, (grid_ptr == grid_smallest_ptr ) );
    }
}


void ps_draw (trace, psfile, sig_ptr, sig_end_ptr, printtime)
    TRACE	*trace;
    FILE	*psfile;
    SIGNAL	*sig_ptr;	/* Vertical signal to start on */
    SIGNAL	*sig_end_ptr;	/* Last signal to print */
    DTime	printtime;	/* Time to start on */
{
    int c=0,adj,ymdpt,xloc,xend,xstart,ystart;
    int y1,y2;
    SIGNAL_LW *cptr,*nptr;
    char strg[MAXVALUELEN];
    char vstrg[MAXVALUELEN];
    unsigned int value;
    int unstroked=0;		/* Number commands not stroked */
    
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

	/* Compute starting points for signal */
	xstart = global->xstart;
	switch ( cptr->sttime.state )
	    {
	  case STATE_0: ystart = y2; break;
	  case STATE_1: ystart = y1; break;
	  case STATE_U: ystart = ymdpt; break;
	  case STATE_Z: ystart = ymdpt; break;
	  case STATE_B32: ystart = ymdpt; break;
	  case STATE_B128: ystart = ymdpt; break;
	  default: printf ("Error: State=%d\n",cptr->sttime.state); break;
	    }
	
	/* output y information - note reverse from draw() due to y-axis */
	/* output starting positional information */
	fprintf (psfile,"%d %d %d %d %d START_SIG\n",
		 ymdpt, y1, y2, xstart, ystart);
	
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

		if (trace->busrep == HBUS)
		    sprintf (vstrg,"%x", value);
		else if (trace->busrep == OBUS)
		    sprintf (vstrg,"%o", value);

		/* Below evaluation left to right important to prevent error */
		if ( (sig_ptr->decode != NULL) &&
		    (value < sig_ptr->decode->numstates) &&
		    (sig_ptr->decode->statename[value][0] != '\0')) {
		    strcpy (strg, sig_ptr->decode->statename[value]);
		    fprintf (psfile,"%d (%s) (%s) STATE_B_FB\n",xloc,vstrg,strg);
		    }
		else {
		    fprintf (psfile,"%d (%s) STATE_B\n",xloc,vstrg);
		    }
		
		break;
		
	      case STATE_B128:
		if ( xloc > xend ) xloc = xend;

		value_to_string (trace, strg, cptr+1, ' ');

		fprintf (psfile,"%d (%s) STATE_B\n",xloc,vstrg);
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
    ps_draw_grids (trace, psfile, printtime);

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
    int		y1;
    
    if (DTPRINT_ENTRY) printf ("In ps_drawsig - filename=%s\n",trace->filename);
    
    /* don't draw anything if there is no file is loaded */
    if (!trace->loaded) return;
    
    /* loop thru all the visible signals */
    for (; sig_ptr && sig_ptr!=sig_end_ptr; sig_ptr = sig_ptr->forward) {

	/* calculate the location to start drawing the signal name */
	/* printf ("xstart=%d, %s\n",global->xstart, sig_ptr->signame); */
	
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
    char 	strg[MAXTIMELEN];
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
	xend = trace->width - XMARGIN;
	adj = printtime * global->res - global->xstart;

	/* Start cursor, for now == start grid */
	fprintf (psfile,"%d %d %d START_GRID\n", y2, y1, y2-8);

	for (csr_ptr = global->cursor_head; csr_ptr; csr_ptr = csr_ptr->next) {

	    /* check if cursor is on the screen */
	    if (csr_ptr->time > printtime) {
		
		/* draw the vertical cursor line */
		x1 = csr_ptr->time * global->res - adj;
		if (x1 > xend) break;	/* past end of screen, since sorted list no more to do */
		
		/* draw the cursor value */
		time_to_string (trace, strg, csr_ptr->time, FALSE);
		fprintf (psfile,"%d (%s) GRID\n", x1, strg);
		
		/* if there is a previous visible cursor, draw delta line */
		if ( csr_ptr->prev && csr_ptr->prev->time > printtime ) {

		    x2 = csr_ptr->prev->time * global->res - adj;
		    time_to_string (trace, strg, csr_ptr->time - csr_ptr->prev->time, TRUE);
		    fprintf (psfile,"%d %d %d (%s) CURSOR_DELTA\n", x1, x2, y2+7, strg);
		}
	    }
	}
    }
}

