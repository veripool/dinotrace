/******************************************************************************
 *
 * Filename:
 *     dt_config.c
 *
 * Subsystem:
 *     Dinotrace
 *
 * Version:
 *     Dinotrace V5.0
 *
 * Author:
 *     Wilson Snyder
 *
 * Abstract:
 *
 * Modification History:
 *     WPS	 5-Jan-93	Original Version
 *
 ******************************************************************************
 *
 *	%		Comment
 *	!		Comment
 *	#		Eventual preprocessor (#INCLUDE at least)
 *
! Undocumented:
!	debug		ON | OFF
!	print		<number> | ON | OFF
!
!	! Comment
! Config:
!	click_to_edge	ON | OFF
!	cursor		ON | OFF
!	refreshing	AUTO | MANUAL
!	grid		ON | OFF	<grid_number>
!	grid_align	<number> | ASSERTION | DEASSERTION	<grid_number>
!	grid_resolution	<number> | AUTO | EDGE 	 <grid_number>
!	grid_type	NORMAL | WIDE		 <grid_number>
!	grid_signal	<signal_pattern> 	 <grid_number>
!	grid_color	<color>		 	 <grid_number>
!	page_inc	4 | 2 | 1
!	print_size	A | B | EPSPORT | EPSLAND
!	rise_fall_time	<number>
!	signal_height	<number>
!	time_format	%0.6lf | %0.6lg | "" 
!	time_precision	US | NS | PS
!	time_rep	<number> | US | NS | PS | CYCLE
! File Format:
!	file_format	DECSIM | TEMPEST | VERILOG
!	save_enables	ON | OFF
!	save_ordering	ON | OFF
!	signal_states	<signal_pattern> = {<State>, <State>...}
!	vector_seperator "<char>"
!       time_multiplier	<number>
! Geometry/resources:
!	open_geometry	<width>[%]x<height>[%]+<xoffset>[%]+<yoff>[%]
!	shrink_geometry	<width>%x<height>%+<xoffset>%+<yoff>%
!	start_geometry	<width>x<height>+<xoffset>+<yoffset>
! Modification of traces:
!	signal_delete	<signal_pattern>
!	signal_delete_constant	<signal_pattern>
!	signal_add	<signal_pattern>	[<after_signal_first_matches>]
!	signal_copy	<signal_pattern>	[<after_signal_first_matches>]
!	signal_move	<signal_pattern>	[<after_signal_first_matches>]
!	signal_highlight <color> <signal_name>
!	cursor_add	<color> <time>	[-USER]
!	value_highlight <color>	<value>	[-CURSOR] [-VALUE]
! Display changes:
!	time_goto	<time>
!	signal_goto	<signal_pattern>	(if not on screen, first match)
!	resolution	<res>
!	refresh
!	annotate
 *	
 */
static char rcsid[] = "$Id$";


#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef VMS
# include <file.h>
# include <strdef.h>
# include <math.h> /* removed for Ultrix support... */
#endif

#include <X11/Xlib.h>
#include <Xm/Xm.h>

#include "dinotrace.h"
#include "callbacks.h"

/* See the ascii map to have this make sense */
#define issigchr(ch)  ( ((ch)>' ') && ((ch)!='=') && ((ch)!='\"') && ((ch)!='\'') )

#define isstatechr(ch)  ( ((ch)>' ') && ((ch)!=',') && ((ch)!='=') && ((ch)!='{') && ((ch)!='}') && ((ch)!='\"') && ((ch)!='\'') )

#define isstatesep(ch)  (!isstatechr(ch) && (ch)!='}' )


/* File locals */
static Boolean config_report_errors;
static char *config_file="";
static int config_line_num=0;
static Boolean config_reading_socket;

/**********************************************************************
*	READING FUNCTIONS
**********************************************************************/

void upcase_string (tp)
    char *tp;
{
    for (;*tp;tp++) *tp = toupper (*tp);
    }

void config_get_line(line, len, readfp)
    char *line;
    int len;
    FILE *readfp;
{
    char *tp;

    /* Read line & kill EOL at end */
    fgets (line, len, readfp);
    if (*(tp=(line+strlen(line)-1))=='\n') *tp='\0';
    config_line_num++;
    }

int	config_read_signal (line, out)
    char *line;
    char *out;
{
    char *tp;
    int outlen=0;

    while (*line && !issigchr(*line)) {
	line++;
	outlen++;
	}

    if (!*line) {
	out[0]='\0';
	return(outlen);
	}

    /* extract signal */
    strncpy(out, line, MAXSIGLEN);
    out[MAXSIGLEN-1]='\0';
    for (tp=out; *tp && issigchr(*tp); tp++) ;
    *tp='\0';

    return (strlen(out)+outlen);
    }

int	config_read_state (line, out, statenum_ptr)
    char *line;
    char *out;
    int *statenum_ptr;
{
    char *tp;
    int outlen=0;

    while (*line && isstatesep(*line)) {
	line++;
	outlen++;
	}

    if (!*line) {
	out[0]='\0';
	return(outlen);
	}

    for (tp=line; *tp && isdigit(*tp); tp++) ;
    if (*tp++ == '=') {
	/* 22=statename type assignment */
	*statenum_ptr = atoi (line);
	outlen += (tp - line);
	line = tp;
	}

    /* extract state */
    strncpy(out, line, MAXVALUELEN);
    out[MAXVALUELEN-1]='\0';
    for (tp=out; *tp && isstatechr(*tp); tp++) ;
    *tp='\0';

    return (strlen(out)+outlen);
    }

int	config_read_int (line, out)
    char *line;
    int *out;
{
    int outlen=0;

    while (*line && !isalnum(*line) && *line!='-') {
	line++;
	outlen++;
	}

    if (!*line) {
	*out=0;
	return(outlen);
	}

    /* extract command */
    *out = atoi(line);
    while (*line && isdigit(*line)) {
	line++;
	outlen++;
	}

    return (outlen);
    }


/**********************************************************************
*	pattern matching from GNU (see wildmat.c)
**********************************************************************/

/* Double procedures, both inlined, unrolls loop much better */
int wildmati(s, p)
    register unsigned char	*s;	/* Buffer */
    register unsigned char	*p;	/* Pattern */
{
    for ( ; *p; s++, p++) {
	if (*p!='*') {
	    /* df is magic conversion to lower case */
	    if (((*s & 0xdf)!=(*p & 0xdf)) && *p != '?')
		return(FALSE);
	    }
	else {
	    /* Trailing star matches everything. */
	    if (!*++p) return (TRUE);
	    while (wildmat(s, p) == FALSE)
		if (*++s == '\0')
		    return(FALSE);
	    return(TRUE);
	    }
	}
    return(*s == '\0');
    }
#pragma inline (wildmati)

int wildmat(s, p)
    register unsigned char	*s;	/* Buffer */
    register unsigned char	*p;	/* Pattern */
{
    for ( ; *p; s++, p++) {
	if (*p!='*') {
	    /* df is magic conversion to lower case */
	    if (((*s & 0xdf)!=(*p & 0xdf)) && *p != '?')
		return(FALSE);
	    }
	else {
	    /* Trailing star matches everything. */
	    if (!*++p) return (TRUE);
	    while (wildmati(s, p) == FALSE)
		if (*++s == '\0')
		    return(FALSE);
	    return(TRUE);
	    }
	}
    return(*s == '\0');
    }
#pragma inline (wildmat)

/**********************************************************************
*	SUPPORT FUNCTIONS
**********************************************************************/

SIGNALSTATE	*find_signal_state (trace, name)
    TRACE	*trace;
    char *name;
{
    register SIGNALSTATE *sig;

    for (sig=trace->signalstate_head; sig; sig=sig->next) {
	/* printf ("'%s'\t'%s'\n", name, sig->signame); */
	if (wildmat(name, sig->signame))
	    return (sig);
	}
    return (NULL);
    }

void	add_signal_state (trace, info)
    TRACE	*trace;
    SIGNALSTATE *info;
{
    SIGNALSTATE *new;
    int t;

    new = (SIGNALSTATE *)(XtMalloc (sizeof(SIGNALSTATE)));
    memcpy ((void *)new, (void *)info, sizeof(SIGNALSTATE));
    new->next = trace->signalstate_head;
    trace->signalstate_head = new;

    /*printf ("Signal '%s' Assigned States:\n", new->signame);*/
    for (t=0; t<MAXSTATENAMES; t++) {
	if (new->statename[t][0] != '\0') {
	    new->numstates = t+1;
	    /*printf ("State %d = '%s'\n", t, new->statename[t]);*/
	    }
	}
    }

void	free_signal_states (trace)
    TRACE	*trace;
{
    SIGNALSTATE *sstate_ptr, *last_ptr;

    sstate_ptr = trace->signalstate_head;
    while (sstate_ptr) {
	last_ptr = sstate_ptr->next;
	DFree (sstate_ptr);
	sstate_ptr = last_ptr;
	}
    trace->signalstate_head = NULL;
    }

void	print_signal_states (w,trace)
    Widget	w;
    TRACE	*trace;
{
    SIGNALSTATE *sstate_ptr;
    int i;

    sstate_ptr = trace->signalstate_head;
    while (sstate_ptr) {
	printf("Signal %s, %d states:\n",sstate_ptr->signame, sstate_ptr->numstates);
	for (i=0; i<MAXSTATENAMES; i++)
	    if (sstate_ptr->statename[i][0])
		printf ("\t%d=%s\n", i, sstate_ptr->statename[i]);
	sstate_ptr = sstate_ptr->next;
	}
    printf ("\n");
    }

void	config_parse_geometry (line, geometry)
    char	*line;
    GEOMETRY	*geometry;
    /* Like XParseGeometry, but handles percentages */
{
    int		flags, x, y;
    unsigned int wid, hei;
    char	noper_line[100];
    char	*tp;

    /* Copy line into a temp, remove the percentage symbols, and parse it */
    strcpy (noper_line, line);
    while (tp=strchr (noper_line, '%')) strcpy (tp, tp+1);

    flags = XParseGeometry (noper_line, &x, &y, &wid, &hei);
    if (flags & WidthValue)	geometry->width = wid;
    if (flags & HeightValue)	geometry->height = hei;
    if (flags & XValue)		geometry->x = x;
    if (flags & YValue)		geometry->y = y;

    /* Now figure out what the percentage symbols apply to */
    geometry->xp = geometry->yp = geometry->heightp = geometry->widthp = FALSE;
    for (tp = line; *tp; ) {
	while (*tp && !isdigit(*tp)) tp++;
	while (isdigit(*tp)) tp++;
	if (*tp=='%') {
	    if (flags & WidthValue)	geometry->widthp = TRUE;
	    else if (flags & HeightValue) geometry->heightp = TRUE;
	    else if (flags & XValue)	geometry->xp = TRUE;
	    else if (flags & YValue)	geometry->yp = TRUE;
	    }
	if (flags & WidthValue) 	flags &= flags & (~ WidthValue);
	else if (flags & HeightValue)	flags &= flags & (~ HeightValue);
	else if (flags & XValue)	flags &= flags & (~ XValue);
	else if (flags & YValue)	flags &= flags & (~ YValue);
	}

    if (DTPRINT_CONFIG) printf ("geometry %s = %dx%d+%d+%d   %c%c%c%c\n", line,
				geometry->width, geometry->height, 
				geometry->x, geometry->y,
				geometry->widthp?'%':'-', geometry->heightp?'%':'-', 
				geometry->xp?'%':'-', geometry->yp?'%':'-');
    }


void	config_error_ack (trace, message)
    TRACE	*trace;
    char	*message;
{
    char	newmessage[1000];

    if (!config_report_errors) return;

    strcpy (newmessage, message);
    if (config_reading_socket) {
	sprintf (newmessage + strlen(newmessage), "on command # %d from socket %s\n", config_line_num, config_file);
	}
    else {
	sprintf (newmessage + strlen(newmessage), "on line %d of %s\n", config_line_num, config_file);
	}
    dino_error_ack (trace, newmessage);
    }

/**********************************************************************
*	reading functions w/ error messages
**********************************************************************/

int	config_read_on_off (trace, line, out)
    TRACE	*trace;
    char *line;
    int *out;
    /* Read boolean flag line, return <= 0 and print msg if bad */
{
    char cmd[MAXSIGLEN];
    int outlen;

    outlen = config_read_signal (line, cmd);
    upcase_string (cmd);

    if (!strcmp (cmd, "ON") || !strcmp (cmd, "1"))
	*out = 1;
    else if (!strcmp (cmd, "OFF") || !strcmp (cmd, "0"))
	*out = 0;
    else {
	sprintf (message, "Expected ON or OFF switch\n");
	config_error_ack (trace, message);
	*out = -1;
	}

    return (outlen);
    }

int	config_read_color (trace, line, color)
    TRACE	*trace;
    char 	*line;
    ColorNum	*color;
    /* Read color name from line, return <= 0 and print msg if bad */
{
    int outlen;

    switch (*line) {
      case 'N': case 'n':
	*color = global->highlight_color;
	break;
      case 'C': case 'c':
	/* Duplicate code in submenu_to_color */
	if ((++global->highlight_color) > MAX_SRCH) global->highlight_color = 1;
	*color = global->highlight_color;
	break;
      default:
	outlen = config_read_int (line, color);
	if (*color < 0 || *color > MAX_SRCH) {
	    sprintf (message, "Color numbers must be 0 to %d, NEXT or CURRENT\n", MAX_SRCH);
	    config_error_ack (trace, message);
	    *color = -1;
	    }
	}

    return (outlen);
    }

int	config_read_grid (trace, line, grid_pptr)
    TRACE	*trace;
    char 	*line;
    GRID	**grid_pptr;
    /* Read grid number name from line, return < 0 and print msg if bad */
{
    int 	outlen;
    int		grid_num;
    char	message [MAXTIMELEN];

    outlen = config_read_int (line, &grid_num);

    if (grid_num < 0 || grid_num > MAXGRIDS) {
	sprintf (message, "Grid numbers must be 0 to %d\n", MAXGRIDS);
	grid_num = -1;
	*grid_pptr = NULL;
    }
    else {
	*grid_pptr = &(trace->grid[grid_num]);
    }

    return (outlen);
}

/**********************************************************************
*	config_process_states
**********************************************************************/

int	config_process_state (trace, line, sstate_ptr)
    TRACE	*trace;
    char 	*line;
    SIGNALSTATE *sstate_ptr;
{
    char newstate[MAXVALUELEN];
    int	statenum = sstate_ptr->numstates;
    int outlen=0;

    if (*line && *line!='}') {
	outlen = config_read_state (line, newstate, &statenum);
	line += outlen;
	/*printf ("Got = %d '%s'\n", statenum, newstate);*/
	if (statenum < 0 || statenum >= MAXSTATENAMES) {
	    sprintf (message, "Over maximum of %d SIGNAL_STATES for signal %s\n",
		     MAXSTATENAMES, sstate_ptr->signame);
	    config_error_ack (trace,message);
	    /* No return, as will just ignore remaining errors */
	    }
	else {
	    if (newstate[0]) {
		sstate_ptr->numstates = statenum+1;
		strcpy (sstate_ptr->statename[statenum], newstate);
		}
	    }
	}
    return (outlen);
    }


/**********************************************************************
*	config_process_line
**********************************************************************/

void	config_process_line_internal (trace, line, eof)
    TRACE	*trace;
    char	*line;
    Boolean	eof;		/* Final call of process_line with EOF */
{
    char cmd[MAXSIGLEN];
    int value;
    GRID	*grid_ptr;
    char pattern[MAXSIGLEN];
    
    static SIGNALSTATE newsigst;
    static Boolean processing_sig_state = FALSE;

  re_process_line:
    
    /* Comment */
    while (*line && isspace(*line)) line++;
    if ((line[0]=='!') || (line[0]==';') || (line[0]=='\0')) return;

    /* if (DTPRINT_CONFIG) printf ("Cmd='%s'\n",cmd); */

    if (processing_sig_state) {
	if (*line == ';' || *line=='}' || eof) {
	    if (eof) {
		config_error_ack (trace, "Unexpected EOF during SIGNAL_STATES\n");
		}
	    add_signal_state (trace, &newsigst);
	    processing_sig_state = FALSE;
	    }
	else {
	    line += config_process_state (trace, line, &newsigst);
	    goto re_process_line;
	    }
	}
    else {
	/* General commands */

	/* Preprocessor #INCLUDE eventually */
	/* extract command */
	line += config_read_signal (line, cmd);
	while (*line && isspace(*line)) line++;
	upcase_string (cmd);

	if (!strcmp(cmd, "DEBUG")) {
	    value=DTDEBUG;
	    line += config_read_on_off (trace, line, &value);
	    if (value >= 0) {
		if (DTPRINT) printf ("Config: DTDEBUG=%d\n",value);
		DTDEBUG=value;
		}
	    }
	else if (!strcmp(cmd, "PRINT")) {
	    value=DTPRINT;
	    if (toupper(line[0])=='O' && toupper(line[1])=='N') DTPRINT = -1;
	    else if (toupper(line[0])=='O' && toupper(line[1])=='F') DTPRINT = 0;
	    else {
		sscanf (line, "%lx", &value);
		if (value > 0) {
		    DTPRINT=value;
		    }
		else {
		    config_error_ack (trace, "Print must be set ON, OFF, or > 0\n");
		    }
		}
	    if (DTPRINT) printf ("Config: DTPRINT=0x%x\n",value);
	    }
	else if (!strcmp(cmd, "SAVE_ENABLES")) {
	    value=global->save_enables;
	    line += config_read_on_off (trace, line, &value);
	    if (value >= 0) {
		global->save_enables=value;
		}
	    }
	else if (!strcmp(cmd, "SAVE_ORDERING")) {
	    value=global->save_ordering;
	    line += config_read_on_off (trace, line, &value);
	    if (value >= 0) {
		global->save_ordering=value;
		}
	    }
	else if (!strcmp(cmd, "CURSOR")) {
	    value = trace->cursor_vis;
	    line += config_read_on_off (trace, line, &value);
	    if (value >= 0) {
		if (DTPRINT_CONFIG) printf ("Config: cursor_vis=%d\n",value);
		trace->cursor_vis = value;
		}
	    }
	else if (!strcmp(cmd, "CLICK_TO_EDGE")) {
	    value = global->click_to_edge;
	    line += config_read_on_off (trace, line, &value);
	    if (value >= 0) {
		if (DTPRINT_CONFIG) printf ("Config: click_to_edge=%d\n",value);
		global->click_to_edge = value;
		}
	    }
	else if (!strcmp(cmd, "REFRESHING")) {
	    if (toupper(line[0])=='A')
		global->redraw_manually = FALSE;
	    else if (toupper(line[0])=='M')
		global->redraw_manually = TRUE;
	    else {
		config_error_ack (trace, "Refreshing must be AUTO, or MANUAL\n");
	        }
	    }
	else if (!strcmp(cmd, "SIGNAL_HEIGHT")) {
	    value = trace->sighgt;
	    line += config_read_int (line, &value);
	    if (value >= 15 && value <= 50)
		trace->sighgt = value;
	    else {
		config_error_ack (trace, "Signal_height must be 15-50\n");
		}
	    }
	else if (!strcmp(cmd, "GRID")) {
	    line += config_read_on_off (trace, line, &value);
	    line += config_read_grid (trace, line, &grid_ptr);
	    if (grid_ptr && value >= 0) {
		if (DTPRINT_CONFIG) printf ("Config: grid_vis=%d\n",value);
		grid_ptr->visible = value;
		}
	    }
	else if (!strcmp(cmd, "GRID_RESOLUTION")) {
	    line += config_read_int (line, &value);
	    if (isalpha(line[0])) { line += config_read_signal (line, pattern); }
	    line += config_read_grid (trace, line, &grid_ptr);
	    if (grid_ptr) {
		if (value >= 1) {
		    grid_ptr->period = value;
		    grid_ptr->period_auto = PA_USER;
		}
		else {
		    if (toupper(pattern[0])=='A')
			grid_ptr->period_auto = PA_AUTO;
		    else if (toupper(pattern[0])=='E')
			grid_ptr->period_auto = PA_EDGE;
		    else {
			config_error_ack (trace, "Grid_res must be >0, ASSERTION, or EDGE\n");
		    }
		}
	    }
	}
	else if (!strcmp(cmd, "GRID_ALIGN")) {
	    line += config_read_int (line, &value);
	    if (isalpha(line[0])) { line += config_read_signal (line, pattern); }
	    line += config_read_grid (trace, line, &grid_ptr);
	    if (grid_ptr) {
		if (value >= 1) {
		    grid_ptr->alignment = value;
		    grid_ptr->align_auto = AA_USER;
		}
		else {
		    if (toupper(pattern[0])=='A')
			grid_ptr->align_auto = AA_ASS;
		    else if (toupper(pattern[0])=='D')
			grid_ptr->align_auto = AA_DEASS;
		    else {
			config_error_ack (trace, "Grid_align must be >0, ASSERTION, or DEASSERTION\n");
		    }
		}
	    }
	}
	else if (!strcmp(cmd, "GRID_TYPE")) {
	    line += config_read_int (line, &value);
	    if (isalpha(line[0])) { line += config_read_signal (line, pattern); }
	    line += config_read_grid (trace, line, &grid_ptr);
	    if (grid_ptr) {
		if (toupper(pattern[0])=='N')
		    grid_ptr->wide_line = FALSE;
		else if (toupper(pattern[0])=='W')
		    grid_ptr->wide_line = TRUE;
		else {
		    config_error_ack (trace, "Grid_type must be NORMAL or WIDE\n");
		}
	    }
	}
	else if (!strcmp(cmd, "GRID_SIGNAL")) {
	    line += config_read_signal (line, pattern);
	    line += config_read_grid (trace, line, &grid_ptr);
	    if (grid_ptr && *pattern) {
		strcpy (grid_ptr->signal, pattern);
	    }
	}
	else if (!strcmp(cmd, "GRID_COLOR")) {
	    ColorNum color;
	    line += config_read_color (trace, line, &color);
	    line += config_read_grid (trace, line, &grid_ptr);
	    if (grid_ptr && color>=0) {
		grid_ptr->color = color;
	    }
	}
	else if (!strcmp(cmd, "RISE_FALL_TIME")) {
	    value = trace->sigrf;
	    line += config_read_int (line, &value);
	    /* Valid values not known... */
	    trace->sigrf = value;
	    }
	else if (!strcmp(cmd, "PAGE_INC")) {
	    value = global->pageinc;
	    line += config_read_int (line, &value);
	    if (value == 1) global->pageinc = FPAGE;
	    else if (value == 2) global->pageinc = HPAGE;
	    else if (value == 4) global->pageinc = QPAGE;
	    else {
		config_error_ack (trace, "Page_Inc must be 1, 2, or 4\n");
		}
	    if (DTPRINT_CONFIG) printf ("page_inc = %d\n", global->pageinc);
	    }
	else if (!strcmp(cmd, "TIME_REP")) {
	    switch (toupper(line[0])) {
	      case 'N':	trace->timerep = TIMEREP_NS;	break;
	      case 'U':	trace->timerep = TIMEREP_US;	break;
	      case 'P':	trace->timerep = TIMEREP_PS;	break;
	      case 'C':	trace->timerep = TIMEREP_CYC;	break;
	      case '0': case '1': case '2': case '3': case '4':
	      case '5': case '6': case '7': case '8': case '9': case '.':
		trace->timerep = atof(line);	break;
	      default:
		config_error_ack (trace, "Time_Rep must be PS, NS, US, or CYCLE\n");
		}
	    if (DTPRINT_CONFIG) printf ("timerep = %lf\n", trace->timerep);
	    }
	else if (!strcmp(cmd, "TIME_PRECISION")) {
	    switch (toupper(line[0])) {
	      case 'N':	global->time_precision = TIMEREP_NS;	break;
	      case 'U':	global->time_precision = TIMEREP_US;	break;
	      case 'P':	global->time_precision = TIMEREP_PS;	break;
	      default:
		config_error_ack (trace, "Time_Precision must be PS, NS, or US\n");
		}
	    if (DTPRINT_CONFIG) printf ("time_precision = %lf\n", global->time_precision);
	    }
	else if (!strcmp(cmd, "TIME_FORMAT")) {
	    line += config_read_signal (line, cmd);
	    strcpy (global->time_format, cmd);
	    if (DTPRINT_CONFIG) printf ("time_format = '%s'\n", global->time_format);
	    }
	else if (!strcmp(cmd, "TIME_MULTIPLIER")) {
	    value = global->tempest_time_mult;
	    line += config_read_int (line, &value);
	    if (value > 0) global->tempest_time_mult = value;
	    else {
		config_error_ack (trace, "Time_Mult must be > 0\n");
		}
	    }
	else if (!strcmp(cmd, "PRINT_SIZE")) {
	    switch (toupper(line[0])) {
	      case 'A':
		global->print_size = PRINTSIZE_A;
		break;
	      case 'B':
		global->print_size = PRINTSIZE_B;
		break;
	      case 'E':
		if (strchr (cmd, 'L'))
		    global->print_size = PRINTSIZE_EPSLAND;
		else global->print_size = PRINTSIZE_EPSPORT;
		break;
	      default:
		config_error_ack (trace, "Print_Size must be A, B, EPSLAND, or EPSPORT\n");
		}
	    }
	else if (!strcmp(cmd, "FILE_FORMAT")) {
	    char *tp;
	    for (tp = line; *tp && *tp!='Z' && *tp!='z'; tp++) ;
	    switch (toupper(line[0])) {
	      case 'D':
		file_format = FF_DECSIM;
		break;
	      case 'T':
		file_format = FF_TEMPEST;
		break;
	      case 'V':
		file_format = FF_VERILOG;
		break;
	      default:
		config_error_ack (trace, "File_Format must be DECSIM, TEMPEST, or VERILOG\n");
		}
	    if (DTPRINT_CONFIG) printf ("File_format = %d\n", file_format);
	    }
	else if (!strcmp(cmd, "VECTOR_SEPERATOR")) {
	    if (*line=='"') line++;
	    if (*line && *line!='"') {
		trace->vector_seperator = line[0];
		}
	    else {
		trace->vector_seperator = '\0';
		}
	    /* Take a stab at the ending character */
	    switch (trace->vector_seperator) {
	      case '`':	trace->vector_endseperator = '\''; break;
	      case '(':	trace->vector_endseperator = ')'; break;
	      case '[':	trace->vector_endseperator = ']'; break;
	      case '{':	trace->vector_endseperator = '}'; break;
	      case '<':	trace->vector_endseperator = '>'; break;
	      default:  trace->vector_endseperator = trace->vector_seperator; break;	/* a wild guess SIG$20:1$? */
		}
	    if (DTPRINT_CONFIG) printf ("Vector_seperator = '%c'  End='%c'\n", 
					trace->vector_seperator, trace->vector_endseperator);
	    }
	else if (!strcmp(cmd, "SIGNAL_HIGHLIGHT")) {
	    ColorNum color;
	    line += config_read_color (trace, line, &color);
	    if (color >= 0) {
		line += config_read_signal (line, pattern);
		if (!pattern[0]) {
		    config_error_ack (trace, "Signal_Highlight signal name must not be null\n");
		    }
		else {
		    sig_wildmat_select (NULL, pattern);
		    sig_highlight_selected (color);
		    }
		}
	    }
	else if (!strcmp(cmd, "SIGNAL_ADD")
		 || !strcmp(cmd, "SIGNAL_MOVE")) {
	    char pattern2[MAXSIGLEN];
	    line += config_read_signal (line, pattern);
	    if (!pattern[0]) {
		config_error_ack (trace, "Signal_Add signal name must not be null\n");
		}
	    else {
		line += config_read_signal (line, pattern2);
		sig_wildmat_select (NULL, pattern);
		sig_move_selected (trace, pattern2);
		}
	    }
	else if (!strcmp(cmd, "SIGNAL_COPY")) {
	    char pattern2[MAXSIGLEN];
	    line += config_read_signal (line, pattern);
	    if (!pattern[0]) {
		config_error_ack (trace, "Signal_copy signal name must not be null\n");
		}
	    else {
		line += config_read_signal (line, pattern2);
		sig_wildmat_select (NULL, pattern);
		sig_copy_selected (trace, pattern2);
		}
	    }
	else if (!strcmp(cmd, "SIGNAL_DELETE")) {
	    line += config_read_signal (line, pattern);
	    if (!pattern[0]) {
		config_error_ack (trace, "Signal_Delete signal name must not be null\n");
		}
	    else {
		sig_wildmat_select (trace, pattern);
		sig_delete_selected (TRUE);
		}
	    }
	else if (!strcmp(cmd, "SIGNAL_DELETE_CONSTANT")) {
	    line += config_read_signal (line, pattern);
	    if (!pattern[0]) {
		config_error_ack (trace, "Signal_Delete_Constant signal name must not be null\n");
		}
	    else {
		sig_wildmat_select (trace, pattern);
		sig_delete_selected (FALSE);
		}
	    }
	else if (!strcmp(cmd, "VALUE_HIGHLIGHT")) {
	    char strg[MAXSIGLEN],flag[MAXSIGLEN];
	    Boolean show_value=FALSE, add_cursor=FALSE;
	    VSearchNum search_pos;
	    line += config_read_color (trace, line, &search_pos);
	    search_pos--;
	    if (search_pos >= 0) {
		line += config_read_signal (line, strg);
		do {
		    line += config_read_signal (line, flag);
		    upcase_string (flag);
		    if (!strcmp(flag, "-CURSOR")) add_cursor=TRUE;
		    if (!strcmp(flag, "-VALUE")) show_value=TRUE;
		    } while (flag[0]=='-');
		/* Add it */
		global->val_srch[search_pos].color = (show_value) ? search_pos+1 : 0;
		global->val_srch[search_pos].cursor = (add_cursor) ? search_pos+1 : 0;
		string_to_value (trace, strg, global->val_srch[search_pos].value);
		val_update_search ();
		}
	    }
	else if (!strcmp(cmd, "CURSOR_ADD")) {
	    ColorNum color;
	    DTime ctime;
	    char strg[MAXSIGLEN],flag[MAXSIGLEN];
	    
	    line += config_read_color (trace, line, &color);
	    if (color >= 0) {
		line += config_read_signal (line, strg);
		ctime = string_to_time (trace, strg);
		line += config_read_signal (line, flag);
		upcase_string (flag);
		if (!strcmp(flag, "-USER"))
		    cur_add (ctime, color, USER);
		else cur_add (ctime, color, CONFIG);
		}
	    }
	else if (!strcmp(cmd, "TIME_GOTO")) {
	    DTime ctime;
	    char strg[MAXSIGLEN];
	    int end_time = global->time + (( trace->width - XMARGIN - global->xstart ) / global->res);
	    
	    line += config_read_signal (line, strg);
	    ctime = string_to_time (trace, strg);

	    if ((ctime < global->time) || (ctime > end_time)) {
		/* Slide time if it isn't on the screen already */
		global->time = ctime - (int)((trace->width-global->xstart)/global->res/2);
		new_time (trace);
		}
	    }
	else if (!strcmp(cmd, "RESOLUTION")) {
	    DTime restime;
	    char strg[MAXSIGLEN];
	    
	    line += config_read_signal (line, strg);
	    restime = string_to_time (trace, strg);

	    if (restime > 0) {
		global->res = RES_SCALE / (float)restime;
		new_res (trace);
		}
	    }
	else if (!strcmp(cmd, "SIGNAL_GOTO")) {
	    line += config_read_signal (line, pattern);
	    if (!pattern[0]) {
		config_error_ack (trace, "Signal_Goto signal name must not be null\n");
		}
	    else {
		sig_goto_pattern (trace, pattern);
		}
	    }
	else if (!strcmp(cmd, "REFRESH")) {
	    draw_all_needed ();
	    draw_manual_needed ();
	    /* Main loop won't refresh because widget's weren't activated on socket calls */
	    if (config_reading_socket) {
		draw_perform();
	        }
	    }
	else if (!strcmp(cmd, "ANNOTATE")) {
	    val_annotate_do_cb (NULL,trace,NULL);
	    }
	else if (!strcmp(cmd, "START_GEOMETRY")) {
	    if (*line=='"') line++;
	    config_parse_geometry (line, &(global->start_geometry));
	    }
	else if (!strcmp(cmd, "OPEN_GEOMETRY")) {
	    if (*line=='"') line++;
	    config_parse_geometry (line, &(global->open_geometry));
	    }
	else if (!strcmp(cmd, "SHRINK_GEOMETRY")) {
	    if (*line=='"') line++;
	    config_parse_geometry (line, &(global->shrink_geometry));
	    }
	else if (!strcmp(cmd, "SIGNAL_STATES")) {
	    memset (&newsigst, 0, sizeof (SIGNALSTATE));
	    line += config_read_signal (line, newsigst.signame);
	    processing_sig_state = TRUE;
	    /* if (DTPRINT) printf ("config_process_states  signal=%s\n", newsigst.signame); */
	    goto re_process_line;
	    }
	else {
	    sprintf (message, "Unknown command '%s'\n", cmd);
	    config_error_ack (trace, message);
	    }
	}
    }
    
/* Normal call */
#define config_process_line(trace, line)	config_process_line_internal(trace, line, FALSE)

/* EOF */
void	config_process_eof (TRACE *trace)
{
    char	line[3];
    line[0]='\0';	/* MIPS: no automatic aggregate initialization */
    line[1]='\0';
    line[2]='\0';
    config_process_line_internal (trace, line, TRUE);
    }

/**********************************************************************
 *	config_read_file
 **********************************************************************/

void config_read_file (trace, filename, report_notfound, report_errors)
    TRACE	*trace;
    char	*filename;	/* Specific filename of CONFIG file */
    Boolean	report_notfound, report_errors;
{
    FILE	*readfp;
    char line[1000];
    
    if (DTPRINT_CONFIG || DTPRINT_ENTRY) printf("Reading config file %s\n", filename);
    
    config_report_errors = report_errors;
    
    /* Open File For Reading */
    if (!(readfp=fopen(filename,"r"))) {
	if (report_notfound) {
	    if (DTPRINT) printf("%%E, Can't Open File %s\n", filename);
	    sprintf(message,"Can't open file %s",filename);
	    dino_error_ack(trace, message);
	    }
	return;
	}
    
    config_line_num=0;
    config_file = filename;
    config_reading_socket = FALSE;
    while (!feof(readfp)) {
	/* Read line & kill EOL at end */
	config_get_line (line, 1000, readfp);
	/* 	printf ("line='%s'\n",line);	*/
	config_process_line (trace, line);
	}

    config_process_eof (trace);

    fclose (readfp);
    }

/**********************************************************************
 *	config_read_socket
 **********************************************************************/

void config_read_socket (
			 char		*line,
			 char		*name,
			 int		cmdnum,
			 Boolean	eof
			 )
{
    config_report_errors = TRUE;
    config_line_num = cmdnum;
    config_file = name;
    config_reading_socket = TRUE;

    if (eof) {
	config_process_eof (global->trace_head);
	}
    else {
	config_process_line (global->trace_head, line);
	}
    }

/**********************************************************************
*	config_read_defaults
**********************************************************************/

void config_read_defaults (trace, report_errors)
    TRACE	*trace;
    Boolean	report_errors;
{
    char newfilename[MAXFNAMELEN];
    char *pchar;

    if (DTPRINT_ENTRY) printf ("In config_read_defaults\n");

    /* Erase old cursors */
    cur_delete_of_type (CONFIG);

    if (!global->suppress_config) {
#ifdef VMS
	config_read_file (trace, "DINODISK:DINOTRACE.DINO", FALSE, report_errors);
	config_read_file (trace, "DINOCONFIG:", FALSE, report_errors);
	config_read_file (trace, "SYS$LOGIN:DINOTRACE.DINO", FALSE, report_errors);
#else
	newfilename[0] = '\0';
	if (NULL != (pchar = getenv ("DINODISK"))) strcpy (newfilename, pchar);
	if (newfilename[0]) strcat (newfilename, "/");
	strcat (newfilename, "dinotrace.dino");
	config_read_file (trace, newfilename, FALSE, report_errors);
	
	newfilename[0] = '\0';
	if (NULL != (pchar = getenv ("DINOCONFIG"))) strcpy (newfilename, pchar);
	config_read_file (trace, newfilename, FALSE, report_errors);
	
	newfilename[0] = '\0';
	if (NULL != (pchar = getenv ("HOME"))) strcpy (newfilename, pchar);
	if (newfilename[0]) strcat (newfilename, "/");
	strcat (newfilename, "dinotrace.dino");
	config_read_file (trace, newfilename, FALSE, report_errors);
#endif
	
	/* Same directory as trace, dinotrace.dino */
	if (trace->filename != '\0') {
	    strcpy (newfilename, trace->filename);
	    file_directory (newfilename);
	    strcat (newfilename, "dinotrace.dino");
	    config_read_file (trace, newfilename, FALSE, report_errors);
	    }
	}

    /* Same file as trace, but .dino extension */
    if (trace->filename != '\0') {
	strcpy (newfilename, trace->filename);
	if ((pchar=strrchr(newfilename,'.')) != NULL )
	    *pchar = '\0';
	strcat (newfilename, ".dino");
	config_read_file (trace, newfilename, FALSE, report_errors);
	/* Chop one more period & try again */
	if (pchar) {
	    *pchar = '\0';
	    if ((pchar=strrchr(newfilename,'.')) != NULL ) {
		*pchar = '\0';
		strcat (newfilename, ".dino");
		config_read_file (trace, newfilename, FALSE, report_errors);
		}
	    }
	}

    /* Apply the statenames */
    update_signal_states (trace);
    val_update_search ();
    sig_update_search ();

    if (DTPRINT_ENTRY) printf ("Exit config_read_defaults\n");
    }

/**********************************************************************
*	config_writing
**********************************************************************/

void config_write_file (filename)
    char	*filename;	/* Specific filename of CONFIG file */
{
    TRACE	*trace;
    FILE	*writefp;
    SIGNAL	*sig_ptr;
    int		grid_num;
    GRID	*grid_ptr;
    
    if (DTPRINT_CONFIG || DTPRINT_ENTRY) printf("Writing config file %s\n", filename);
    
    /* Open File For Writing */
    if (!(writefp=fopen(filename,"w"))) {
	if (DTPRINT) printf("%%E, Can't Write File %s\n", filename);
	sprintf(message,"Can't write file %s",filename);
	dino_error_ack(trace, message);
	return;
	}

    fprintf (writefp, "! Customization Write by %s\n", DTVERSION);
    fprintf (writefp, "! Created %s\n", date_string(0));

    fprintf (writefp, "\n! ** GLOBAL FLAGS **\n");
    /* Debug and Print skipped */
    fprintf (writefp, "!debug\t\t%s\n", DTDEBUG?"ON":"OFF");
    fprintf (writefp, "!print\t\t%d\n", DTPRINT);
    fprintf (writefp, "!refreshing\t%d\n", global->redraw_manually?"MANUAL":"AUTO");
    fprintf (writefp, "save_enables\t%s\n", global->save_enables?"ON":"OFF");
    fprintf (writefp, "save_ordering\t%s\n", global->save_ordering?"ON":"OFF");
    fprintf (writefp, "click_to_edge\t%s\n", global->click_to_edge?"ON":"OFF");
    fprintf (writefp, "page_inc\t%d\n", 
	     global->pageinc==QPAGE ? 4 : (global->pageinc==QPAGE?2:1) );
    fprintf (writefp, "print_size\t");
    switch (global->print_size) {
      case PRINTSIZE_A:		fprintf (writefp, "A\n");	break;
      case PRINTSIZE_B:		fprintf (writefp, "B\n");	break;
      case PRINTSIZE_EPSLAND:	fprintf (writefp, "EPSLAND\n");	break;
      case PRINTSIZE_EPSPORT:	fprintf (writefp, "EPSPORT\n");	break;
	}
    if (global->time_format[0])
	fprintf (writefp, "time_format\t%s\n", global->time_format);
    fprintf (writefp, "time_multiplier\t%d\n", global->tempest_time_mult);
    fprintf (writefp, "time_precision\t%s\n", time_units_to_string (global->time_precision, TRUE));

    /*start_geometry*/
    /*open_geometry*/
    /*shrink_geometry*/
    /*signal_states*/
    /*signal_highlight*/
    /*value_highlight*/
    /*cursor_add*/
    /*time & position*/

    fprintf (writefp, "\n! ** TRACE FLAGS **\n");
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	if (trace->loaded) {
	    fprintf (writefp, "!set_trace\t%s\n", trace->filename);
	    fprintf (writefp, "file_format\t%s\n", filetypes[trace->fileformat].name);
	    fprintf (writefp, "cursor\t\t%s\n", trace->cursor_vis?"ON":"OFF");
	    fprintf (writefp, "signal_height\t%d\n", trace->sighgt);
	    fprintf (writefp, "vector_seperator\t\"%c\"\n", trace->vector_seperator);
	    fprintf (writefp, "rise_fall_time\t%d\n", trace->sigrf);
	    fprintf (writefp, "time_rep\t%s\n", time_units_to_string (trace->timerep, TRUE));
	    for (grid_num=0; grid_num<MAXGRIDS; grid_num++) {
		grid_ptr = &(trace->grid[grid_num]);

		fprintf (writefp, "grid\t\t%s\t%d\n", grid_ptr->visible?"ON":"OFF", grid_num);
		fprintf (writefp, "grid_resolution\t");
		switch (grid_ptr->period_auto) {
		  case PA_AUTO:		fprintf (writefp, "ASSERTION\t%d\n", grid_num);	break;
		  default:		fprintf (writefp, "%d\t%d\n", grid_ptr->period, grid_num);	break;
		}
		fprintf (writefp, "grid_align\t");
		switch (grid_ptr->align_auto) {
		  case AA_ASS:		fprintf (writefp, "ASSERTION\t%d\n", grid_num);	break;
		  case AA_DEASS:	fprintf (writefp, "DEASSERTION\t%d\n", grid_num);	break;
		  default:		fprintf (writefp, "%d\t%d\n", grid_ptr->alignment, grid_num);	break;
		}
	    }
	}
    }

    fprintf (writefp, "\n! ** GLOBAL INFORMATION **\n");
    cur_print (writefp);
    
    fprintf (writefp, "\n! ** TRACE INFORMATION **\n");
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	if (trace->loaded) {
	    fprintf (writefp, "!set_trace %s\n", trace->filename);
	    /* Save signal colors */
	    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
		if (sig_ptr->color && !sig_ptr->search) {
		    fprintf (writefp, "signal_highlight %d %s\n", sig_ptr->color, sig_ptr->signame);
		    }
		}
	    }
	}

    fclose (writefp);
    }

void config_write_cb (w,trace,cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    char newfilename[MAXFNAMELEN];
    char *pchar;

#ifdef VMS
    config_write_file ("SYS$LOGIN:DINOTRACE.WCONFIG");
#else
    newfilename[0] = '\0';
    if (NULL != (pchar = getenv ("HOME"))) strcpy (newfilename, pchar);
    if (newfilename[0]) strcat (newfilename, "/");
    strcat (newfilename, "dinotrace.wconfig");
    config_write_file (newfilename);
#endif
    }

/**********************************************************************
*	config_restore_defaults
**********************************************************************/

void config_restore_defaults(trace)
    TRACE	*trace;
{
    if (trace->signalstate_head != NULL)
	free_signal_states (trace);
    
    grid_reset_cb (NULL, trace, NULL);

    global->pageinc = FPAGE;
    global->save_ordering = TRUE;

    trace->sighgt = 20;	/* was 25 */
    trace->cursor_vis = TRUE;
    trace->sigrf = SIG_RF;
    trace->timerep = global->time_precision;
    trace->vector_seperator = '<';
    trace->vector_endseperator = '>';

    update_signal_states (trace);
    }
