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


#include <X11/DECwDwtApplProg.h>
#include <X11/Xlib.h>

#include "dinotrace.h"
#include "callbacks.h"



void
grid_res_cb( w, ptr, cb)
Widget			w;
DISPLAY_SB		*ptr;
DwtAnyCallbackStruct	*cb;
{

    if (DTPRINT) printf("In grid_res_cb - ptr=%d\n",ptr);

    /* input the grid resolution from the user */
    get_data_popup(ptr,"Grid Res",IO_GRIDRES);
}

void
grid_align_cb(w, ptr, cb)
Widget			w;
DISPLAY_SB		*ptr;
DwtAnyCallbackStruct	*cb;
{

    if (DTPRINT) printf("In grid_align_cb - ptr=%d\n",ptr);

    /* remove any previous events */
    remove_all_events(ptr);

    /* process all subsequent button presses as grid aligns */
    XtAddEventHandler(ptr->work,ButtonPressMask,TRUE,align_grid,ptr);

}

void
grid_reset_cb(w, ptr, cb)
Widget			w;
DISPLAY_SB		*ptr;
DwtAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In grid_reset_cb - ptr=%d\n",ptr);

    /* reset the alignment back to zero and resolution to 100 */
    ptr->grid_align = 0;
    ptr->grid_res = 100;

    update_signal_states (ptr);

    /* cancel the button actions */
    remove_all_events(ptr);

    /* redraw the screen */
    get_geometry(ptr);
    XClearWindow(ptr->disp,ptr->wind);
    drawsig(ptr);
    draw(ptr);

    return;
}

void
align_grid(w, ptr, ev)
Widget			w;
DISPLAY_SB		*ptr;
XButtonPressedEvent	*ev;
{
    int		i,j,time;

    if (DTPRINT) printf("In align_grid - ptr=%d x=%d y=%d\n",ptr,ev->x,ev->y);

    /* check if button has been clicked on trace portion of screen */
    if ( ev->x < ptr->xstart || ev->x > ptr->width - XMARGIN )
	return;

    /* convert x value to a new grid_align (time) value */
    ptr->grid_align = (ev->x + ptr->time * ptr->res - ptr->xstart) / ptr->res;

    /* redraw the screen with new cursors */
    get_geometry(ptr);
    XClearWindow(ptr->disp,ptr->wind);
    drawsig(ptr);
    draw(ptr);

    return;
}
