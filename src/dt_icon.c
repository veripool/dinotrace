/******************************************************************************
 *
 * Filename:
 *     dt_icon.c
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
 *     AAG	 5-Jul-89	Original Version
 *     AAG	22-Aug-90	Base Level V4.1
 *     AAG	29-Apr-91	Use X11 for Ultrix support
 *
 */


#include <X11/DECwDwtApplProg.h>
#include <X11/Xlib.h>

#include "dinotrace.h"

Pixmap
make_icon(disp, root, data, width, height) 
Display			*disp;
Drawable		root;
short			*data;
Dimension		width,height;
{
    XImage		ximage;
    GC			pgc;
    XGCValues		gcv;
    Pixmap		pid;

    /* create the pixmap */
    pid = XCreatePixmap(disp, root, width, height,
                        (unsigned int) DefaultDepth(disp, 0));

    /* create a gc */
    gcv.foreground = BlackPixel(disp, 0);
    gcv.background = WhitePixel(disp, 0);
    pgc = XCreateGC(disp, pid, GCForeground | GCBackground, &gcv);

    /* create the ximage structure */
    ximage.height = height;
    ximage.width = width;
    ximage.xoffset = 0;
    ximage.format = XYBitmap;
    ximage.data = (char *)data;
    ximage.byte_order = MSBFirst;
    ximage.bitmap_unit = 16; 
    ximage.bitmap_bit_order = MSBFirst;
    ximage.bitmap_pad = 16;
    ximage.bytes_per_line = (width+15)/16 * 2;
    ximage.depth = 1;

    /* put image into the pixmap */
    XPutImage(disp, pid, pgc, &ximage, 0, 0, 0, 0, width, height);

    /* free gc */
    XFreeGC(disp, pgc);

    return(pid);
}

