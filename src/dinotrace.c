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


#include <X11/Xlib.h>
#include <Xm/Xm.h>
#include <stdio.h>
#include <time.h>

#include "dinotrace.h"
#include "callbacks.h"
#include "compile_date.h"

int		DTDEBUG=FALSE,		/* Debugging mode */
		DTPRINT=FALSE;		/* Information printing mode */
int		file_format=FF_DECSIM;	/* Type of trace to support */
char		message[100];		/* generic string for messages */
XGCValues	xgcv;
Arg		arglist[20];
GLOBAL		*global;

int    main(argc, argv)
    unsigned int	argc;
    char		**argv;
{
    int i,x_siz=800,y_siz=600,x_pos=100,y_pos=100,res=250;
    char *start_filename = NULL;
    TRACE *trace;
    
    for (i=1; i<argc; i++)  {
        if ( !strcmp(argv[i], "-debug") ) {
            DTDEBUG = TRUE;
	    }
        else if ( !strcmp(argv[i], "-print") ) {
            DTPRINT = TRUE;
	    }
        else if ( !strcmp(argv[i], "-tempest") ) {
            file_format = FF_TEMPEST;
	    }
        else if ( !strcmp(argv[i], "-decsim") ) {
            file_format = FF_DECSIM;
	    }
        else if ( !strcmp(argv[i], "-verilog") ) {
            file_format = FF_VERILOG;
	    }
        else if ( !strcmp(argv[i], "-siz") ) {
            sscanf(argv[++i],"%d",&x_siz);
            sscanf(argv[++i],"%d",&y_siz);
	    }
        else if ( !strcmp(argv[i], "-pos") ) {
            sscanf(argv[++i],"%d",&x_pos);
            sscanf(argv[++i],"%d",&y_pos);
	    }
        else if ( !strcmp(argv[i], "-res") ) {
            sscanf(argv[++i],"%d",&res);
	    }
        else if ( argv[i][0]!='-' && argc==(i+1)) {
	    start_filename = argv[i];
	    }
        else {
            printf("Invalid %s Option: %s\n", DTVERSION, argv[i]);
            printf("DINOTRACE\t[-debug] [-print] [-tempest] [-screen #] [-size #,#]\n");
            printf("\t\t[-pos #,#] [-res #]   file_name\n");
            printf("\n%s\n", help_message());
            exit(-1);
	    }
	}
    
    /* quick structure portability check */
#ifndef lint	/* constant in conditional context */
    if ((sizeof (SIGNAL_LW) != sizeof (unsigned int))
	|| (sizeof (VALUE) != 4*sizeof (unsigned int))) {
	printf ("%%E, Internal structure portability problem %d!=%d!=%d/4.\n",
		sizeof (SIGNAL_LW), sizeof (unsigned int), sizeof(VALUE));
	}

    /* FAILS on MIPS Ultrix, so don't rely on this:
    if ((1<<32) != 0) {
	printf ("%%E, Internal shifting portability problem %d!=%d.\n", (1<<32), 0);
	}
	*/

#endif

    /* create global information */
    create_globals (0, argv, res);

    /* create the main dialog window */
    trace = create_trace (x_siz,y_siz,x_pos,y_pos);
    
    /* expiration check */
#ifdef EXPIRATION
    if ((COMPILE_DATE + (EXPIRATION)) < time(NULL)) {
	XSync (global->display,0);
	dino_information_ack (trace, "This version of DinoTrace has expired.\n\
Please install a newer version.\n\
See the Help menu for more information.");
	}
#endif

    /* Load config options (such as file_format) */
    config_read_defaults (trace);

    /* Load up the file on the command line, if any */
    if (start_filename != NULL) {
	XSync (global->display,0);
	strcpy (trace->filename, start_filename);
	cb_fil_read (trace);
	}

    /* loop forever */
    XtAppMainLoop(global->appcontext);
    }

char	*help_message()
    {
    static char msg[2000];

    sprintf (msg,
	     "%s\n\
Compiled %s\n\
\n\
Version 4.3 up by Wilson Snyder, RICKS::SNYDER\n\
Prior versions by Allen Gallotta.\n\
\n\
Please see %sDINOTRACE.TXT for documentation.\n\
\n\
A complete Dinotrace kit is available on:\n\
     DIPS::DISK$DIPS_PAGE:[SNYDER.RELEASE.DINOTRACE]\n\
\n\
For configuration information, Dinotrace reads:\n\
     %sdinotrace.dino\n\
     %sdinotrace.dino\n\
     %sdinotrace.dino\n\
     %sCURRENT_TRACE_NAME.dino\n",
	     DTVERSION,
	     COMPILE_DATE_STRG,
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



