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

extern void	create_display(), delete_display(), clear_display(),
		ps_rf(),ps_grid(),ps_cursor(),
		cb_window_expose(), cb_window_focus(),
		cb_read_trace(), cb_chg_res(), cb_inc_res(), cb_dec_res(),
		cb_reread_trace(), cb_begin(), cb_end(), cb_zoom_res(), cb_full_res(),
                res_zoom_click_ev(), 

		/* customize routines */
		cus_dialog_cb(),cus_read_cb(),cus_save_cb(),cus_page_cb(),
		cus_bus_cb(),cus_sighgt_cb(),cus_rf_cb(),cus_grid_cb(),
		cus_cur_cb(),cus_ok_cb(),cus_apply_cb(),cus_cancel_cb(),
		cus_hit_return(),cus_restore_cb(),cus_reread_cb(),

		/* cursor routines */
		cur_add_cb(),cur_mov_cb(),cur_del_cb(),cur_clr_cb(),
		add_cursor(),move_cursor(),delete_cursor(),clear_cursor(),

		/* config routines */
		config_read_defaults(),config_read_file(),

		/* grid routines */
		grid_align_cb(),grid_res_cb(),grid_reset_cb(),
		align_grid(),

		/* signal routines */
		sig_add_cb(),sig_mov_cb(),sig_del_cb(),sig_reset_cb(),
		sig_selected_cb(),sig_ok_cb(),sig_cancel_cb(),
		add_signal(),move_signal(),delete_signal(),

		/* print screen routines */
		ps_print(),ps_print_all(),ps_cancel(),ps_numpag(),ps_dialog(),
		ps_hit_return(),ps_reset(),

		/* utility routines */
		cancel_all_events(),remove_all_events(),
		dino_message_info(),dino_message_ack(),message_ack(),
		cb_fil_ok(),cb_fil_can(),cb_fil_read(),

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

static DwtCallback dino_cb[2] =
{          
    {NULL, NULL},
    {NULL, NULL}
};

static DwtCallback win_exp_cb[2] =
{         
    {cb_window_expose, NULL},
    {NULL,             NULL}
};

static DwtCallback win_foc_cb[2] =
{
    {cb_window_focus, NULL},
    {NULL,            NULL}
};

static DwtCallback del_tr_cb[2] =
{
    {delete_display, NULL},
    {NULL,              NULL}
};

static DwtCallback clr_tr_cb[2] =
{
    {clear_display,  NULL},
    {NULL,              NULL}
};

static DwtCallback rd_tr_cb[2] =
{
    {cb_read_trace,   NULL},
    {NULL,            NULL}
};

static DwtCallback rerd_tr_cb[2] =
{
    {cb_reread_trace,   NULL},
    {NULL,            NULL}
};

static DwtCallback hsc_inc_cb[2] =
{
    {hscroll_unitinc,   NULL},
    {NULL,            NULL}
};

static DwtCallback hsc_dec_cb[2] =
{
    {hscroll_unitdec,   NULL},
    {NULL,            NULL}
};

static DwtCallback hsc_pginc_cb[2] =
{
    {hscroll_pageinc,   NULL},
    {NULL,            NULL}
};

static DwtCallback hsc_pgdec_cb[2] =
{
    {hscroll_pagedec,   NULL},
    {NULL,            NULL}
};

static DwtCallback hsc_drg_cb[2] =
{
    {hscroll_drag,   NULL},
    {NULL,            NULL}
};

static DwtCallback hsc_bot_cb[2] =
{
    {hscroll_bot,   NULL},
    {NULL,            NULL}
};

static DwtCallback hsc_top_cb[2] =
{
    {hscroll_top,   NULL},
    {NULL,            NULL}
};

static DwtCallback vsc_inc_cb[2] =
{
    {vscroll_unitinc,   NULL},
    {NULL,            NULL}
};

static DwtCallback vsc_dec_cb[2] =
{
    {vscroll_unitdec,   NULL},
    {NULL,            NULL}
};

static DwtCallback vsc_pginc_cb[2] =
{
    {vscroll_pageinc,   NULL},
    {NULL,            NULL}
};

static DwtCallback vsc_pgdec_cb[2] =
{
    {vscroll_pagedec,   NULL},
    {NULL,            NULL}
};

static DwtCallback vsc_drg_cb[2] =
{
    {vscroll_drag,   NULL},
    {NULL,            NULL}
};

static DwtCallback vsc_bot_cb[2] =
{
    {vscroll_bot,   NULL},
    {NULL,            NULL}
};

static DwtCallback vsc_top_cb[2] =
{
    {vscroll_top,   NULL},
    {NULL,            NULL}
};

static DwtCallback chg_res_cb[2] =
{
    {cb_chg_res,      NULL},
    {NULL,            NULL}
};

static DwtCallback inc_res_cb[2] =
{
    {cb_inc_res,      NULL},
    {NULL,            NULL}
};

static DwtCallback dec_res_cb[2] =
{
    {cb_dec_res,      NULL},
    {NULL,            NULL}
};

static DwtCallback full_res_cb[2] =
{
    {cb_full_res,     NULL},
    {NULL,            NULL}
};

static DwtCallback zoom_res_cb[2] =
{
    {cb_zoom_res,	NULL},
    {NULL,		NULL}
};

static DwtCallback begin_cb[2] =
{
    {cb_begin,		NULL},
    {NULL,		NULL}
};

static DwtCallback end_cb[2] =
{
    {cb_end,		NULL},
    {NULL,		NULL}
};

static DwtCallback custom_cb[28] =
{
    {cus_dialog_cb,	NULL},
    {NULL,		NULL},
    {cus_read_cb,	NULL},
    {NULL,		NULL},
    {cus_save_cb,	NULL},
    {NULL,		NULL},
    {cus_page_cb,	NULL},
    {NULL,		NULL},
    {cus_bus_cb,	NULL},
    {NULL,		NULL},
    {cus_sighgt_cb,	NULL},
    {NULL,		NULL},
    {cus_rf_cb,		NULL},
    {NULL,		NULL},
    {cus_grid_cb,	NULL},
    {NULL,		NULL},
    {cus_cur_cb,	NULL},
    {NULL,		NULL},
    {cus_ok_cb,		NULL},
    {NULL,		NULL},
    {cus_apply_cb,	NULL},
    {NULL,		NULL},
    {cus_cancel_cb,	NULL},
    {NULL,		NULL},
    {cus_restore_cb,	NULL},
    {NULL,		NULL},
    {cus_reread_cb,	NULL},
    {NULL,		NULL}
};

static DwtCallback cursor_cb[10] =
{
    {cur_add_cb,	NULL},
    {NULL,		NULL},
    {cur_mov_cb,	NULL},
    {NULL,		NULL},
    {cur_del_cb,	NULL},
    {NULL,		NULL},
    {cur_clr_cb,	NULL},
    {NULL,		NULL},
    {cancel_all_events,	NULL},
    {NULL,		NULL}
};

static DwtCallback grid_cb[8] =
{
    {grid_res_cb,	NULL},
    {NULL,		NULL},
    {grid_align_cb,	NULL},
    {NULL,		NULL},
    {grid_reset_cb,	NULL},
    {NULL,		NULL},
    {cancel_all_events,	NULL},
    {NULL,		NULL}
};

static DwtCallback signal_cb[10] =
{
    {sig_add_cb,	NULL},
    {NULL,		NULL},
    {sig_mov_cb,	NULL},
    {NULL,		NULL},
    {sig_del_cb,	NULL},
    {NULL,		NULL},
    {sig_reset_cb,	NULL},
    {NULL,		NULL},
    {cancel_all_events,	NULL},
    {NULL,		NULL}
};

static DwtCallback note_cb[6] =
{
    {sig_add_cb,	NULL},
    {NULL,		NULL},
    {sig_mov_cb,	NULL},
    {NULL,		NULL},
    {sig_del_cb,	NULL},
    {NULL,		NULL}
};

static DwtCallback print_screen_cb[2] =
{
    {ps_dialog,      NULL},
    {NULL,            NULL}
};

static DwtCallback print_sig_names_cb[2] =
{
    {print_sig_names,      NULL},
    {NULL,            NULL}
};

static DwtCallback print_screen_traces_cb[2] =
{
    {print_screen_traces,      NULL},
    {NULL,            NULL}
};

static DwtCallback print_all_traces_cb[2] =
{
    {print_all_traces,      NULL},
    {NULL,            NULL}
};

