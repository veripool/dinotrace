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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <X11/Xlib.h>
#include <Xm/Xm.h>

#include "dinotrace.h"
#include "callbacks.h"



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
    if (DTPRINT_ENTRY) printf ("In grid_reset_cb - trace=%d\n",trace);

    /* reset the alignment back to zero and resolution to 100 */
    trace->grid_align = 0;
    trace->grid_res = 100;

    update_signal_states (trace);

    /* cancel the button actions */
    remove_all_events (trace);

    /* redraw the screen */
    redraw_all (trace);
    }

void grid_align_ev (w, trace, ev)
    Widget		w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    DTime		time;

    if (DTPRINT_ENTRY) printf ("In grid_align_ev - trace=%d x=%d y=%d\n",trace,ev->x,ev->y);

    /* convert x value to a new grid_align (time) value */
    time = posx_to_time_edge (trace, ev->x, ev->y);
    if (time<0) return;

    trace->grid_align = time;

    /* redraw the screen with new cursors */
    redraw_all (trace);
    }
