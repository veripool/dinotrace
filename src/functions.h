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
#define draw_expose_needed(_trace_)	{ global->redraw_needed |= GRD_TRACE; (_trace_)->redraw_needed |= TRD_EXPOSE; }
#define draw_needed(_trace_)		{ global->redraw_needed |= GRD_TRACE; (_trace_)->redraw_needed |= TRD_REDRAW; }
#define draw_all_needed() 		{ global->redraw_needed |= GRD_ALL; }
#define draw_manual_needed() 		{ global->redraw_needed |= GRD_MANUAL; }


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

#define max_sigs_on_screen(_trace_) \
        ((int)(((_trace_)->height - (_trace_)->ystart) / (_trace_)->sighgt))

#define TIME_TO_XPOS(_xtime_) \
        ( ((_xtime_) - global->time) * global->res + global->xstart )

#define	TIME_WIDTH(_trace_) \
	((DTime)( ((_trace_)->width - XMARGIN - global->xstart) / global->res ))

/* Avoid binding error messages on XtFree, NOTE ALSO clears the pointer! */
#define DFree(ptr) { XtFree((char *)ptr); ptr = NULL; }

/* Xt provides a nice XtNew, but a XtNewCalloc is nice too (clear storage) */
#define DNewCalloc(type) ((type *) XtCalloc(1, (unsigned) sizeof(type)))

/* Useful for debugging messages */
#define DeNull(_str_) ( ((_str_)==NULL) ? "NULL" : (_str_) )
#define DeNullSignal(_sig_) ( ((_sig_)==NULL) ? "NULLPTR" : (DeNull((_sig_)->signame) ) )

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
    SIGNAL_LW	*cptr;\
    long diff;\
    cptr = ((SIGNAL *)(sig_ptr))->cptr - ((SIGNAL *)(sig_ptr))->lws;\
    if ( !(nocheck)\
	|| ( cptr->sttime.state != ((VALUE *)(value_ptr))->siglw.sttime.state )\
	|| ( cptr[1].number != ((VALUE *)(value_ptr))->number[0] )\
	|| ( cptr[2].number != ((VALUE *)(value_ptr))->number[1] )\
	|| ( cptr[3].number != ((VALUE *)(value_ptr))->number[2] )\
	|| ( cptr[4].number != ((VALUE *)(value_ptr))->number[3] ) )\
	{\
	diff = ((SIGNAL *)sig_ptr)->cptr - ((SIGNAL *)sig_ptr)->bptr;\
	if ((diff*sizeof(SIGNAL_LW)) > ( BLK_SIZE * (sig_ptr)->blocks - sizeof(VALUE)*2 - 2 )) {\
	    ((SIGNAL *)(sig_ptr))->blocks++;\
	    ((SIGNAL *)(sig_ptr))->bptr = (SIGNAL_LW *)XtRealloc ((char*)((SIGNAL *)(sig_ptr))->bptr, ((SIGNAL *)(sig_ptr))->blocks*BLK_SIZE);\
	    ((SIGNAL *)(sig_ptr))->cptr = ((SIGNAL *)(sig_ptr))->bptr+diff;\
	    }\
	(((SIGNAL *)(sig_ptr))->cptr)[0].number = ((VALUE *)(value_ptr))->siglw.number;\
	(((SIGNAL *)(sig_ptr))->cptr)[1].number = ((VALUE *)(value_ptr))->number[0];\
	(((SIGNAL *)(sig_ptr))->cptr)[2].number = ((VALUE *)(value_ptr))->number[1];\
	(((SIGNAL *)(sig_ptr))->cptr)[3].number = ((VALUE *)(value_ptr))->number[2];\
	(((SIGNAL *)(sig_ptr))->cptr)[4].number = ((VALUE *)(value_ptr))->number[3];\
	(((SIGNAL *)(sig_ptr))->cptr) += ((SIGNAL *)(sig_ptr))->lws;\
	}\
    }


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
extern DTime cur_time_first();
extern DTime cur_time_last();
    
/* dt_config.c routines */
extern void
    config_write_cb(),
    print_signal_states(),
    config_read_defaults (),
    config_read_file (), upcase_string(),
    config_trace_defaults (TRACE *),
    config_global_defaults (TRACE *),
    config_parse_geometry (char *, GEOMETRY *);
extern SIGNALSTATE *find_signal_state (TRACE *, char *);
extern int	wildmat ();
    
/* dt_grid.c routines */
extern void
    grid_customize_cb(),
    grid_align_cb(), grid_res_cb(), grid_reset_cb(),
    grid_align_ev();
extern void grid_calc_autos();
    
/* dt_signal.c routines */
extern void
    sig_update_search(), sig_free(),
    sig_highlight_selected (int),
    /**/
    sig_add_cb(), sig_add_ev(), sig_mov_cb(), sig_move_ev(),
    sig_del_cb(), sig_delete_ev(), sig_copy_cb(), sig_copy_ev(),
    sig_highlight_cb(), sig_highlight_ev(), sig_reset_cb(), 
    sig_selected_cb(), sig_cancel_cb(),
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
extern void val_states_update();
    
/* dt_printscreen routines */
extern void
    ps_print_internal(), ps_print_req_cb(), ps_print_direct_cb(),
    ps_print_all(), ps_dialog(), ps_reset();
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
    read_trace_end(), help_cb(), help_trace_cb();
extern void fgets_dynamic ();
#ifndef fil_add_cptr
extern void fil_add_cptr();
#endif

/* dt_util routines */
extern void
    unmanage_cb(),
    add_event(), remove_all_events(), 
    file_directory(), change_title(),
    new_time(TRACE *), new_res(TRACE *, float), get_geometry(),
    get_file_name(),
    cancel_all_events(),
    dino_message_ack(), 
    fil_read_cb(), get_data_popup(), time_to_string(),
    fil_format_option_cb(),
    print_sig_names(), print_screen_traces(), print_sig_info(SIGNAL *);
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
extern int	option_to_number ();
extern SIGNAL	*sig_find_signame ();
extern SIGNAL	*sig_wildmat_signame ();
    
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
extern void	vscroll_new();

/* dt_socket */
extern void	socket_create();

/* dt_draw */
extern void
    draw_update_sigstart(),
    draw_perform(),
    draw();

/* dt_icon */
extern Pixmap	make_icon (Display *, Drawable, char *, Dimension, Dimension);

/* dt_verilog */
extern void	verilog_read();
extern void	verilog_womp_128s(TRACE *);

