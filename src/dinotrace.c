/******************************************************************************
 *
 * Filename:
 *     Dinotrace.c
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
 *     This module contains the main routine that initializes Dinotrace and
 *	sets up the environment.
 *
 * Modification History:
 *     AAG	 5-Jul-89	Original Version
 *     AAG	22-Aug-90	Base Level V4.1
 *     AAG	 6-Nov-90	Added screen option
 *     AAG	20-Nov-90	Added support for siz, pos, and res options
 *     AAG	29-Apr-91	Use X11 for Ultrix support
 *     AAG	 9-Jul-91	Added trace format support
 *
 */
static char rcsid[] = "$Id$";


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#include <X11/Xlib.h>
#include <Xm/Xm.h>
#include <X11/Xutil.h>

#include "dinotrace.h"
#include "callbacks.h"
#include "compile_date.h"

Boolean		DTDEBUG=FALSE;		/* Debugging mode */
int		DTPRINT=0;		/* Information printing mode */
int		DebugTemp=0;		/* Temp value for trying things */
int		file_format=FF_DECSIM;	/* Type of trace to support */
char		message[1000];		/* generic string for messages */
XGCValues	xgcv;
Arg		arglist[20];
GLOBAL		*global;
/* filetypes must be in the same order that the FF_* defines are */
struct st_filetypes filetypes[FF_NUMFORMATS] = {
    { 0, "Auto",		"?",	"*.*"		},
    { 1, "DECSIM",		"TRA",	"*.tra*"	},
    { 1, "Tempest CCLI",	"BT",	"*.bt*"		},
    { 1, "Verilog",		"DMP",	"*.dmp*"	},
    { 0, "DECSIM Binary",	"TRA",	"*.tra*"	},
    { 0, "DECSIM Ascii",	"TRA",	"*.tra*"	},
    };

int    main (argc, argv)
    unsigned int	argc;
    char		**argv;
{
    int		i;
    Boolean	sync = FALSE;
    Boolean	opened_a_file = FALSE;
    TRACE	*trace;
    
    /* Create global structure */
    global = XtNew (GLOBAL);
    memset (global, 0, sizeof (GLOBAL));
    global->trace_head = NULL;
    global->directory[0] = '\0';
    global->res = RES_SCALE/ (float)(250);
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
	    /* Switch */
	    if ( !strcmp (argv[i], "-debug") ) {
		DTDEBUG = TRUE;
		}
	    else if ( !strcmp (argv[i], "-print") ) {
		if ((i+1)<argc && isdigit(argv[i+1][0])) {
		    shift;
		    sscanf (argv[i], "%x", & DTPRINT );
		    }
		else DTPRINT = -1;
		}
	    else if ( !strcmp (argv[i], "-tempest") ) {
		file_format = FF_TEMPEST;
		}
	    else if ( !strcmp (argv[i], "-decsim")
		     || !strcmp (argv[i], "-decsim_z") ) {
		file_format = FF_DECSIM;
		}
	    else if ( !strcmp (argv[i], "-verilog") ) {
		file_format = FF_VERILOG;
		}
	    else if ( !strcmp (argv[i], "-sync") ) {
		sync = TRUE;
		}
	    else if ( !strcmp (argv[i], "-geometry") && (i+1)<argc ) {
		shift;
		config_parse_geometry (argv[i], & (global->start_geometry));
		}
	    else if ( !strcmp (argv[i], "-siz") && (i+2)<argc ) {
		shift;
		sscanf (argv[i], "%hd", & (global->start_geometry.width) );
		shift;
		sscanf (argv[i], "%hd", & (global->start_geometry.height) );
		}
	    else if ( !strcmp (argv[i], "-pos") && (i+2)<argc ) {
		shift;
		sscanf (argv[i], "%hd", & (global->start_geometry.x) );
		shift;
		sscanf (argv[i], "%hd", & (global->start_geometry.y) );
		}
	    else if ( !strcmp (argv[i], "-res") && (i+1)<argc ) {
		float res;
		
		shift;
		sscanf (argv[i],"%f",&res);
		global->res = RES_SCALE/ (float)res;
		global->res_default = FALSE;
		}
	    else if ( !strcmp (argv[i], "-noconfig") ) {
		global->suppress_config = TRUE;
		}
	    else {
		printf ("Invalid %s Option: %s\n", DTVERSION, argv[i]);
		printf ("\n%s\n\n", help_message ());
		printf ("DINOTRACE\t[-switches...]  [trace_name] [trace_name] ...\n");
		printf ("Switches:\n");
		printf ("\tDebug:\t-debug\t\tEnable debugging menus.\n");
		printf ("\t\t-print value\tPrint debugging information.\n");
		printf ("\tFormat:\t-decsim\t\tRead traces in DECSIM format.\n");
		printf ("\t\t-tempest\tRead traces in Tempest format.\n");
		printf ("\t\t-verilog\tRead traces in Verilog format.\n");
		printf ("\tConfig:\t-noconfig\tSkip reading global config files.\n");
		printf ("\tX-11:\t-geometry XxY+x+y  Specify starting geometry.\n\n");
		exit (-1);
		}
	    shift;
	    }
	}
    
    /* quick structure portability check */
#ifndef lint	/* constant in conditional context */
    if ((sizeof (SIGNAL_LW) != sizeof (unsigned int))
	|| (sizeof (VALUE) != 5*sizeof (unsigned int))) {
	printf ("%%E, Internal structure portability problem %d!=%d!=%d/4.\n",
		sizeof (SIGNAL_LW), sizeof (unsigned int), sizeof (VALUE));
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
    DFree (trace);
    global->trace_head = NULL;

    /* create the main dialog window */
    trace = create_trace (global->start_geometry.width, global->start_geometry.height, 
			  global->start_geometry.x, global->start_geometry.y);
    
    /* expiration check */
#ifdef EXPIRATION
    if ((COMPILE_DATE + (EXPIRATION)) < time (NULL)) {
	XSync (global->display,0);
	dino_information_ack (trace, "This version of DinoTrace has expired.\n\
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
	    fil_read_cb (trace);
	    }
	}

    /* loop forever */
    /*This code ~= XtAppMainLoop (global->appcontext);*/
    while (1) {
	XEvent event;

	if (global->redraw_needed && !XtAppPending (global->appcontext) && !global->redraw_manually) {
	    draw_perform();
	    }

	XtAppNextEvent (global->appcontext, &event);
	XtDispatchEvent (&event);
	}
    }

char	*help_message ()
    {
    static char msg[2000];

    sprintf (msg,
	     "%s\n\
Compiled %s for %s\n\
\n\
Written by Wilson Snyder\n\
snyder@ricks.enet.dec.com or RICKS::SNYDER\n\
Versions before 4.3 by Allen Gallotta.\n\
\n\
Copyright 1993,1994,1995,1996 by Digital Equipment Corporation.\n\
All Rights Reserved.  For Internal Use Only.\n\
\n\
Please see %sDINOTRACE.TXT for documentation.\n\
\n\
A complete Dinotrace kit is available on:\n\
     CADSYS::CORE$KITS:DINOTRACE*.*\n\
 or     CAD::CORE$KITS:DINOTRACE*.*\n\
\n\
For configuration information, Dinotrace reads in order:\n\
     %sdinotrace.dino\n\
     %s\n\
     %sdinotrace.dino\n\
     %sdinotrace.dino\n\
     %sCURRENT_TRACE_NAME.dino\n",
	     DTVERSION,
	     COMPILE_DATE_STRG,
#ifdef VMS
#ifdef __alpha
	     "VMS Alpha",
#else
	     "VMS VAX",
#endif /* __alpha */
#else /* not VMS */
#ifdef __alpha
	     "OSF Alpha",
#else
#ifdef __mips
	     "Ultrix MIPS",
#else
	     "Unknown OS",
#endif /* __mips */
#endif /* __alpha */
#endif /* VMS */

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



