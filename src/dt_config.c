#pragma ident "$Id$"
/******************************************************************************
 * dt_config.c --- configuration file reading
 *
 * This file is part of Dinotrace.  
 *
 * Author: Wilson Snyder <wsnyder@ultranet.com> or <wsnyder@iname.com>
 *
 * Code available from: http://www.ultranet.com/~wsnyder/veripool/dinotrace
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
 ******************************************************************************
# Undocumented:
#	debug		ON | OFF
#	print		<number> | ON | OFF
#
#	# Comment
# Config:
#	click_to_edge	ON | OFF
#	cursor		ON | OFF
#	refreshing	AUTO | MANUAL
#	grid		<grid_number>	ON | OFF	
#	grid_align	<grid_number>	<number> | ASSERTION | DEASSERTION
#	grid_resolution	<grid_number>	<number> | AUTO | EDGE 	 
#	grid_type	<grid_number>	NORMAL | WIDE
#	grid_signal	<grid_number>	<signal_pattern>
#	grid_color	<grid_number>	<color>
#	page_inc	4 | 2 | 1
#	print_size	A | B | EPSPORT | EPSLAND
#	rise_fall_time	<number>
#	signal_height	<number>
#	time_format	%0.6lf | %0.6lg | "" 
#	time_precision	US | NS | PS
#	time_rep	<number> | US | NS | PS | CYCLE
# File Format:
#	file_format	DECSIM | TEMPEST | VERILOG
#	save_enables	ON | OFF
#	save_ordering	ON | OFF
#	save_duplicates	ON | OFF
#	signal_states	<signal_pattern> = {<State>, <State>...}
#	vector_separator "<char>"
#	hierarchy_separator "<chars>"
#       time_multiplier	<number>
# Geometry/resources:
#	open_geometry	<width>[%]x<height>[%]+<xoffset>[%]+<yoff>[%]
#	shrink_geometry	<width>%x<height>%+<xoffset>%+<yoff>%
#	start_geometry	<width>x<height>+<xoffset>+<yoffset>
# Modification of traces:
#	signal_delete	<signal_pattern>
#	signal_delete_constant	<signal_pattern> [-IGNOREXZ]
#	signal_add	<signal_pattern>	[<after_signal_first_matches>]	[<color>]
#	signal_copy	<signal_pattern>	[<after_signal_first_matches>]	[<color>]
#	signal_move	<signal_pattern>	[<after_signal_first_matches>]	[<color>]
#	signal_radix	<signal_pattern> <HEX|BINARY|OCT|ASCII|DEC>
#	signal_rename	<signal_pattern> <new_signal_name>	[<color>]
#	signal_highlight <signal_pattern> <color> 
#	signal_note	<signal_pattern> <note> 
#	cursor_add	<time> <color>	[-USER]
#	value_highlight <value>	<color>	[<signal_pattern>] [-CURSOR] [-VALUE]
# Display changes:
#	signal_goto	<signal_pattern>	(if not on screen, first match)
#	time_goto	<time>
#	resolution	<res>
#	refresh
#	annotate
 *****************************************************************************/


#include "dinotrace.h"

#include "functions.h"

/**********************************************************************/

/* See the ascii map to have this make sense */
#define issigchr(ch)  ( ((ch)>' ') && ((ch)!='=') && ((ch)!='\"') && ((ch)!='\'') )

#define isstatechr(ch)  ( ((ch)>' ') && ((ch)!=',') && ((ch)!='=') && ((ch)!='{') && ((ch)!='}') && ((ch)!='\"') && ((ch)!='\'') )

#define isstatesep(ch)  (!isstatechr(ch) && (ch)!='}' )


/* File locals */
static Boolean_t config_report_errors;
static char *config_file="";
static int config_line_num=0;
static Boolean_t config_reading_socket;

/**********************************************************************
 * Utils
 */

void	config_error_ack (
    Trace	*trace,
    char	*message)
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
*	READING FUNCTIONS
**********************************************************************/

static void config_get_line (
    char *line,
    int len,
    FILE *readfp)
{
    char *tp;

    /* Read line & kill EOL at end */
    fgets (line, len, readfp);
    if (*(tp=(line+strlen(line)-1))=='\n') *tp='\0';
    config_line_num++;
}

static int config_read_string (
    Trace *trace,
    char *line,
    char *out)
{
    int outlen=0;
    Boolean_t quote = FALSE;

    while (*line && isspace(*line)) {
	line++; outlen++;
    }

    if (*line=='"') {
	line++; outlen++;
	quote = TRUE;
    }

    while (*line && (quote ? *line!='"' : issigchr(*line))) {
	if (*line == '\\') {
	    line++;
	    outlen++;
	    switch (*line) {
	    case '\0':
		line--;
		break;
	    case 'n':
		*out++ = '\n';
		break;
	    default:
		*out++ = *line;
	    }
	}
	else {
	    *out++ = *line;
	}
	line++; outlen++;
    }

    if (quote) { 
	if (*line == '"') {
	    line++; outlen++;
	} else {
	    config_error_ack (trace, "Double quotes aren't terminated\n");
	}
    }
    *out++ = '\0';
    return (outlen);
}

static int config_read_value (
    Trace *trace,
    char *line,
    char *out)
{
    int outlen=0;
    Boolean_t quote = FALSE;

    while (*line && isspace(*line)) {
	line++; outlen++;
    }

    if (*line=='"') {
	*out++ = *line;
	line++; outlen++;
	quote = TRUE;
    }

    while (*line && (quote ? *line!='"' : !isspace(*line))) {
	if (*line == '\\') {
	    line++;
	    outlen++;
	    switch (*line) {
	    case '\0':
		line--;
		break;
	    case 'n':
		*out++ = '\n';
		break;
	    default:
		*out++ = *line;
	    }
	}
	else {
	    *out++ = *line;
	}
	line++; outlen++;
    }

    if (quote) { 
	if (*line == '"') {
	    *out++ = *line;
	    line++; outlen++;
	} else {
	    config_error_ack (trace, "Double quotes aren't terminated\n");
	}
    }
    *out++ = '\0';
    return (outlen);
}

static int config_read_pattern (
    Trace	*trace,
    char *line,
    char *pattern)
{
    int outlen;
    outlen = config_read_string (trace, line, pattern);
    if (!pattern[0]) {
        config_error_ack (trace, "Signal pattern name must not be null\n");
    }
    return (outlen);
}

static int config_read_state (
    char *line,
    char *out,
    int *statenum_ptr)
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

static int config_read_int (
    char *line,
    int *out)
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
*	SUPPORT FUNCTIONS
**********************************************************************/

SignalState_t	*signalstate_find (
    const Trace	*trace,
    const char *name)
{
    register SignalState_t *sig;

    for (sig=global->signalstate_head; sig; sig=sig->next) {
	/* printf ("'%s'\t'%s'\n", name, sig->signame); */
	if (wildmat(name, sig->signame))
	    return (sig);
    }
    return (NULL);
}

static void	signalstate_add (
    Trace	*trace,
    SignalState_t *info)
{
    SignalState_t *new;
    int t;

    for (new = global->signalstate_head; new ; new=new->next) {
      if (!strcmp (new->signame, info->signame)) break;
    }
    if (! new) {
      /* Not found, add & relink */
      new = XtNew (SignalState_t);
      memcpy ((void *)new, (void *)info, sizeof(SignalState_t));
      new->next = global->signalstate_head;
      global->signalstate_head = new;
      draw_needupd_val_states ();
    }
    else {
      /* Found, overwrite old */
      info->next = new->next;	/* Don't loose the link */
      memcpy ((void *)new, (void *)info, sizeof(SignalState_t));
    }

    /*printf ("Signal '%s' Assigned States:\n", new->signame);*/
    for (t=0; t<MAXSTATENAMES; t++) {
	if (new->statename[t][0] != '\0') {
	    new->numstates = t+1;
	    /*printf ("State %d = '%s'\n", t, new->statename[t]);*/
	}
    }
}

static void	signalstate_free (void)
{
    SignalState_t *sstate_ptr, *last_ptr;

    sstate_ptr = global->signalstate_head;
    while (sstate_ptr) {
	last_ptr = sstate_ptr->next;
	DFree (sstate_ptr);
	sstate_ptr = last_ptr;
    }
    global->signalstate_head = NULL;
}

static void	signalstate_write (
    FILE *writefp, char *c)
{
    SignalState_t *sstate_ptr;
    int i;

    fprintf (writefp, "\n#### Global Signal States ####\n");
    
    for (sstate_ptr = global->signalstate_head; 
	 sstate_ptr; sstate_ptr = sstate_ptr->next) {
	fprintf (writefp, "%ssignal_states %s {\n",c,sstate_ptr->signame);
	for (i=0; i<MAXSTATENAMES; i++) {
	    if (sstate_ptr->statename[i][0]) {
		fprintf (writefp, "%s\t%d=\"%s\",\n",c, i, sstate_ptr->statename[i]);
	    }
	}
	fprintf (writefp, "%s\t}\n",c);
    }
    fprintf (writefp,"\n");
}

void	config_parse_geometry (
    char	*line,
    Geometry_t	*geometry)
    /* Like XParseGeometry, but handles percentages */
{
    int		flags;
    int		x,y;
    uint_t	wid, hei;
    char	noper_line[100];
    char	*tp;

    /* Copy line into a temp, remove the percentage symbols, and parse it */
    strcpy (noper_line, line);
    while ((tp=strchr (noper_line, '%'))) strcpy (tp, tp+1);

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

static void	config_geometry_string (
    Geometry_t	*geometry,
    char *strg)
{
    sprintf (strg, "%d%sx%d%s+%d%s+%d%s",
	     geometry->width,	geometry->widthp?"%":"",
	     geometry->height,	geometry->heightp?"%":"", 
	     geometry->x,	geometry->xp?"%":"",
	     geometry->y,	geometry->xp?"%":"");
}


/**********************************************************************
*	reading functions w/ error messages
**********************************************************************/

static int	config_read_on_off (
    Trace	*trace,
    char *line,
    int *out)
    /* Read boolean flag line, return <= 0 and print msg if bad */
{
    char cmd[MAXSIGLEN];
    int outlen;

    outlen = config_read_string (trace, line, cmd);

    if (!strcasecmp (cmd, "ON") || !strcmp (cmd, "1"))
	*out = 1;
    else if (!strcasecmp (cmd, "OFF") || !strcmp (cmd, "0"))
	*out = 0;
    else {
	sprintf (message, "Expected ON or OFF switch\n");
	config_error_ack (trace, message);
	*out = -1;
    }

    return (outlen);
}

static int	config_read_color (
    Trace	*trace,
    char 	*line,
    ColorNum_t	*color,
    Boolean_t	warn)
    /* Read color name from line, return <= 0 and print msg if bad */
{
    int outlen=0;
    char cmd[MAXSIGLEN];

    while (*line && isspace(*line)) {
	line++; outlen++;
    }

    switch (*line) {
    case 'N': case 'n':
	outlen += config_read_string (trace, line, cmd);
	*color = global->highlight_color;
	break;
    case 'C': case 'c':
	/* Duplicate code in submenu_to_color */
	outlen += config_read_string (trace, line, cmd);
	if ((++global->highlight_color) > MAX_SRCH) global->highlight_color = 1;
	*color = global->highlight_color;
	break;
    case '0': case '1': case '2':case '3':case '4':
    case '5': case '6': case '7':case '8':case '9':
	outlen += config_read_int (line, color);
	if (*color < 0 || *color > MAX_SRCH) {
	    if (warn) {
		sprintf (message, "Color numbers must be 0 to %d, NEXT or CURRENT\n", MAX_SRCH);
		config_error_ack (trace, message);
	    }
	    *color = -1;
	}
	break;
    default:
	*color = -1;
	break;
    }

    return (outlen);
}

int	config_read_grid (
    Trace	*trace,
    char 	*line,
    Grid_t	**grid_pptr)
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

static int config_process_state (
    Trace	*trace,
    char 	*line,
    SignalState_t *sstate_ptr)
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

static void	config_process_line_internal (
    Trace	*trace,
    char	*line,
    Boolean_t	format_only,
    Boolean_t	eof)		/* Final call of process_line with EOF */
{
    char cmd[MAXSIGLEN];
    int value;
    Grid_t	*grid_ptr;
    char pattern[MAXSIGLEN];
    char *cmt;
    
    static SignalState_t newsigst;
    static Boolean_t processing_sig_state = FALSE;

  re_process_line:
    
    /* Strip spaces and comments */
    while (*line && isspace(*line)) line++;
    cmt = line;
    while (cmt != NULL) {
	cmt=strpbrk(cmt,"!#\"");
	if (cmt!=NULL) {
	    if (*cmt=='\"') {
		cmt=strpbrk(cmt+1,"\"");	/* Skip to closing quote */
		continue;
	    }
	    *cmt = '\0';
	    break;
	}
    }
    if ((line[0]==';') || (line[0]=='\0')) return;

    /* if (DTPRINT_CONFIG) printf ("Cmd='%s'\n",cmd); */

    if (processing_sig_state) {
	if (*line == ';' || *line=='}' || eof) {
	    if (eof) {
		config_error_ack (trace, "Unexpected EOF during SIGNAL_STATES\n");
	    }
	    signalstate_add (trace, &newsigst);
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
	line += config_read_string (trace, line, cmd);
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
		sscanf (line, "%x", &value);
		if (value > 0) {
		    DTPRINT=value;
		}
		else {
		    config_error_ack (trace, "Print must be set ON, OFF, or > 0\n");
		}
	    }
	    if (DTPRINT) printf ("Config: DTPRINT=0x%x\n",value);
	}
	/************************************************************/
	/* Commands needed before a file is read */
	/* These should be only global or dfile parameters (not trace parameters) */
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
		if (toupper(line[1]) == 'P') {
		    file_format = FF_VERILOG_VPD;
		} else {
		    file_format = FF_VERILOG;
		}
		break;
	      default:
		config_error_ack (trace, "File_Format must be DECSIM, TEMPEST, VPD or VERILOG\n");
	    }
	    if (DTPRINT_CONFIG) printf ("File_format = %d\n", file_format);
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
	else if (!strcmp(cmd, "SAVE_DUPLICATES")) {
	    value=global->save_duplicates;
	    line += config_read_on_off (trace, line, &value);
	    if (value >= 0) {
		global->save_duplicates=value;
	    }
	}
	else if (!strcmp(cmd, "TIME_REP")) {
	    switch (toupper(line[0])) {
	      case 'N':	global->timerep = TIMEREP_NS;	break;
	      case 'U':	global->timerep = TIMEREP_US;	break;
	      case 'P':	global->timerep = TIMEREP_PS;	break;
	      case 'C':	global->timerep = TIMEREP_CYC;	break;
	      case '0': case '1': case '2': case '3': case '4':
	      case '5': case '6': case '7': case '8': case '9': case '.':
		global->timerep = atof(line);	break;
	      default:
		config_error_ack (trace, "Time_Rep must be PS, NS, US, or CYCLE\n");
	    }
	    if (DTPRINT_CONFIG) printf ("timerep = %f\n", global->timerep);
	}
	else if (!strcmp(cmd, "TIME_PRECISION")) {
	    switch (toupper(line[0])) {
	      case 'N':	global->time_precision = TIMEREP_NS;	break;
	      case 'U':	global->time_precision = TIMEREP_US;	break;
	      case 'P':	global->time_precision = TIMEREP_PS;	break;
	      default:
		config_error_ack (trace, "Time_Precision must be PS, NS, or US\n");
	    }
	    if (DTPRINT_CONFIG) printf ("time_precision = %f\n", global->time_precision);
	}
	else if (!strcmp(cmd, "TIME_FORMAT")) {
	    line += config_read_string (trace, line, cmd);
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
	else if (!strcmp(cmd, "HIERARCHY_SEPARATOR")) {
	    line += config_read_string (trace, line, pattern);
	    if (pattern[0]) {
		trace->dfile.hierarchy_separator = pattern[0];
	    }
	}
	else if (!strcmp(cmd, "VECTOR_SEPERATOR")	/* Backward compatible spelling! */
		 || !strcmp(cmd, "VECTOR_SEPARATOR")) {
	    if (*line=='"') line++;
	    if (*line && *line!='"') {
		trace->dfile.vector_separator = line[0];
	    }
	    else {
		trace->dfile.vector_separator = '\0';
	    }
	    /* Take a stab at the ending character */
	    switch (trace->dfile.vector_separator) {
	      case '`':	trace->dfile.vectorend_separator = '\''; break;
	      case '(':	trace->dfile.vectorend_separator = ')'; break;
	      case '[':	trace->dfile.vectorend_separator = ']'; break;
	    case '{':	trace->dfile.vectorend_separator = '}'; break;
	      case '<':	trace->dfile.vectorend_separator = '>'; break;
	      default:  trace->dfile.vectorend_separator = trace->dfile.vector_separator; break;	/* a wild guess SIG$20:1$? */
	    }
	    if (DTPRINT_CONFIG) printf ("vector_separator = '%c'  End='%c'\n", 
					trace->dfile.vector_separator, trace->dfile.vectorend_separator);
	}
	else if (format_only) {
	    return;
	}
	/**************************************************************/
	/** beginning of non-file_format commands */
	/**************************************************************/
	else if (!strcmp(cmd, "CURSOR")) {
	    value = global->cursor_vis;
	    line += config_read_on_off (trace, line, &value);
	    if (value >= 0) {
		if (DTPRINT_CONFIG) printf ("Config: cursor_vis=%d\n",value);
		global->cursor_vis = value;
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
	    value = global->sighgt;
	    line += config_read_int (line, &value);
	    if (value >= 10 && value <= 50)
		global->sighgt = value;
	    else {
		config_error_ack (trace, "Signal_height must be 10-50\n");
	    }
	}
	else if (!strcmp(cmd, "GRID")) {
	    line += config_read_grid (trace, line, &grid_ptr);
	    line += config_read_on_off (trace, line, &value);
	    if (grid_ptr && value >= 0) {
		if (DTPRINT_CONFIG) printf ("Config: grid_vis=%d\n",value);
		grid_ptr->visible = value;
	    }
	}
	else if (!strcmp(cmd, "GRID_RESOLUTION")) {
	    line += config_read_grid (trace, line, &grid_ptr);
	    line += config_read_int (line, &value);
	    if (isalpha(line[0])) { line += config_read_string (trace, line, pattern); }
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
			config_error_ack (trace, "Grid_res must be >0, AUTO or EDGE\n");
		    }
		}
	    }
	}
	else if (!strcmp(cmd, "GRID_ALIGN")) {
	    line += config_read_grid (trace, line, &grid_ptr);
	    line += config_read_int (line, &value);
	    if (isalpha(line[0])) { line += config_read_string (trace, line, pattern); }
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
		    else if (toupper(pattern[0])=='B')
			grid_ptr->align_auto = AA_BOTH;
		    else {
			config_error_ack (trace, "Grid_align must be >0, ASSERTION, or DEASSERTION, or BOTH\n");
		    }
		}
	    }
	}
	else if (!strcmp(cmd, "GRID_TYPE")) {
	    line += config_read_grid (trace, line, &grid_ptr);
	    line += config_read_int (line, &value);
	    if (isalpha(line[0])) { line += config_read_string (trace, line, pattern); }
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
	    line += config_read_grid (trace, line, &grid_ptr);
	    line += config_read_string (trace, line, pattern);
	    if (grid_ptr && *pattern) {
		strcpy (grid_ptr->signal, pattern);
	    }
	}
	else if (!strcmp(cmd, "GRID_COLOR")) {
	    ColorNum_t color;
	    line += config_read_grid (trace, line, &grid_ptr);
	    line += config_read_color (trace, line, &color, TRUE);
	    if (grid_ptr && color>=0) {
		grid_ptr->color = color;
	    }
	}
	else if (!strcmp(cmd, "RISE_FALL_TIME")) {
	    value = global->sigrf;
	    line += config_read_int (line, &value);
	    /* Valid values not known... */
	    global->sigrf = value;
	}
	else if (!strcmp(cmd, "PAGE_INC")) {
	    value = global->pageinc;
	    line += config_read_int (line, &value);
	    if (value == 1) global->pageinc = PAGEINC_FULL;
	    else if (value == 2) global->pageinc = PAGEINC_HALF;
	    else if (value == 4) global->pageinc = PAGEINC_QUARTER;
	    else {
		config_error_ack (trace, "Page_Inc must be 1, 2, or 4\n");
	    }
	    if (DTPRINT_CONFIG) printf ("page_inc = %d\n", global->pageinc);
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
	else if (!strcmp(cmd, "SIGNAL_HIGHLIGHT")) {
	    ColorNum_t color;
	    line += config_read_pattern (trace, line, pattern);
	    if (pattern[0]) {
		line += config_read_color (trace, line, &color, TRUE);
		if (color >= 0) {
		    sig_wildmat_select (NULL, pattern);
		    sig_highlight_selected (color);
		}
	    }
	}
	else if (!strcmp(cmd, "SIGNAL_NOTE")) {
	    char pattern2[MAXSIGLEN];
	    line += config_read_pattern (trace, line, pattern);
	    line += config_read_string (trace, line, pattern2);
	    if (pattern[0] && pattern2[0]) {
	        sig_wildmat_select (NULL, pattern);
	        sig_note_selected (pattern2);
	    }
	}
	else if (!strcmp(cmd, "SIGNAL_RADIX")) {
	    line += config_read_pattern (trace, line, pattern);
	    if (pattern[0]) {
		Radix_t *radix_ptr;
		char strg[MAXSIGLEN];
		line += config_read_string (trace,line, strg);
		radix_ptr = val_radix_find (strg);
		if (radix_ptr==NULL) {
		    config_error_ack (trace, "Undefined radix name\n");
		} else {
		    sig_wildmat_select (NULL, pattern);
		    sig_radix_selected (radix_ptr	);
		}
	    }
	}
	else if (!strcmp(cmd, "SIGNAL_ADD")) {
	    char pattern2[MAXSIGLEN];
	    line += config_read_pattern (trace, line, pattern);
	    if (pattern[0]) {
		ColorNum_t color;
		line += config_read_string (trace, line, pattern2);
		line += config_read_color (trace, line, &color, FALSE);
		sig_wildmat_select (global->deleted_trace_head, pattern);
		sig_move_selected (trace, pattern2);
		if (color >= 0) {
		    sig_highlight_selected (color);
		}
	    }
	}
	else if (!strcmp(cmd, "SIGNAL_MOVE")) {
	    char pattern2[MAXSIGLEN];
	    line += config_read_pattern (trace, line, pattern);
	    if (pattern[0]) {
		ColorNum_t color;
		line += config_read_pattern (trace, line, pattern2);
		line += config_read_color (trace, line, &color, FALSE);
		sig_wildmat_select (NULL, pattern);
		sig_move_selected (trace, pattern2);
		if (color >= 0) {
		    sig_highlight_selected (color);
		}
	    }
	}
	else if (!strcmp(cmd, "SIGNAL_RENAME")) {
	    char pattern2[MAXSIGLEN];
	    line += config_read_pattern (trace, line, pattern);
	    line += config_read_pattern (trace, line, pattern2);
	    if (pattern[0] && pattern2[0]) {
		ColorNum_t color;
		line += config_read_color (trace, line, &color, FALSE);
		sig_wildmat_select (NULL, pattern);
		sig_rename_selected (pattern2);
		if (color >= 0) {
		    sig_highlight_selected (color);
		}
	    }
	}
	else if (!strcmp(cmd, "SIGNAL_COPY")) {
	    char pattern2[MAXSIGLEN];
	    line += config_read_pattern (trace, line, pattern);
	    if (pattern[0]) {
		ColorNum_t color;
		line += config_read_pattern (trace, line, pattern2);
		line += config_read_color (trace, line, &color, FALSE);
		sig_wildmat_select (NULL, pattern);
		sig_copy_selected (trace, pattern2);
		if (color >= 0) {
		    sig_highlight_selected (color);
		}
	    }
	}
	else if (!strcmp(cmd, "SIGNAL_DELETE")) {
	    line += config_read_pattern (trace, line, pattern);
	    if (pattern[0]) {
		sig_wildmat_select (trace, pattern);
		sig_delete_selected (TRUE, FALSE);
	    }
	}
	else if (!strcmp(cmd, "SIGNAL_DELETE_CONSTANT")) {
	    char flag[MAXSIGLEN];
	    Boolean_t ignorexz = FALSE;
	    line += config_read_pattern (trace, line, pattern);
	    if (pattern[0]) {
		do {
		    line += config_read_string (trace, line, flag);
		    if (!strcasecmp(flag, "-IGNOREXZ")) ignorexz=TRUE;
		} while (flag[0]);
		sig_wildmat_select (trace, pattern);
		sig_delete_selected (FALSE, ignorexz);
	    }
	}
	else if (!strcmp(cmd, "VALUE_HIGHLIGHT")) {
	    char strg[MAXSIGLEN],flag[MAXSIGLEN],signal[MAXSIGLEN]="*";
	    ValSearch_t *vs_ptr;
	    Boolean_t show_value=FALSE, add_cursor=FALSE;
	    VSearchNum_t search_pos;
	    line += config_read_value (trace, line, strg);
	    line += config_read_color (trace, line, &search_pos, TRUE);
	    search_pos--;
	    if (search_pos >= 0) {
		do {
		    line += config_read_string (trace, line, flag);
		    if (!strcasecmp(flag, "-CURSOR")) add_cursor=TRUE;
		    else if (!strcasecmp(flag, "-VALUE")) show_value=TRUE;
		    else if (flag[0]) strcpy (signal, flag);
		} while (flag[0]);
		/* Add it */
		vs_ptr = &global->val_srch[search_pos];
		vs_ptr->color = (show_value) ? search_pos+1 : 0;
		vs_ptr->cursor = (add_cursor) ? search_pos+1 : 0;
		string_to_value (&vs_ptr->radix, strg, &vs_ptr->value);
		strcpy (vs_ptr->signal, signal);
		draw_needupd_val_search ();
	    }
	}
	else if (!strcmp(cmd, "CURSOR_ADD")) {
	    ColorNum_t color;
	    DTime_t ctime;
	    char strg[MAXSIGLEN],flag[MAXSIGLEN];
	    char note[MAXSIGLEN]="";
	    CursorType_t type = CONFIG;
	    
	    line += config_read_string (trace, line, strg);
	    ctime = string_to_time (trace, strg);
	    line += config_read_color (trace, line, &color, TRUE);
	    if (color >= 0) {
		do {
		    line += config_read_string (trace, line, flag);
		    if (!strcasecmp(flag, "-USER")) type=USER;
		    else if (!strcasecmp(flag, "-SIMVIEW") && global->simview_info_ptr) type=SIMVIEW;
		    else if (flag[0]) strcpy (note, flag);
		} while (flag[0]);
		cur_new (ctime, color, type, note);
	    }
	}
	else if (!strcmp(cmd, "CURSOR_STEP_FORWARD")) {
	    cur_step (grid_primary_period (trace));
	}
	else if (!strcmp(cmd, "CURSOR_STEP_BACKWARD")) {
	    cur_step ( - grid_primary_period (trace));
	}
	else if (!strcmp(cmd, "TIME_GOTO")) {
	    DTime_t ctime;
	    char strg[MAXSIGLEN];
	    DTime_t end_time = global->time + (( trace->width - XMARGIN - global->xstart ) / global->res);
	    
	    line += config_read_string (trace, line, strg);
	    ctime = string_to_time (trace, strg);

	    if ((ctime < global->time) || (ctime > end_time)) {
		/* Slide time if it isn't on the screen already */
		global->time = ctime - ( TIME_WIDTH (trace) /2 );
		new_time (trace);
	    }
	}
	else if (!strcmp(cmd, "RESOLUTION")) {
	    DTime_t restime;
	    char strg[MAXSIGLEN];
	    
	    line += config_read_string (trace, line, strg);
	    restime = string_to_time (trace, strg);

	    if (restime > 0) {
		new_res (trace, RES_SCALE / (float)restime);
	    }
	}
	else if (!strcmp(cmd, "SIGNAL_GOTO")) {
	    line += config_read_pattern (trace, line, pattern);
	    if (pattern[0]) {
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
	    memset (&newsigst, 0, sizeof (SignalState_t));
	    line += config_read_pattern (trace, line, newsigst.signame);
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
#define config_process_line(trace, line, fmt)	config_process_line_internal(trace, line, fmt, FALSE)

/* EOF */
static void	config_process_eof (Trace *trace)
{
    char	line[3];
    line[0]='\0';	/* MIPS: no automatic aggregate initialization */
    line[1]='\0';
    line[2]='\0';
    config_process_line_internal (trace, line, FALSE, TRUE);
}

/**********************************************************************
 *	config_read_file
 **********************************************************************/

void config_read_file (
    Trace	*trace,
    char	*filename,	/* Specific filename of CONFIG file */
    Boolean_t	report_notfound,
    Boolean_t	format_only)
{
    FILE	*readfp;
    char line[1000];
    struct stat		newstat;	/* New status on the reread file*/
    int	read_fd;
    int		prev_cursor;
    
    if (DTPRINT_CONFIG || DTPRINT_ENTRY) printf("Reading config file %s\n", filename);

    if (filename[strlen(filename)-1] == '/') {
	/* Ignore it, It's a directory */
	return;
    }
    
    config_report_errors = !format_only;
    
#ifndef VMS
    /* Check if regular file (not directory) */
    read_fd = open (filename, O_RDONLY, 0);
    if (read_fd>0) {
	fstat (read_fd, &newstat);
	if (! S_ISREG(newstat.st_mode)) {
	    /* Not regular file */
	    read_fd = -1;
	}
	close (read_fd);
    }
#endif

    /* Open File For Reading */
    if (!(readfp=fopen(filename,"r"))) {
	read_fd = -1;
    }

    if (read_fd < 0) {
	if (report_notfound) {
	    if (DTPRINT) printf("%%E, Can't Open File %s\n", filename);
	    sprintf(message,"Can't open file %s",filename);
	    dino_error_ack(trace, message);
	}
	return;
    }
    
    prev_cursor = last_set_cursor ();
    set_cursor (DC_BUSY);

    config_line_num=0;
    config_file = filename;
    config_reading_socket = FALSE;
    while (!feof(readfp)) {
	/* Read line & kill EOL at end */
	config_get_line (line, 1000, readfp);
	/* 	printf ("line='%s'\n",line);	*/
	config_process_line (trace, line, format_only);
    }

    config_process_eof (trace);

    fclose (readfp);
    set_cursor (prev_cursor);
}

/**********************************************************************
 *	config_read_socket
 **********************************************************************/

void config_read_socket (
    char	*line,
    char	*name,
    int		cmdnum,
    Boolean_t	eof
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
	config_process_line (global->trace_head, line, FALSE);
    }
}

/**********************************************************************
**********************************************************************/

void config_update_filenames (Trace *trace)
{
    char *pchar;

    if (DTPRINT_ENTRY) printf ("In config_update_filenames\n");
	
    global->config_filename[3][0] = '\0';
    global->config_filename[4][0] = '\0';

    /* Same directory as trace, dinotrace.dino */
    if (trace->dfile.filename != '\0') {
	strcpy (global->config_filename[3], trace->dfile.filename);
	file_directory (global->config_filename[3]);
	strcat (global->config_filename[3], "dinotrace.dino");
    }

    /* Same file as trace, but .dino extension */
    if (trace->dfile.filename != '\0') {
	strcpy (global->config_filename[4], trace->dfile.filename);
	if ((pchar=strrchr(global->config_filename[4],'.')) != NULL )
	    *pchar = '\0';
	if ((pchar=strrchr(global->config_filename[4],'.')) != NULL )
	    *pchar = '\0';
	strcat (global->config_filename[4], ".dino");
    }
}

void config_read_defaults (
    Trace	*trace,
    Boolean_t	format_only)
{
    int		cfg_num;
    Signal	*new_dispsig;
    Signal	*sig_ptr;

    if (DTPRINT_ENTRY) printf ("In config_read_defaults\n");

    new_dispsig = trace->dispsig;

    /* Erase old cursors */
    cur_delete_of_type (CONFIG);

    config_update_filenames(trace);

    for (cfg_num=0; cfg_num<MAXCFGFILES; cfg_num++) {
	if ( global->config_enable[cfg_num] ) {
	    config_read_file (trace, global->config_filename[cfg_num], FALSE, format_only);
	}
    }

    /* If user deleted and readded signals, we might have reset */
    /* dispsig.  Attempt to restore */
    if (new_dispsig && trace->dispsig == trace->firstsig) {
	for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	    if (sig_ptr == new_dispsig) {
		/* Is still actively displayed (else deleted) */
		trace->dispsig = new_dispsig;
		vscroll_new (trace, 0);
		break;
	    }
	}

    }

    /* Apply the statenames */
    grid_calc_autos (trace);

    draw_all_needed ();
    if (DTPRINT_ENTRY) printf ("Exit config_read_defaults\n");
}

/**********************************************************************
*	config_writing
**********************************************************************/

void config_write_file (
    Trace	*trace,
    char	*filename)	/* Specific filename of CONFIG file */
{
    FILE	*writefp;
    Signal	*sig_ptr;
    int		grid_num;
    Grid_t	*grid_ptr;
    int		i;
    char	strg[MAXSIGLEN];
    char	*c;	/* Comment or null */
    Trace	*trace_top = trace;
    
    if (DTPRINT_CONFIG || DTPRINT_ENTRY) printf("Writing config file %s\n", filename);
    
    /* Open File For Writing */
    if (!(writefp=fopen(filename,"w"))) {
	if (DTPRINT) printf("%%E, Can't Write File %s\n", filename);
	sprintf(message,"Can't write file %s",filename);
	dino_error_ack(trace, message);
	return;
    }

    c = (global->cuswr_item[CUSWRITEM_COMMENT]) ? "#" : "";

    fprintf (writefp, "##### Customization Write by %s\n", DTVERSION);
    fprintf (writefp, "##### Created %s\n", date_string(0));

    if (global->cuswr_item[CUSWRITEM_PERSONAL]) {
        fprintf (writefp, "\n#### Global Personal Preferences ####\n");
	if (DTDEBUG) {
	    fprintf (writefp, "%sdebug\t\t%s\n", c, DTDEBUG?"ON":"OFF");
	    /*fprintf (writefp, "%sprint\t\t%x\n", c, DTPRINT?DTPRINT:"OFF"); prints too much on reread*/
	}
	fprintf (writefp, "%srefreshing\t%s\n", c, global->redraw_manually?"MANUAL":"AUTO");
	fprintf (writefp, "%ssave_enables\t%s\n", c, global->save_enables?"ON":"OFF");
	fprintf (writefp, "%ssave_ordering\t%s\n", c, global->save_ordering?"ON":"OFF");
	fprintf (writefp, "%ssave_duplicates\t%s\n", c, global->save_duplicates?"ON":"OFF");
	fprintf (writefp, "%sclick_to_edge\t%s\n", c, global->click_to_edge?"ON":"OFF");
	fprintf (writefp, "%ssignal_height\t%d\n", c, global->sighgt);
	fprintf (writefp, "%srise_fall_time\t%d\n", c, global->sigrf);
	fprintf (writefp, "%stime_rep\t%s\n", c, time_units_to_string (global->timerep, TRUE));
	fprintf (writefp, "%scursor\t\t%s\n", c, global->cursor_vis?"ON":"OFF");
	fprintf (writefp, "%spage_inc\t%d\n", c,
		 global->pageinc==PAGEINC_QUARTER ? 4 : (global->pageinc==PAGEINC_HALF?2:1) );
	fprintf (writefp, "%sprint_size\t", c);
	switch (global->print_size) {
	  case PRINTSIZE_A:		fprintf (writefp, "A\n");	break;
	  case PRINTSIZE_B:		fprintf (writefp, "B\n");	break;
	  case PRINTSIZE_EPSLAND:	fprintf (writefp, "EPSLAND\n");	break;
	  case PRINTSIZE_EPSPORT:	fprintf (writefp, "EPSPORT\n");	break;
	}
	
	config_geometry_string (&global->start_geometry, strg);
	fprintf (writefp, "%sstart_geometry\t%s\n",c, strg);
	config_geometry_string (&global->open_geometry, strg);
	fprintf (writefp, "%sopen_geometry\t%s\n",c,  strg);
	config_geometry_string (&global->shrink_geometry, strg);
	fprintf (writefp, "%sshrink_geometry\t%s\n",c, strg);
	
	if (global->time_format[0])
	  fprintf (writefp, "%stime_format\t%s\n",c, global->time_format);
	
	signalstate_write (writefp, c);
    }
    
    if (global->cuswr_item[CUSWRITEM_VALSEARCH]) {
	fprintf (writefp, "\n#### Value Searches ####\n");
	for (i=1; i<=MAX_SRCH; i++) {
	    ValSearch_t *vs_ptr = &global->val_srch[i-1];
	    char strg[MAXSIGLEN];
	    if (vs_ptr->color || vs_ptr->cursor) {
		val_to_string (vs_ptr->radix, strg, &vs_ptr->value, FALSE);
		fprintf (writefp, "%svalue_highlight %s %d \"%s\" %s %s\n",c,
			 strg, i, vs_ptr->signal, 
			 vs_ptr->color ? "-VALUE":"",
			 vs_ptr->cursor ? "-CURSOR":"");
	    }
	}
    }
    
    if (global->cuswr_item[CUSWRITEM_CURSORS]) {
	fprintf (writefp, "\n#### Cursors ####\n");
	cur_write (writefp, c);
    }
    
    if (global->cuswr_item[CUSWRITEM_FORMAT]) {
	fprintf (writefp, "\n#### Trace Format ####\n");
	for (trace = global->trace_head; trace; trace = trace->next_trace) {
	    if ((   global->cuswr_traces == TRACESEL_THIS && trace!=trace_top)
		|| (global->cuswr_traces == TRACESEL_ALL && trace==global->deleted_trace_head)) {
		continue;
	    }
	    if (trace->loaded) {
		fprintf (writefp, "###set_trace\t%s\n", trace->dfile.filename);
		fprintf (writefp, "%sfile_format\t%s\n",c, filetypes[trace->dfile.fileformat].name);
		fprintf (writefp, "%svector_separator\t\"%c\"\n",c, trace->dfile.vector_separator);
		fprintf (writefp, "%shierarchy_separator\t\"%c\"\n",c, trace->dfile.hierarchy_separator);
		fprintf (writefp, "%stime_multiplier\t%d\n",c, global->tempest_time_mult);
		fprintf (writefp, "%stime_precision\t%s\n",c, time_units_to_string (global->time_precision, TRUE));
	    }
	}
    }
    
    if (global->cuswr_item[CUSWRITEM_GRIDS]) {
	fprintf (writefp, "\n#### Grids ####\n");
	for (trace = global->trace_head; trace; trace = trace->next_trace) {
	    if ((   global->cuswr_traces == TRACESEL_THIS && trace!=trace_top)
		|| (global->cuswr_traces == TRACESEL_ALL && trace==global->deleted_trace_head)) {
		continue;
	    }
	    if (trace->loaded) {
		fprintf (writefp, "###set_trace\t%s\n", trace->dfile.filename);
		for (grid_num=0; grid_num<MAXGRIDS; grid_num++) {
		    grid_ptr = &(trace->grid[grid_num]);
		    
		    fprintf (writefp, "%sgrid\t\t%d\t%s\n",c, grid_num, grid_ptr->visible?"ON":"OFF");
		    fprintf (writefp, "%sgrid_type\t%d\t%s\n",c, grid_num, grid_ptr->wide_line?"WIDE":"NORMAL");
		    fprintf (writefp, "%sgrid_signal\t%d\t\"%s\"\n",c, grid_num, grid_ptr->signal);
		    fprintf (writefp, "%sgrid_color\t%d\t%d\n",c, grid_num, grid_ptr->color);
		    fprintf (writefp, "%sgrid_resolution\t%d\t",c, grid_num);
		    switch (grid_ptr->period_auto) {
		      case PA_AUTO:		fprintf (writefp, "ASSERTION\n");	break;
		      default:		fprintf (writefp, "%d\n", grid_ptr->period);	break;
		    }
		    fprintf (writefp, "%sgrid_align\t%d\t",c, grid_num);
		    switch (grid_ptr->align_auto) {
		      case AA_ASS:		fprintf (writefp, "ASSERTION\n");	break;
		      case AA_DEASS:	fprintf (writefp, "DEASSERTION\n");	break;
		      default:		fprintf (writefp, "%d\n", grid_ptr->alignment);	break;
		    }
		}
	    }
	}
    }
	
    if (global->cuswr_item[CUSWRITEM_SIGHIGHLIGHT]) {
	fprintf (writefp, "\n#### Signal Highlighting, Commenting, etc ####\n");
	for (trace = global->deleted_trace_head; trace; trace = trace->next_trace) {
	    if ((   global->cuswr_traces == TRACESEL_THIS && trace!=trace_top)
		|| (global->cuswr_traces == TRACESEL_ALL && trace==global->deleted_trace_head)) {
		continue;
	    }
	    fprintf (writefp, "###set_trace %s\n", trace->dfile.filename);
	    /* Save signal colors */
	    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
		if (sig_ptr->color && !sig_ptr->search) {
		    fprintf (writefp, "%ssignal_highlight %s %d\n",c, sig_ptr->signame, sig_ptr->color);
		}
		if (sig_ptr->note) fprintf (writefp, "%ssignal_note %s \"%s\"\n",c, sig_ptr->signame, sig_ptr->note);
		if (sig_ptr->radix != global->radixs[0]) {
		    fprintf (writefp, "%ssignal_radix %s %s\n",c, sig_ptr->signame, sig_ptr->radix->name);
		}
	    }
	}
    }
	    
    if (global->cuswr_item[CUSWRITEM_SIGORDER]) {
	fprintf (writefp, "\n#### Signal Ordering ####\n");
	for (trace = global->trace_head; trace; trace = trace->next_trace) {
	    if ((   global->cuswr_traces == TRACESEL_THIS && trace!=trace_top)
		|| (global->cuswr_traces == TRACESEL_ALL && trace==global->deleted_trace_head)) {
		continue;
	    }
	    fprintf (writefp, "###set_trace %s\n", trace->dfile.filename);
	    fprintf (writefp, "%ssignal_delete *\n",c);
	    /* Save signal colors */
	    for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
		fprintf (writefp, "%ssignal_add %s\n",c, sig_ptr->signame);
	    }
	}
    }
	
    fprintf (writefp, "\n\n##### Customization Write by %s\n", DTVERSION);
    fprintf (writefp, "##### Created %s\n", date_string(0));

    fclose (writefp);
}
	    
/**********************************************************************
*	config_restore_defaults
**********************************************************************/

void config_trace_defaults (
    Trace	*trace)
{
    trace->dfile.hierarchy_separator = '.';
    trace->dfile.vector_separator = '[';
    trace->dfile.vectorend_separator = ']';

    grid_reset_cb (trace->main);
}


void config_global_defaults(void)
{
    signalstate_free ();
    draw_needupd_val_states ();
    draw_needupd_sig_start ();
    
    global->sighgt = 15;
    global->pageinc = PAGEINC_FULL;
    global->save_ordering = TRUE;
    global->cursor_vis = TRUE;
    global->sigrf = SIG_RF;
    global->timerep = global->time_precision;
}


