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
struct st_filetypes filetypes[8] = {
    { 0, "Auto",		"?", "*.*" },
    { 1, "DECSIM",		"TRA", "*.tra" },
#ifdef VMS
    { 0, "DECSIM Compressed",	"TRA.Z", "*_tra.Z" },
#else
    { 1, "DECSIM Compressed",	"TRA.Z", "*.tra.Z" },
#endif
    { 1, "Tempest CCLI",	"BT", "*.bt" },
    { 1, "Verilog",		"DMP", "*.dmp" },
    { 0, "DECSIM Binary",	"TRA", "*.tra" },
    { 0, "DECSIM Ascii",	"TRA", "*.tra" },
    };

int    main (argc, argv)
    unsigned int	argc;
    char		**argv;
{
    int		i;
    Boolean	sync = FALSE;
    char	*start_filename = NULL;
    TRACE	*trace;
    int		benchmark = FALSE;
    
    /* Create global structure */
    global = XtNew (GLOBAL);
    memset (global, 0, sizeof (GLOBAL));
    global->trace_head = NULL;
    global->directory[0] = '\0';
    global->res = RES_SCALE/ (float)(250);
    init_globals ();

    /* Parse parameters */
    for (i=1; i<argc; i++)  {
        if ( !strcmp (argv[i], "-debug") ) {
            DTDEBUG = TRUE;
	    }
        else if ( !strcmp (argv[i], "-print") ) {
	    if (isdigit(argv[i+1][0])) {
		sscanf (argv[++i], "%x", & DTPRINT );
		}
	    else DTPRINT = -1;
	    }
        else if ( !strcmp (argv[i], "-benchmark") ) {
            benchmark = TRUE;
	    }
        else if ( !strcmp (argv[i], "-tempest") ) {
            file_format = FF_TEMPEST;
	    }
        else if ( !strcmp (argv[i], "-decsim") ) {
            file_format = FF_DECSIM;
	    }
        else if ( !strcmp (argv[i], "-verilog") ) {
            file_format = FF_VERILOG;
	    }
        else if ( !strcmp (argv[i], "-decsim_z") ) {
            file_format = FF_DECSIM_Z;
	    }
        else if ( !strcmp (argv[i], "-sync") ) {
            sync = TRUE;
	    }
        else if ( !strcmp (argv[i], "-geometry") ) {
	    config_parse_geometry (argv[++i], & (global->start_geometry));
	    }
        else if ( !strcmp (argv[i], "-siz") ) {
            sscanf (argv[++i], "%hd", & (global->start_geometry.width) );
            sscanf (argv[++i], "%hd", & (global->start_geometry.height) );
	    }
        else if ( !strcmp (argv[i], "-pos") ) {
            sscanf (argv[++i], "%hd", & (global->start_geometry.x) );
            sscanf (argv[++i], "%hd", & (global->start_geometry.y) );
	    }
        else if ( !strcmp (argv[i], "-res") ) {
	    float res;

            sscanf (argv[++i],"%f",&res);
	    global->res = RES_SCALE/ (float)res;
	    global->res_default = FALSE;
	    }
        else if ( !strcmp (argv[i], "-noconfig") ) {
	    global->suppress_config = TRUE;
	    }
        else if ( argv[i][0]!='-' && argc== (i+1)) {
	    start_filename = argv[i];
	    }
        else {
            printf ("Invalid %s Option: %s\n", DTVERSION, argv[i]);
            printf ("DINOTRACE\t[-debug] [-print] [-tempest] [-screen #] [-siz # #]\n");
            printf ("\t\t[-pos # #] [-res #]   file_name\n");
            printf ("\n%s\n", help_message ());
            exit (-1);
	    }
	}
    
    /* quick structure portability check */
#ifndef lint	/* constant in conditional context */
    if ((sizeof (SIGNAL_LW) != sizeof (unsigned int))
	|| (sizeof (VALUE) != 4*sizeof (unsigned int))) {
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

    /* Load up the file on the command line, if any */
    if (start_filename != NULL) {
	XSync (global->display,0);
	strcpy (trace->filename, start_filename);
	fil_read_cb (trace);
	}

    /* loop forever */
    if (!benchmark) {
	XtAppMainLoop (global->appcontext);
	}
    else {
	int t;

	file_format = FF_TEMPEST;
	strcpy (trace->filename, "../benchmark.bt");
	fil_read_cb (trace);
	fil_read_cb (trace);
	fil_read_cb (trace);
	for (t=1; t<100; t++) {
	    redraw_all (trace);
	    }
	}

    return(0);	/* to please lint */
    }

char	*help_message ()
    {
    static char msg[2000];

    sprintf (msg,
	     "%s\n\
Compiled %s for %s\n\
\n\
Version 4.3 up by Wilson Snyder, RICKS::SNYDER\n\
Prior versions by Allen Gallotta.\n\
\n\
Please see %sDINOTRACE.TXT for documentation.\n\
\n\
A complete Dinotrace kit is available on:\n\
     CADSYS::CORE$KITS:DINOTRACE*.*\n\
\n\
For configuration information, Dinotrace reads:\n\
     %sdinotrace.dino\n\
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
	     "SYS$LOGIN:",
	     "[current.trace.directory]",
	     "[current.trace.directory]"
#else
	     "$DINODISK/",
	     "$DINODISK/",
	     "~/",
	     "/current/trace/directory/",
	     "/current/trace/directory/"
#endif
	     );

    return (msg);
    }



