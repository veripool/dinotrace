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
#include <X11/Xm.h>
#include <stdio.h>

#include "dinotrace.h"

int    main(argc, argv)
    unsigned int	argc;
    char		**argv;
{
    int i,x_siz=800,y_siz=600,x_pos=100,y_pos=100,res=500;
    char *start_filename = NULL;
    FILE *tempfp;
    
    for (i=1; i<argc; i++) 
	{
        if ( !strcmp(argv[i], "-debug") )
	    {
            DTDEBUG = TRUE;
	    }
        else if ( !strcmp(argv[i], "-print") )
	    {
            DTPRINT = TRUE;
	    }
        else if ( !strcmp(argv[i], "-tempest") )
	    {
            trace_format = HLO_TEMPEST;
	    }
        else if ( !strcmp(argv[i], "-siz") )
	    {
            sscanf(argv[++i],"%d",&x_siz);
            sscanf(argv[++i],"%d",&y_siz);
	    }
        else if ( !strcmp(argv[i], "-pos") )
	    {
            sscanf(argv[++i],"%d",&x_pos);
            sscanf(argv[++i],"%d",&y_pos);
	    }
        else if ( !strcmp(argv[i], "-res") )
	    {
            sscanf(argv[++i],"%d",&res);
	    }
        else if ( argv[i][0]!='-' && argc==(i+1))
	    {
	    start_filename = argv[i];
	    /* requester will pop up
	       if (tempfp = fopen (start_filename, "r"))
	       close (tempfp);
	       else {
	       printf("Can't open %s\n", start_filename);
	       exit (-1);
	       }
	       */
	    }
        else
	    {
            printf("Invalid %s Option: %s\n", DTVERSION, argv[i]);
            printf("DINOTRACE\t[-debug] [-print] [-tempest] [-screen #] [-size #,#]\n");
            printf("\t\t[-pos #,#] [-res #]   file_name\n");
            exit(-1);
	    }
	}
    
    /* create the main dialog window */
    create_display(argc,argv,x_siz,y_siz,x_pos,y_pos,res,start_filename);
    
    /* loop forever */
    XtMainLoop();
    }

