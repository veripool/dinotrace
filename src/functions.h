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

TRACE *create_trace();

extern void
    ps_rf(), ps_grid(), ps_cursor(), 
    cb_window_expose(), cb_window_focus(), 
    cb_chg_res(), cb_inc_res(), cb_dec_res(), 
    cb_begin(), cb_end(), cb_zoom_res(), cb_full_res(), 
    res_zoom_click_ev(), 
    
    /* trace routines */
    trace_open_cb(), trace_close_cb(), trace_exit_cb(), trace_clear_cb(),
    trace_read_cb(), trace_reread_cb(), 
    init_globals(),

    /* customize routines */
    cus_dialog_cb(), cus_read_cb(), cus_save_cb(),
    cus_page1_cb(), cus_page2_cb(), cus_page4_cb(), 
    cus_buso_cb(), cus_bush_cb(),
    cus_sighgt_cb(), cus_rf_cb(), cus_grid_cb(), 
    cus_cur_cb(), cus_ok_cb(), cus_apply_cb(), cus_cancel_cb(), 
    cus_hit_return(), cus_restore_cb(), cus_reread_cb(), 
    
    /* cursor routines */
    cur_add_cb(), cur_mov_cb(), cur_del_cb(), cur_clr_cb(),
    cur_highlight_cb(), cur_highlight_ev(),
    cur_add_ev(), cur_move_ev(), cur_delete_ev(), clear_cursor(), 
    
    /* config routines */
    config_read_defaults(), config_read_file(), 
    
    /* grid routines */
    grid_align_cb(), grid_res_cb(), grid_reset_cb(), 
    grid_align_ev(), 
    
    /* signal routines */
    sig_add_cb(), sig_add_ev(), sig_mov_cb(), sig_move_ev(),
    sig_del_cb(), sig_delete_ev(), sig_copy_cb(), sig_copy_ev(),
    sig_highlight_cb(), sig_highlight_ev(), sig_reset_cb(), 
    sig_selected_cb(), sig_ok_cb(), sig_cancel_cb(),
    sig_search_cb(), sig_search_ok_cb(), sig_search_cancel_cb(),
    sig_search_apply_cb(),
    
    /* print screen routines */
    ps_print(), ps_print_all(), ps_cancel(), ps_numpag(), ps_dialog(), 
    ps_hit_return(), ps_reset(), 
    
    /* File routines */
    read_hlo_tempest(), read_decsim_ascii(), read_decsim(),
    read_trace_end(),

    /* utility routines */
    cancel_all_events(), remove_all_events(), 
    dino_message_ack(), message_ack(), 
    cb_fil_read(), get_data_popup(),
    
    cb_cancel_action(), 
    draw(), drawsig(), quit(), 
    customize(), savecustomize(), readcustomize(), 
    hscroll_unitinc(), hscroll_unitdec(), hscroll_drag(), 
    hscroll_pageinc(), hscroll_pagedec(), 
    hscroll_bot(), hscroll_top(), 
    vscroll_unitinc(), vscroll_unitdec(), vscroll_drag(), 
    vscroll_pageinc(), vscroll_pagedec(), 
    vscroll_bot(), vscroll_top(), 
    print_sig_names(), print_all_traces(), print_screen_traces();

extern void	newtime(),get_geometry();
extern void	upcase_string(), file_directory(), change_title();
extern char	*extract_first_xms_segment();
extern int	posx_to_time();
extern SIGNALSTATE *find_signal_state();
extern SIGNAL_SB *posy_to_signal();
