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
#define	CKPT() printf ("CKPT %s:%d\n", __FILE__, __LINE__)

#define TIME_TO_XPOS(_xtime_) \
        ( ((_xtime_) - global->time) * global->res + global->xstart )

#define	TIME_WIDTH(_trace_) \
	((DTime)( ((_trace_)->width - XMARGIN - global->xstart) / global->res ))

/* Point to next (prev) cptr.  Sizeof(value) is larger then we increment, */
/* so force types to get pointer += 4 bytes */
#define CPTR_NEXT(cptr) ((Value_t*) (((uint_t*)(cptr)) \
					+ ((cptr)->siglw.stbits.size)))
#define CPTR_PREV(cptr) ((Value_t*) (((uint_t*)(cptr)) \
					- ((cptr)->siglw.stbits.size_prev)))
#define CPTR_TIME(cptr) ((cptr)->time)
#define	val_copy(to_cptr,from_cptr) \
{\
     (to_cptr)->siglw.number = (from_cptr)->siglw.number;\
     (to_cptr)->time         = (from_cptr)->time;\
     (to_cptr)->number[0]    = (from_cptr)->number[0];\
     (to_cptr)->number[1]    = (from_cptr)->number[1];\
     (to_cptr)->number[2]    = (from_cptr)->number[2];\
     (to_cptr)->number[3]    = (from_cptr)->number[3];\
     (to_cptr)->number[4]    = (from_cptr)->number[4];\
     (to_cptr)->number[5]    = (from_cptr)->number[5];\
     (to_cptr)->number[6]    = (from_cptr)->number[6];\
     (to_cptr)->number[7]    = (from_cptr)->number[7];\
 }
#define	val_zero(to_cptr) \
{\
     (to_cptr)->siglw.number = 0;\
     (to_cptr)->time         = 0;\
     (to_cptr)->number[0]    = 0;\
     (to_cptr)->number[1]    = 0;\
     (to_cptr)->number[2]    = 0;\
     (to_cptr)->number[3]    = 0;\
     (to_cptr)->number[4]    = 0;\
     (to_cptr)->number[5]    = 0;\
     (to_cptr)->number[6]    = 0;\
     (to_cptr)->number[7]    = 0;\
 }

/* Also identical commented function in dt_file if this isn't defined */
/* It is critical for speed that this be inlined, so we'll just define it */
#define	fil_add_cptr(sig_ptr, value_ptr, nocheck) \
{\
    Value_t	*cptr;\
    ulong_t diff;\
    cptr = sig_ptr->cptr;\
    if ( (nocheck)\
	|| ( cptr->siglw.stbits.state != (value_ptr)->siglw.stbits.state )\
	|| ( cptr->number[0] != (value_ptr)->number[0] )\
	|| ( cptr->number[1] != (value_ptr)->number[1] )\
	| ( cptr->number[2] != (value_ptr)->number[2] )\
	| ( cptr->number[3] != (value_ptr)->number[3] )\
	|| ( cptr->number[4] != (value_ptr)->number[4] )\
	| ( cptr->number[5] != (value_ptr)->number[5] )\
	| ( cptr->number[6] != (value_ptr)->number[6] )\
	| ( cptr->number[7] != (value_ptr)->number[7] ) ) {\
	diff = (uint_t*)sig_ptr->cptr - (uint_t*)sig_ptr->bptr;\
	if (diff > sig_ptr->blocks) {\
	    sig_ptr->blocks += BLK_SIZE;\
	    sig_ptr->bptr = (Value_t *)XtRealloc ((char*)(sig_ptr)->bptr,\
		       ((sig_ptr)->blocks*sizeof(unsigned int)) + (sizeof(Value_t)*2 + 2));\
	    sig_ptr->cptr = (Value_t *)((uint_t*)sig_ptr->bptr + diff);\
	}\
	(value_ptr)->siglw.stbits.size_prev = sig_ptr->cptr->siglw.stbits.size;\
	(value_ptr)->siglw.stbits.size = STATE_SIZE((value_ptr)->siglw.stbits.state);\
	if (sig_ptr->cptr->siglw.number || sig_ptr->cptr!=sig_ptr->bptr) {\
	    sig_ptr->cptr = CPTR_NEXT(sig_ptr->cptr);\
	}\
	val_copy (sig_ptr->cptr, (value_ptr));\
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
extern void	create_globals (int argc, char **argv, Boolean_t sync);
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
extern void	cur_add (DTime time, ColorNum color, CursorType_t type);
extern void	cur_remove (DCursor *);
extern void	cur_delete_of_type (CursorType_t type);
extern DTime	cur_time_first (Trace *trace);
extern DTime	cur_time_last (Trace *trace);
extern void	cur_print (FILE *);
extern char *	cur_examine_string (Trace *trace, DCursor *cursor_ptr);

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
extern void	config_read_defaults (Trace *trace, Boolean_t);
extern void	config_read_file (Trace *trace, char *, Boolean_t, Boolean_t);
extern void	config_trace_defaults (Trace *);
extern void	config_global_defaults (void);
extern void	config_parse_geometry (char *, Geometry *);
extern void	config_update_filenames (Trace *trace);
extern void	config_read_socket (char *line, char *name, int cmdnum, Boolean_t eof);
extern SignalState *find_signal_state (Trace *, char *);
extern int	wildmat ();

extern void	debug_print_signal_states_cb (Widget w);
extern void	config_write_cb (Widget w);

/* dt_grid.c routines */
extern void	grid_calc_autos (Trace *trace);
extern char *	grid_examine_string (Trace *trace, Grid *grid_ptr, DTime time);

extern void	grid_customize_cb (Widget w);
extern void	grid_reset_cb (Widget w);

/* dt_signal.c routines */
extern void	sig_update_search (void);
extern void	sig_free (Trace *trace, Signal *sig_ptr, Boolean_t select, Boolean_t recurse);
extern void	sig_highlight_selected (int color);
extern void	sig_base_selected (Base_t *base_ptr);
extern void	sig_modify_enables (Trace *);
extern void	sig_move_selected (Trace *new_trace, char *after_pattern);
extern void	sig_rename_selected (char *new_name);
extern void	sig_copy_selected (Trace *new_trace, char *after_pattern);
extern void	sig_delete_selected (Boolean_t constant_flag, Boolean_t ignorexz);
extern void	sig_wildmat_select (Trace *, char *);
extern void	sig_goto_pattern (Trace *, char *);
extern void	sig_cross_preserve (Trace *trace);
extern void	sig_cross_restore (Trace *trace) ;
extern Signal *	sig_find_signame (Trace *trace, char *signame);
extern Signal *	sig_wildmat_signame (Trace *trace, char *signame);
extern void	sig_print_names (Trace *trace);
extern char *	sig_examine_string (Trace *trace, Signal *sig_ptr);

extern void	sig_add_cb (Widget w);
extern void	sig_mov_cb (Widget w);
extern void	sig_copy_cb (Widget w);
extern void	sig_del_cb (Widget w);
extern void	sig_base_cb (Widget w);
extern void	sig_add_ev (Widget w, Trace *trace, XButtonPressedEvent *ev);
extern void	sig_move_ev (Widget w, Trace *trace, XButtonPressedEvent *ev);
extern void	sig_copy_ev (Widget w, Trace *trace, XButtonPressedEvent *ev);
extern void	sig_delete_ev (Widget w, Trace *trace, XButtonPressedEvent *ev);
extern void	sig_base_ev (Widget w, Trace *trace, XButtonPressedEvent *ev);
extern void	sig_highlight_ev (Widget w, Trace *trace, XButtonPressedEvent *ev);
extern void	sig_highlight_cb (Widget w);
extern void	sig_select_cb (Widget w);
extern void	sig_cancel_cb (Widget w);
extern void	sig_search_cb (Widget w);
extern void	sig_search_ok_cb (Widget w, Trace *trace, XmSelectionBoxCallbackStruct *cb);
extern void	sig_search_apply_cb (Widget w, Trace *trace, XmSelectionBoxCallbackStruct *cb);

/* dt_value.c routines */
extern void	val_minimize (Value_t *value_ptr);
extern void	val_update_search (void);
extern void	val_states_update (void);
extern void	val_to_string (const Base_t *base_ptr, char *strg, const Value_t *value_ptr, Boolean_t display);
extern Boolean_t  val_equal (const Value_t *vptra, const Value_t *vptrb);
extern void	val_bases_init (void);
extern Base_t	*val_base_find (const char *name);

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

/* dt_print routines */
extern void	ps_print_internal (Trace *trace);
extern void	ps_reset  (Trace *trace);

extern void	ps_dialog_cb (Widget w);
extern void	ps_reset_cb  (Widget w);
extern void	ps_print_direct_cb (Widget w);
extern void	ps_print_req_cb (Widget w);
extern void	ps_range_sensitives_cb (Widget w, RangeWidgets_t *range_ptr, XmSelectionBoxCallbackStruct *cb);

/* dt_binary.c routines */
extern void	tempest_read (Trace *trace, int read_fd);
extern void	decsim_read_binary (Trace *trace, int read_fd);

/* dt_file.c routines */
extern void	fil_read (Trace *trace);
extern void	free_data (Trace *trace);
extern void	fil_make_busses (Trace *trace, Boolean_t not_tempest);
extern void	decsim_read_ascii (Trace *trace, int read_fd, FILE *decsim_z_readfp);
extern void	fil_trace_end (Trace *trace);
extern void 	fil_select_set_pattern (Trace *trace, Widget dialog, char *pattern);
extern void	fil_string_add_cptr (Signal *sig_ptr, const char *value_strg, DTime time, Boolean_t nocheck);
#ifndef fil_add_cptr
extern void	fil_add_cptr (Signal *sig_ptr, Value_t *value_ptr, Boolean_t nocheck);
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

extern void	new_time (Trace *);
extern void	get_geometry (Trace *trace);
extern void	get_file_name (Trace *trace);
extern void	get_data_popup (Trace *trace, char *string, int type);
extern void	print_sig_info (Signal *sig_ptr);
extern Trace *	widget_to_trace (Widget w);
extern Value_t *value_at_time (Signal *sig_ptr, DTime ctime);
extern DTime	posx_to_time (Trace *trace, Position x);
extern DTime	posx_to_time_edge (Trace *trace, Position x, Position y);
extern Signal *	posy_to_signal (Trace *trace, Position y);
extern Grid *	posx_to_grid (Trace *trace, Position x, DTime *time_ptr);
extern DCursor *posx_to_cursor (Trace *trace, Position x);
extern DCursor *time_to_cursor (DTime xtime);
extern void	time_to_string (Trace *trace, char *strg, DTime ctime, Boolean_t relative);
extern DTime 	time_units_to_multiplier (TimeRep_t timerep);
extern char *	time_units_to_string (TimeRep_t timerep, Boolean_t showvalue);
extern void	string_to_value (Base_t **base_pptr, const char *strg, Value_t *value_ptr);
extern DTime	string_to_time (Trace *trace, char *strg);

extern void	DManageChild (Widget w, Trace *trace, MCKeys_t keys);
extern void	add_event (int type, void (*callback)());
extern void	remove_all_events (Trace *trace);
extern void	change_title (Trace *trace);
extern void	dino_message_ack (Trace *trace, int type, char *msg);
extern XmString	string_create_with_cr (char *);
extern ColorNum submenu_to_color (Trace *trace, Widget, int);
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
extern void	win_namescroll_change_cb (Widget w);
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

