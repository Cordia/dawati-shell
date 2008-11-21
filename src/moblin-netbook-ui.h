/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2008 Intel Corp.
 *
 * Author: Tomas Frydrych <tf@linux.intel.com>
 *         Thomas Wood <thomas@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef MOBLIN_NETBOOK_UI_H
#define MOBLIN_NETBOOK_UI_H

#include <nbtk/nbtk.h>
#include "nutter/nutter-ws-icon.h"
#include "nutter/nutter-scale-group.h"

#define APP_SWITCHER_CELL_WIDTH  200
#define APP_SWITCHER_CELL_HEIGHT 200

#define WORKSPACE_CELL_WIDTH  100
#define WORKSPACE_CELL_HEIGHT 80

#define WORKSPACE_BORDER 20

#define WORKSPACE_CHOOSER_TIMEOUT 3000
#define WORKSPACE_CHOOSER_BORDER_WIDTH 1

#define PANEL_HEIGHT 40

#define MAX_WORKSPACES 8

ClutterActor *make_panel (gint width);
void          hide_panel (void);

void          switcher_clone_weak_notify (gpointer data, GObject *object);
void          switcher_origin_weak_notify (gpointer data, GObject *object);

ClutterActor *ensure_nth_workspace (GList **list, gint n, gint active);

#endif
