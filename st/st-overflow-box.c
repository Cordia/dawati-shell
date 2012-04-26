/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/*
 * Portions derived from st-box-layout.c, which is
 *   Copyright 2009 Intel Corporation.
 * Modified into -overflow-box, by Colin Walters <walters@verbum.org>, which is
 *   Copyright 2009 Red Hat, Inc.
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
 */

/**
 * SECTION:st-overflow-box
 * @short_description: A vertical box which paints as many actors as it can fit
 *
 * This is a "flexible" box which will paint as many actors as it can within
 * its given allocation; its minimum height request will be the sum of the
 * mimimum size for the #StOverflowBox:min-children property, which is
 * by default 0.
 *
 * Every child will be allocated the full width of the box, and always be
 * given its preferred height.  Even if not actually painted, every child
 * is counted for overall preferred width/height.
 */

#include <stdlib.h>

#include "st-overflow-box.h"

#include "st-private.h"
#include "st-box-layout-child.h"

G_DEFINE_TYPE (StOverflowBox, st_overflow_box, ST_TYPE_CONTAINER)

#define OVERFLOW_BOX_LAYOUT_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), ST_TYPE_OVERFLOW_BOX, StOverflowBoxPrivate))

enum {
  PROP_0,

  PROP_MIN_CHILDREN
};

struct _StOverflowBoxPrivate
{
  guint         min_children;
  guint         n_visible;

  guint         spacing;
};


static void
st_overflow_box_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  StOverflowBoxPrivate *priv = ST_OVERFLOW_BOX (object)->priv;

  switch (property_id)
    {
    case PROP_MIN_CHILDREN:
      g_value_set_uint (value, priv->min_children);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
st_overflow_box_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  StOverflowBox *box = ST_OVERFLOW_BOX (object);

  switch (property_id)
    {
    case PROP_MIN_CHILDREN:
      st_overflow_box_set_min_children (box, g_value_get_uint (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
get_content_preferred_width (StOverflowBox *self,
                             gfloat         for_height,
                             gfloat        *min_width_p,
                             gfloat        *natural_width_p)
{
  StOverflowBoxPrivate *priv = self->priv;
  gint n_children = 0;
  gint n_fixed = 0;
  gfloat min_width, natural_width;
  GList *l, *children;

  min_width = 0;
  natural_width = 0;

  children = st_container_get_children_list (ST_CONTAINER (self));
  for (l = children; l; l = g_list_next (l))
    {
      ClutterActor *child = l->data;
      gfloat child_min = 0, child_nat = 0;

      if (!CLUTTER_ACTOR_IS_VISIBLE (child))
        continue;

      n_children++;

      if (clutter_actor_get_fixed_position_set (child))
        {
          n_fixed++;
          continue;
        }

      clutter_actor_get_preferred_width (child,
                                         -1,
                                         &child_min,
                                         &child_nat);

      min_width = MAX (child_min, min_width);
      natural_width = MAX (child_nat, natural_width);
    }

  if ((n_children - n_fixed) > 1)
    {
      min_width += priv->spacing * (n_children - n_fixed - 1);
      natural_width += priv->spacing * (n_children - n_fixed - 1);
    }

  if (min_width_p)
    *min_width_p = min_width;

  if (natural_width_p)
    *natural_width_p = natural_width;
}

static void
st_overflow_box_get_preferred_width (ClutterActor *actor,
                                     gfloat        for_height,
                                     gfloat       *min_width_p,
                                     gfloat       *natural_width_p)
{
  StThemeNode *theme_node = st_widget_get_theme_node (ST_WIDGET (actor));

  st_theme_node_adjust_for_height (theme_node, &for_height);

  get_content_preferred_width (ST_OVERFLOW_BOX (actor), for_height,
                               min_width_p, natural_width_p);

  st_theme_node_adjust_preferred_width (theme_node,
                                        min_width_p, natural_width_p);
}

static void
get_content_preferred_height (StOverflowBox *self,
                              gfloat         for_width,
                              gfloat        *min_height_p,
                              gfloat        *natural_height_p)
{
  StOverflowBoxPrivate *priv = self->priv;
  gint n_min_children = 0;
  gint n_children = 0;
  gint n_fixed = 0;
  gfloat min_height, natural_height;
  GList *l, *children;

  min_height = 0;
  natural_height = 0;

  children = st_container_get_children_list (ST_CONTAINER (self));
  for (l = children; l; l = g_list_next (l))
    {
      ClutterActor *child = l->data;
      gfloat child_min = 0, child_nat = 0;

      if (!CLUTTER_ACTOR_IS_VISIBLE (child))
        continue;

      n_children++;

      if (clutter_actor_get_fixed_position_set (child))
        {
          n_fixed++;
          continue;
        }

      clutter_actor_get_preferred_height (child,
                                          for_width,
                                          &child_min,
                                          &child_nat);

      if (n_children < priv->min_children)
        {
          n_min_children++;
          min_height += child_min;
        }
      natural_height += child_nat;
    }

  min_height += priv->spacing * MAX(0, n_min_children - 1);
  natural_height += priv->spacing * MAX(0, n_children - n_fixed - 1);

  if (min_height_p)
    *min_height_p = min_height;

  if (natural_height_p)
    *natural_height_p = natural_height;
}

static void
st_overflow_box_get_preferred_height (ClutterActor *actor,
                                      gfloat        for_width,
                                      gfloat       *min_height_p,
                                      gfloat       *natural_height_p)
{
  StThemeNode *theme_node = st_widget_get_theme_node (ST_WIDGET (actor));

  st_theme_node_adjust_for_width (theme_node, &for_width);

  get_content_preferred_height (ST_OVERFLOW_BOX (actor), for_width,
                                min_height_p, natural_height_p);

  st_theme_node_adjust_preferred_height (theme_node,
                                         min_height_p, natural_height_p);
}

static void
st_overflow_box_allocate (ClutterActor          *actor,
                          const ClutterActorBox *box,
                          ClutterAllocationFlags flags)
{
  StOverflowBoxPrivate *priv = ST_OVERFLOW_BOX (actor)->priv;
  StThemeNode *theme_node = st_widget_get_theme_node (ST_WIDGET (actor));
  ClutterActorBox content_box;
  gfloat position;
  float avail_width, avail_height;
  GList *l, *children;
  int i;
  gboolean done_non_fixed;

  CLUTTER_ACTOR_CLASS (st_overflow_box_parent_class)->allocate (actor, box,
                                                                flags);

  children = st_container_get_children_list (ST_CONTAINER (actor));
  if (children == NULL)
    return;

  st_theme_node_get_content_box (theme_node, box, &content_box);

  avail_width = content_box.x2 - content_box.x1;
  avail_height = content_box.y2 - content_box.y1;

  position = content_box.y1;
  priv->n_visible = 0;

  done_non_fixed = FALSE;
  for (l = children, i = 0; l; l = l->next, i++)
    {
      ClutterActor *child = (ClutterActor*) l->data;
      ClutterActorBox child_box;
      gfloat child_min, child_nat;
      gboolean fixed;

      if (!CLUTTER_ACTOR_IS_VISIBLE (child))
        continue;

      fixed = clutter_actor_get_fixed_position_set (child);
      if (fixed)
        {
          clutter_actor_allocate_preferred_size (child, flags);
          continue;
        }
      else if (done_non_fixed)
        continue;

      clutter_actor_get_preferred_height (child, avail_width,
                                          &child_min, &child_nat);

      if (position + child_nat > content_box.y2)
        {
          done_non_fixed = TRUE;  /* Continue iterating on non fixed */
          continue;
        }

      priv->n_visible++;
      child_box.y1 = (int)(0.5 + position);
      child_box.y2 = child_box.y1 + (int)(0.5 + child_nat);
      child_box.x1 = content_box.x1;
      child_box.x2 = content_box.x2;

      position += child_nat + priv->spacing;

      clutter_actor_allocate (child, &child_box, flags);
    }
}

static void
st_overflow_box_internal_paint (StOverflowBox *box)
{
  StOverflowBoxPrivate *priv = box->priv;
  GList *l, *children;
  int i;

  i = 0;
  children = st_container_get_children_list (ST_CONTAINER (box));
  for (l = children; i < priv->n_visible && l; l = l->next)
    {
      ClutterActor *child = (ClutterActor*) l->data;

      if (!CLUTTER_ACTOR_IS_VISIBLE (child))
        continue;
      if (!clutter_actor_get_fixed_position_set (child))
        i++;

      clutter_actor_paint (child);
    }

  for (;l; l = l->next)
    {
      ClutterActor *child = (ClutterActor*) l->data;

      if (!CLUTTER_ACTOR_IS_VISIBLE (child))
        continue;

      if (clutter_actor_get_fixed_position_set (child))
        clutter_actor_paint (child);
    }
}

static void
st_overflow_box_paint (ClutterActor *actor)
{
  CLUTTER_ACTOR_CLASS (st_overflow_box_parent_class)->paint (actor);

  st_overflow_box_internal_paint (ST_OVERFLOW_BOX (actor));
}

static void
st_overflow_box_pick (ClutterActor       *actor,
                      const ClutterColor *color)
{
  CLUTTER_ACTOR_CLASS (st_overflow_box_parent_class)->pick (actor, color);

  st_overflow_box_internal_paint (ST_OVERFLOW_BOX (actor));
}

static void
st_overflow_box_style_changed (StWidget *self)
{
  StOverflowBoxPrivate *priv = ST_OVERFLOW_BOX (self)->priv;
  StThemeNode *theme_node = st_widget_get_theme_node (self);
  int old_spacing = priv->spacing;
  double spacing = 0;

  st_theme_node_get_length (theme_node, "spacing", FALSE, &spacing);
  priv->spacing = (int)(spacing + 0.5);
  if (priv->spacing != old_spacing)
    clutter_actor_queue_relayout (CLUTTER_ACTOR (self));

  ST_WIDGET_CLASS (st_overflow_box_parent_class)->style_changed (self);
}

static void
st_overflow_box_class_init (StOverflowBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  StWidgetClass *widget_class = ST_WIDGET_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (StOverflowBoxPrivate));

  object_class->get_property = st_overflow_box_get_property;
  object_class->set_property = st_overflow_box_set_property;

  actor_class->allocate = st_overflow_box_allocate;
  actor_class->get_preferred_width = st_overflow_box_get_preferred_width;
  actor_class->get_preferred_height = st_overflow_box_get_preferred_height;
  actor_class->paint = st_overflow_box_paint;
  actor_class->pick = st_overflow_box_pick;

  widget_class->style_changed = st_overflow_box_style_changed;

  pspec = g_param_spec_uint ("min-children",
                             "Min Children",
                             "The actor will request a minimum size large enough to include this many children",
                             0, G_MAXUINT, 0,
                             ST_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_MIN_CHILDREN, pspec);
}

static void
st_overflow_box_init (StOverflowBox *self)
{
  self->priv = OVERFLOW_BOX_LAYOUT_PRIVATE (self);
}

/**
 * st_overflow_box_get_min_children:
 * @box: A #StOverflowBox
 *
 * Get the value of the #StOverflowBox::pack-start property.
 *
 * Returns: %TRUE if pack-start is enabled
 */
gboolean
st_overflow_box_get_min_children (StOverflowBox *box)
{
  g_return_val_if_fail (ST_IS_OVERFLOW_BOX (box), FALSE);

  return box->priv->min_children;
}

/**
 * st_box_layout_set_min_children:
 * @box: A #StOverflowBox
 * @min_children: Minimum children value
 *
 * Set the minimum number of children to be visible.
 */
void
st_overflow_box_set_min_children (StOverflowBox *box,
                                  guint          min_children)
{
  g_return_if_fail (ST_IS_OVERFLOW_BOX (box));

  if (box->priv->min_children != min_children)
    {
      box->priv->min_children = min_children;
      clutter_actor_queue_relayout ((ClutterActor*) box);

      g_object_notify (G_OBJECT (box), "min-children");
    }
}

/**
 * st_overflow_box_get_n_children:
 * @self: a #StOverflowBox
 *
 * Returns the number of children in this box.
 */
guint
st_overflow_box_get_n_children  (StOverflowBox *self)
{
  GList *children = st_container_get_children_list (ST_CONTAINER (self));
  return g_list_length (children);
}

/**
 * st_overflow_box_get_n_visible:
 * @self: a #StOverflowBox
 *
 * Returns the number of children we will paint.  Only valid
 * after the actor has been allocated.
 */
guint
st_overflow_box_get_n_visible  (StOverflowBox *self)
{
  return self->priv->n_visible;
}
