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
 *	page_inc	4 | 2 | 1
 *	rise_fall_time	Number
 *	signal_height	Number
 *	grid		ON | OFF
 *	grid_align	Number | AUTO_ASSERTION | AUTO_DEASSERTION
 *	grid_resolution	Number | AUTO_ASSERTION | AUTO_DEASSERTION
 *	cursor		ON | OFF
 *NYI	cursor_add	Number
 *	signal_states	Signal_Name = {State, State, State}
 *NYI	signal_rename	Signal_Name = New_Signal_Name
 *	debug		ON | OFF
 *	print		ON | OFF
 *	
 */


#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <file.h>
#include <strdef.h>

#ifdef VMS
#include <math.h> /* removed for Ultrix support... */
#endif VMS

#include <X11/DECwDwtApplProg.h>
#include <X11/Xlib.h>

#include "dinotrace.h"

#define issigchr(ch)  (isalnum(ch) || (ch)=='%' || (ch)=='.' || (ch)=='*' ||\
		       (ch)=='_' || (ch)=='>'|| (ch)=='<' || (ch)==':')
#define isstatechr(ch)  (isalnum(ch) || (ch)=='%' || (ch)=='.' || \
		       (ch)=='_' || (ch)=='/'|| (ch)=='*' || (ch)=='(' || (ch)==')')

int line_num=0;
char *current_file="";

SIGNALSTATE *symbol_head=NULL;

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
    if (*(tp=(line+strlen(line)-1))='\n') *tp='\0';
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

    while (*line && !isstatechr(*line)) {
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
    char *tp;
    int outlen=0;

    while (*line && !isalnum(*line)) {
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
    char *tp;
    int outlen;

    outlen += config_read_signal (line, cmd);
    upcase_string (cmd);

    if (!strcmp (cmd, "ON") || !strcmp (cmd, "1"))
	*out = 1;
    else if (!strcmp (cmd, "OFF") || !strcmp (cmd, "0"))
	*out = 0;
    else {
	if (DTPRINT) printf ("%%E, Looking for ON/OFF, found '%s' on config line %d\n", cmd, line_num);
	sprintf (message, "Looking for ON/OFF, found '%s'\non line %d of %s\n",
		 cmd, line_num, current_file);
	dino_message_ack(ptr,message);
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
    register char	*s;
    register char	*p;
{
    /* Skip leading spaces on source signal */
    while (*s==' ') s++;

    for ( ; *p; s++, p++) {
	if (*p!='*') {
	    if (toupper(*s) != toupper(*p))
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

SIGNALSTATE	*find_signal_state (ptr, name)
    DISPLAY_SB	*ptr;
    char *name;
{
    register SIGNALSTATE *sig;

    for (sig=ptr->signalstate_head; sig; sig=sig->next) {
	/* printf ("'%s'\t'%s'\n", name, sig->signame); */
	if (wildmat(name, sig->signame))
	    return (sig);
	}
    return (NULL);
    }

void	add_signal_state (ptr, info)
    DISPLAY_SB	*ptr;
    SIGNALSTATE *info;
{
    SIGNALSTATE *new;

    new = (SIGNALSTATE *)(malloc (sizeof(SIGNALSTATE)));
    memcpy ((void *)new, (void *)info, sizeof(SIGNALSTATE));
    new->next = ptr->signalstate_head;
    ptr->signalstate_head = new;
    }

void	free_signal_states (ptr)
    DISPLAY_SB	*ptr;
{
    SIGNALSTATE *sig_ptr, *last_ptr;

    sig_ptr = ptr->signalstate_head;
    while (sig_ptr) {
	last_ptr = sig_ptr->next;
	free (sig_ptr);
	sig_ptr = last_ptr;
	}
    ptr->signalstate_head = NULL;
    }

void	print_signal_states (ptr)
    DISPLAY_SB	*ptr;
{
    SIGNALSTATE *sig_ptr;
    int i;

    sig_ptr = ptr->signalstate_head;
    while (sig_ptr) {
	printf("Signal %s:\n",sig_ptr->signame);
	for (i=0; i<MAXSTATENAMES; i++)
	    if (sig_ptr->statename[i][0])
		printf ("\t%d=%s", i, sig_ptr->statename[i]);
	sig_ptr = sig_ptr->next;
	}
    printf ("\n");
    }

/**********************************************************************
*	config_process_states
**********************************************************************/

void	config_process_states (ptr, oldline, readfp)
    DISPLAY_SB	*ptr;
    char *oldline;
    FILE *readfp;
{
    SIGNALSTATE newsigst;
    char newline[1000];
    char newstate[MAXSTATELEN];
    char *line;
    int statenum=0;
    int t;

    memset (&newsigst, 0, sizeof (SIGNALSTATE));

    strcpy (newline, oldline);
    line = newline;
    line += config_read_signal (line, newsigst.signame);

    if (DTPRINT) printf ("config_process_states  signal=%s\n", newsigst.signame);

    while (*line!='}') {
	if (!*line) {
	    if (feof(readfp)) {
		dino_message_ack(ptr,"Unexpected EOF during SIGNAL_STATES command in config file");
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
		sprintf (message, "Over maximum of %d SIGNAL_STATES for signal",
			 MAXSTATENAMES, newsigst.signame);
		dino_message_ack(ptr,message);
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

    add_signal_state (ptr, &newsigst);

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

void	config_process_line (ptr, line, readfp)
    DISPLAY_SB	*ptr;
    char *line;
    FILE *readfp;
{
    char *tp;
    char signal[MAXSIGLEN];
    char cmd[MAXSIGLEN];
    char name[MAXSIGLEN];
    int value;

    /* Comment */
    while (*line && isspace(*line)) line++;
    if ((line[0]=='!') || (line[0]==';') || (line[0]=='\0')) return;

    /* Preprocessor #INCLUDE eventually */
    /* extract command */
    line += config_read_signal (line, cmd);
    upcase_string (cmd);

    /* if (DTPRINT) printf ("Cmd='%s'\n",cmd); */

    if (!strcmp(cmd, "DEBUG")) {
	value=DTDEBUG;
	line += config_read_on_off (line, &value);
	if (DTDEBUG) printf ("Config: DTDEBUG=%d\n",value);
	DTDEBUG=value;
	}
    else if (!strcmp(cmd, "PRINT")) {
	value=DTPRINT;
	line += config_read_on_off (line, &value);
	if (DTPRINT) printf ("Config: DTPRINT=%d\n",value);
	DTPRINT=value;
	}
    else if (!strcmp(cmd, "CURSOR")) {
	value = ptr->cursor_vis;
	line += config_read_on_off (line, &value);
	if (DTPRINT) printf ("Config: cursor_vis=%d\n",value);
	ptr->cursor_vis = value;
	}
    else if (!strcmp(cmd, "GRID")) {
	value = ptr->grid_vis;
	line += config_read_on_off (line, &value);
	if (DTPRINT) printf ("Config: grid_vis=%d\n",value);
	ptr->grid_vis = value;
	}
    else if (!strcmp(cmd, "SIGNAL_HEIGHT")) {
	value = ptr->sighgt;
	line += config_read_int (line, &value);
	if (value >= 15 && value <= 50)
	    ptr->sighgt = value;
	else {
	    if (DTPRINT) printf ("%%E, Signal_height must be 15-50 on config line %d\n", line_num);
	    sprintf (message, "Signal_height must be 15-50\non line %d of %s\n",
		     line_num, current_file);
	    dino_message_ack(ptr,message);
	    }
	}
    else if (!strcmp(cmd, "GRID_RESOLUTION")) {
	value = ptr->grid_res;
	line += config_read_int (line, &value);
	if (value >= 1) {
	    ptr->grid_res = value;
	    ptr->grid_res_auto = value;
	    }
	else {
	    ptr->grid_res_auto = GRID_AUTO_ASS;
	    }
	}
    else if (!strcmp(cmd, "GRID_ALIGN")) {
	value = ptr->grid_align;
	line += config_read_int (line, &value);
	if (value >= 1) {
	    ptr->grid_align = value;
	    ptr->grid_align_auto = value;
	    }
	else {
	    if (toupper(line[0])=='A')
		ptr->grid_align_auto = GRID_AUTO_ASS;
	    else if (toupper(line[0])=='D')
		ptr->grid_align_auto = GRID_AUTO_DEASS;
	    else {
		if (DTPRINT) printf ("%%E, Grid_align must be >0, ASSERTION, or DEASSERTION on config line %d\n", line_num);
		sprintf (message, "Grid_align must be >0, ASSERTION, or DEASSERTION\non line %d of %s\n",
			 line_num, current_file);
		dino_message_ack(ptr,message);
		}
	    }
	if (DTPRINT) printf ("grid_align_auto = %d\n", ptr->grid_align_auto);
	}
    else if (!strcmp(cmd, "RISE_FALL_TIME")) {
	value = ptr->sigrf;
	line += config_read_int (line, &value);
	/* Valid values not known... */
	ptr->sigrf = value;
	}
    else if (!strcmp(cmd, "PAGE_INC")) {
	value = ptr->pageinc;
	line += config_read_int (line, &value);
	if (value == 1 || value == 2 || value == 4)
	    ptr->pageinc = value;
	else {
	    if (DTPRINT) printf ("%%E, Page_Inc must be 1, 2, or 4 on config line %d\n", line_num);
	    sprintf (message, "Page_Inc must be 1, 2, or 4\non line %d of %s\n",
		     line_num, current_file);
	    dino_message_ack(ptr,message);
	    }
	}
    else if (!strcmp(cmd, "SIGNAL_STATES")) {
	config_process_states (ptr, line, readfp);
	}
    else {
	if (DTPRINT) printf ("%%E, Unknown command '%s' on config line %d\n", cmd, line_num);
	sprintf (message, "Unknown command '%s'\non line %d of %s\n",
		 cmd, line_num, current_file);
	dino_message_ack(ptr,message);
	}
    }

/**********************************************************************
*	config_read_file
**********************************************************************/

void config_read_file(ptr, filename, report_error)
    DISPLAY_SB	*ptr;
    char *filename;	/* Specific filename of CONFIG file */
    int report_error;
{
    FILE	*readfp;
    char line[1000];

    if (DTPRINT) printf("Reading config file %s\n", filename);

    /* Open File For Reading */
    if (!(readfp=fopen(filename,"r"))) {
	char temp[100];
	/* Try filename .dino */
	/* strcpy (temp, filename);
	   strcat (temp, ".dino");
	   if (!(readfp=fopen(temp,"r"))) */
	if (report_error) {
	    if (DTPRINT) printf("%%E, Can't Open File %s\n", filename);
	    sprintf(message,"Can't open file %s",filename);
	    dino_message_ack(ptr, message);
	    }
	return;
	}

    line_num=0;
    current_file = filename;
    while (!feof(readfp)) {
	/* Read line & kill EOL at end */
	config_get_line (line, 1000, readfp);
	/* 	printf ("line='%s'\n",line);	*/
	config_process_line (ptr, line, readfp);
	}

    fclose (readfp);
    }


/**********************************************************************
*	config_read_defaults
**********************************************************************/

void config_read_defaults(ptr)
    DISPLAY_SB	*ptr;
{
    char newfilename[200];
    char *pchar;

    config_read_file (ptr, "DINO$DISK:DINOTRACE.DINO", 0);
    config_read_file (ptr, "SYS$LOGIN:DINOTRACE.DINO", 0);

    /* Same directory as trace, dinotrace.dino */
    if (ptr->filename != '\0') {
	strcpy (newfilename, ptr->filename);
	file_directory (newfilename);
	strcat (newfilename, "DINOTRACE.DINO");
	config_read_file (ptr, newfilename, 0);
	}
    
    /* Same directory as trace, same file, but .dino extension */
    if (ptr->filename != '\0') {
	strcpy (newfilename, ptr->filename);
	if ((pchar=strrchr(newfilename,'.')) != NULL )
	    *pchar = '\0';
	strcat (newfilename, ".DINO");
	config_read_file (ptr, newfilename, 0);
	}

    /* Apply the statenames */
    update_signal_states (ptr);
    }

/**********************************************************************
*	config_restore_defaults
**********************************************************************/

void config_restore_defaults(ptr)
    DISPLAY_SB	*ptr;
{
    if ( ptr->signalstate_head != NULL)
	free_signal_states (ptr);
    
    ptr->sighgt = 20;	/* was 25 */
    ptr->cursor_vis = TRUE;
    ptr->grid_vis = TRUE;
    ptr->grid_res = 100;
    ptr->grid_align = 0;
    ptr->numpag = 1;
    ptr->sigrf = SIG_RF;

    update_signal_states (ptr);
    }
