#ident "$Id$"
/******************************************************************************
 * dinotrace.c --- main routine and documentation
 *
 * This file is part of Dinotrace.  
 *
 * Author: Wilson Snyder <wsnyder@world.std.com> or <wsnyder@ultranet.com>
 *
 * Code available from: http://www.ultranet.com/~wsnyder/dinotrace
 *
 ******************************************************************************
 *
 * Some of the code in this file was originally developed for Digital
 * Semiconductor, a division of Digital Equipment Corporation.  They
 * gratefuly have agreed to share it, and thus the bas version has been
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

#include <X11/Xutil.h>

#include "functions.h"

#if HAVE_COMPILE_DATE_H
#include "compile_date.h"
#else
#define COMPILE_DATE_STRG "Unknown"
#endif

/**********************************************************************/

Boolean_t	DTDEBUG=FALSE;		/* Debugging mode */
uint_t		DTPRINT=0;		/* Information printing mode */
int		DebugTemp=0;		/* Temp value for trying things */
uint_t		file_format=FF_VERILOG;	/* Type of trace to support */
char		message[1000];		/* generic string for messages */
XGCValues	xgcv;
Arg		arglist[20];
Global		*global;
/* filetypes must be in the same order that the FF_* defines are */
struct st_filetypes filetypes[FF_NUMFORMATS] = {
    { 0, "Auto",		"?",	"*.*"		},
    { 1, "DECSIM",		"TRA",	"*.tra*"	},
    { 1, "Tempest CCLI",	"BT",	"*.bt*"		},
    { 1, "Verilog",		"DMP",	"*.dmp*"	},
    { 0, "DECSIM Binary",	"TRA",	"*.tra*"	},
    { 0, "DECSIM Ascii",	"TRA",	"*.tra*"	},
};

void version(), usage();

int    main (
    uint_t	argc,
    char	**argv)
{
    uint_t	i;
    Boolean_t	sync = FALSE;
    Boolean_t	opened_a_file = FALSE;
    Trace	*trace;
    
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
	    }
	    else if ( !strcmp (sw, "print") ) {
		if ((i+1)<argc && isdigit(argv[i+1][0])) {
		    shift;
		    sscanf (sw, "%x", & DTPRINT );
		}
		else DTPRINT = -1;
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

    /* create global information */
    create_globals (0, argv, sync);

    /* Load config options (such as file_format) */
    /* Create a temporary trace structure for this, as config will want to write to the trace structure */
    trace = malloc_trace ();
    config_read_defaults (trace, TRUE);

    /* Make the temporary trace the deleted signal trace */
    strcpy (trace->filename, "DELETED");
    global->deleted_trace_head = trace;
    global->trace_head = NULL;

    /* create the main dialog window */
    trace = create_trace (global->start_geometry.width, global->start_geometry.height, 
			  global->start_geometry.x, global->start_geometry.y);
    
    /* expiration check */
#if EXPIRATION
    if ((COMPILE_DATE + (EXPIRATION)) < time (NULL)) {
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
	    strcpy (trace->filename, argv[i]);
	    fil_read (trace);
	}
    }

    /* loop forever */
    /*This code ~= XtAppMainLoop (global->appcontext);*/
    while (1) {
	XEvent event;

	if (global->redraw_needed && !XtAppPending (global->appcontext)
	    && (!global->redraw_manually || (global->redraw_needed & GRD_MANUAL))) {
	    draw_perform();
	}

	XtAppNextEvent (global->appcontext, &event);
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
    printf ("%s\n", DTVERSION);
    exit (0);
}

char	*help_message ()
{
    static char msg[2000];

    sprintf (msg,
	     "%s\n\
Compiled %s for %s\n\
\n\
Written by Wilson Snyder\n\
<wsnyder@world.std.com> or <wsnyder@ultranet.com>\n\
\n\
Copyright 1993,1994,1995,1996 by Digital Equipment Corporation.\n\
Copyright 1997,1998 by Wilson Snyder\n\
This software is covered by the GNU Public License\n\
\n\
Please see %sdinotrace.txt for documentation.\n\
\n\
A complete Dinotrace kit is available on:\n\
http://www.ultranet.com/~wsnyder/dinotrace\n\
\n\
For configuration information, Dinotrace reads in order:\n\
     %sdinotrace.dino\n\
     %s\n\
     %sdinotrace.dino\n\
     %sdinotrace.dino\n\
     %sCURRENT_TRACE_NAME.dino\n",
	     DTVERSION,
	     COMPILE_DATE_STRG,
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



