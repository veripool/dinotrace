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
!	grid		ON | OFF
!	grid_align	<number> | ASSERTION | DEASSERTION | TWOCLOCK
!	grid_resolution	<number> | AUTO | DOUBLE
!	page_inc	4 | 2 | 1
!	print_size	A | B | EPSPORT | EPSLAND
!	rise_fall_time	<number>
!	signal_height	<number>
!	time_format	%0.6lf | %0.6lg | "" 
!	time_precision	US | NS | PS
!	time_rep	<number> | US | NS | PS | CYCLE
! File Format:
!	file_format	DECSIM | TEMPEST | VERILOG | DECSIM_Z
!	save_enables	ON | OFF
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
!	cursor_add	<color> <time>
!	signal_highlight <color> <signal_name>
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

int line_num=0;
Boolean config_report_errors;
char *current_file="";

/* Line reading support */
char UnGetLine[1000];
int  UnGot=0;

#define unget_line(line) {strcpy(UnGetLine,line);UnGot=1;}

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

    /* Allow 1 line of 'unget' */
    if (UnGot) {
	strcpy(line, UnGetLine);
	UnGot=0;
	return;
	}

    /* Read line & kill EOL at end */
    fgets (line, len, readfp);
    if (*(tp=(line+strlen(line)-1))=='\n') *tp='\0';
    line_num++;
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
    strncpy(out, line, MAXSTATELEN);
    out[MAXSTATELEN-1]='\0';
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


int	config_read_on_off (line, out)
    char *line;
    int *out;
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
	*out = -1;
	}

    return (outlen);
    }

/**********************************************************************
*	pattern matching from GNU (see wildmat.c)
**********************************************************************/

static int wildStar(s, p)
    register char	*s;
    register char	*p;
{
    while (wildmat(s, p) == FALSE)
	if (*++s == '\0')
	    return(FALSE);
    return(TRUE);
    }

int wildmat(s, p)
    register char	*s;	/* Buffer */
    register char	*p;	/* Pattern */
{
    for ( ; *p; s++, p++) {
	if (*p!='*') {
	    if (toupper(*s) != toupper(*p)  && *p != '?')
		return(FALSE);
	    }
	else {
	    /* Trailing star matches everything. */
	    return(*++p ? wildStar(s, p) : TRUE);
	    }
	}
    return(*s == '\0');
    }

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

    new = (SIGNALSTATE *)(XtMalloc (sizeof(SIGNALSTATE)));
    memcpy ((void *)new, (void *)info, sizeof(SIGNALSTATE));
    new->next = trace->signalstate_head;
    trace->signalstate_head = new;
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
	printf("Signal %s:\n",sstate_ptr->signame);
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
    sprintf (newmessage + strlen(newmessage), "on line %d of %s\n", line_num, current_file);
    dino_error_ack (trace, newmessage);
    }


/**********************************************************************
*	config_process_states
**********************************************************************/

void	config_process_states (trace, oldline, readfp)
    TRACE	*trace;
    char *oldline;
    FILE *readfp;
{
    SIGNALSTATE newsigst;
    char newline[1000];
    char newstate[MAXSTATELEN];
    char *line;
    int statenum=0;

    memset (&newsigst, 0, sizeof (SIGNALSTATE));

    strcpy (newline, oldline);
    line = newline;
    line += config_read_signal (line, newsigst.signame);

    /* if (DTPRINT) printf ("config_process_states  signal=%s\n", newsigst.signame); */

    while (*line!='}') {
	if (!*line) {
	    if (feof(readfp)) {
		config_error_ack (trace, "Unexpected EOF during SIGNAL_STATES\n");
		return;
		}
	    config_get_line (newline, 1000, readfp);
	    line = newline;
	    if (*line=='!' || *line==';') line[0]='\0';  /*comment*/
	    }

	while (*line && isspace(*line)) 
	    line++;

	if (*line && *line!='}') {
	    line += config_read_state (line, newstate, &statenum);
	    /* printf ("Got = %d '%s'\n", statenum, newstate); */
	    if (statenum < 0 || statenum >= MAXSTATENAMES) {
		sprintf (message, "Over maximum of %d SIGNAL_STATES for signal %s\n",
			 MAXSTATENAMES, newsigst.signame);
		config_error_ack (trace,message);
		/* No return, as will just ignore remaining errors */
		}
	    else {
		if (newstate[0]) {
		    strcpy (newsigst.statename[statenum], newstate);
		    statenum++;
		    }
		}
	    }
	}

    add_signal_state (trace, &newsigst);
    /*
    printf ("Signal '%s' Assigned States:\n",newsigst.signame);
    for (t=0; t<MAXSTATENAMES; t++)
	if (newsigst.statename[t][0] != '\0')
	    printf ("State %d = '%s'\n", t, newsigst.statename[t]);
	    */
    }

/**********************************************************************
*	config_process_line
**********************************************************************/

void	config_process_line (trace, line, readfp)
    TRACE	*trace;
    char *line;
    FILE *readfp;
{
    char cmd[MAXSIGLEN];
    int value;

    /* Comment */
    while (*line && isspace(*line)) line++;
    if ((line[0]=='!') || (line[0]==';') || (line[0]=='\0')) return;

    /* Preprocessor #INCLUDE eventually */
    /* extract command */
    line += config_read_signal (line, cmd);
    while (*line && isspace(*line)) line++;
    upcase_string (cmd);

    /* if (DTPRINT_CONFIG) printf ("Cmd='%s'\n",cmd); */

    if (!strcmp(cmd, "DEBUG")) {
	value=DTDEBUG;
	line += config_read_on_off (line, &value);
	if (DTPRINT) printf ("Config: DTDEBUG=%d\n",value);
	if (value >= 0) {
	    DTDEBUG=value;
	    }
	else {
	    config_error_ack (trace, "Debug must be set ON or OFF\n");
	    }
	}
    else if (!strcmp(cmd, "PRINT")) {
	value=DTPRINT;
	if (toupper(line[0])=='O' && toupper(line[0])=='N') DTPRINT = -1;
	if (toupper(line[0])=='O' && toupper(line[0])=='F') DTPRINT = 0;
	if (value >= 0) {
	    sscanf (line, "%lx", &value);
	    DTPRINT=value;
	    }
	else {
	    config_error_ack (trace, "Print must be set ON or OFF\n");
	    }
	if (DTPRINT) printf ("Config: DTPRINT=0x%x\n",value);
	}
    else if (!strcmp(cmd, "SAVE_ENABLES")) {
	value=global->save_enables;
	line += config_read_on_off (line, &value);
	if (value >= 0) {
	    global->save_enables=value;
	    }
	else {
	    config_error_ack (trace, "Save_enables must be set ON or OFF\n");
	    }
	}
    else if (!strcmp(cmd, "CURSOR")) {
	value = trace->cursor_vis;
	line += config_read_on_off (line, &value);
	if (DTPRINT_CONFIG) printf ("Config: cursor_vis=%d\n",value);
	if (value >= 0) {
	    trace->cursor_vis = value;
	    }
	else {
	    config_error_ack (trace, "Cursor must be set ON or OFF\n");
	    }
	}
    else if (!strcmp(cmd, "GRID")) {
	value = trace->grid_vis;
	line += config_read_on_off (line, &value);
	if (DTPRINT_CONFIG) printf ("Config: grid_vis=%d\n",value);
	if (value >= 0) {
	    trace->grid_vis = value;
	    }
	else {
	    config_error_ack (trace, "Grid must be set ON or OFF\n");
	    }
	}
    else if (!strcmp(cmd, "CLICK_TO_EDGE")) {
	value = global->click_to_edge;
	line += config_read_on_off (line, &value);
	if (DTPRINT_CONFIG) printf ("Config: click_to_edge=%d\n",value);
	if (value >= 0) {
	    global->click_to_edge = value;
	    }
	else {
	    config_error_ack (trace, "Click_to_edge must be set ON or OFF\n");
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
    else if (!strcmp(cmd, "GRID_RESOLUTION")) {
	value = trace->grid_res;
	line += config_read_int (line, &value);
	if (value >= 1) {
	    trace->grid_res = value;
	    trace->grid_res_auto = value;
	    }
	else {
	    if (toupper(line[0])=='A')
		trace->grid_res_auto = GRID_RES_AUTO;
	    else if (toupper(line[0])=='D')
		trace->grid_res_auto = GRID_RES_AUTO_DOUBLE;
	    else {
		config_error_ack (trace, "Grid_res must be >0, ASSERTION, or DOUBLE\n");
		}
	    }
	}
    else if (!strcmp(cmd, "GRID_ALIGN")) {
	value = trace->grid_align;
	line += config_read_int (line, &value);
	if (value >= 1) {
	    trace->grid_align = value;
	    trace->grid_align_auto = value;
	    }
	else {
	    if (toupper(line[0])=='A')
		trace->grid_align_auto = GRID_ALN_AUTO_ASS;
	    else if (toupper(line[0])=='D')
		trace->grid_align_auto = GRID_ALN_AUTO_DEASS;
	    else if (toupper(line[0])=='T')
		trace->grid_align_auto = GRID_ALN_AUTO_TWOCLOCK;
	    else {
		config_error_ack (trace, "Grid_align must be >0, ASSERTION, DEASSERTION, or TWOCLOCK\n");
		}
	    }
	if (DTPRINT_CONFIG) printf ("grid_align_auto = %d\n", trace->grid_align_auto);
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
	    if (toupper (*tp) == 'Z')
		file_format = FF_DECSIM_Z;
	    else file_format = FF_DECSIM;
	    break;
	  case 'T':
	    file_format = FF_TEMPEST;
	    break;
	  case 'V':
	    file_format = FF_VERILOG;
	    break;
	  default:
	    config_error_ack (trace, "File_Format must be DECSIM, TEMPEST or VERILOG\n");
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
	if (DTPRINT_CONFIG) printf ("Vector_seperator = '%c'\n", trace->vector_seperator);
	}
    else if (!strcmp(cmd, "SIGNAL_HIGHLIGHT")) {
	char pattern[MAXSIGLEN];
	int color;
	line += config_read_int (line, &color);
	if (color < 0 || color >= MAXCOLORS) {
	    sprintf (message, "Signal_Highlight color must be 0 to %d\n", MAXCOLORS);
	    config_error_ack (trace, message);
	    }
	else {
	    line += config_read_signal (line, pattern);
	    if (!pattern[0]) {
		config_error_ack (trace, "Signal_Highlight signal name must not be null\n");
		}
	    else {
		sig_highlight_pattern (trace, color, pattern);
		}
	    }
	}
    else if (!strcmp(cmd, "SIGNAL_DELETE")) {
	char pattern[MAXSIGLEN];
	line += config_read_signal (line, pattern);
	if (!pattern[0]) {
	    config_error_ack (trace, "Signal_Delete signal name must not be null\n");
	    }
	else {
	    sig_delete_pattern (trace, pattern);
	    }
	}
    else if (!strcmp(cmd, "SIGNAL_DELETE_CONSTANT")) {
	char pattern[MAXSIGLEN];
	line += config_read_signal (line, pattern);
	if (!pattern[0]) {
	    config_error_ack (trace, "Signal_Delete_Constant signal name must not be null\n");
	    }
	else {
	    sig_delete_constant_pattern (trace, pattern);
	    }
	}
    else if (!strcmp(cmd, "CURSOR_ADD")) {
	int color;
	DTime ctime;
	char strg[MAXSIGLEN];

	line += config_read_int (line, &color);
	if (color < 0 || color >= MAXCOLORS) {
	    sprintf (message, "Cursor_Add color must be 0 to %d\n", MAXCOLORS);
	    config_error_ack (trace,message);
	    }
	else {
	    line += config_read_signal (line, strg);
	    ctime = string_to_time (trace, strg);
	    cur_add (ctime, color, CONFIG);
	    }
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
	config_process_states (trace, line, readfp);
	}
    else {
	sprintf (message, "Unknown command '%s'\n", cmd);
	config_error_ack (trace, message);
	}
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

    line_num=0;
    current_file = filename;
    while (!feof(readfp)) {
	/* Read line & kill EOL at end */
	config_get_line (line, 1000, readfp);
	/* 	printf ("line='%s'\n",line);	*/
	config_process_line (trace, line, readfp);
	}

    fclose (readfp);
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

    /* Erase old cursors */
    cur_delete_of_type (CONFIG);

    if (!global->suppress_config) {
#ifdef VMS
	config_read_file (trace, "DINODISK:DINOTRACE.DINO", FALSE, report_errors);
	config_read_file (trace, "SYS$LOGIN:DINOTRACE.DINO", FALSE, report_errors);
#else
	newfilename[0] = '\0';
	if (NULL != (pchar = getenv ("DINODISK"))) strcpy (newfilename, pchar);
	if (newfilename[0]) strcat (newfilename, "/");
	strcat (newfilename, "dinotrace.dino");
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
	}

    /* Apply the statenames */
    update_signal_states (trace);
    val_update_search ();
    sig_update_search ();
    }

/**********************************************************************
*	config_restore_defaults
**********************************************************************/

void config_restore_defaults(trace)
    TRACE	*trace;
{
    if (trace->signalstate_head != NULL)
	free_signal_states (trace);
    
    global->pageinc = FPAGE;

    trace->sighgt = 20;	/* was 25 */
    trace->cursor_vis = TRUE;
    trace->grid_vis = TRUE;
    trace->grid_res = 100;
    trace->grid_align = 0;
    trace->numpag = 1;
    trace->sigrf = SIG_RF;
    trace->timerep = global->time_precision;
    trace->vector_seperator = '<';

    update_signal_states (trace);
    }
