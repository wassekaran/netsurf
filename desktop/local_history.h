/*
 * Copyright 2017 Vincent Sanders <vince@netsurf-browser.org>
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
 
#ifndef NETSURF_DESKTOP_LOCAL_HISTORY_H
#define NETSURF_DESKTOP_LOCAL_HISTORY_H

#include <stdbool.h>
#include <stdint.h>

#include "utils/errors.h"
#include "netsurf/mouse.h"

struct core_window_callback_table;
struct redraw_context;
struct nsurl;
struct rect;
struct local_history_session;
struct browser_window;

/**
 * Initialise the local history.
 *
 * This iterates through the URL database, generating the local history data,
 * and creates a treeview.
 *
 * This must be called before any other local_history_* function.
 *
 * \param cw_t Callback table for core_window containing the treeview.
 * \param core_window_handle The core_window in which the treeview is shown.
 * \param bw browser window to show history of.
 * \param[out] session The created local history session context.
 * \return NSERROR_OK on success and session set, appropriate error code otherwise
 */
nserror local_history_init(struct core_window_callback_table *cw_t,
			   void *core_window_handle,
			   struct browser_window *bw,
			   struct local_history_session **session);

/**
 * Finalise the local history.
 *
 * This destroys the local history treeview and the local history module's
 * internal data.  After calling this if ocall history is required again,
 * local_history_init must be called.
 *
 * \param session The local history session to finalise.
 * \return NSERROR_OK on success, appropriate error otherwise
 */
nserror local_history_fini(struct local_history_session *session);


/**
 * Redraw the local history.
 *
 * \param session The local history session context.
 * \param x     X coordinate to render history at
 * \param y     Y coordinate to render history at
 * \param clip  Current clip rectangle (wrt tree origin)
 * \param ctx   Current redraw context
 */
nserror local_history_redraw(struct local_history_session *session, int x, int y, struct rect *clip, const struct redraw_context *ctx);

/**
 * Handles all kinds of mouse action
 *
 * \param session The local history session context.
 * \param mouse		The current mouse state
 * \param x		X coordinate
 * \param y		Y coordinate
 */
void local_history_mouse_action(struct local_history_session *session, enum browser_mouse_state mouse, int x, int y);

/**
 * Key press handling.
 *
 * \param session The local history session context.
 * \param key The ucs4 character codepoint
 * \return true if the keypress is dealt with, false otherwise.
 */
bool local_history_keypress(struct local_history_session *session, uint32_t key);

/**
 * Change the browser window to draw local history for.
 *
 * \param session The local history session context.
 * \param bw browser window to show history of.
 * \return NSERROR_OK or appropriate error code.
 */
nserror local_history_set(struct local_history_session *session, struct browser_window *bw);

/**
 * get size of local history content area.
 *
 * \param[in] session The local history session context.
 * \param[out] width on sucessful return the width of the localhistory content
 * \param[out] height on sucessful return the height of the localhistory content
 * \return NSERROR_OK or appropriate error code.
 */
nserror local_history_get_size(struct local_history_session *session, int *width, int *height);

/**
 * get url of entry at position in local history content area.
 *
 * \todo the returned url should be a referenced nsurl.
 *
 * \param[in] session The local history session context.
 * \param[in] x The x coordinate to get url of.
 * \param[in] y The y coordinate to get url of.
 * \param[out] url_out string representation of the url at the coordinates.
 * \return NSERROR_OK and url_out updated or NSERROR_NOT_FOUND if no url at
 *          location.
 */
nserror local_history_get_url(struct local_history_session *session, int x, int y, const char **url_out);

#endif
