/*
 * Copyright 2017 Chris Young <chris@unsatisfactorysoftware.co.uk>
 *
 * This file is part of NetSurf, http://www.netsurf-browser.org/
 *
 * NetSurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * NetSurf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file
 * Amiga core window interface.
 *
 * Provides interface for core renderers to the Amiga Intuition drawable area.
 *
 * This module is an object that must be encapsulated. Client users
 * should embed a struct ami_corewindow at the beginning of their
 * context for this display surface, fill in relevant data and then
 * call ami_corewindow_init()
 *
 * The Amiga core window structure requires the callback for draw, key and
 * mouse operations.
 */

#include "amiga/os3support.h"

#include <assert.h>
#include <string.h>
#include <math.h>

#include "utils/log.h"
#include "utils/utils.h"
#include "utils/messages.h"
#include "utils/utf8.h"
#include "netsurf/keypress.h"
#include "netsurf/mouse.h"
#include "desktop/plot_style.h"

#include <proto/intuition.h>
#include <proto/utility.h>

#include <classes/window.h>
#include <gadgets/scroller.h>
#include <intuition/icclass.h>

#include "amiga/corewindow.h"
#include "amiga/misc.h"
#include "amiga/object.h"

static void
ami_cw_close(void *w)
{
	struct ami_corewindow *ami_cw = (struct ami_corewindow *)w;

	ami_cw->close(ami_cw);
}

HOOKF(void, ami_cw_idcmp_hook, Object *, object, struct IntuiMessage *) 
{
	ULONG gid;
	struct ami_corewindow *ami_cw = hook->h_Data;
	struct IntuiWheelData *wheel;

	switch(msg->Class)
	{
		case IDCMP_IDCMPUPDATE:
			gid = GetTagData( GA_ID, 0, msg->IAddress ); 

			switch( gid ) 
			{ 
 				case GID_CW_HSCROLL: 
 				case GID_CW_VSCROLL:
					/* redraw */
 				break; 
			} 
		break;
#ifdef __amigaos4__
		case IDCMP_EXTENDEDMOUSE:
			if(msg->Code == IMSGCODE_INTUIWHEELDATA)
			{
				wheel = (struct IntuiWheelData *)msg->IAddress;

				//ami_tree_scroll(twin, (wheel->WheelX * 20), (wheel->WheelY * 20));
			}
		break;
#endif
	}
} 

/**
 * Main event loop for our core window
 */
static BOOL
ami_cw_event(void *w)
{
	struct ami_corewindow *ami_cw = (struct ami_corewindow *)w;
//event loop goes here
	return FALSE;
}

static const struct ami_win_event_table ami_cw_table = {
	ami_cw_event,
	ami_cw_close,
};

/**
 * callback from core to request a redraw
 */
static void
ami_cw_redraw_request(struct core_window *cw, const struct rect *r)
{
	struct ami_corewindow *ami_cw = (struct ami_corewindow *)cw;

/*
        toolkit_widget_queue_draw_area(example_cw->widget,
                                       r->x0, r->y0,
                                       r->x1 - r->x0, r->y1 - r->y0);
*/
}


static void
ami_cw_get_window_dimensions(struct core_window *cw, int *width, int *height)
{
	struct ami_corewindow *ami_cw = (struct ami_corewindow *)cw;
	struct IBox *bbox;

	if(ami_gui_get_space_box((Object *)ami_cw->objects[GID_CW_DRAW], &bbox) != NSERROR_OK) {
		amiga_warn_user("NoMemory", "");
		return;
	}

	*width = bbox->Width;
	*height = bbox->Height;

	ami_gui_free_space_box(bbox);
}


static void
ami_cw_update_size(struct core_window *cw, int width, int height)
{
	struct ami_corewindow *ami_cw = (struct ami_corewindow *)cw;

	/* I'm assuming this is telling me the new page size, not wanting the window physically resized */
	int win_w, win_h;
	ami_cw_get_window_dimensions((struct core_window *)ami_cw, &win_w, &win_h);

	if(ami_cw->objects[GID_CW_VSCROLL]) {
		RefreshSetGadgetAttrs((struct Gadget *)ami_cw->objects[GID_CW_VSCROLL], ami_cw->win, NULL,
			SCROLLER_Total, (ULONG)height,
			SCROLLER_Visible, win_h,
		TAG_DONE);
	}

	if(ami_cw->objects[GID_CW_HSCROLL]) {
		RefreshSetGadgetAttrs((struct Gadget *)ami_cw->objects[GID_CW_HSCROLL], ami_cw->win, NULL,
			SCROLLER_Total, (ULONG)width,
			SCROLLER_Visible, win_w,
		TAG_DONE);
	}
}


static void
ami_cw_scroll_visible(struct core_window *cw, const struct rect *r)
{
	struct ami_corewindow *ami_cw = (struct ami_corewindow *)cw;

	int scrollsetx;
	int scrollsety;
	int win_w, win_h;
	int win_x0, win_x1;
	int win_y0, win_y1;

	ami_cw_get_window_dimensions((struct core_window *)ami_cw, &win_w, &win_h);

	GetAttr(SCROLLER_Top, ami_cw->objects[GID_CW_VSCROLL], (ULONG *)&win_y0);
	GetAttr(SCROLLER_Top, ami_cw->objects[GID_CW_HSCROLL], (ULONG *)&win_x0);

	win_x1 = win_x0 + win_w;
	win_y1 = win_y0 + win_h;

	if(r->y1 > win_y1) scrollsety = r->y1 - win_h;
	if(r->y0 < win_y0) scrollsety = r->y0;
	if(r->x1 > win_x1) scrollsetx = r->x1 - win_w;
	if(r->x0 < win_x0) scrollsetx = r->x0;

	RefreshSetGadgetAttrs((APTR)ami_cw->objects[GID_CW_VSCROLL], ami_cw->win, NULL,
			SCROLLER_Top, scrollsety,
			TAG_DONE);

	RefreshSetGadgetAttrs((APTR)ami_cw->objects[GID_CW_HSCROLL], ami_cw->win, NULL,
			SCROLLER_Top, scrollsetx,
			TAG_DONE);

	/* probably need to redraw here */
}


static void
ami_cw_drag_status(struct core_window *cw, core_window_drag_status ds)
{
	struct ami_corewindow *ami_cw = (struct ami_corewindow *)cw;
	ami_cw->drag_status = ds;
}


struct core_window_callback_table ami_cw_cb_table = {
        .redraw_request = ami_cw_redraw_request,
        .update_size = ami_cw_update_size,
        .scroll_visible = ami_cw_scroll_visible,
        .get_window_dimensions = ami_cw_get_window_dimensions,
        .drag_status = ami_cw_drag_status
};

/* exported function documented example/corewindow.h */
nserror ami_corewindow_init(struct ami_corewindow *ami_cw)
{
	/* setup the core window callback table */
	ami_cw->cb_table = &ami_cw_cb_table;

	/* allocate drawing area etc */
	ami_init_layers(&ami_cw->gg, 0, 0, false);
	ami_cw->gg.shared_pens = ami_AllocMinList();

	/* add the core window to our window list so we process events */
	ami_gui_win_list_add(ami_cw, AMINS_COREWINDOW, &ami_cw_table);

	/* attach the scrollbars for event processing */
	ami_cw->idcmp_hook.h_Entry = (void *)ami_cw_idcmp_hook;
	ami_cw->idcmp_hook.h_Data = ami_cw;
	/* probably set this when defining the window
	SetAttrs(ami_cw->objects[GID_CW_WIN],
		WINDOW_IDCMPHook, &ami_cw->idcmp_hook,
		TAG_DONE); */

	GetAttr(WINDOW_HorizObject, ami_cw->objects[GID_CW_WIN],
				(ULONG *)&ami_cw->objects[GID_CW_HSCROLL]);
	GetAttr(WINDOW_VertObject, ami_cw->objects[GID_CW_WIN],
				(ULONG *)&ami_cw->objects[GID_CW_VSCROLL]);

	RefreshSetGadgetAttrs((APTR)ami_cw->objects[GID_CW_VSCROLL], ami_cw->win, NULL,
		GA_ID, GID_CW_VSCROLL,
		ICA_TARGET, ICTARGET_IDCMP,
		TAG_DONE);

	RefreshSetGadgetAttrs((APTR)ami_cw->objects[GID_CW_HSCROLL], ami_cw->win, NULL,
		GA_ID, GID_CW_HSCROLL,
		ICA_TARGET, ICTARGET_IDCMP,
		TAG_DONE);

	return NSERROR_OK;
}

/* exported interface documented in example/corewindow.h */
nserror ami_corewindow_fini(struct ami_corewindow *ami_cw)
{
	/* remove the core window from our window list */
	ami_gui_win_list_remove(ami_cw);

	/* destroy the window */
	ami_cw->win = NULL;
	DisposeObject(ami_cw->objects[GID_CW_WIN]);

	/* release off-screen bitmap stuff */
	ami_plot_release_pens(ami_cw->gg.shared_pens);
	ami_free_layers(&ami_cw->gg);

	return NSERROR_OK;
}

