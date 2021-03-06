/*
 * Copyright 2008 Vincent Sanders <vince@simtec.co.uk>
 * Copyright 2009 Mark Benjamin <netsurf-browser.org.MarkBenjamin@dfgh.net>
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

#ifndef _NETSURF_WINDOWS_GUI_H_
#define _NETSURF_WINDOWS_GUI_H_

struct gui_window;
struct gui_clipboard_table *win32_clipboard_table;

extern HINSTANCE hinst;

/** Directory where all configuration files are held. */
extern char *nsw32_config_home;

/* bounding box */
typedef struct bbox_s {
        int x0;
        int y0;
        int x1;
        int y1;
} bbox_t;

/**
 * Run the win32 message loop with scheduling
 */
void win32_run(void);

/**
 * cause the main message loop to exit
 */
void win32_set_quit(bool q);

/**
 * Warn the user of an event.
 *
 * \param[in] warning A warning looked up in the message translation table
 * \param[in] detail Additional text to be displayed or NULL.
 * \return NSERROR_OK on success or error code if there was a
 *           faliure displaying the message to the user.
 */
nserror win32_warning(const char *warning, const char *detail);

#endif 
