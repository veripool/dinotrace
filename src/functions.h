/* $Id$ */
/******************************************************************************
 * functions.h --- Dinotrace functions and callback externs
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

/***********************************************************************/
/* Defined functions.... Really inlines, but old VMS compiler chokes */

/***********************************************************************/
/* Relatively generic stuff */

/* Utilities */
#ifndef MAX
# if __GNUC__
#   define MAX(a,b) \
       ({typedef _ta = (a), _tb = (b);  \
         _ta _a = (a); _tb _b = (b);     \
         _a > _b ? _a : _b; })
# else
#  define MAX(_a_,_b_) ( ( ( _a_ ) > ( _b_ ) ) ? ( _a_ ) : ( _b_ ) )
# endif
#endif

#ifndef MIN
# if __GNUC__
#   define MIN(a,b) \
       ({typedef _ta = (a), _tb = (b);  \
         _ta _a = (a); _tb _b = (b);     \
         _a < _b ? _a : _b; })
# else
#   define MIN(_a_,_b_) ( ( ( _a_ ) < ( _b_ ) ) ? ( _a_ ) : ( _b_ ) )
# endif
#endif

#ifndef ABS
# if __GNUC__
#   define ABS(a) \
       ({typedef _ta = (a);  \
         _ta _a = (a);     \
         _a < 0 ? - _a : _a; })
# else
#   define ABS(_a_) ( ( ( _a_ ) < 0 ) ? ( - (_a_) ) : ( _a_ ) )
# endif
#endif

/* Avoid binding error messages on XtFree, NOTE ALSO clears the pointer! */
#define DFree(ptr) { XtFree((char *)ptr); ptr = NULL; }

/* Xt provides a nice XtNew, but a XtNewCalloc is nice too (clear storage) */
#define DNewCalloc(type) ((type *) XtCalloc(1, (unsigned) sizeof(type)))

/* Useful for debugging messages */
#define DeNull(_str_) ( ((_str_)==NULL) ? "NULL" : (_str_) )
#define DeNullSignal(_sig_) ( ((_sig_)==NULL) ? "NULLPTR" : (DeNull((_sig_)->signame) ) )

/* Correct endianness (little is stored in files, since that's what the program originally used) */
#if WORDS_BIGENDIAN
#define LITTLEENDIANIZE32(_lw_) \
    ( (  (_lw_<<24)&0xff000000) | ((_lw_<< 8)&0x00ff0000) \
      | ((_lw_>> 8)&0x0000ff00)  | ((_lw_>>24)&0x000000ff))
#else
#define LITTLEENDIANIZE32(_lw_) (_lw_)
#endif

/***********************************************************************/
/* Dinotrace specifics */

/* Callback.. force a prototype declaration to avoid warnings */
#define DAddCallback(widget, callback, func, trace) \
    (XtAddCallback ((widget), (callback),  (XtCallbackProc)(func), (trace)))

/* Schedule a redraw for trace or everybody */
#define draw_expose_needed(_trace_)	{ global->redraw_needed |= GRD_TRACE; (_trace_)->redraw_needed |= TRD_EXPOSE; }
#define draw_needed(_trace_)		{ global->redraw_needed |= GRD_TRACE; (_trace_)->redraw_needed |= TRD_REDRAW; }
#define draw_all_needed() 		{ global->redraw_needed |= GRD_ALL; }
#define draw_manual_needed() 		{ global->redraw_needed |= GRD_MANUAL; }
#define draw_needupd_sig_start()	{ global->updates_needed |= GUD_SIG_START; }
#define draw_needupd_sig_search()	{ global->updates_needed |= GUD_SIG_SEARCH; }
#define draw_needupd_val_search()	{ global->updates_needed |= GUD_VAL_SEARCH; }
#define draw_needupd_val_states()	{ global->updates_needed |= GUD_VAL_STATES; }

/* dino_message_ack types */
#define	dino_error_ack(tr,msg)		dino_message_ack (tr, 0, msg)
#define	dino_warning_ack(tr,msg)	dino_message_ack (tr, 1, msg)
#define	dino_information_ack(tr,msg)	dino_message_ack (tr, 2, msg)

#define TIME_TO_XPOS(_xtime_) \
        ( ((_xtime_) - global->time) * global->res + global->xstart )

#define	TIME_WIDTH(_trace_) \
	((DTime)( ((_trace_)->width - XMARGIN - global->xstart) / global->res ))

#define CPTR_TIME(cptr) ((cptr)->stbits.time)
#define CPTR_SIZE(cptr) (sig_ptr->lws)
#define CPTR_SIZE_PREV(cptr) (sig_ptr->lws)
#define	cptr_to_value(cptr,value_ptr) \
{\
     (value_ptr)->siglw.number = (cptr)[0].number;\
     (value_ptr)->number[0] = (cptr)[1].number;\
     (value_ptr)->number[1] = (cptr)[2].number;\
     (value_ptr)->number[2] = (cptr)[3].number;\
     (value_ptr)->number[3] = (cptr)[4].number;\
 }

/* Also identical commented function in dt_file if this isn't defined */
#define	fil_add_cptr(sig_ptr, value_ptr, nocheck) \
{\
    SignalLW_t	*cptr;\
    ulong_t diff;\
    cptr = ((Signal *)(sig_ptr))->cptr - ((Signal *)(sig_ptr))->lws;\
    if ( (nocheck)\
	|| ( cptr->stbits.state != ((Value_t *)(value_ptr))->siglw.stbits.state )\
	|| ( cptr[1].number != ((Value_t *)(value_ptr))->number[0] )\
	|| ( cptr[2].number != ((Value_t *)(value_ptr))->number[1] )\
	|| ( cptr[3].number != ((Value_t *)(value_ptr))->number[2] )\
	|| ( cptr[4].number != ((Value_t *)(value_ptr))->number[3] ) )\
	{\
	diff = ((Signal *)sig_ptr)->cptr - ((Signal *)sig_ptr)->bptr;\
	if (diff > (sig_ptr)->blocks) {\
	    ((Signal *)(sig_ptr))->blocks += BLK_SIZE;\
	    ((Signal *)(sig_ptr))->bptr = (SignalLW_t *)XtRealloc ((char*)((Signal *)(sig_ptr))->bptr,\
		       (((Signal *)(sig_ptr))->blocks*sizeof(unsigned int)) + (sizeof(Value_t)*2 + 2));\
	    ((Signal *)(sig_ptr))->cptr = ((Signal *)(sig_ptr))->bptr+diff;\
	}\
	(((Signal *)(sig_ptr))->cptr)[0].number = ((Value_t *)(value_ptr))->siglw.number;\
	(((Signal *)(sig_ptr))->cptr)[1].number = ((Value_t *)(value_ptr))->number[0];\
	(((Signal *)(sig_ptr))->cptr)[2].number = ((Value_t *)(value_ptr))->number[1];\
	(((Signal *)(sig_ptr))->cptr)[3].number = ((Value_t *)(value_ptr))->number[2];\
	(((Signal *)(sig_ptr))->cptr)[4].number = ((Value_t *)(value_ptr))->number[3];\
	(((Signal *)(sig_ptr))->cptr) += ((Signal *)(sig_ptr))->lws;\
    }\
 }


/***********************************************************************/
/* Prototypes */

/* Dinotrace.c */
extern char *	help_message (void);

/* dt_dispmgr.c */
extern Trace *	create_trace (int xs, int ys, int xp, int yp);
extern Trace *	trace_create_split_window (Trace *trace);
extern Trace *	malloc_trace (void);
extern void	init_globals (void);
extern void	create_globals (int argc, char **argv, Boolean sync);
extern void	set_cursor (Trace *trace, int cursor_num);
extern int	last_set_cursor (void);

extern void	trace_open_cb (Widget w);
extern void	trace_close_cb (Widget w);
extern void	trace_clear_cb (Widget w);
extern void	trace_exit_cb (Widget w);

/* dt_customize.c routines */
extern void	cus_dialog_cb (Widget w, Trace *trace, XmAnyCallbackStruct *cb);
extern void	cus_read_cb (Widget w, Trace *trace, XmFileSelectionBoxCallbackStruct *cb);
extern void	cus_read_ok_cb (Widget w, Trace *trace, XmFileSelectionBoxCallbackStruct *cb);
extern void	cus_ok_cb (Widget w, Trace *trace, XmAnyCallbackStruct *cb);
extern void	cus_apply_cb (Widget w, Trace *trace, XmAnyCallbackStruct *cb);
extern void	cus_restore_cb (Widget w, Trace *trace, XmAnyCallbackStruct *cb);
extern void	cus_reread_cb (Widget w, Trace *trace, XmAnyCallbackStruct *cb);

/* dt_cursor.c routines */
extern void	cur_add (DTime, ColorNum, CursorType);
extern void	cur_remove (DCursor *);
extern void	cur_delete_of_type (CursorType type);
extern DTime	cur_time_first (Trace *trace);
extern DTime	cur_time_last (Trace *trace);
extern void	cur_print (FILE *);

extern void	cur_add_cb (Widget w);
extern void	cur_mov_cb (Widget w);
extern void	cur_del_cb (Widget w);
extern void	cur_clr_cb (Widget w);
extern void	cur_highlight_cb (Widget w);
extern void	cur_highlight_ev (Widget w, Trace *trace, XButtonPressedEvent *ev);
extern void	cur_add_ev (Widget w, Trace *trace, XButtonPressedEvent *ev);
extern void	cur_move_ev (Widget w, Trace *trace, XButtonPressedEvent *ev);
extern void	cur_delete_ev (Widget w, Trace *trace, XButtonPressedEvent *ev);

/* dt_config.c routines */
extern void	config_read_defaults (Trace *trace, Boolean);
extern void	config_read_file (Trace *trace, char *, Boolean, Boolean);
extern void	config_trace_defaults (Trace *);
extern void	config_global_defaults (void);
extern void	config_parse_geometry (char *, Geometry *);
extern void	config_update_filenames (Trace *trace);
extern void	config_read_socket (char *line, char *name, int cmdnum, Boolean eof);
extern SignalState *find_signal_state (Trace *, char *);
extern int	wildmat ();

extern void	debug_print_signal_states_cb (Widget w);
extern void	config_write_cb (Widget w);

/* dt_grid.c routines */
extern void	grid_calc_autos (Trace *trace);

extern void	grid_customize_cb (Widget w);
extern void	grid_reset_cb (Widget w);

/* dt_signal.c routines */
extern void	sig_update_search (void);
extern void	sig_free (Trace *trace, Signal *sig_ptr, Boolean select, Boolean recurse);
extern void	sig_highlight_selected (int color);
extern void	sig_modify_enables (Trace *);
extern void	sig_move_selected (Trace *new_trace, char *after_pattern);
extern void	sig_rename_selected (char *new_name);
extern void	sig_copy_selected (Trace *new_trace, char *after_pattern);
extern void	sig_delete_selected (Boolean constant_flag, Boolean ignorexz);
extern void	sig_wildmat_select (Trace *, char *);
extern void	sig_goto_pattern (Trace *, char *);
extern void	sig_cross_preserve (Trace *trace);
extern void	sig_cross_restore (Trace *trace) ;
extern Signal *	sig_find_signame (Trace *trace, char *signame);
extern Signal *	sig_wildmat_signame (Trace *trace, char *signame);
extern void	sig_print_names (Trace *trace);

extern void	sig_add_cb (Widget w);
extern void	sig_mov_cb (Widget w);
extern void	sig_copy_cb (Widget w);
extern void	sig_del_cb (Widget w);
extern void	sig_add_ev (Widget w, Trace *trace, XButtonPressedEvent *ev);
extern void	sig_move_ev (Widget w, Trace *trace, XButtonPressedEvent *ev);
extern void	sig_copy_ev (Widget w, Trace *trace, XButtonPressedEvent *ev);
extern void	sig_delete_ev (Widget w, Trace *trace, XButtonPressedEvent *ev);
extern void	sig_highlight_ev (Widget w, Trace *trace, XButtonPressedEvent *ev);
extern void	sig_highlight_cb (Widget w);
extern void	sig_select_cb (Widget w);
extern void	sig_cancel_cb (Widget w);
extern void	sig_search_cb (Widget w);
extern void	sig_search_ok_cb (Widget w, Trace *trace, XmSelectionBoxCallbackStruct *cb);
extern void	sig_search_apply_cb (Widget w, Trace *trace, XmSelectionBoxCallbackStruct *cb);

/* dt_value.c routines */
extern void	val_update_search (void);
extern void	val_states_update (void);
extern void	value_to_string (Trace *trace, char *strg, unsigned int cptr[], char seperator);
extern void	cptr_to_search_value (SignalLW_t *cptr, uint_t value[]);

extern void	val_annotate_cb (Widget w);
extern void	val_annotate_ok_cb (Widget w, Trace *trace, XmAnyCallbackStruct *cb);
extern void	val_annotate_apply_cb (Widget w, Trace *trace, XmAnyCallbackStruct *cb);
extern void	val_annotate_do_cb (Widget w, Trace *trace, XmAnyCallbackStruct *cb);
extern void	val_examine_cb (Widget w);
extern void	val_examine_ev (Widget w, Trace *trace, XButtonPressedEvent *ev);
extern void	val_search_cb (Widget w);
extern void	val_search_ok_cb (Widget w, Trace *trace, XmSelectionBoxCallbackStruct *cb);
extern void	val_search_apply_cb (Widget w, Trace *trace, XmSelectionBoxCallbackStruct *cb);
extern void	val_highlight_cb (Widget w);
extern void	val_highlight_ev (Widget w, Trace *trace, XButtonPressedEvent *ev);

/* dt_printscreen routines */
extern void	ps_print_internal (Trace *trace);
extern void	ps_reset  (Trace *trace);

extern void	ps_dialog_cb (Widget w);
extern void	ps_reset_cb  (Widget w);
extern void	ps_print_direct_cb (Widget w);
extern void	ps_print_req_cb (Widget w);
extern void	ps_range_sensitives_cb (Widget w, RangeWidgets *range_ptr, XmSelectionBoxCallbackStruct *cb);

/* dt_binary.c routines */
extern void	tempest_read (Trace *trace, int read_fd);
extern void	decsim_read_binary (Trace *trace, int read_fd);

/* dt_file.c routines */
extern void	fil_read (Trace *trace);
extern void	free_data (Trace *trace);
extern void	read_make_busses (Trace *trace, Boolean not_tempest);
extern void	decsim_read_ascii (Trace *trace, int read_fd, FILE *decsim_z_readfp);
extern void	read_trace_end (Trace *trace);
extern void 	fil_select_set_pattern (Trace *trace, Widget dialog, char *pattern);
extern void	fil_string_add_cptr (Signal	*sig_ptr, char *value_strg, DTime time, Boolean nocheck);
#ifndef fil_add_cptr
extern void	fil_add_cptr ();
#endif

extern void	fil_format_option_cb (Widget w, Trace *trace, XmSelectionBoxCallbackStruct *cb);
extern void	fil_ok_cb (Widget w, Trace *trace, XmFileSelectionBoxCallbackStruct *cb);
extern void	trace_read_cb (Widget w, Trace *trace);
extern void	trace_reread_all_cb (Widget w, Trace *trace);
extern void	trace_reread_cb (Widget w, Trace *trace);
extern void	help_cb (Widget w, Trace *trace, XmAnyCallbackStruct *cb);
extern void	help_trace_cb (Widget w, Trace *trace, XmAnyCallbackStruct *cb);
extern void	help_doc_cb (Widget w, Trace *trace, XmAnyCallbackStruct *cb);

/* dt_util routines */
extern void	fgets_dynamic (char **line_pptr, uint_t *length_ptr, FILE *readfp);
extern void	upcase_string (char *tp);
extern void	file_directory (char *strg);
extern char *	extract_first_xms_segment (XmString);
extern char *	date_string (time_t time_num);
#if ! HAVE_STRDUP
extern char *	strdup (char *);
#endif

extern void	DManageChild (Widget w, Trace *trace, MCKeys_t keys);
extern void	add_event (int type, void (*callback)());
extern void	remove_all_events (Trace *trace);
extern void	change_title (Trace *trace);
extern void	new_time (Trace *);
extern void	get_geometry (Trace *trace);
extern void	get_file_name (Trace *trace);
extern void	dino_message_ack (Trace *trace, int type, char *msg);
extern void	get_data_popup (Trace *trace, char *string, int type);
extern void	print_sig_info (Signal *);
extern Trace *	widget_to_trace (Widget w);
extern SignalLW_t *cptr_at_time (Signal *sig_ptr, DTime ctime);
extern XmString	string_create_with_cr (char *);
extern DTime	posx_to_time (Trace *trace, Position x);
extern DTime	posx_to_time_edge (Trace *trace, Position x, Position y);
extern DTime	string_to_time (Trace *trace, char *strg);
extern DTime 	time_units_to_multiplier (TimeRep timerep);
extern void	time_to_string (Trace *trace, char *strg, DTime ctime, Boolean relative);
extern char *	time_units_to_string (TimeRep timerep, Boolean showvalue);
extern Signal *	posy_to_signal (Trace *trace, Position y);
extern DCursor *posx_to_cursor (Trace *trace, Position x);
extern DCursor *time_to_cursor (DTime xtime);
extern ColorNum submenu_to_color (Trace *trace, Widget, int);
extern void	cptr_to_string (SignalLW_t *cptr, char *strg);
extern void	string_to_value (Trace *trace, char *strg, uint_t cptr[]);
extern int	option_to_number (Widget w, Widget *entry0_ptr, int maxnumber);

extern void	cancel_all_events_cb (Widget w);
extern void	unmanage_cb (Widget w, Widget tag, XmAnyCallbackStruct *cb);
extern void	debug_print_screen_traces_cb (Widget w);

/* dt_window routines */
extern void	vscroll_new (Trace *trace, int inc);
extern void	new_res (Trace *trace, float res_new);
extern void	win_full_res (Trace *trace);

extern void	win_expose_cb (Widget w, Trace *trace);
extern void	win_resize_cb (Widget w, Trace *trace);
extern void	win_refresh_cb (Widget w, Trace *trace);
extern void	win_goto_cb (Widget w);
extern void	win_goto_option_cb (Widget w, Trace *trace, XmSelectionBoxCallbackStruct *cb);
extern void	win_goto_ok_cb (Widget w, Trace *trace, XmSelectionBoxCallbackStruct *cb);
extern void	win_goto_cancel_cb (Widget w, Trace *trace, XmAnyCallbackStruct *cb);
extern void	win_chg_res_cb (Widget w);
extern void	win_begin_cb (Widget w);
extern void	win_end_cb (Widget w);
extern void	win_inc_res_cb (Widget w);
extern void	win_dec_res_cb (Widget w);
extern void	win_full_res_cb (Widget w);
extern void	win_zoom_res_cb (Widget w);
extern void	res_zoom_click_ev (Widget w, Trace *trace, XButtonPressedEvent *ev);
extern void	debug_toggle_print_cb (Widget w);
extern void	debug_integrity_check_cb (Widget w);
extern void	debug_increase_debugtemp_cb (Widget w);
extern void	debug_decrease_debugtemp_cb (Widget w);
extern void	hscroll_unitinc_cb (Widget w);
extern void	hscroll_unitdec_cb (Widget w);
extern void	hscroll_pageinc_cb (Widget w);
extern void	hscroll_pagedec_cb (Widget w);
extern void	hscroll_drag_cb (Widget w);
extern void	vscroll_unitinc_cb (Widget w);
extern void	vscroll_unitdec_cb (Widget w);
extern void	vscroll_pageinc_cb (Widget w);
extern void	vscroll_pagedec_cb (Widget w);
extern void	vscroll_drag_cb (Widget w);
extern void	win_namescroll_change_cb (Widget w);

/* dt_socket */
extern void	socket_create (void);

/* dt_draw */
extern void	draw_update_sigstart (void);
extern void	draw_perform (void);
extern void	draw_update (void);
extern void	draw_scroll_hook_cb_expose (void);

/* dt_icon */
extern Pixmap	icon_dinos (void);

/* dt_verilog */
extern void	verilog_read (Trace *trace, int read_fd);
extern void	verilog_womp_128s (Trace *);

