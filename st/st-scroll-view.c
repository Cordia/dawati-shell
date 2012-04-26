/*
 * st-scroll-view.h: Container with scroll-bars
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

/**
 * SECTION:st-scroll-view
 * @short_description: a container for scrollable children
 *
 * #StScrollView is a single child container for actors that implement
 * #StScrollable. It provides scrollbars around the edge of the child to
 * allow the user to move around the scrollable area.
 */

#include "st-scroll-view.h"
#include "st-marshal.h"
#include "st-scroll-bar.h"
#include "st-scrollable.h"
#include "st-stylable.h"
#include <clutter/clutter.h>

static void clutter_container_iface_init (ClutterContainerIface *iface);
static void st_stylable_iface_init (StStylableIface *iface);

static ClutterContainerIface *st_scroll_view_parent_iface = NULL;

G_DEFINE_TYPE_WITH_CODE (StScrollView, st_scroll_view, ST_TYPE_BIN,
                         G_IMPLEMENT_INTERFACE (CLUTTER_TYPE_CONTAINER,
                                                clutter_container_iface_init)
                         G_IMPLEMENT_INTERFACE (ST_TYPE_STYLABLE,
                                                st_stylable_iface_init))

#define SCROLL_VIEW_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
                                                             ST_TYPE_SCROLL_VIEW, \
                                                             StScrollViewPrivate))

struct _StScrollViewPrivate
{
  /* a pointer to the child; this is actually stored
   * inside StBin:child, but we keep it to avoid
   * calling st_bin_get_child() every time we need it
   */
  ClutterActor *child;

  ClutterActor *hscroll;
  ClutterActor *vscroll;

  gfloat        row_size;
  gfloat        column_size;

  gboolean      row_size_set : 1;
  gboolean      column_size_set : 1;
  guint         mouse_scroll : 1;
};

enum {
  PROP_0,

  PROP_HSCROLL,
  PROP_VSCROLL,
  PROP_MOUSE_SCROLL
};

static void
st_scroll_view_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  StScrollViewPrivate *priv = ((StScrollView *) object)->priv;

  switch (property_id)
    {
    case PROP_HSCROLL:
      g_value_set_object (value, priv->hscroll);
      break;
    case PROP_VSCROLL:
      g_value_set_object (value, priv->vscroll);
      break;
    case PROP_MOUSE_SCROLL:
      g_value_set_boolean (value, priv->mouse_scroll);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
st_scroll_view_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  switch (property_id)
    {
    case PROP_MOUSE_SCROLL:
      st_scroll_view_set_mouse_scrolling ((StScrollView *) object,
                                          g_value_get_boolean (value));
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
st_scroll_view_dispose (GObject *object)
{
  StScrollViewPrivate *priv = ST_SCROLL_VIEW (object)->priv;

  priv->child = NULL;

  if (priv->vscroll)
    {
      clutter_actor_unparent (priv->vscroll);
      priv->vscroll = NULL;
    }

  if (priv->hscroll)
    {
      clutter_actor_unparent (priv->hscroll);
      priv->hscroll = NULL;
    }

  G_OBJECT_CLASS (st_scroll_view_parent_class)->dispose (object);
}

static void
st_scroll_view_finalize (GObject *object)
{
  G_OBJECT_CLASS (st_scroll_view_parent_class)->finalize (object);
}

static void
st_scroll_view_paint (ClutterActor *actor)
{
  StScrollViewPrivate *priv = ST_SCROLL_VIEW (actor)->priv;

  /* StBin will paint the child */
  CLUTTER_ACTOR_CLASS (st_scroll_view_parent_class)->paint (actor);

  /* paint our custom children */
  if (CLUTTER_ACTOR_IS_VISIBLE (priv->hscroll))
    clutter_actor_paint (priv->hscroll);
  if (CLUTTER_ACTOR_IS_VISIBLE (priv->vscroll))
    clutter_actor_paint (priv->vscroll);
}

static void
st_scroll_view_pick (ClutterActor       *actor,
                     const ClutterColor *color)
{
  StScrollViewPrivate *priv = ST_SCROLL_VIEW (actor)->priv;

  /* Chain up so we get a bounding box pained (if we are reactive) */
  CLUTTER_ACTOR_CLASS (st_scroll_view_parent_class)->pick (actor, color);

  /* paint our custom children */
  if (CLUTTER_ACTOR_IS_VISIBLE (priv->hscroll))
    clutter_actor_paint (priv->hscroll);
  if (CLUTTER_ACTOR_IS_VISIBLE (priv->vscroll))
    clutter_actor_paint (priv->vscroll);
}

static void
st_scroll_view_get_preferred_width (ClutterActor *actor,
                                    gfloat        for_height,
                                    gfloat       *min_width_p,
                                    gfloat       *natural_width_p)
{
  StPadding padding;
  guint xthickness;

  StScrollViewPrivate *priv = ST_SCROLL_VIEW (actor)->priv;

  if (!priv->child)
    return;

  st_widget_get_padding (ST_WIDGET (actor), &padding);
  st_stylable_get (ST_STYLABLE (actor),
                   "scrollbar-width", &xthickness,
                   NULL);

  /* Our natural width is the natural width of the child */
  clutter_actor_get_preferred_width (priv->child,
                                     for_height,
                                     NULL,
                                     natural_width_p);

  /* Add space for the scroll-bar if we can determine it will be necessary */
  if ((for_height >= 0) && natural_width_p)
    {
      gfloat natural_height;

      clutter_actor_get_preferred_height (priv->child, -1.0,
                                          NULL,
                                          &natural_height);
      if (for_height < natural_height)
        *natural_width_p += xthickness;
    }

  /* Add space for padding */
  if (min_width_p)
    *min_width_p = padding.left + padding.right;

  if (natural_width_p)
    *natural_width_p += padding.left + padding.right;
}

static void
st_scroll_view_get_preferred_height (ClutterActor *actor,
                                     gfloat        for_width,
                                     gfloat       *min_height_p,
                                     gfloat       *natural_height_p)
{
  StPadding padding;
  guint ythickness;

  StScrollViewPrivate *priv = ST_SCROLL_VIEW (actor)->priv;

  if (!priv->child)
    return;

  st_widget_get_padding (ST_WIDGET (actor), &padding);
  st_stylable_get (ST_STYLABLE (actor),
                   "scrollbar-height", &ythickness,
                   NULL);

  /* Our natural height is the natural height of the child */
  clutter_actor_get_preferred_height (priv->child,
                                      for_width,
                                      NULL,
                                      natural_height_p);

  /* Add space for the scroll-bar if we can determine it will be necessary */
  if ((for_width >= 0) && natural_height_p)
    {
      gfloat natural_width;

      clutter_actor_get_preferred_width (priv->child, -1.0,
                                         NULL,
                                         &natural_width);
      if (for_width < natural_width)
        *natural_height_p += ythickness;
    }

  /* Add space for padding */
  if (min_height_p)
    *min_height_p = padding.top + padding.bottom;

  if (natural_height_p)
    *natural_height_p += padding.top + padding.bottom;
}

static void
st_scroll_view_allocate (ClutterActor          *actor,
                         const ClutterActorBox *box,
                         ClutterAllocationFlags flags)
{
  StPadding padding;
  ClutterActorBox child_box;
  ClutterActorClass *parent_parent_class;
  gfloat avail_width, avail_height, sb_width, sb_height;

  StScrollViewPrivate *priv = ST_SCROLL_VIEW (actor)->priv;

  /* Chain up to the parent's parent class
   *
   * We do this because we do not want StBin to allocate the child, as we
   * give it a different allocation later, depending on whether the scrollbars
   * are visible
   */
  parent_parent_class
    = g_type_class_peek_parent (st_scroll_view_parent_class);

  CLUTTER_ACTOR_CLASS (parent_parent_class)->
  allocate (actor, box, flags);


  st_widget_get_padding (ST_WIDGET (actor), &padding);

  avail_width = (box->x2 - box->x1) - padding.left - padding.right;
  avail_height = (box->y2 - box->y1) - padding.top - padding.bottom;

  st_stylable_get (ST_STYLABLE (actor),
                   "scrollbar-width", &sb_width,
                   "scrollbar-height", &sb_height,
                   NULL);

  if (!CLUTTER_ACTOR_IS_VISIBLE (priv->vscroll))
    sb_width = 0;

  if (!CLUTTER_ACTOR_IS_VISIBLE (priv->hscroll))
    sb_height = 0;

  /* Vertical scrollbar */
  if (CLUTTER_ACTOR_IS_VISIBLE (priv->vscroll))
    {
      child_box.x1 = avail_width - sb_width;
      child_box.y1 = padding.top;
      child_box.x2 = avail_width;
      child_box.y2 = child_box.y1 + avail_height - sb_height;

      clutter_actor_allocate (priv->vscroll, &child_box, flags);
    }

  /* Horizontal scrollbar */
  if (CLUTTER_ACTOR_IS_VISIBLE (priv->hscroll))
    {
      child_box.x1 = padding.left;
      child_box.x2 = child_box.x1 + avail_width - sb_width;
      child_box.y1 = avail_height - sb_height;
      child_box.y2 = avail_height;

      clutter_actor_allocate (priv->hscroll, &child_box, flags);
    }


  /* Child */
  child_box.x1 = padding.left;
  child_box.x2 = avail_width - sb_width;
  child_box.y1 = padding.top;
  child_box.y2 = avail_height - sb_height;

  if (priv->child)
    clutter_actor_allocate (priv->child, &child_box, flags);
}

static void
st_scroll_view_style_changed (StWidget *widget)
{
  StScrollViewPrivate *priv = ST_SCROLL_VIEW (widget)->priv;

  st_stylable_changed ((StStylable *) priv->hscroll);
  st_stylable_changed ((StStylable *) priv->vscroll);
}

static gboolean
st_scroll_view_scroll_event (ClutterActor       *self,
                             ClutterScrollEvent *event)
{
  StScrollViewPrivate *priv = ST_SCROLL_VIEW (self)->priv;
  gdouble lower, value, upper, step;
  StAdjustment *vadjustment, *hadjustment;

  /* don't handle scroll events if requested not to */
  if (!priv->mouse_scroll)
    return FALSE;

  hadjustment = st_scroll_bar_get_adjustment (ST_SCROLL_BAR(priv->hscroll));
  vadjustment = st_scroll_bar_get_adjustment (ST_SCROLL_BAR(priv->vscroll));

  switch (event->direction)
    {
    case CLUTTER_SCROLL_UP:
    case CLUTTER_SCROLL_DOWN:
      if (vadjustment)
        g_object_get (vadjustment,
                      "lower", &lower,
                      "step-increment", &step,
                      "value", &value,
                      "upper", &upper,
                      NULL);
      else
        return FALSE;
      break;
    case CLUTTER_SCROLL_LEFT:
    case CLUTTER_SCROLL_RIGHT:
      if (vadjustment)
        g_object_get (hadjustment,
                      "lower", &lower,
                      "step-increment", &step,
                      "value", &value,
                      "upper", &upper,
                      NULL);
      else
        return FALSE;
      break;
    }

  switch (event->direction)
    {
    case CLUTTER_SCROLL_UP:
      if (value == lower)
        return FALSE;
      else
        st_adjustment_set_value (vadjustment, value - step);
      break;
    case CLUTTER_SCROLL_DOWN:
      if (value == upper)
        return FALSE;
      else
        st_adjustment_set_value (vadjustment, value + step);
      break;
    case CLUTTER_SCROLL_LEFT:
      if (value == lower)
        return FALSE;
      else
        st_adjustment_set_value (hadjustment, value - step);
      break;
    case CLUTTER_SCROLL_RIGHT:
      if (value == upper)
        return FALSE;
      else
        st_adjustment_set_value (hadjustment, value + step);
      break;
    }

  return TRUE;
}

static void
st_scroll_view_class_init (StScrollViewClass *klass)
{
  GParamSpec *pspec;
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (StScrollViewPrivate));

  object_class->get_property = st_scroll_view_get_property;
  object_class->set_property = st_scroll_view_set_property;
  object_class->dispose= st_scroll_view_dispose;
  object_class->finalize = st_scroll_view_finalize;

  actor_class->paint = st_scroll_view_paint;
  actor_class->pick = st_scroll_view_pick;
  actor_class->get_preferred_width = st_scroll_view_get_preferred_width;
  actor_class->get_preferred_height = st_scroll_view_get_preferred_height;
  actor_class->allocate = st_scroll_view_allocate;
  actor_class->scroll_event = st_scroll_view_scroll_event;

  g_object_class_install_property (object_class,
                                   PROP_HSCROLL,
                                   g_param_spec_object ("hscroll",
                                                        "StScrollBar",
                                                        "Horizontal scroll indicator",
                                                        ST_TYPE_SCROLL_BAR,
                                                        G_PARAM_READABLE));

  g_object_class_install_property (object_class,
                                   PROP_VSCROLL,
                                   g_param_spec_object ("vscroll",
                                                        "StScrollBar",
                                                        "Vertical scroll indicator",
                                                        ST_TYPE_SCROLL_BAR,
                                                        G_PARAM_READABLE));

  pspec = g_param_spec_boolean ("enable-mouse-scrolling",
                                "Enable Mouse Scrolling",
                                "Enable automatic mouse wheel scrolling",
                                TRUE,
                                G_PARAM_READWRITE);
  g_object_class_install_property (object_class,
                                   PROP_MOUSE_SCROLL,
                                   pspec);

}

static void
st_stylable_iface_init (StStylableIface *iface)
{
  static gboolean is_initialized = FALSE;

  if (!is_initialized)
    {
      GParamSpec *pspec;

      is_initialized = TRUE;

      pspec = g_param_spec_uint ("scrollbar-width",
                                 "Vertical scroll-bar thickness",
                                 "Thickness of vertical scrollbar, in px",
                                 0, G_MAXUINT, 24,
                                 G_PARAM_READWRITE);
      st_stylable_iface_install_property (iface, ST_TYPE_SCROLL_VIEW, pspec);

      pspec = g_param_spec_uint ("scrollbar-height",
                                 "Horizontal scroll-bar thickness",
                                 "Thickness of horizontal scrollbar, in px",
                                 0, G_MAXUINT, 24,
                                 G_PARAM_READWRITE);
      st_stylable_iface_install_property (iface, ST_TYPE_SCROLL_VIEW, pspec);
    }
}

static void
child_adjustment_changed_cb (StAdjustment *adjustment,
                             ClutterActor *bar)
{
  StScrollView *scroll;
  gdouble lower, upper, page_size;

  scroll = ST_SCROLL_VIEW (clutter_actor_get_parent (bar));

  /* Determine if this scroll-bar should be visible */
  st_adjustment_get_values (adjustment, NULL,
                            &lower, &upper,
                            NULL, NULL,
                            &page_size);

  if ((upper - lower) > page_size)
    clutter_actor_show (bar);
  else
    clutter_actor_hide (bar);

  /* Request a resize */
  clutter_actor_queue_relayout (CLUTTER_ACTOR (scroll));
}

static void
child_hadjustment_notify_cb (GObject    *gobject,
                             GParamSpec *arg1,
                             gpointer    user_data)
{
  StAdjustment *hadjust;

  ClutterActor *actor = CLUTTER_ACTOR (gobject);
  StScrollViewPrivate *priv = ST_SCROLL_VIEW (user_data)->priv;

  hadjust = st_scroll_bar_get_adjustment (ST_SCROLL_BAR(priv->hscroll));
  if (hadjust)
    g_signal_handlers_disconnect_by_func (hadjust,
                                          child_adjustment_changed_cb,
                                          priv->hscroll);

  st_scrollable_get_adjustments (ST_SCROLLABLE (actor), &hadjust, NULL);
  if (hadjust)
    {
      /* Force scroll step if neede. */
      if (priv->column_size_set)
        {
          g_object_set (hadjust,
                        "step-increment", priv->column_size,
                        NULL);
        }

      st_scroll_bar_set_adjustment (ST_SCROLL_BAR(priv->hscroll), hadjust);
      g_signal_connect (hadjust, "changed", G_CALLBACK (
                          child_adjustment_changed_cb), priv->hscroll);
      child_adjustment_changed_cb (hadjust, priv->hscroll);
    }
}

static void
child_vadjustment_notify_cb (GObject    *gobject,
                             GParamSpec *arg1,
                             gpointer    user_data)
{
  StAdjustment *vadjust;

  ClutterActor *actor = CLUTTER_ACTOR (gobject);
  StScrollViewPrivate *priv = ST_SCROLL_VIEW (user_data)->priv;

  vadjust = st_scroll_bar_get_adjustment (ST_SCROLL_BAR(priv->vscroll));
  if (vadjust)
    g_signal_handlers_disconnect_by_func (vadjust,
                                          child_adjustment_changed_cb,
                                          priv->vscroll);

  st_scrollable_get_adjustments (ST_SCROLLABLE(actor), NULL, &vadjust);
  if (vadjust)
    {
      /* Force scroll step if neede. */
      if (priv->row_size_set)
        {
          g_object_set (vadjust,
                        "step-increment", priv->row_size,
                        NULL);
        }

      st_scroll_bar_set_adjustment (ST_SCROLL_BAR(priv->vscroll), vadjust);
      g_signal_connect (vadjust, "changed", G_CALLBACK (
                          child_adjustment_changed_cb), priv->vscroll);
      child_adjustment_changed_cb (vadjust, priv->vscroll);
    }
}

static void
st_scroll_view_init (StScrollView *self)
{
  StScrollViewPrivate *priv = self->priv = SCROLL_VIEW_PRIVATE (self);

  priv->hscroll = CLUTTER_ACTOR (st_scroll_bar_new (NULL));
  priv->vscroll = g_object_new (ST_TYPE_SCROLL_BAR, "vertical", TRUE, NULL);

  clutter_actor_set_parent (priv->hscroll, CLUTTER_ACTOR (self));
  clutter_actor_set_parent (priv->vscroll, CLUTTER_ACTOR (self));

  /* mouse scroll is enabled by default, so we also need to be reactive */
  priv->mouse_scroll = TRUE;
  g_object_set (G_OBJECT (self), "reactive", TRUE, "clip-to-allocation", TRUE,
                NULL);

  g_signal_connect (self, "style-changed",
                    G_CALLBACK (st_scroll_view_style_changed), NULL);
}

static void
st_scroll_view_add (ClutterContainer *container,
                    ClutterActor     *actor)
{
  StScrollView *self = ST_SCROLL_VIEW (container);
  StScrollViewPrivate *priv = self->priv;

  if (ST_IS_SCROLLABLE (actor))
    {
      priv->child = actor;

      /* chain up to StBin::add() */
      st_scroll_view_parent_iface->add (container, actor);

      /* Get adjustments for scroll-bars */
      g_signal_connect (actor, "notify::hadjustment",
                        G_CALLBACK (child_hadjustment_notify_cb),
                        container);
      g_signal_connect (actor, "notify::vadjustment",
                        G_CALLBACK (child_vadjustment_notify_cb),
                        container);
      child_hadjustment_notify_cb (G_OBJECT (actor), NULL, container);
      child_vadjustment_notify_cb (G_OBJECT (actor), NULL, container);
    }
  else
    {
      g_warning ("Attempting to add an actor of type %s to "
                 "a StScrollView, but the actor does "
                 "not implement StScrollable.",
                 g_type_name (G_OBJECT_TYPE (actor)));
    }
}

static void
st_scroll_view_remove (ClutterContainer *container,
                       ClutterActor     *actor)
{
  StScrollViewPrivate *priv = ST_SCROLL_VIEW (container)->priv;

  if (actor == priv->child)
    {
      g_object_ref (priv->child);

      /* chain up to StBin::remove() */
      st_scroll_view_parent_iface->remove (container, actor);

      g_signal_handlers_disconnect_by_func (priv->child,
                                            child_hadjustment_notify_cb,
                                            container);
      g_signal_handlers_disconnect_by_func (priv->child,
                                            child_vadjustment_notify_cb,
                                            container);
      st_scrollable_set_adjustments ((StScrollable*) priv->child, NULL, NULL);

      g_object_unref (priv->child);
      priv->child = NULL;
    }
}

static void
st_scroll_view_foreach_with_internals (ClutterContainer *container,
                                       ClutterCallback   callback,
                                       gpointer          user_data)
{
  StScrollViewPrivate *priv = ST_SCROLL_VIEW (container)->priv;

  if (priv->child != NULL)
    callback (priv->child, user_data);

  if (priv->hscroll != NULL)
    callback (priv->hscroll, user_data);

  if (priv->vscroll != NULL)
    callback (priv->vscroll, user_data);
}

static void
clutter_container_iface_init (ClutterContainerIface *iface)
{
  /* store a pointer to the StBin implementation of
   * ClutterContainer so that we can chain up when
   * overriding the methods
   */
  st_scroll_view_parent_iface = g_type_interface_peek_parent (iface);

  iface->add = st_scroll_view_add;
  iface->remove = st_scroll_view_remove;
  iface->foreach_with_internals = st_scroll_view_foreach_with_internals;
}

StWidget *
st_scroll_view_new (void)
{
  return g_object_new (ST_TYPE_SCROLL_VIEW, NULL);
}

ClutterActor *
st_scroll_view_get_hscroll_bar (StScrollView *scroll)
{
  g_return_val_if_fail (ST_IS_SCROLL_VIEW (scroll), NULL);

  return scroll->priv->hscroll;
}

ClutterActor *
st_scroll_view_get_vscroll_bar (StScrollView *scroll)
{
  g_return_val_if_fail (ST_IS_SCROLL_VIEW (scroll), NULL);

  return scroll->priv->vscroll;
}

gfloat
st_scroll_view_get_column_size (StScrollView *scroll)
{
  StAdjustment *adjustment;
  gdouble column_size;

  g_return_val_if_fail (scroll, 0);

  adjustment = st_scroll_bar_get_adjustment (
    ST_SCROLL_BAR (scroll->priv->hscroll));
  g_object_get (adjustment,
                "step-increment", &column_size,
                NULL);

  return column_size;
}

void
st_scroll_view_set_column_size (StScrollView *scroll,
                                gfloat        column_size)
{
  StAdjustment *adjustment;

  g_return_if_fail (scroll);

  if (column_size < 0)
    {
      scroll->priv->column_size_set = FALSE;
      scroll->priv->column_size = -1;
    }
  else
    {
      scroll->priv->column_size_set = TRUE;
      scroll->priv->column_size = column_size;

      adjustment = st_scroll_bar_get_adjustment (
        ST_SCROLL_BAR (scroll->priv->hscroll));

      if (adjustment)
        g_object_set (adjustment,
                      "step-increment", (gdouble) scroll->priv->column_size,
                      NULL);
    }
}

gfloat
st_scroll_view_get_row_size (StScrollView *scroll)
{
  StAdjustment *adjustment;
  gdouble row_size;

  g_return_val_if_fail (scroll, 0);

  adjustment = st_scroll_bar_get_adjustment (
    ST_SCROLL_BAR (scroll->priv->vscroll));
  g_object_get (adjustment,
                "step-increment", &row_size,
                NULL);

  return row_size;
}

void
st_scroll_view_set_row_size (StScrollView *scroll,
                             gfloat        row_size)
{
  StAdjustment *adjustment;

  g_return_if_fail (scroll);

  if (row_size < 0)
    {
      scroll->priv->row_size_set = FALSE;
      scroll->priv->row_size = -1;
    }
  else
    {
      scroll->priv->row_size_set = TRUE;
      scroll->priv->row_size = row_size;

      adjustment = st_scroll_bar_get_adjustment (
        ST_SCROLL_BAR (scroll->priv->vscroll));

      if (adjustment)
        g_object_set (adjustment,
                      "step-increment", (gdouble) scroll->priv->row_size,
                      NULL);
    }
}

void
st_scroll_view_set_mouse_scrolling (StScrollView *scroll,
                                    gboolean      enabled)
{
  StScrollViewPrivate *priv;

  g_return_if_fail (ST_IS_SCROLL_VIEW (scroll));

  priv = ST_SCROLL_VIEW (scroll)->priv;

  if (priv->mouse_scroll != enabled)
    {
      priv->mouse_scroll = enabled;

      /* make sure we can receive mouse wheel events */
      if (enabled)
        clutter_actor_set_reactive ((ClutterActor *) scroll, TRUE);
    }
}

gboolean
st_scroll_view_get_mouse_scrolling (StScrollView *scroll)
{
  StScrollViewPrivate *priv;

  g_return_val_if_fail (ST_IS_SCROLL_VIEW (scroll), FALSE);

  priv = ST_SCROLL_VIEW (scroll)->priv;

  return priv->mouse_scroll;
}
