/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2009, 2010 Intel Corp.
 *
 * Author: Tomas Frydrych <tf@linux.intel.com>
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

#include <glib/gi18n.h>

#include "mnb-panel-frame.h"

G_DEFINE_TYPE (MnbPanelFrame, mnb_panel_frame, MX_TYPE_WIDGET);

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_PANEL_FRAME, MnbPanelFramePrivate))

enum {
  PROP_0,
};

struct _MnbPanelFramePrivate
{
  gboolean disposed : 1;
};

static void
mnb_panel_frame_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mnb_panel_frame_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mnb_panel_frame_dispose (GObject *object)
{
  MnbPanelFramePrivate *priv = MNB_PANEL_FRAME (object)->priv;

  if (priv->disposed)
    return;

  priv->disposed = TRUE;

  G_OBJECT_CLASS (mnb_panel_frame_parent_class)->dispose (object);
}

static void
mnb_panel_frame_finalize (GObject *object)
{
  G_OBJECT_CLASS (mnb_panel_frame_parent_class)->finalize (object);
}

static void
mnb_panel_frame_get_preferred_width (ClutterActor *self,
                                     gfloat        for_height,
                                     gfloat       *min_width_p,
                                     gfloat       *natural_width_p)
{
  gfloat    min_width, natural_width;
  MxPadding padding = { 0, };

  mx_widget_get_padding (MX_WIDGET (self), &padding);

  min_width = natural_width = padding.left + padding.right;

  if (min_width_p)
    *min_width_p = min_width;

  if (natural_width_p)
    *natural_width_p = natural_width;
}

static void
mnb_panel_frame_get_preferred_height (ClutterActor *self,
                                      gfloat        for_width,
                                      gfloat       *min_height_p,
                                      gfloat       *natural_height_p)
{
  gfloat    min_height, natural_height;
  MxPadding padding = { 0, };

  mx_widget_get_padding (MX_WIDGET (self), &padding);

  min_height = natural_height = padding.top + padding.bottom;

  if (min_height_p)
    *min_height_p = min_height;

  if (natural_height_p)
    *natural_height_p = natural_height;
}

static void
mnb_panel_frame_class_init (MnbPanelFrameClass *klass)
{
  GObjectClass      *object_class  = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbPanelFramePrivate));

  object_class->get_property = mnb_panel_frame_get_property;
  object_class->set_property = mnb_panel_frame_set_property;
  object_class->dispose      = mnb_panel_frame_dispose;
  object_class->finalize     = mnb_panel_frame_finalize;

  actor_class->get_preferred_width  = mnb_panel_frame_get_preferred_width;
  actor_class->get_preferred_height = mnb_panel_frame_get_preferred_height;
}

static void
mnb_panel_frame_init (MnbPanelFrame *self)
{
  self->priv = GET_PRIVATE (self);
}

MnbPanelFrame*
mnb_panel_frame_new (void)
{
  return g_object_new (MNB_TYPE_PANEL_FRAME, NULL);
}

