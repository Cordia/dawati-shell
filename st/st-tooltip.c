/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/*
 * st-tooltip.c: Plain tooltip actor
 *
 * Copyright 2008, 2009 Intel Corporation
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
 * Boston, MA 02111-1307, USA.
 *
 * Written by: Thomas Wood <thomas@linux.intel.com>
 *
 */

/**
 * SECTION:st-tooltip
 * @short_description: A tooltip widget
 *
 * #StTooltip implements a single tooltip. It should not normally be created
 * by the application but by the widget implementing tooltip capabilities, for
 * example, #st_button_set_tooltip().
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <clutter/clutter.h>

#include "st-tooltip.h"

#include "st-widget.h"
#include "st-private.h"

enum
{
  PROP_0,

  PROP_LABEL,
  PROP_TIP_AREA
};

#define ST_TOOLTIP_GET_PRIVATE(obj)    \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), ST_TYPE_TOOLTIP, StTooltipPrivate))

struct _StTooltipPrivate
{
  ClutterActor    *label;

  gfloat           arrow_offset;
  gboolean         actor_below;

  ClutterGeometry *tip_area;
};

extern gfloat st_slow_down_factor;

G_DEFINE_TYPE (StTooltip, st_tooltip, ST_TYPE_WIDGET);

static void
st_tooltip_set_property (GObject      *gobject,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  StTooltip *tooltip = ST_TOOLTIP (gobject);

  switch (prop_id)
    {
    case PROP_LABEL:
      st_tooltip_set_label (tooltip, g_value_get_string (value));
      break;

    case PROP_TIP_AREA:
      st_tooltip_set_tip_area (tooltip, g_value_get_boxed (value));

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
st_tooltip_get_property (GObject    *gobject,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  StTooltipPrivate *priv = ST_TOOLTIP (gobject)->priv;

  switch (prop_id)
    {
    case PROP_LABEL:
      g_value_set_string (value, clutter_text_get_text (CLUTTER_TEXT (priv->label)));
      break;

    case PROP_TIP_AREA:
      g_value_set_boxed (value, priv->tip_area);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
st_tooltip_style_changed (StWidget *self)
{
  StTooltipPrivate *priv;
  StThemeNode *theme_node;
  ClutterColor color;
  const PangoFontDescription *font;
  gchar *font_string;

  priv = ST_TOOLTIP (self)->priv;
  theme_node = st_widget_get_theme_node (self);

  st_theme_node_get_foreground_color (theme_node, &color);
  clutter_text_set_color (CLUTTER_TEXT (priv->label), &color);

  font = st_theme_node_get_font (theme_node);
  font_string = pango_font_description_to_string (font);
  clutter_text_set_font_name (CLUTTER_TEXT (priv->label), font_string);
  g_free (font_string);

  ST_WIDGET_CLASS (st_tooltip_parent_class)->style_changed (self);
}

static void
st_tooltip_get_preferred_width (ClutterActor *self,
                                gfloat        for_height,
                                gfloat       *min_width_p,
                                gfloat       *natural_width_p)
{
  StTooltipPrivate *priv = ST_TOOLTIP (self)->priv;
  StThemeNode *theme_node = st_widget_get_theme_node (ST_WIDGET (self));
  gfloat label_height;

  st_theme_node_adjust_for_height (theme_node, &for_height);

  if (for_height > -1)
    {
      label_height = for_height;
    }
  else
    {
      label_height = -1;
    }

  if (priv->label)
    {
      clutter_actor_get_preferred_width (priv->label,
                                         label_height,
                                         min_width_p,
                                         natural_width_p);
    }

  st_theme_node_adjust_preferred_width (theme_node, min_width_p, natural_width_p);
}

static void
st_tooltip_get_preferred_height (ClutterActor *self,
                                 gfloat        for_width,
                                 gfloat       *min_height_p,
                                 gfloat       *natural_height_p)
{
  StTooltipPrivate *priv = ST_TOOLTIP (self)->priv;
  StThemeNode *theme_node = st_widget_get_theme_node (ST_WIDGET (self));
  gfloat min_label_h, natural_label_h;

  st_theme_node_adjust_for_width (theme_node, &for_width);

  if (priv->label)
    {
      clutter_actor_get_preferred_height (priv->label,
                                          for_width,
                                          &min_label_h,
                                          &natural_label_h);
    }
  else
    {
      min_label_h = 0;
      natural_label_h = 0;
    }

  if (min_height_p)
    *min_height_p = min_label_h;

  if (natural_height_p)
    *natural_height_p = natural_label_h;

  st_theme_node_adjust_preferred_height (theme_node, min_height_p, natural_height_p);
}

static void
st_tooltip_allocate (ClutterActor          *self,
                     const ClutterActorBox *box,
                     ClutterAllocationFlags flags)
{
  StTooltipPrivate *priv = ST_TOOLTIP (self)->priv;
  StThemeNode *theme_node = st_widget_get_theme_node (ST_WIDGET (self));
  ClutterActorBox content_box, child_box;

  CLUTTER_ACTOR_CLASS (st_tooltip_parent_class)->allocate (self,
                                                           box,
                                                           flags);

  st_theme_node_get_content_box (theme_node, box, &content_box);

  child_box.x1 = child_box.y1 = 0;
  child_box.x2 = (box->x2 - box->x1);
  child_box.y2 = (box->y2 - box->y1);

  if (priv->label)
    {
      child_box = content_box;

      clutter_actor_allocate (priv->label, &child_box, flags);
    }
}

static void
st_tooltip_paint (ClutterActor *self)
{
  StTooltipPrivate *priv = ST_TOOLTIP (self)->priv;

  clutter_actor_paint (priv->label);
}

static void
st_tooltip_map (ClutterActor *self)
{
  StTooltipPrivate *priv = ST_TOOLTIP (self)->priv;

  CLUTTER_ACTOR_CLASS (st_tooltip_parent_class)->map (self);

  clutter_actor_map (priv->label);
}

static void
st_tooltip_unmap (ClutterActor *self)
{
  StTooltipPrivate *priv = ST_TOOLTIP (self)->priv;

  CLUTTER_ACTOR_CLASS (st_tooltip_parent_class)->unmap (self);

  clutter_actor_unmap (priv->label);
}

static void
st_tooltip_dispose (GObject *self)
{
  StTooltipPrivate *priv = ST_TOOLTIP (self)->priv;

  if (priv->label)
    {
      clutter_actor_destroy (priv->label);
      priv->label = NULL;
    }

  G_OBJECT_CLASS (st_tooltip_parent_class)->dispose (self);
}

static void
st_tooltip_class_init (StTooltipClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  StWidgetClass *widget_class = ST_WIDGET_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (StTooltipPrivate));

  gobject_class->set_property = st_tooltip_set_property;
  gobject_class->get_property = st_tooltip_get_property;
  gobject_class->dispose = st_tooltip_dispose;

  actor_class->get_preferred_width = st_tooltip_get_preferred_width;
  actor_class->get_preferred_height = st_tooltip_get_preferred_height;
  actor_class->allocate = st_tooltip_allocate;
  actor_class->paint = st_tooltip_paint;
  actor_class->map = st_tooltip_map;
  actor_class->unmap = st_tooltip_unmap;

  widget_class->style_changed = st_tooltip_style_changed;

  pspec = g_param_spec_string ("label",
                               "Label",
                               "Label of the tooltip",
                               NULL, G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_LABEL, pspec);

  pspec = g_param_spec_boxed ("tip-area",
                              "Tip Area",
                              "Area on the stage the tooltip applies to",
                              CLUTTER_TYPE_GEOMETRY,
                              ST_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_TIP_AREA, pspec);
}

static void
st_tooltip_init (StTooltip *tooltip)
{
  tooltip->priv = ST_TOOLTIP_GET_PRIVATE (tooltip);

  tooltip->priv->label = g_object_new (CLUTTER_TYPE_TEXT,
                                       "line-alignment", PANGO_ALIGN_CENTER,
                                       "ellipsize", PANGO_ELLIPSIZE_END,
                                       "use-markup", TRUE,
                                       NULL);

  tooltip->priv->tip_area = NULL;

  clutter_actor_set_parent (CLUTTER_ACTOR (tooltip->priv->label),
                            CLUTTER_ACTOR (tooltip));

  g_object_set (tooltip, "show-on-set-parent", FALSE, NULL);

  clutter_actor_set_reactive (CLUTTER_ACTOR (tooltip), FALSE);
}

static void
st_tooltip_update_position (StTooltip *tooltip)
{
  StTooltipPrivate *priv = tooltip->priv;
  ClutterGeometry *tip_area = tooltip->priv->tip_area;
  gfloat tooltip_w, tooltip_h, tooltip_x, tooltip_y;
  gfloat stage_w, stage_h;
  ClutterActor *stage;

  /* ensure the tooltip with is not fixed size */
  clutter_actor_set_size ((ClutterActor*) tooltip, -1, -1);

  /* if no area set, just position ourselves top left */
  if (!priv->tip_area)
    {
      clutter_actor_set_position ((ClutterActor*) tooltip, 0, 0);
      return;
    }

  /* we need to have a style in case there are padding/border values to take into
   * account when calculating width/height */
  st_widget_ensure_style ((StWidget *) tooltip);

  /* find out the tooltip's size */
  clutter_actor_get_size ((ClutterActor*) tooltip, &tooltip_w, &tooltip_h);

  /* attempt to place the tooltip */
  tooltip_x = (int)(tip_area->x + (tip_area->width / 2) - (tooltip_w / 2));
  tooltip_y = (int)(tip_area->y + tip_area->height);

  stage = clutter_actor_get_stage ((ClutterActor *) tooltip);
  if (!stage)
    {
      return;
    }
  clutter_actor_get_size (stage, &stage_w, &stage_h);

  /* make sure the tooltip is not off screen vertically */
  if (tooltip_w > stage_w)
    {
      tooltip_x = 0;
      clutter_actor_set_width ((ClutterActor*) tooltip, stage_w);
    }
  else if (tooltip_x < 0)
    {
      tooltip_x = 0;
    }
  else if (tooltip_x + tooltip_w > stage_w)
    {
      tooltip_x = (int)(stage_w) - tooltip_w;
    }

  /* make sure the tooltip is not off screen horizontally */
  if (tooltip_y + tooltip_h > stage_h)
    {
      priv->actor_below = TRUE;

      /* re-query size as may have changed */
      clutter_actor_get_preferred_height ((ClutterActor*) tooltip,
                                          -1, NULL, &tooltip_h);
      tooltip_y = tip_area->y - tooltip_h;
    }
  else
    {
      priv->actor_below = FALSE;
    }

  /* calculate the arrow offset */
  priv->arrow_offset = tip_area->x + tip_area->width / 2 - tooltip_x;

  clutter_actor_set_position ((ClutterActor*) tooltip, tooltip_x, tooltip_y);
}

/**
 * st_tooltip_get_label:
 * @tooltip: a #StTooltip
 *
 * Get the text displayed on the tooltip
 *
 * Returns: the text for the tooltip. This must not be freed by the application
 */
G_CONST_RETURN gchar *
st_tooltip_get_label (StTooltip *tooltip)
{
  g_return_val_if_fail (ST_IS_TOOLTIP (tooltip), NULL);

  return clutter_text_get_text (CLUTTER_TEXT (tooltip->priv->label));
}

/**
 * st_tooltip_set_label:
 * @tooltip: a #StTooltip
 * @text: text to set the label to
 *
 * Sets the text displayed on the tooltip
 */
void
st_tooltip_set_label (StTooltip   *tooltip,
                      const gchar *text)
{
  StTooltipPrivate *priv;

  g_return_if_fail (ST_IS_TOOLTIP (tooltip));

  priv = tooltip->priv;

  clutter_text_set_text (CLUTTER_TEXT (priv->label), text);

  g_object_notify (G_OBJECT (tooltip), "label");
}

/**
 * st_tooltip_show:
 * @tooltip: a #StTooltip
 *
 * Show the tooltip relative to the associated widget.
 */
void
st_tooltip_show (StTooltip *tooltip)
{
  StTooltipPrivate *priv;
  ClutterActor *parent;
  ClutterActor *stage;
  ClutterActor *self = CLUTTER_ACTOR (tooltip);
  ClutterAnimation *animation;

  /* make sure we're not currently already animating (e.g. hiding) */
  animation = clutter_actor_get_animation (CLUTTER_ACTOR (tooltip));
  if (animation)
    clutter_animation_completed (animation);

  priv = tooltip->priv;
  parent = clutter_actor_get_parent (self);
  stage = clutter_actor_get_stage (self);

  if (!stage)
    {
      g_warning ("StTooltip is not on any stage.");
      return;
    }

  /* make sure we're parented on the stage */
  if (G_UNLIKELY (parent != stage))
    {
      g_object_ref (self);
      clutter_actor_unparent (self);
      clutter_actor_set_parent (self, stage);
      g_object_unref (self);
      parent = stage;
    }

  /* raise the tooltip to the top */
  clutter_container_raise_child (CLUTTER_CONTAINER (stage),
                                 CLUTTER_ACTOR (tooltip),
                                 NULL);

  st_tooltip_update_position (tooltip);

  /* finally show the tooltip... */
  CLUTTER_ACTOR_CLASS (st_tooltip_parent_class)->show (self);

  /* and give it some bounce! */
  g_object_set (G_OBJECT (self),
                "scale-center-x", priv->arrow_offset,
                "scale-center-y", (priv->actor_below) ? clutter_actor_get_height (self) : 0,
                NULL);
  clutter_actor_set_scale (self, 0.0, 0.0);
  clutter_actor_animate (self, CLUTTER_EASE_OUT_ELASTIC,
                         500,
                         "scale-x", 1.0,
                         "scale-y", 1.0,
                         NULL);
}

static void
st_tooltip_hide_complete (ClutterAnimation *animation,
                          ClutterActor     *actor)
{
  CLUTTER_ACTOR_CLASS (st_tooltip_parent_class)->hide (actor);
  g_signal_handlers_disconnect_by_func (actor,
                                        st_tooltip_hide_complete,
                                        actor);
}

/**
 * st_tooltip_hide:
 * @tooltip: a #StTooltip
 *
 * Hide the tooltip
 */
void
st_tooltip_hide (StTooltip *tooltip)
{
  ClutterAnimation *animation;

  g_return_if_fail (ST_TOOLTIP (tooltip));

  /* make sure we're not currently already animating (e.g. hiding) */
  animation = clutter_actor_get_animation (CLUTTER_ACTOR (tooltip));
  if (animation)
    clutter_animation_completed (animation);

  g_object_set (G_OBJECT (tooltip),
                "scale-center-x", tooltip->priv->arrow_offset,
                NULL);
  animation =
    clutter_actor_animate (CLUTTER_ACTOR (tooltip), CLUTTER_EASE_IN_SINE,
                           (guint)(150 * st_slow_down_factor),
                           "scale-x", 0.0,
                           "scale-y", 0.0,
                           NULL);
  g_signal_connect (animation, "completed",
                    G_CALLBACK (st_tooltip_hide_complete), tooltip);
}

/**
 * st_tooltip_set_tip_area:
 * @tooltip: A #StTooltip
 * @area: A #ClutterGeometry
 *
 * Set the area on the stage that the tooltip applies to.
 */
void
st_tooltip_set_tip_area (StTooltip             *tooltip,
                         const ClutterGeometry *area)
{
  g_return_if_fail (ST_IS_TOOLTIP (tooltip));

  if (tooltip->priv->tip_area)
    g_boxed_free (CLUTTER_TYPE_GEOMETRY, tooltip->priv->tip_area);
  tooltip->priv->tip_area = g_boxed_copy (CLUTTER_TYPE_GEOMETRY, area);

  st_tooltip_update_position (tooltip);
}

/**
 * st_tooltip_get_tip_area:
 * @tooltip: A #StTooltip
 *
 * Retrieve the area on the stage that the tooltip currently applies to
 *
 * Returns: the #ClutterGeometry, owned by the tooltip which must not be freed
 * by the application.
 */
G_CONST_RETURN ClutterGeometry*
st_tooltip_get_tip_area (StTooltip *tooltip)
{
  g_return_val_if_fail (ST_IS_TOOLTIP (tooltip), NULL);

  return tooltip->priv->tip_area;
}
