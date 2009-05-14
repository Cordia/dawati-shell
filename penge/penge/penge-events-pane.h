/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef _PENGE_EVENTS_PANE
#define _PENGE_EVENTS_PANE

#include <glib-object.h>
#include <nbtk/nbtk.h>

G_BEGIN_DECLS

#define PENGE_TYPE_EVENTS_PANE penge_events_pane_get_type()

#define PENGE_EVENTS_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_EVENTS_PANE, PengeEventsPane))

#define PENGE_EVENTS_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_EVENTS_PANE, PengeEventsPaneClass))

#define PENGE_IS_EVENTS_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_EVENTS_PANE))

#define PENGE_IS_EVENTS_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_EVENTS_PANE))

#define PENGE_EVENTS_PANE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_EVENTS_PANE, PengeEventsPaneClass))

typedef struct {
  NbtkTable parent;
} PengeEventsPane;

typedef struct {
  NbtkTableClass parent_class;
} PengeEventsPaneClass;

GType penge_events_pane_get_type (void);

G_END_DECLS

#endif /* _PENGE_EVENTS_PANE */

