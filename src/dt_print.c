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
 *
 */


#include <stdio.h>

#ifdef VMS
#include <math.h>
#include <descrip.h>
#endif VMS

#include <X11/DECwDwtApplProg.h>
#include <X11/Xlib.h>

#include "dinotrace.h"
#include "callbacks.h"
#include "dinopost.h"



void
ps_dialog(w,ptr,cb)
Widget			w;
DISPLAY_SB		*ptr;
DwtAnyCallbackStruct	*cb;

{
    char		ps_trans_table[100];
    XtTranslations	ps_trans_parsed;
    static XtActionsRec	ps_action_table[] = 
    {
	{"ps_hit_return",	(XtActionProc)ps_hit_return},
	{NULL,			NULL}
    };

    if (DTPRINT) printf("In print_screen - ptr=%d\n",ptr);

    if (!ptr->prntscr.customize)
    {
	sprintf(ps_trans_table,"<KeyPress>0xff0d: ps_hit_return(%d)",ptr);
	XtAddActions(ps_action_table,1);
	ps_trans_parsed = XtParseTranslationTable(ps_trans_table);

	XtSetArg(arglist[0],DwtNdefaultPosition, TRUE);
	XtSetArg(arglist[1],DwtNwidth, 300);
	XtSetArg(arglist[2],DwtNheight, 150);
	XtSetArg(arglist[3],DwtNtitle, DwtLatin1String("Print Screen Menu"));
	XtSetArg(arglist[4], DwtNtextMergeTranslations,ps_trans_parsed);
	ptr->prntscr.customize = DwtDialogBoxPopupCreate(ptr->work,
							"",arglist,5);

	/* create label widget for text widget */
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("File Name") );
	XtSetArg(arglist[1], DwtNx, 10);
	XtSetArg(arglist[2], DwtNy, 5);
	ptr->prntscr.label = DwtLabelCreate(ptr->prntscr.customize,"",arglist,3);
	XtManageChild(ptr->prntscr.label);

	/* create the file name text widget */
	XtSetArg(arglist[0], DwtNrows, 1);
	XtSetArg(arglist[1], DwtNcols, 30);
	XtSetArg(arglist[2], DwtNx, 10);
	XtSetArg(arglist[3], DwtNy, 20);
	XtSetArg(arglist[4], DwtNresizeHeight, FALSE);
	ptr->prntscr.text = DwtSTextCreate(ptr->prntscr.customize,"",arglist,5);
	XtManageChild(ptr->prntscr.text);

	/* Create number of pages slider */
	XtSetArg(arglist[0], DwtNtitle, DwtLatin1String("Number of Pages") );
	XtSetArg(arglist[1], DwtNx, 20);
	XtSetArg(arglist[2], DwtNy, 50);
	XtSetArg(arglist[3], DwtNscaleWidth, 200);
	XtSetArg(arglist[4], DwtNvalue, ptr->numpag);
	XtSetArg(arglist[5], DwtNminValue, 1);
	XtSetArg(arglist[6], DwtNmaxValue, 50);
	dino_cb[0].proc = ps_numpag;
	dino_cb[0].tag = ptr;
	XtSetArg(arglist[7], DwtNvalueChangedCallback, dino_cb);
	ptr->prntscr.s1 = DwtScaleCreate(ptr->prntscr.customize,"",arglist,8);
	XtManageChild(ptr->prntscr.s1);

	/* Create Print button */
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Print") );
	XtSetArg(arglist[1], DwtNx, 10 );
	XtSetArg(arglist[2], DwtNy, 100 );
	dino_cb[0].proc = ps_print;
	dino_cb[0].tag = ptr;
	XtSetArg(arglist[3], DwtNactivateCallback, dino_cb);
	ptr->prntscr.b1 = DwtPushButtonCreate(ptr->prntscr.customize,
								"",arglist,4);
	XtManageChild(ptr->prntscr.b1);

	/* Create Print button */
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Print All") );
	XtSetArg(arglist[1], DwtNx, 50 );
	XtSetArg(arglist[2], DwtNy, 100 );
	dino_cb[0].proc = ps_print_all;
	dino_cb[0].tag = ptr;
	XtSetArg(arglist[3], DwtNactivateCallback, dino_cb);
	ptr->prntscr.b2 = DwtPushButtonCreate(ptr->prntscr.customize,
								"",arglist,4);
	XtManageChild(ptr->prntscr.b2);

	/* create cancel button */
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Cancel") );
	XtSetArg(arglist[1], DwtNx, 100 );
	XtSetArg(arglist[2], DwtNy, 100 );
	dino_cb[0].proc = ps_cancel;
	dino_cb[0].tag = ptr;
	XtSetArg(arglist[3], DwtNactivateCallback, dino_cb);
	ptr->prntscr.b3 = DwtPushButtonCreate(ptr->prntscr.customize,"",arglist,4);
	XtManageChild(ptr->prntscr.b3);
    }

    /* reset number of pages to one */
    ptr->numpag = 1;
    XtSetArg(arglist[0], DwtNvalue, ptr->numpag);
    XtSetValues(ptr->prntscr.s1,arglist,1);

    /* if a file has been read in, make printscreen buttons active */
    if (ptr->filename[0] == '\0')
	XtSetArg(arglist[0],DwtNsensitive, FALSE);
    else
	XtSetArg(arglist[0],DwtNsensitive, TRUE);
    XtSetValues(ptr->prntscr.b1,arglist,1);
    XtSetValues(ptr->prntscr.b2,arglist,1);

    /* manage the popup on the screen */
    XtManageChild(ptr->prntscr.customize);
}

void
ps_hit_return(w,ev,params,numparams)
Widget		w;
XEvent		*ev;
char		**params;
int		*numparams;
{
    if (DTPRINT) printf("In ps_hit_return\n");

    /* null routine to prevent <cr>'s from disturbing ps filename */

    return;
}

void
ps_numpag(w,ptr,cb)
Widget		w;
DISPLAY_SB	*ptr;
DwtSelectionCallbackStruct *cb;
{
    int		ns,max;

    if (DTPRINT) printf("In ps_numpag - ptr=%d value passed=%d\n",ptr,cb->value);

    /* calculate ns per page */
    ns = (int)((ptr->width - ptr->xstart)/ptr->res);

    /* calculate max number of pages from current time to end */
    max = (ptr->end_time - ptr->time)/ns;
    if ( (ptr->end_time - ptr->time) % ns )
	max++;;

    /* update num pages making sure user didn't select too many */
    ptr->numpag = MIN((int)cb->value,max);
}

void
ps_print(w,ptr,cb)
Widget			w;
DISPLAY_SB		*ptr;
DwtAnyCallbackStruct	*cb;
{
    FILE	*psfile;
    int		i,ns;
    char	*psfilename;

    /* text descriptor structure */

    struct textstr {
	short int	dsc$w_length;
	unsigned char	dsc$b_dtype;
	unsigned char	dsc$b_class;
	char		*dsc$a_pointer;
    } date;

#ifdef VMS
    date.dsc$w_length = 0;
    date.dsc$b_dtype = DSC$K_DTYPE_T;
    date.dsc$b_class = DSC$K_CLASS_S;
#endif VMS

    if (DTPRINT) printf("In ps_print - ptr=%d\n",ptr);

    /* hide the print screen window */
    XtUnmanageChild(ptr->prntscr.customize);

    /* open output file */
    psfilename = DwtSTextGetString(ptr->prntscr.text);
    psfile = fopen(psfilename,"w");
    if (psfile == NULL)
    {
	sprintf(message,"Bad Filename: %s\n",psfilename);
	dino_message_ack(ptr,message);
	return;
    }

    /* get the date */

#ifdef VMS
    date.dsc$a_pointer = malloc(24*sizeof(char));
    date.dsc$w_length = 23;
    sys$asctim(0,&date,0,0);
    date.dsc$a_pointer[24] = '\0';
#endif VMS

    /* include the postscript macro information */
    fputs(dinopost,psfile);

    /* print out each page */
    for (i=0;i<ptr->numpag && ptr->filename[0] != '\0';i++)
    {

	ns = (int)((ptr->width - ptr->xstart)/ptr->res);

	/* output the page header macro */
	fprintf(psfile,"%d %d %d %d (%s) (%s) %d PAGEHDR\n",
	    ptr->time + ns,                                  /* end time */
	    ptr->time,					     /* start time */
	    ns,						     /* resolution */
	    ptr->numsigvis,                                  /* num signals */
	    date.dsc$a_pointer,                              /* time & date */
	    ptr->filename,                                   /* filename */
	    i+1);                                            /* page number */

	/* output the page scaling and rf time */
	fprintf(psfile,"%d %d %d PAGESCALE\n",ptr->height,ptr->width,ptr->sigrf);

	/* draw the signal names and the traces */
	ps_drawsig(ptr,psfile);
	ps_draw(ptr,psfile);

	/* print the page */
	fprintf(psfile,"stroke\nshowpage\n");

	/* if not the last page, draw the next page */
	if (i < ptr->numpag-1)
	{
	    /* increment to next page */
	    ptr->time += ns;
	    new_time(ptr);

	    /* redraw the display */
	    get_geometry(ptr);
	    XClearWindow(ptr->disp,ptr->wind);
	    draw(ptr);
	    drawsig(ptr);
	}

    }

    /* free the date memory */
    free(date.dsc$a_pointer);

    /* free the memory from getting the filename */
    XtFree(psfilename);

    /* close output file */
    fclose(psfile);
}

void
ps_print_all(w,ptr,cb)
Widget			w;
DISPLAY_SB		*ptr;
DwtAnyCallbackStruct	*cb;
{
    FILE	*psfile;
    int		i,ns;
    char	*psfilename;

    if (DTPRINT) printf("In ps_print_all - ptr=%d\n",ptr);

    /* reset the drawing back to the beginning */
    ptr->time = ptr->start_time;
    new_time(ptr);

    /* redraw the display */
    get_geometry(ptr);
    XClearWindow(ptr->disp,ptr->wind);
    draw(ptr);
    drawsig(ptr);

    /* calculate ns per page */
    ns = (int)((ptr->width - ptr->xstart)/ptr->res);

    /* calculate number of pages needed to draw the entire trace */
    ptr->numpag = (ptr->end_time - ptr->start_time)/ns;
    if ( (ptr->end_time - ptr->start_time) % ns )
	ptr->numpag++;;

    /* draw the entire trace */
    ps_print(NULL,ptr,NULL);
}

void
ps_cancel(w,ptr,cb)
Widget			w;
DISPLAY_SB		*ptr;
DwtAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In ps_cancel - ptr=%d\n",ptr);

    /* hide the window */
    XtUnmanageChild(ptr->prntscr.customize);
}

void
ps_reset(w,ptr,cb)
Widget			w;
DISPLAY_SB		*ptr;
DwtAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In ps_reset - ptr=%d",ptr);

    /* ADD RESET CODE !!! */

    return;
}

ps_draw(ptr,psfile)
DISPLAY_SB	*ptr;
FILE		*psfile;
{
    int c=0,i,j,k=2,d,cnt,adj,ymdpt,inc,xt,yt,xloc,xend,len,mid,xstart,ystart;
    float iff,xlocf,xtimf;
    SIGNAL_LW *cptr,*nptr;
    SIGNAL_SB *tmp_sig_ptr;
    char tmp[32];

    if (DTPRINT) printf("In draw - filename=%s\n",ptr->filename);

    xend = ptr->width - XMARGIN;
    adj = ptr->time * ptr->res - ptr->xstart;

    if (DTPRINT) printf("ptr->res=%f adj=%d\n",ptr->res,adj);

    /* Loop and draw each signal individually */
    tmp_sig_ptr = ptr->startsig;
    for (i=0;i<ptr->numsigvis;i++)
    {
	y1 = ptr->height - ptr->ystart - c * ptr->sighgt - SIG_SPACE;
	ymdpt = y1 - (int)(ptr->sighgt/2) + SIG_SPACE;
	y2 = y1 - ptr->sighgt + 2*SIG_SPACE;
	cptr = tmp_sig_ptr->cptr;
	c++;
	xloc = 0;

	/* output y information - note reverse from draw() due to y-axis */
	fprintf(psfile,"/y1 %d YADJ def /ym %d YADJ def /y2 %d YADJ def\n",
			y2,ymdpt,y1);

	/* Compute starting points for signal */
	xstart = ptr->xstart;
	switch( cptr->state )
	{
	    case STATE_0: ystart = y2; break;
	    case STATE_1: ystart = y1; break;
	    case STATE_U: ystart = ymdpt; break;
	    case STATE_Z: ystart = ymdpt; break;
	    case STATE_B32: ystart = ymdpt; break;
	    case STATE_B64: ystart = ymdpt; break;
	    case STATE_B96: ystart = ymdpt; break;
	    default: printf("Error: State=%d\n",cptr->state); break;
	}

	/* output starting positional information */
	fprintf(psfile,"%d %d START\n",xstart,ystart);

	/* Loop as long as the time and end of trace are in current screen */
	while ( cptr->time != EOT && xloc < xend )
	{
	    /* find the next transition */
	    nptr = cptr + tmp_sig_ptr->inc;

	    /* if next transition is the end, don't draw */
	    if (nptr->time == EOT) break;

	    /* find the x location for the end of this segment */
	    xloc = nptr->time * ptr->res - adj;

	    /* Determine what the state of the signal is and build transition */
	    switch( cptr->state )
	    {
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
		    if (ptr->busrep == HBUS)
                      sprintf(tmp,"%X",*((unsigned int *)cptr+1));
		    else if (ptr->busrep == OBUS)
                      sprintf(tmp,"%o",*((unsigned int *)cptr+1));
		    fprintf(psfile,"%d (%s) STATE_B32\n",xloc,tmp);
                    break;

	        case STATE_B64: if ( xloc > xend ) xloc = xend;
		    if (ptr->busrep == HBUS)
                      sprintf(tmp,"%X %08X",*((unsigned int *)cptr+1),
					  *((unsigned int *)cptr+2));
		    else if (ptr->busrep == OBUS)
                      sprintf(tmp,"%o %o",*((unsigned int *)cptr+1),
					  *((unsigned int *)cptr+2));

		    fprintf(psfile,"%d (%s) STATE_B32\n",xloc,tmp);
                    break;

	        case STATE_B96: if ( xloc > xend ) xloc = xend;
		    if (ptr->busrep == HBUS)
                      sprintf(tmp,"%X %08X %08X",*((unsigned int *)cptr+1),
					         *((unsigned int *)cptr+2),
					         *((unsigned int *)cptr+2));
		    else if (ptr->busrep == OBUS)
                      sprintf(tmp,"%o %o %o",*((unsigned int *)cptr+1),
					     *((unsigned int *)cptr+2),
					     *((unsigned int *)cptr+2));
		    fprintf(psfile,"%d (%s) STATE_B32\n",xloc,tmp);
                    break;

	        default: printf("Error: State=%d\n",cptr->state); break;
	    } /* end switch */

	    cptr += tmp_sig_ptr->inc;
	}

	/* get out of loop if ptr->numsigvis > signals left */
	if (tmp_sig_ptr->forward == NULL) break;

	tmp_sig_ptr = tmp_sig_ptr->forward;

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
    yt = ptr->height - 20;
    y1 = ptr->height - ptr->ystart + SIG_SPACE/4;
    y2 = ptr->sighgt;

    /* create the dash pattern for the vertical grid lines */
    tmp[0] = SIG_SPACE/2;
    tmp[1] = ptr->sighgt - tmp[0];

    /* set the line attributes as the specified dash pattern */
    fprintf(psfile,"stroke\n[%d YTRN %d YTRN] 0 setdash\n",tmp[0],tmp[1]);

    /* check if there is a reasonable amount of increments to draw the time grid */
    if ( ((float)xend - xlocf)/(ptr->grid_res*ptr->res) < MIN_GRID_RES )
    {
        for (iff=xlocf;iff<(float)xend; iff+=ptr->grid_res*ptr->res)
        {
	    /* compute the time value and draw it if it fits */
	    sprintf(tmp,"%d",x1);
	    if ( (int)iff - i >= XTextWidth(ptr->text_font,tmp,strlen(tmp)) + 5 )
	    {
		fprintf(psfile,"%d XADJ %d YADJ MT (%s) show\n",
					(int)iff,yt-10,tmp);
	        i = (int)iff;
	    }

	    /* if the grid is visible, draw a vertical dashed line */
	    if (ptr->grid_vis)
	    {
		fprintf(psfile,"%d XADJ %d YADJ MT %d XADJ %d YADJ LT\n",
				(int)iff,y1,(int)iff,y2);
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
    fprintf(psfile,"stroke [] 0 setdash\n");

    /* draw the cursors if they are visible */
    if ( ptr->cursor_vis )
    {
	/* initial the y values for drawing */
	y1 = ptr->height - 25 - 10;
	y2 = ptr->height - ( (int)((ptr->height-ptr->ystart)/ptr->sighgt)-1) *
			ptr->sighgt - ptr->sighgt/2 - ptr->ystart - 2;
	for (i=0; i < ptr->numcursors; i++)
	{
	    /* check if cursor is on the screen */
	    if (ptr->cursors[i] > ptr->time)
	    {
		/* draw the vertical cursor line */
		x1 = ptr->cursors[i] * ptr->res - adj;
		fprintf(psfile,"%d XADJ %d YADJ MT %d XADJ %d YADJ LT\n",
			x1,y1,x1,y2);

		/* draw the cursor value */
		sprintf(tmp,"%d",ptr->cursors[i]);
 		len = XTextWidth(ptr->text_font,tmp,strlen(tmp));
		fprintf(psfile,"%d XADJ %d YADJ MT (%s) show\n",
			x1-len/2,y2-8,tmp);

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
			fprintf(psfile,"%d XADJ %d YADJ MT %d XADJ %d YADJ LT\n",
				x2,y2+5,mid-len/2-2,y2+5);
			fprintf(psfile,"%d XADJ %d YADJ MT %d XADJ %d YADJ LT\n",
				mid+len/2+2,y2+5,x1,y2+5);

			fprintf(psfile,"%d XADJ %d YADJ MT (%s) show\n",
				mid-len/2,y2+2,tmp);
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

int
ps_drawsig(ptr,psfile)
DISPLAY_SB	*ptr;
FILE		*psfile;
{
    SIGNAL_SB *tmp_sig_ptr;
    int c=0,i,ymdpt;

    /* don't draw anything if there is no file is loaded */
    if (ptr->filename[0] == '\0') return;

    /* initialize the signal pointer to the first visible one */
    tmp_sig_ptr = ptr->startsig;

    /* loop thru all the visible signals */
    for (i=0;i<ptr->numsigvis;i++)
    {
	/* calculate the location to start drawing the signal name */
	x1 = ptr->xstart - XTextWidth(ptr->text_font,tmp_sig_ptr->signame,
		strlen(tmp_sig_ptr->signame)) - 10;

	/* calculate the y location to draw the signal name and draw it */
	y1 = ptr->height - ptr->ystart - c * ptr->sighgt - SIG_SPACE;
	ymdpt = y1 - (int)(ptr->sighgt/2) + SIG_SPACE;

	fprintf(psfile,"%d XADJ %d YADJ 3 sub MT (%s) RIGHTSHOW\n",x1,ymdpt,
			tmp_sig_ptr->signame);
	c++;

	/* get out of loop if ptr->numsigvis > signals left */
	if (tmp_sig_ptr->forward == NULL) break;

	/* get next signal pointer */
	tmp_sig_ptr = tmp_sig_ptr->forward;
    }
}

