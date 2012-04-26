/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/*
 * st-scrollable.c: Scrollable interface
 *
 * Copyright 2008 OpenedHand
 * Copyright 2009 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Written by: Chris Lord <chris@openedhand.com>
 * Port to St by: Robert Staudinger <robsta@openedhand.com>
 *
 */

#include "st-scrollable.h"

static void
st_scrollable_base_init (gpointer g_iface)
{
  static gboolean initialized = FALSE;

  if (!initialized)
    {
      g_object_interface_install_property (g_iface,
                                           g_param_spec_object ("hadjustment",
                                                                "StAdjustment",
                                                                "Horizontal adjustment",
                                                                ST_TYPE_ADJUSTMENT,
                                                                G_PARAM_READWRITE));

      g_object_interface_install_property (g_iface,
                                           g_param_spec_object ("vadjustment",
                                                                "StAdjustment",
                                                                "Vertical adjustment",
                                                                ST_TYPE_ADJUSTMENT,
                                                                G_PARAM_READWRITE));

      initialized = TRUE;
    }
}

GType
st_scrollable_get_type (void)
{
  static GType type = 0;
  if (type == 0)
    {
      static const GTypeInfo info =
      {
        sizeof (StScrollableInterface),
        st_scrollable_base_init,          /* base_init */
        NULL,
      };
      type = g_type_register_static (G_TYPE_INTERFACE,
                                     "StScrollable", &info, 0);
    }
  return type;
}

void
st_scrollable_set_adjustments (StScrollable *scrollable,
                               StAdjustment *hadjustment,
                               StAdjustment *vadjustment)
{
  ST_SCROLLABLE_GET_INTERFACE (scrollable)->set_adjustments (scrollable,
                                                             hadjustment,
                                                             vadjustment);
}

/**
 * st_scroll_bar_get_adjustments:
 * @hadjustment: (transfer none) (out) (allow-none): location to store the horizontal adjustment, or %NULL
 * @vadjustment: (transfer none) (out) (allow-none): location to store the vertical adjustment, or %NULL
 *
 * Gets the adjustment objects that store the offsets of the scrollable widget
 * into its possible scrolling area.
 */
void
st_scrollable_get_adjustments (StScrollable  *scrollable,
                               StAdjustment **hadjustment,
                               StAdjustment **vadjustment)
{
  ST_SCROLLABLE_GET_INTERFACE (scrollable)->get_adjustments (scrollable,
                                                             hadjustment,
                                                             vadjustment);
}
