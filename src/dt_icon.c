#ident "$Id$"
/******************************************************************************
 * DESCRIPTION: Dinotrace source: create Dinotrace icon
 *
 * This file is part of Dinotrace.
 *
 * Author: Wilson Snyder <wsnyder@wsnyder.org>
 *
 * Code available from: http://www.veripool.org/dinotrace
 *
 ******************************************************************************
 *
 * Some of the code in this file was originally developed for Digital
 * Semiconductor, a division of Digital Equipment Corporation.  They
 * gratefuly have agreed to share it, and thus the base version has been
 * released to the public with the following provisions:
 *
 *
 * This software is provided 'AS IS'.
 *
 * DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THE INFORMATION
 * (INCLUDING ANY SOFTWARE) PROVIDED, INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR ANY PARTICULAR PURPOSE, AND
 * NON-INFRINGEMENT. DIGITAL NEITHER WARRANTS NOR REPRESENTS THAT THE USE
 * OF ANY SOURCE, OR ANY DERIVATIVE WORK THEREOF, WILL BE UNINTERRUPTED OR
 * ERROR FREE.  In no event shall DIGITAL be liable for any damages
 * whatsoever, and in particular DIGITAL shall not be liable for special,
 * indirect, consequential, or incidental damages, or damages for lost
 * profits, loss of revenue, or loss of use, arising out of or related to
 * any use of this software or the information contained in it, whether
 * such damages arise in contract, tort, negligence, under statute, in
 * equity, at law or otherwise. This Software is made available solely for
 * use by end users for information and non-commercial or personal use
 * only.  Any reproduction for sale of this Software is expressly
 * prohibited. Any rights not expressly granted herein are reserved.
 *
 ******************************************************************************
 *
 * Changes made over the basic version are covered by the GNU public licence.
 *
 * Dinotrace is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * Dinotrace is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Dinotrace; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 *****************************************************************************/

#include "dinotrace.h"

/**********************************************************************/

/*
 * Editor's Note: Thanks go to Sally C. Barry, former employee of DEC,
 * for her painstaking effort in the creation of the infamous 'Dino'
 * bitmap icons.
 */

#define dino_icon_width 16
#define dino_icon_height 16
#define dino_icon_x_hot -1
#define dino_icon_y_hot -1
static char dino_icon_bits[] = {
    0x00, 0x00,
    0x1c, 0x00,
    0x3e, 0x00,
    0x70, 0x00,
    0x60, 0x00,
    0x61, 0xe0,
    0x77, 0xf8,
    0x7f, 0xfc,
    0x3f, 0xfe,
    0x3f, 0xff,
    0x1f, 0xfb,
    0x0e, 0x73,
    0x0c, 0x33,
    0x1c, 0x77,
    0x00, 0x06,
    0x00, 0x1c};

#define bigdino_icon_width 32
#define bigdino_icon_height 32
static char bigdino_icon_bits[] = {
    0x00, 0x00, 0x00, 0x00,
    0x07, 0xe0, 0x00, 0x00,
    0x0f, 0xb0, 0x00, 0x00,
    0x1f, 0xf8, 0x00, 0x00,
    0x3f, 0xf0, 0x00, 0x00,
    0x7e, 0x00, 0x00, 0x00,
    0x7c, 0x00, 0x00, 0x00,
    0x78, 0x00, 0x00, 0x00,
    0x78, 0x00, 0x00, 0x00,
    0x78, 0x07, 0xfc, 0x00,
    0x78, 0x3f, 0xff, 0x80,
    0x7c, 0x7f, 0xff, 0xe0,
    0x7e, 0xff, 0xff, 0xf0,
    0x7f, 0xff, 0xff, 0xf8,
    0x7f, 0xff, 0xff, 0xf8,
    0x3f, 0xff, 0xff, 0xfc,
    0x3f, 0xff, 0xff, 0xfc,
    0x1f, 0xff, 0xff, 0xfe,
    0x1f, 0xff, 0xff, 0xfe,
    0x0f, 0xff, 0xff, 0xbe,
    0x07, 0xff, 0xff, 0x1e,
    0x03, 0xf8, 0x7e, 0x1e,
    0x01, 0xf0, 0x3e, 0x1e,
    0x01, 0xe0, 0x1e, 0x1e,
    0x01, 0xe0, 0x1e, 0x1e,
    0x01, 0xe0, 0x1e, 0x1e,
    0x03, 0xe0, 0x3e, 0x1e,
    0x00, 0x00, 0x00, 0x3c,
    0x00, 0x00, 0x00, 0x3c,
    0x00, 0x00, 0x00, 0xf8,
    0x00, 0x00, 0x1f, 0xe0,
    0x00, 0x00, 0x00, 0x00};

static Pixmap    icon_make (
    Display		*display,
    Drawable		root,
    char		*data,
    Dimension		width,
    Dimension		height)
{
    XImage		ximage;
    GC			pgc;
    XGCValues		gcv;
    Drawable		pid;

    /* create the pixmap */
    pid = XCreatePixmap (display, root, width, height,
                        (unsigned int) DefaultDepth (display, 0));

    /* create a gc */
    gcv.foreground = BlackPixel (display, 0);
    gcv.background = WhitePixel (display, 0);
    pgc = XCreateGC (display, pid, GCForeground | GCBackground, &gcv);

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
    ximage.bits_per_pixel = 1;

    XPutImage (display, pid, pgc, &ximage, 0, 0, 0, 0, width, height);

    /* free gc */
    XFreeGC (display, pgc);

    return (pid);
}

void icon_dinos ()
{
    /*** create small dino pixmap from data ***/
    global->dpm = icon_make (global->display, DefaultRootWindow (global->display),
			     dino_icon_bits,dino_icon_width,dino_icon_height);

    /*** create big dino pixmap from data ***/
    global->bdpm = icon_make (global->display, DefaultRootWindow (global->display),
			      bigdino_icon_bits,bigdino_icon_width,bigdino_icon_height);
}
