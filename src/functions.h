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

extern void
    create_display(), delete_display(), clear_display(), 
    ps_rf(), ps_grid(), ps_cursor(), 
    cb_window_expose(), cb_window_focus(), 
    cb_read_trace(), cb_chg_res(), cb_inc_res(), cb_dec_res(), 
    cb_reread_trace(), cb_begin(), cb_end(), cb_zoom_res(), cb_full_res(), 
    res_zoom_click_ev(), 
    
    /* customize routines */
    cus_dialog_cb(), cus_read_cb(), cus_save_cb(), cus_page_cb(), 
    cus_bus_cb(), cus_sighgt_cb(), cus_rf_cb(), cus_grid_cb(), 
    cus_cur_cb(), cus_ok_cb(), cus_apply_cb(), cus_cancel_cb(), 
    cus_hit_return(), cus_restore_cb(), cus_reread_cb(), 
    
    /* cursor routines */
    cur_add_cb(), cur_mov_cb(), cur_del_cb(), cur_clr_cb(), 
    add_cursor(), move_cursor(), delete_cursor(), clear_cursor(), 
    
    /* config routines */
    config_read_defaults(), config_read_file(), 
    
    /* grid routines */
    grid_align_cb(), grid_res_cb(), grid_reset_cb(), 
    align_grid(), 
    
    /* signal routines */
    sig_add_cb(), sig_mov_cb(), sig_del_cb(), sig_reset_cb(), 
    sig_selected_cb(), sig_ok_cb(), sig_cancel_cb(), 
    add_signal(), move_signal(), delete_signal(), 
    sig_highlight_cb(), 
    
    /* print screen routines */
    ps_print(), ps_print_all(), ps_cancel(), ps_numpag(), ps_dialog(), 
    ps_hit_return(), ps_reset(), 
    
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

extern void upcase_string(), file_directory(), change_title();
extern char *extract_first_xms_segment();

