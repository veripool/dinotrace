#ident "$Id$"
/******************************************************************************
 * DESCRIPTION: Dinotrace source: main routine and documentation
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
 * the Free Software Foundation; either version 3, or (at your option)
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

#include <X11/Xutil.h>

#include "functions.h"

#include "dist_date.h"

/**********************************************************************/

Boolean_t	DTDEBUG=FALSE;		/* Debugging mode */
uint_t		DTPRINT=0;		/* Information printing mode */
int		DebugTemp=0;		/* Temp value for trying things */
uint_t		file_format=FF_VERILOG;	/* Type of trace to support */
char		message[1000];		/* generic string for messages */
XGCValues	xgcv;
Arg		arglist[20];
Global_t	*global;
/* filetypes must be in the same order that the FF_* defines are */
struct st_filetypes filetypes[FF_NUMFORMATS] = {
    { 0, "Auto",		"?",	"*.*"		},
    { 1, "DECSIM",		"TRA",	"*.tra*"	},
    { 1, "Tempest CCLI",	"BT",	"*.bt*"		},
    { 1, "Verilog VCD",		"DMP",	"*.vcd*"	},
    { 1, "Verilog VPD+",	"VPD",	"*.vpd*"	},
    { 0, "DECSIM Binary",	"TRA",	"*.tra*"	},
    { 0, "DECSIM Ascii",	"TRA",	"*.tra*"	},
};

char *events[40] = {"","", "KeyPress", "KeyRelease", "ButtonPress", "ButtonRelease", "MotionNotify",
		    "EnterNotify", "LeaveNotify", "FocusIn", "FocusOut", "KeymapNotify", "Expose", "GraphicsExpose",
		    "NoExpose", "VisibilityNotify", "CreateNotify", "DestroyNotify", "UnmapNotify", "MapNotify",
		    "MapRequest", "ReparentNotify", "ConfigureNotify", "ConfigureRequest", "GravityNotify",
		    "ResizeRequest", "CirculateNotify", "CirculateRequest", "PropertyNotify", "SelectionClear",
		    "SelectionRequest", "SelectionNotify", "ColormapNotify", "ClientMessage", "MappingNotify",
		    "LASTEvent"};

/**********************************************************************/

void version(), usage();

int    main (
    int		argc,
    char	**argv)
{
    int		i;
    Boolean_t	sync = FALSE;
    Boolean_t	opened_a_file = FALSE;
    Trace_t	*trace;

    /* Create global structure */
    init_globals ();

    /* Parse parameters */
#define shift {argv[i][0]='\0'; ++i; }
    for (i=1; i<argc; )  {
	/*printf ("Arg %d of %d: '%s'\n", i, argc, argv[i]);*/
	if (argv[i][0]!='-') {
	    /* Filename */
	    i++;
	}
	else {
	    char *sw = argv[i];
	    /* Skip the - */
	    sw++;
	    /* Allow gnu -- switches */
	    if (*sw=='-') sw++;

	    /* Switch tests */
	    if ( !strcmp (sw, "debug") ) {
		DTDEBUG = TRUE;
	    }
	    else if ( !strcmp (sw, "version") ) {
		version();
		exit(0);
	    }
	    else if ( !strcmp (sw, "print") ) {
		if ((i+1)<argc && isdigit(argv[i+1][0])) {
		    shift;
		    sscanf (sw, "%x", & DTPRINT );
		}
		else DTPRINT = ~0;
	    }
	    else if ( !strcmp (sw, "tempest") ) {
		file_format = FF_TEMPEST;
	    }
	    else if ( !strcmp (sw, "decsim")
		     || !strcmp (sw, "decsim_z") ) {
		file_format = FF_DECSIM;
	    }
	    else if ( !strcmp (sw, "verilog") ) {
		file_format = FF_VERILOG;
	    }
	    else if ( !strcmp (sw, "vpd") ) {
		file_format = FF_VERILOG_VPD;
	    }
	    else if ( !strcmp (sw, "sync") ) {
		sync = TRUE;
	    }
	    else if ( !strcmp (sw, "geometry") && (i+1)<argc ) {
		shift;
		config_parse_geometry (argv[i], & (global->start_geometry));
	    }
	    else if ( !strcmp (sw, "res") && (i+1)<argc ) {
		float res;
		shift;
		sscanf (argv[i],"%f",&res);
		global->res = RES_SCALE/ (float)res;
		global->res_default = FALSE;
	    }
	    else if ( !strcmp (sw, "noconfig") ) {
		int cfg_num;
		for (cfg_num=0; cfg_num<MAXCFGFILES; cfg_num++) {
		    global->config_enable[cfg_num] = FALSE;
		}
	    }
            else if ( !strcmp (sw, "simview") ) {
		/* Enable use of SimView */
		/* Next parameter is associates with "-simview" */
		shift;
		if (i >= argc) {
		    printf ("Error: -simview requires a second argument.\n");
		    exit(1);
		}
		/* Indicate the SimView is used by allocating a structure for it. */
		global->simview_info_ptr = (SimViewInfo_t*)XtMalloc (sizeof (SimViewInfo_t));
		/* Store the simview argument for future SimView initialization.
		 * We can't initialize SimView now because Xt has not been initialized yet. */
		global->simview_info_ptr->application_name_with_args = strdup(argv[i]);
		global->simview_info_ptr->handshake = TRUE;
            }
	    else {
		printf ("Invalid %s Option: %s\n", DTVERSION, sw);
		exit (-1);
	    }
	    shift;
	}
    }

    /* quick structure portability check */
#ifndef lint	/* constant in conditional context */
    if ((sizeof (Value_t) != 10*sizeof (uint_t))
	|| (sizeof (uint_t) != 4)) {
	printf ("%%E, Internal structure portability problem %d!=%d/4.\n",
		(int)sizeof (uint_t), (int)sizeof (Value_t));
    }

    /* FAILS on MIPS Ultrix, so don't rely on this:
    if ((1<<32) != 0) {
	printf ("%%E, Internal shifting portability problem %d!=%d.\n", (1<<32), 0);
	}
	*/

#endif

    /* info in case someone mails me the print listing */
    if (DTPRINT) {
	version();
    }

    /* create global information */
    create_globals (0, argv, sync);

    /* Initialize SimView. */
    if (global->simview_info_ptr) {
	simview_init (global->simview_info_ptr->application_name_with_args);
    }

    /* Load config options (such as file_format) */
    /* Create a temporary trace structure for this, as config will want to write to the trace structure */
    trace = malloc_trace ();
    config_read_defaults (trace, FALSE);

    /* Make the temporary trace the deleted signal trace */
    strcpy (trace->dfile.filename, "DELETED");
    global->deleted_trace_head = trace;
    global->trace_head = NULL;

    /* create the main dialog window */
    trace = create_trace (global->start_geometry.width, global->start_geometry.height,
			  global->start_geometry.x, global->start_geometry.y);

    /* expiration check */
#if EXPIRATION
    if ((DIST_DATE + (EXPIRATION)) < time (NULL)) {
	XSync (global->display,0);
	dino_information_ack (trace, "This version of Dinotrace has expired.\n\
Please install a newer version.\n\
See the Help menu for more information.");
    }
#endif

    draw_all_needed ();

    /* Load up the file on the command line, if any */
    /* Parse filenames */
    for (i=1; i<argc; i++)  {
	if ( argv[i][0] && argv[i][0] != '-' ) {
	    if (! opened_a_file) {
		opened_a_file = TRUE;
	    }
	    else {
		trace = trace_create_split_window (trace);
	    }
	    XSync (global->display,0);
	    strcpy (trace->dfile.filename, argv[i]);
	    fil_read (trace);
	}
    }

    socket_set_os_signals ();

    /* loop forever */
    /*This code ~= XtAppMainLoop (global->appcontext);*/
    while (1) {
	XEvent event;

	if (global->redraw_needed && !XtAppPending (global->appcontext)
	    && (!global->redraw_manually || (global->redraw_needed & GRD_MANUAL))) {
	    draw_perform();
	}

	XtAppNextEvent (global->appcontext, &event);
	/*if (DTPRINT) { printf ("[Event %s] ", events[event.type]);fflush(stdout); }*/
	XtDispatchEvent (&event);
    }
}

void usage()
{
    printf ("\n%s\n\n", help_message ());
    printf ("DINOTRACE\t[-switches...]  [trace_name] [trace_name] ...\n");
    printf ("Switches:\n");
    /*
    printf ("\tDebug:\t-debug\t\tEnable debugging menus.\n");
    printf ("\t\t-print value\tPrint debugging information.\n");
    */
    printf ("\tFormat:\t-decsim\t\tRead traces in DECSIM format.\n");
    printf ("\t\t-tempest\tRead traces in Tempest format.\n");
    printf ("\t\t-verilog\tRead traces in Verilog format.\n");
    printf ("\tConfig:\t-noconfig\tSkip reading global config files.\n");
    printf ("\tX-11:\t-geometry XxY+x+y  Specify starting geometry.\n\n");
    printf ("\t\t-display\tSpecify display device.\n\n");
    exit (10);
}

void version()
{
    printf ("%s\nDistributed %s\nConfigured %s for %s\n",
	    DTVERSION, DIST_DATE_STRG, CONFIG_DATE_STRG, HOST);
}

char	*help_message ()
{
    static char msg[2000];

    sprintf (msg,
	     "%s\n\nDistributed %s\n\
Configured %s for %s\n\
\n\
Please see the help menu or %sdinotrace.txt for documentation.\n\
\n\
Downloads and documentation at:\n\
http://www.veripool.org/dinotrace\n\
\n\
Written by Wilson Snyder\n\
<wsnyder@wsnyder.org>\n\
\n\
Copyright 1998-2009 by Wilson Snyder\n\
Copyright 1993-1997 by Digital Equipment Corporation.\n\
This software is covered by the GNU Public License,\n\
and is WITHOUT ANY WARRANTY.\n\
\n\
\n\
For configuration information, Dinotrace reads in order:\n\
     %sdinotrace.dino\n\
     %s\n\
     %sdinotrace.dino\n\
     %sdinotrace.dino\n\
     %sCURRENT_TRACE_NAME.dino\n",
	     DTVERSION,
	     DIST_DATE_STRG,
	     CONFIG_DATE_STRG,
	     HOST,
#ifdef VMS
	     "DINODISK:",
	     "DINODISK:",
	     "DINOCONFIG:",
	     "SYS$LOGIN:",
	     "[current.trace.directory]",
	     "[current.trace.directory]"
#else
	     "$DINODISK/",
	     "$DINODISK/",
	     "$DINOCONFIG",
	     "~/",
	     "/current/trace/directory/",
	     "/current/trace/directory/"
#endif
	     );

    return (msg);
}



