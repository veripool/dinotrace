/******************************************************************************
 *
 * Filename:
 *     dt_grid.c
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
 *     AAG	14-Aug-90	Original Version
 *     AAG	22-Aug-90	Base Level V4.1
 *     AAG	29-Apr-91	Use X11 for Ultrix support
 *
 */
static char rcsid[] = "$Id$";

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <X11/Xlib.h>
#include <Xm/Xm.h>
#include <Xm/Xm.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/SelectioB.h>
#include <Xm/Text.h>
#include <Xm/Label.h>
#include <Xm/BulletinB.h>

#include "dinotrace.h"
#include "callbacks.h"

extern void	grid_customize_ok_cb(), grid_customize_apply_cb();


/****************************** AUTO GRIDS ******************************/

void	grid_calc_auto (trace, grid_ptr)
    TRACE	*trace;
    GRID	*grid_ptr;
{
    SIGNAL	*sig_ptr;
    SIGNAL_LW	*cptr;
    int		rise1=0, rise2=0, rise3=0;
    int		fall1=0, fall2=0, fall3=0;

    if (DTPRINT_ENTRY) printf ("In grid_calc_auto\n");

    /* Determine period, rise point and fall point of first signal */
    sig_ptr = (SIGNAL *)trace->firstsig;

    /* Skip phase_count, as it is a CCLI artifact */
    if (sig_ptr && !strncmp(sig_ptr->signame, "phase_count", 11)) sig_ptr=sig_ptr->forward;

    /* Can't do anything if don't have a signal yet */
    if (!sig_ptr) return;	

    cptr = sig_ptr->cptr;
    /* Skip first one, as is often not representative of period */
    if ( cptr->sttime.time != EOT) cptr += sig_ptr->lws;

    while ( cptr->sttime.time != EOT) {
	switch (cptr->sttime.state) {
	  case STATE_1:
	    if (!rise1) rise1 = cptr->sttime.time;
	    else if (!rise2) rise2 = cptr->sttime.time;
	    else if (!rise3) rise3 = cptr->sttime.time;
	    break;
	  case STATE_0:
	    if (!fall1) fall1 = cptr->sttime.time;
	    else if (!fall2) fall2 = cptr->sttime.time;
	    else if (!fall3) fall3 = cptr->sttime.time;
	    break;
	  case STATE_B32:
	  case STATE_B64:
	  case STATE_B96:
	    if (!rise1) rise1 = cptr->sttime.time;
	    else if (!rise2) rise2 = cptr->sttime.time;
	    else if (!rise3) rise3 = cptr->sttime.time;
	    if (!fall1) fall1 = cptr->sttime.time;
	    else if (!fall2) fall2 = cptr->sttime.time;
	    else if (!fall3) fall3 = cptr->sttime.time;
	    break;
	}
	cptr += sig_ptr->lws;
    }
    
    /* Set defaults based on changes */
    switch (grid_ptr->res_auto) {
      case AUTO:
	if (rise1 < rise2)	grid_ptr->spacing = rise2 - rise1;
	else if (fall1 < fall2) grid_ptr->spacing = fall2 - fall1;
	break;
    }
    
    /* Alignment */
    switch (trace->grid[0].align_auto) {
      case ASS:
	if (rise1) grid_ptr->alignment = rise1 % grid_ptr->spacing;
	break;
      case DEASS:
	if (fall1) grid_ptr->alignment = fall1 % grid_ptr->spacing;
	break;
    }
    
    if (DTPRINT_FILE) printf ("grid autoset signal %s align=%d %d\n", sig_ptr->signame,
       grid_ptr->align_auto, grid_ptr->res_auto);
    if (DTPRINT_FILE) printf ("grid rises=%d,%d,%d, falls=%d,%d,%d, spacing=%d, align=%d\n",
       rise1,rise2,rise3, fall1,fall2,fall3,
       grid_ptr->spacing, grid_ptr->alignment);
}

void	grid_calc_autos (trace)
    TRACE		*trace;
{
    int		grid_num;

    for (grid_num=0; grid_num<MAXGRIDS; grid_num++) {
	grid_calc_auto (trace, &(trace->grid[grid_num]));
    }

    draw_needed (trace);
}

/****************************** MENU OPTIONS ******************************/

void grid_res_cb (w, trace, cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT_ENTRY) printf ("In grid_res_cb - trace=%d\n",trace);

    /* input the grid resolution from the user */
    get_data_popup (trace,"Grid Res",IO_GRIDRES);
}

void grid_align_cb (w, trace, cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    if (DTPRINT_ENTRY) printf ("In grid_align_cb - trace=%d\n",trace);

    /* remove any previous events */
    remove_all_events (trace);

    /* process all subsequent button presses as grid aligns */
    add_event (ButtonPressMask, grid_align_ev);
    }

void grid_reset_cb (w, trace, cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    int		grid_num;
    GRID	*grid_ptr;

    if (DTPRINT_ENTRY) printf ("In grid_reset_cb - trace=%d\n",trace);

    /* reset the alignment back to zero and resolution to 100 */
    for (grid_num=0; grid_num<MAXGRIDS; grid_num++) {
	grid_ptr = &(trace->grid[grid_num]);

	grid_ptr->spacing = 100;
	grid_ptr->alignment = 0;
	grid_ptr->res_auto = AUTO;
	grid_ptr->align_auto = ASS;
	if (grid_num == 0) {
	    /* Enable only one grid by default */
	    grid_ptr->visible = TRUE;
	    grid_ptr->double_line = TRUE;
	}
	else {
	    grid_ptr->visible = FALSE;
	    grid_ptr->double_line = FALSE;
	}
     }

    grid_calc_autos (trace);

    /* cancel the button actions */
    remove_all_events (trace);
    }

void grid_align_ev (w, trace, ev)
    Widget		w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    DTime		time;

    if (DTPRINT_ENTRY) printf ("In grid_align_ev - trace=%d x=%d y=%d\n",trace,ev->x,ev->y);
    if (ev->type != ButtonPress || ev->button!=1) return;

    /* convert x value to a new grid_align (time) value */
    time = posx_to_time_edge (trace, ev->x, ev->y);
    if (time<0) return;

    trace->grid[0].alignment = time;

    draw_all_needed ();
    }

