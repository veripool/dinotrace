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

/* Dinotrace.c */
extern char	*help_message();

/* dt_dispmgr.c */
TRACE *create_trace();
extern void
    trace_open_cb(), trace_close_cb(), trace_exit_cb(), trace_clear_cb(),
    trace_read_cb(), trace_reread_cb(), 
    init_globals();

/* dt_customize.c routines */
extern void
    cus_dialog_cb(), cus_read_cb(), cus_save_cb(),
    cus_ok_cb(), cus_apply_cb(), cus_cancel_cb(), 
    cus_hit_return(), cus_restore_cb(), cus_reread_cb();
    
/* dt_cursor.c routines */
extern void
    cur_add_cb(), cur_mov_cb(), cur_del_cb(), cur_clr_cb(),
    cur_highlight_cb(), cur_highlight_ev(),
    cur_add_ev(), cur_move_ev(), cur_delete_ev(), clear_cursor(),
    add_cursor(), remove_cursor();
    
/* dt_config.c routines */
extern void
    config_read_defaults(), config_read_file(), upcase_string();
extern SIGNALSTATE *find_signal_state();
    
/* dt_grid.c routines */
extern void
    grid_align_cb(), grid_res_cb(), grid_reset_cb(),
    grid_align_ev();
    
/* dt_signal.c routines */
extern void
    sig_add_cb(), sig_add_ev(), sig_mov_cb(), sig_move_ev(),
    sig_del_cb(), sig_delete_ev(), sig_copy_cb(), sig_copy_ev(),
    sig_highlight_cb(), sig_highlight_ev(), sig_reset_cb(), 
    sig_selected_cb(), sig_ok_cb(), sig_cancel_cb(),
    sig_search_cb(), sig_search_ok_cb(), sig_search_cancel_cb(),
    sig_search_apply_cb();
    
/* dt_printscreen routines */
extern void
    ps_print(), ps_print_all(), ps_cancel(), ps_numpag(), ps_dialog(), 
    ps_hit_return(), ps_reset();
    
/* dt_file.c routines */
extern void
    quit(), 
    read_hlo_tempest(), read_decsim_ascii(), read_decsim(),
    read_trace_end(), help_cb();

/* dt_util routines */
extern void
    file_directory(), change_title(),
    new_time(),get_geometry(),
    cancel_all_events(), remove_all_events(), 
    dino_message_ack(), message_ack(), 
    cb_fil_read(), get_data_popup(), time_to_string(),
    print_sig_names(), print_all_traces(), print_screen_traces();
extern char	*extract_first_xms_segment();
extern int	posx_to_time();
extern CURSOR	*posx_to_cursor();
extern CURSOR	*time_to_cursor();
extern SIGNAL	*posy_to_signal();
    
/* dt_window routines */
extern void
    win_expose_cb(), win_goto_cb(),
    cb_chg_res(), cb_inc_res(), cb_dec_res(), 
    cb_begin(), cb_end(), cb_zoom_res(), cb_full_res(), 
    res_zoom_click_ev(),
    hscroll_unitinc(), hscroll_unitdec(), hscroll_drag(), 
    hscroll_pageinc(), hscroll_pagedec(), 
    hscroll_bot(), hscroll_top(), 
    vscroll_unitinc(), vscroll_unitdec(), vscroll_drag(), 
    vscroll_pageinc(), vscroll_pagedec(), 
    vscroll_bot(), vscroll_top();

/* dt_draw */
extern void
    draw(), drawsig();

