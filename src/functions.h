/******************************************************************************
 *
 * Filename:
 *	callbacks.h
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
 *
 * Modification History:
 *     AAG	5-JUL-89	Original Version
 *
 */

/***********************************************************************/
/* Defined functions.... Really inlines, but old VMS compiler chokes */

/* Schedule a redraw for trace or everybody */
#define draw_expose_needed(_trace_)	{ (_trace_)->redraw_needed |= 2; global->redraw_needed |= 1; }
#define draw_needed(_trace_)	{ (_trace_)->redraw_needed |= 1; global->redraw_needed |= 1; }
#define draw_all_needed() { global->redraw_needed |= 2; }

#define	cptr_to_value(cptr,value_ptr) \
{\
     (value_ptr)->siglw.number = (cptr)[0].number;\
     (value_ptr)->number[0] = (cptr)[1].number;\
     (value_ptr)->number[1] = (cptr)[2].number;\
     (value_ptr)->number[2] = (cptr)[3].number;\
 }

/* Also identical commented function in dt_file if this isn't defined */
#define	fil_add_cptr(sig_ptr, value_ptr, check) \
{\
    SIGNAL_LW	*cptr;\
    long diff = ((SIGNAL *)(sig_ptr))->cptr - ((SIGNAL *)(sig_ptr))->bptr;\
    if (diff > BLK_SIZE / 4 * ((SIGNAL *)(sig_ptr))->blocks - 4) {\
	((SIGNAL *)(sig_ptr))->blocks++;\
	((SIGNAL *)(sig_ptr))->bptr = (SIGNAL_LW *)XtRealloc ((char*)((SIGNAL *)(sig_ptr))->bptr, ((SIGNAL *)(sig_ptr))->blocks*BLK_SIZE);\
	((SIGNAL *)(sig_ptr))->cptr = ((SIGNAL *)(sig_ptr))->bptr+diff;\
	}\
    cptr = ((SIGNAL *)(sig_ptr))->cptr - ((SIGNAL *)(sig_ptr))->lws;\
    if ( !(check)\
	|| ( cptr->sttime.state != ((VALUE *)(value_ptr))->siglw.sttime.state )\
	|| ( (cptr+1)->number != ((VALUE *)(value_ptr))->number[0] )\
	|| ( (cptr+2)->number != ((VALUE *)(value_ptr))->number[1] )\
	|| ( (cptr+3)->number != ((VALUE *)(value_ptr))->number[2] ) )\
	{\
	(((SIGNAL *)(sig_ptr))->cptr)[0].number = ((VALUE *)(value_ptr))->siglw.number;\
	(((SIGNAL *)(sig_ptr))->cptr)[1].number = ((VALUE *)(value_ptr))->number[0];\
	(((SIGNAL *)(sig_ptr))->cptr)[2].number = ((VALUE *)(value_ptr))->number[1];\
	(((SIGNAL *)(sig_ptr))->cptr)[3].number = ((VALUE *)(value_ptr))->number[2];\
	(((SIGNAL *)(sig_ptr))->cptr) += ((SIGNAL *)(sig_ptr))->lws;\
	}\
    }
/*#undef	fil_add_cptr*/


/***********************************************************************/

/* Dinotrace.c */
extern char	*help_message();

/* dt_dispmgr.c */
extern TRACE	*create_trace();
extern TRACE	*trace_create_split_window();
extern TRACE	*malloc_trace();
extern TRACE	*open_trace();
extern void
    trace_open_cb(), trace_close_cb(), trace_exit_cb(), trace_clear_cb(),
    init_globals(), create_globals(),
    set_cursor();
extern int	last_set_cursor ();

/* dt_customize.c routines */
extern void
    cus_dialog_cb(), cus_read_cb(), 
    cus_ok_cb(), cus_apply_cb(), 
    cus_hit_return(), cus_restore_cb(), cus_reread_cb();
    
/* dt_cursor.c routines */
extern void
    cur_add_cb(), cur_mov_cb(), cur_del_cb(), cur_clr_cb(),
    cur_highlight_cb(), cur_highlight_ev(),
    cur_add_ev(), cur_move_ev(), cur_delete_ev(),
    cur_add (DTime, ColorNum, CursorType),
    cur_remove (CURSOR *),
    cur_delete_of_type ();
    
/* dt_config.c routines */
extern void
    config_write_cb(),
    print_signal_states(),
    config_read_defaults (),
    config_read_file (), upcase_string(),
    config_restore_defaults (TRACE *),
    config_parse_geometry (char *, GEOMETRY *);
extern SIGNALSTATE *find_signal_state (TRACE *, char *);
extern int	wildmat ();
    
/* dt_grid.c routines */
extern void
    grid_align_cb(), grid_res_cb(), grid_reset_cb(),
    grid_align_ev();
    
/* dt_signal.c routines */
extern void
    sig_update_search(), sig_free(),
    sig_highlight_selected (int),
    /**/
    sig_add_cb(), sig_add_ev(), sig_mov_cb(), sig_move_ev(),
    sig_del_cb(), sig_delete_ev(), sig_copy_cb(), sig_copy_ev(),
    sig_highlight_cb(), sig_highlight_ev(), sig_reset_cb(), 
    sig_selected_cb(), sig_ok_cb(), sig_cancel_cb(),
    sig_examine_cb(), sig_examine_ev(),
    sig_search_cb(), sig_search_ok_cb(), sig_search_apply_cb(),
    sig_select_cb(), sig_select_ok_cb(), sig_select_apply_cb();
extern void sig_modify_enables (TRACE *);
extern void sig_delete_selected (/*Boolean*/);
extern void sig_move_selected (/*Boolean*/);
extern void sig_copy_selected (/*Boolean*/);
extern void sig_wildmat_select (TRACE *, char *);
extern void sig_goto_pattern (TRACE *, char *);
extern void sig_cross_preserve (TRACE *);
extern void sig_cross_restore (TRACE *);
#if defined(VMS) || defined(mips)
extern char *strdup(char *);
#endif
    
/* dt_value.c routines */
extern void
    val_annotate_cb(), val_annotate_do_cb(), val_annotate_ok_cb(), val_annotate_apply_cb(),
    val_examine_cb(), val_examine_ev(),
    val_search_cb(), val_search_ok_cb(), val_search_apply_cb(),
    val_highlight_cb(), val_highlight_ev(),
    val_update_search();
    
/* dt_printscreen routines */
extern void
    ps_print_internal(), ps_print_req_cb(), ps_print_direct_cb(),
    ps_print_all(), ps_numpag_cb(), ps_dialog(), ps_reset();
extern void cur_print (FILE *);
    
/* dt_binary.c routines */
extern void
    tempest_read(), decsim_read_binary();

/* dt_file.c routines */
extern void
    trace_read_cb(), trace_reread_cb(), trace_reread_all_cb(),
    free_data(),
    read_make_busses(),
    fil_string_to_value(),
    decsim_read_ascii(), 
    read_trace_end(), help_cb(), help_trace_cb(), update_signal_states();
extern void fgets_dynamic ();
#ifndef fil_add_cptr
extern void fil_add_cptr();
#endif

/* dt_util routines */
extern void
    unmanage_cb(),
    add_event(), remove_all_events(), 
    file_directory(), change_title(),
    new_time(TRACE *), new_res(), get_geometry(),
    get_file_name(),
    cancel_all_events(),
    dino_message_ack(), 
    fil_read_cb(), get_data_popup(), time_to_string(),
    fil_format_option_cb(),
    print_sig_names(), print_all_traces(), print_screen_traces(), print_sig_info(SIGNAL *);
extern TRACE	*widget_to_trace (Widget);
extern SIGNAL_LW *cptr_at_time ();
extern char	*extract_first_xms_segment (XmString);
extern XmString	string_create_with_cr (char *);
extern char	*date_string ();
extern DTime	posx_to_time ();
extern DTime	posx_to_time_edge ();
extern DTime	string_to_time ();
extern DTime	time_units_to_multiplier ();
extern void	time_to_string ();
extern char	*time_units_to_string ();
extern SIGNAL	*posy_to_signal ();
extern CURSOR	*posx_to_cursor ();
extern CURSOR	*time_to_cursor ();
extern ColorNum submenu_to_color (TRACE *, Widget, int);
extern void     cptr_to_string ();
extern void     string_to_value ();
    
/* dt_window routines */
extern void
    win_expose_cb(), win_resize_cb(), win_refresh_cb(),
    win_goto_cb(), win_goto_option_cb(),
    win_goto_ok_cb(), win_goto_cancel_cb(),
    win_chg_res_cb(), win_inc_res_cb(), win_dec_res_cb(), 
    win_begin_cb(), win_end_cb(), win_zoom_res_cb(), win_full_res_cb(), 
    res_zoom_click_ev(),
    debug_toggle_print_cb(),
    debug_integrity_check_cb(),
    debug_increase_debugtemp_cb(),
    debug_decrease_debugtemp_cb(),
    hscroll_unitinc(), hscroll_unitdec(), hscroll_drag(), 
    hscroll_pageinc(), hscroll_pagedec(), 
    hscroll_bot(), hscroll_top(), 
    vscroll_unitinc(), vscroll_unitdec(), vscroll_drag(), 
    vscroll_pageinc(), vscroll_pagedec(), 
    vscroll_bot(), vscroll_top();
extern int	win_goto_number();

/* dt_draw */
extern void
    update_globals(),
    draw_perform(),
    draw();

/* dt_icon */
extern Pixmap	make_icon (Display *, Drawable, char *, Dimension, Dimension);

/* dt_verilog */
extern void	verilog_read();
extern void	verilog_womp_96s(TRACE *);

