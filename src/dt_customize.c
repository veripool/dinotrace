/******************************************************************************
 *
 * Filename:
 *     dt_customize.c
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
 *     This module contains the callback routines that are the interface
 *     that allows the user to customize his trace display.
 *
 * Modification History:
 *     AAG	28-Jul-89	Original Version
 *     AAG	22-Aug-90	Base Level V4.1
 *     AAG	29-Apr-91	Use X11,fixed casts for Ultrix support
 *
 */


#include <X11/DECwDwtApplProg.h>
#include <X11/Xlib.h>

#include "dinotrace.h"
#include "callbacks.h"



static int MANAGED=FALSE;

void
cus_dialog_cb(w,ptr,cb)
Widget		w;
DISPLAY_SB	*ptr;
DwtAnyCallbackStruct *cb;
{
    char		title[100];

    if (DTPRINT) printf("In customize - ptr=%d\n",ptr);

    if (!ptr->custom.customize)
    {
	if (ptr->filename[0] == '\0')
	    sprintf(title,"Customize #%d",ptr);
	else
	    sprintf(title,"Customize %s",ptr->filename);

	XtSetArg(arglist[0],DwtNdefaultPosition, TRUE);
	XtSetArg(arglist[1],DwtNtitle, DwtLatin1String(title));
	XtSetArg(arglist[2],DwtNwidth, 200);
	XtSetArg(arglist[3],DwtNheight, 150);
	ptr->custom.customize = DwtDialogBoxPopupCreate(ptr->work,"",arglist,4);

	/* Create radio box for page increment */
	ptr->custom.rpage = DwtRadioBox(ptr->custom.customize,"",10,20,NULL,NULL);
                     
	/* Create label for page increment */
	ptr->custom.page_label = DwtLabel(ptr->custom.customize,"",10,5,
		DwtLatin1String("Page Inc/Dec"),NULL);
	XtManageChild(ptr->custom.page_label);

	XtSetArg(arglist[0],DwtNlabel,DwtLatin1String("1/4 Page"));
	XtSetArg(arglist[1],DwtNx, 10);
	XtSetArg(arglist[2],DwtNy, 10);
	custom_cb[6].tag = QPAGE;
	XtSetArg(arglist[3],DwtNarmCallback, custom_cb+6);
	ptr->custom.tpage1 = DwtToggleButtonCreate(ptr->custom.rpage,"",arglist,4);
	XtManageChild(ptr->custom.tpage1);

	XtSetArg(arglist[0],DwtNlabel,DwtLatin1String("1/2 Page"));
	XtSetArg(arglist[1],DwtNx, 10);
	XtSetArg(arglist[2],DwtNy, 30);
	custom_cb[6].tag = HPAGE;
	XtSetArg(arglist[3],DwtNarmCallback, custom_cb+6);
	ptr->custom.tpage2 = DwtToggleButtonCreate(ptr->custom.rpage,"",arglist,4);
	XtManageChild(ptr->custom.tpage2);

	XtSetArg(arglist[0],DwtNlabel,DwtLatin1String(" 1  Page"));
	XtSetArg(arglist[1],DwtNx, 10);
	XtSetArg(arglist[2],DwtNy, 50);
	custom_cb[6].tag = FPAGE;
	XtSetArg(arglist[3],DwtNarmCallback, custom_cb+6);
	ptr->custom.tpage3 = DwtToggleButtonCreate(ptr->custom.rpage,"",arglist,4);
	XtManageChild(ptr->custom.tpage3);

	/* Create radio box for bus representation */
	ptr->custom.rbus = DwtRadioBox(ptr->custom.customize,"",90,20,NULL,NULL);
                     
	/* Create label for bus value */
	ptr->custom.bus_label = DwtLabel(ptr->custom.customize,"",90,5,
		DwtLatin1String("Bus Repres."),NULL);
	XtManageChild(ptr->custom.bus_label);

	XtSetArg(arglist[0],DwtNlabel,DwtLatin1String("INDIVIDUAL"));
	XtSetArg(arglist[1],DwtNx, 10);
	XtSetArg(arglist[2],DwtNy, 10);
	custom_cb[8].tag = IBUS;
	XtSetArg(arglist[3],DwtNarmCallback, custom_cb+8);
	XtSetArg(arglist[4],DwtNsensitive, FALSE);
	ptr->custom.tbus1 = DwtToggleButtonCreate(ptr->custom.rbus,"",arglist,5);
	XtManageChild(ptr->custom.tbus1);

	XtSetArg(arglist[0],DwtNlabel,DwtLatin1String("BINARY"));
	XtSetArg(arglist[1],DwtNx, 10);
	XtSetArg(arglist[2],DwtNy, 30);
	custom_cb[8].tag = BBUS;
	XtSetArg(arglist[3],DwtNarmCallback, custom_cb+8);
	XtSetArg(arglist[4],DwtNsensitive, FALSE);
	ptr->custom.tbus2 = DwtToggleButtonCreate(ptr->custom.rbus,"",arglist,5);
	XtManageChild(ptr->custom.tbus2);

	XtSetArg(arglist[0],DwtNlabel,DwtLatin1String("OCTAL"));
	XtSetArg(arglist[1],DwtNx, 10);
	XtSetArg(arglist[2],DwtNy, 30);
	custom_cb[8].tag = OBUS;
	XtSetArg(arglist[3],DwtNarmCallback, custom_cb+8);
	ptr->custom.tbus3 = DwtToggleButtonCreate(ptr->custom.rbus,"",arglist,4);
	XtManageChild(ptr->custom.tbus3);

	XtSetArg(arglist[0],DwtNlabel,DwtLatin1String("HEXADECIMAL"));
	XtSetArg(arglist[1],DwtNx, 10);
	XtSetArg(arglist[2],DwtNy, 50);
	custom_cb[8].tag = HBUS;
	XtSetArg(arglist[3],DwtNarmCallback, custom_cb+8);
	ptr->custom.tbus4 = DwtToggleButtonCreate(ptr->custom.rbus,"",arglist,4);
	XtManageChild(ptr->custom.tbus4);

	/* Create signal height slider */
	XtSetArg(arglist[0], DwtNtitle, DwtLatin1String("Signal Height") );
	XtSetArg(arglist[1], DwtNx, 90);
	XtSetArg(arglist[2], DwtNy, 75);
	XtSetArg(arglist[3], DwtNscaleWidth, 100);
	XtSetArg(arglist[4], DwtNvalue, curptr->sighgt);
	XtSetArg(arglist[5], DwtNminValue, 15);
	XtSetArg(arglist[6], DwtNmaxValue, 50);
	custom_cb[10].tag = (int)ptr;
	XtSetArg(arglist[7], DwtNvalueChangedCallback, custom_cb+10);
	ptr->custom.s1 = DwtScaleCreate(ptr->custom.customize,"",arglist,8);
	XtManageChild(ptr->custom.s1);

	/* Create RF button */
	XtSetArg(arglist[0],DwtNlabel,DwtLatin1String("Rise/Fall Time"));
	XtSetArg(arglist[1],DwtNx, 10);
	XtSetArg(arglist[2],DwtNy, 75);
	custom_cb[12].tag = (int)ptr;
	XtSetArg(arglist[3],DwtNvalueChangedCallback, custom_cb+12);
	ptr->custom.rfwid = DwtToggleButtonCreate(ptr->custom.customize,"",arglist,4);
	XtManageChild(ptr->custom.rfwid);

	/* Create grid state on/off button */
	XtSetArg(arglist[0],DwtNlabel,DwtLatin1String("Grid On/Off"));
	XtSetArg(arglist[1],DwtNx, 10);
	XtSetArg(arglist[2],DwtNy, 85);
	custom_cb[14].tag = (int)ptr;
	XtSetArg(arglist[3],DwtNvalueChangedCallback, custom_cb+14);
	ptr->custom.grid_state = DwtToggleButtonCreate(ptr->custom.customize,"",arglist,4);
	XtManageChild(ptr->custom.grid_state);

	/* Create cursor state on/off button */
	XtSetArg(arglist[0],DwtNlabel,DwtLatin1String("Cursors On/Off"));
	XtSetArg(arglist[1],DwtNx, 10);
	XtSetArg(arglist[2],DwtNy, 95);
	custom_cb[16].tag = (int)ptr;
	XtSetArg(arglist[3],DwtNvalueChangedCallback, custom_cb+16);
	ptr->custom.cursor_state = DwtToggleButtonCreate(ptr->custom.customize,"",arglist,4);
	XtManageChild(ptr->custom.cursor_state);

	/* Create OK button */
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String(" OK ") );
	XtSetArg(arglist[1], DwtNx, 20 );
	XtSetArg(arglist[2], DwtNy, 130 );
	custom_cb[18].tag = (int)ptr;
	XtSetArg(arglist[3], DwtNactivateCallback, custom_cb+18);
	ptr->custom.b1 = DwtPushButtonCreate(ptr->custom.customize,"",arglist,4);
	XtManageChild(ptr->custom.b1);

	/* create apply button */
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Apply") );
	XtSetArg(arglist[1], DwtNx, 60 );
	XtSetArg(arglist[2], DwtNy, 130 );
	custom_cb[20].tag = (int)ptr;
	XtSetArg(arglist[3], DwtNactivateCallback, custom_cb+20);
	ptr->custom.b2 = DwtPushButtonCreate(ptr->custom.customize,"",arglist,4);
	XtManageChild(ptr->custom.b2);

	/* create cancel button */
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String("Cancel") );
	XtSetArg(arglist[1], DwtNx, 100 );
	XtSetArg(arglist[2], DwtNy, 130 );
	custom_cb[22].tag = (int)ptr;
	XtSetArg(arglist[3], DwtNactivateCallback, custom_cb+22);
	ptr->custom.b3 = DwtPushButtonCreate(ptr->custom.customize,"",arglist,4);
	XtManageChild(ptr->custom.b3);

	XtManageChild(ptr->custom.rpage);
	XtManageChild(ptr->custom.rbus);

    }

    /* Update with current custom values */
    XtSetArg(arglist[0],DwtNvalue, (curptr->pageinc==4));
    XtSetValues(ptr->custom.tpage1,arglist,1);
    XtSetArg(arglist[0],DwtNvalue, (curptr->pageinc==2));
    XtSetValues(ptr->custom.tpage2,arglist,1);
    XtSetArg(arglist[0],DwtNvalue, (curptr->pageinc==1));
    XtSetValues(ptr->custom.tpage3,arglist,1);

    XtSetArg(arglist[0],DwtNvalue, (curptr->busrep==OBUS));
    XtSetValues(ptr->custom.tbus3,arglist,1);
    XtSetArg(arglist[0],DwtNvalue, (curptr->busrep==HBUS));
    XtSetValues(ptr->custom.tbus4,arglist,1);

    XtSetArg(arglist[0],DwtNvalue, ptr->grid_vis);
    XtSetValues(ptr->custom.grid_state,arglist,1);

    XtSetArg(arglist[0], DwtNvalue, curptr->sighgt);
    XtSetValues(ptr->custom.s1,arglist,1);

    XtSetArg(arglist[0],DwtNvalue, curptr->sigrf);
    XtSetValues(ptr->custom.rfwid,arglist,1);

    XtSetArg(arglist[0],DwtNvalue, ptr->grid_vis);
    XtSetValues(ptr->custom.grid_state,arglist,1);

    XtSetArg(arglist[0],DwtNvalue, ptr->cursor_vis);
    XtSetValues(ptr->custom.cursor_state,arglist,1);

    /* Do it */
    XtManageChild(ptr->custom.customize);
    }


void cus_read_cb(w,ptr,cb)
    Widget			w;
    DISPLAY_SB		*ptr;
    DwtAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("in cus_read_cb ptr=%d\n",ptr);

    /* create popup to get filename */
    /*
      get_data_popup(ptr,"Custom File To Read",IO_READCUSTOM);
      */

    config_read_file (ptr, "DINO$DISK:DINOTRACE.DINO", 0);

    /* Reformat and refresh */
    get_geometry(ptr);
    XClearWindow(ptr->disp, ptr->wind);
    draw(ptr);
    drawsig(ptr);
    }

void cus_reread_cb(w,ptr,cb)
    Widget			w;
    DISPLAY_SB		*ptr;
    DwtAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("in cus_reread_cb ptr=%d\n",ptr);

    config_read_defaults (ptr);

    /* Reformat and refresh */
    get_geometry(ptr);
    XClearWindow(ptr->disp, ptr->wind);
    draw(ptr);
    drawsig(ptr);
    }

void
cus_restore_cb(w,ptr,cb)
Widget			w;
DISPLAY_SB		*ptr;
DwtAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("in cus_restore_cb ptr=%d\n",ptr);

    config_restore_defaults (ptr);

    /* do the default thing */

    /* redraw the display */
    get_geometry(ptr);
    XClearWindow(ptr->disp,ptr->wind);
    draw(ptr);
    drawsig(ptr);
    }

void
cus_save_cb(w,ptr,cb)
Widget			w;
DISPLAY_SB		*ptr;
DwtAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("in cus_save_cb ptr=%d\n",ptr);

    /* create popup to get filename */
    get_data_popup(ptr,"Custom File To Save",IO_SAVECUSTOM);

    return;
}

void
cus_page_cb(w,tag,cb)
Widget		w;
int		tag;
DwtAnyCallbackStruct *cb;
{
    if (DTPRINT) printf("In cus_page_cb - tag=%d\n",tag);

    /* change the value of the page increment */
    curptr->pageinc = tag;
}

void
cus_bus_cb(w,tag,cb)
Widget		w;
int		tag;
DwtRadioBoxCallbackStruct *cb;
{
    if (DTPRINT) printf("In cus_bus_cb - tag=%d\n",tag);

    /* change the value of the bus representation */
    curptr->busrep = tag;
}

void
cus_rf_cb(w,ptr,cb)
Widget			w;
DISPLAY_SB		*ptr;
DwtTogglebuttonCallbackStruct *cb;
{
    if (DTPRINT) printf("In cus_rf_cb - ptr=%d\n",ptr);

    /* determine value of rise/fall parameter */
    if ( cb->value )
	ptr->sigrf = SIG_RF;
    else
	ptr->sigrf = 0;
}

void
cus_grid_cb(w,ptr,cb)
Widget			w;
DISPLAY_SB		*ptr;
DwtTogglebuttonCallbackStruct *cb;
{
    if (DTPRINT) printf("In cus_grid_cb - ptr=%d\n",ptr);

    /* copy value of toggle button to display structure */
    ptr->grid_vis = cb->value;

    return;
}

void
cus_cur_cb(w,ptr,cb)
Widget			w;
DISPLAY_SB		*ptr;
DwtTogglebuttonCallbackStruct *cb;
{
    if (DTPRINT) printf("In cus_cur_cb - ptr=%d\n",ptr);

    /* copy value of toggle button to display structure */
    ptr->cursor_vis = cb->value;

    return;
}

void
cus_sighgt_cb(w,ptr,cb)
Widget		w;
DISPLAY_SB	*ptr;
DwtSelectionCallbackStruct *cb;
{
    if (DTPRINT) printf("In cus_sighgt_cb - ptr=%d\n",ptr);

    /* update sighgt value in the custom_ptr */
    ptr->sighgt =  (int)cb->value;
}

void
cus_ok_cb(w,ptr,cb)
Widget			w;
DISPLAY_SB		*ptr;
DwtAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In cus_ok_cb - ptr=%d\n",ptr);

    /* hide the customize window */
    XtUnmanageChild(ptr->custom.customize);

    /* redraw the display */
    get_geometry(ptr);
    XClearWindow(ptr->disp,ptr->wind);
    draw(ptr);
    drawsig(ptr);

    return;
}

void
cus_apply_cb(w,ptr,cb)
Widget			w;
DISPLAY_SB		*ptr;
DwtAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In cus_apply_cb - ptr=%d\n",ptr);

    /* redraw the display */
    get_geometry(ptr);
    XClearWindow(ptr->disp,ptr->wind);
    draw(ptr);
    drawsig(ptr);

    return;
}

void
cus_cancel_cb(w,ptr,cb)
Widget			w;
DISPLAY_SB		*ptr;
DwtAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In cus_cancel_cb - ptr=%d\n",ptr);

    /* hide the window, don't copy the custom_ptr to the display */
    XtUnmanageChild(ptr->custom.customize);

    return;
}
