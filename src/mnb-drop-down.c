/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2008, 2010 Intel Corp.
 *
 * Author: Thomas Wood <thomas@linux.intel.com>
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

#include "mnb-drop-down.h"
#include "mnb-panel.h"
#include "mnb-toolbar.h"    /* For MNB_IS_TOOLBAR */
#include "moblin-netbook.h" /* For PANEL_HEIGHT */

#define SLIDE_DURATION 150

static void mnb_panel_iface_init (MnbPanelIface *iface);

G_DEFINE_TYPE_WITH_CODE (MnbDropDown,
                         mnb_drop_down,
                         MX_TYPE_TABLE,
                         G_IMPLEMENT_INTERFACE (MNB_TYPE_PANEL,
                                                mnb_panel_iface_init));

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_DROP_DOWN, MnbDropDownPrivate))

enum {
  PROP_0,

  PROP_MUTTER_PLUGIN,
};

struct _MnbDropDownPrivate {
  MutterPlugin *plugin;

  ClutterActor *child;
  ClutterActor *footer;
  MxButton *button;
  gint x;
  gint y;

  guint reparent_cb;

  gboolean in_show_animation : 1;
  gboolean in_hide_animation : 1;

  gulong show_completed_id;
  gulong hide_completed_id;
  ClutterAnimation *show_anim;
  ClutterAnimation *hide_anim;
};

static void
mnb_drop_down_get_property (GObject *object, guint property_id,
                            GValue *value, GParamSpec *pspec)
{
  MnbDropDown *self = MNB_DROP_DOWN (object);

  switch (property_id)
    {
    case PROP_MUTTER_PLUGIN:
      g_value_set_object (value, self->priv->plugin);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mnb_drop_down_set_property (GObject *object, guint property_id,
                            const GValue *value, GParamSpec *pspec)
{
  MnbDropDown *self = MNB_DROP_DOWN (object);

  switch (property_id)
    {
    case PROP_MUTTER_PLUGIN:
      self->priv->plugin = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mnb_drop_down_dispose (GObject *object)
{
  MnbDropDownPrivate *priv = MNB_DROP_DOWN (object)->priv;

  if (priv->button)
    {
      priv->button = NULL;
    }

  if (priv->footer)
    {
      clutter_actor_destroy (priv->footer);
      priv->footer = NULL;
    }

  G_OBJECT_CLASS (mnb_drop_down_parent_class)->dispose (object);
}

static void
mnb_drop_down_finalize (GObject *object)
{
  G_OBJECT_CLASS (mnb_drop_down_parent_class)->finalize (object);
}

static void
mnb_drop_down_show_completed_cb (ClutterAnimation *anim, ClutterActor *actor)
{
  MnbDropDownPrivate *priv = MNB_DROP_DOWN (actor)->priv;

  priv->in_show_animation = FALSE;
  priv->show_anim = NULL;
  priv->show_completed_id = 0;

  if (priv->button)
    {
      if (!mx_button_get_checked (priv->button))
        mx_button_set_checked (priv->button, TRUE);
    }

  g_signal_emit_by_name (actor, "show-completed");
  g_object_unref (actor);
}

static void
mnb_toolbar_show_completed_cb (MnbToolbar *toolbar, gpointer data)
{
  ClutterActor *dropdown = CLUTTER_ACTOR (data);

  g_signal_handlers_disconnect_by_func (toolbar,
                                        mnb_toolbar_show_completed_cb,
                                        data);

  clutter_actor_show (dropdown);
}

static void
mnb_drop_down_show (ClutterActor *actor)
{
  MnbDropDownPrivate *priv = MNB_DROP_DOWN (actor)->priv;
  gfloat x, y;
  gfloat height, width;
  ClutterAnimation *animation;
  ClutterActor *toolbar;

  if (priv->in_show_animation)
    {
      g_signal_stop_emission_by_name (actor, "show");
      return;
    }

  if (priv->hide_completed_id)
    {
      g_signal_handler_disconnect (priv->hide_anim, priv->hide_completed_id);
      priv->hide_anim = NULL;
      priv->hide_completed_id = 0;
      priv->in_hide_animation = FALSE;
    }

  mnb_panel_ensure_size (MNB_PANEL (actor));

  /*
   * Check the toolbar is visible, if not show it.
   */
  toolbar = clutter_actor_get_parent (actor);
  while (toolbar && !MNB_IS_TOOLBAR (toolbar))
    toolbar = clutter_actor_get_parent (toolbar);

  if (!toolbar)
    {
      g_warning ("Cannot show Panel that is not inside the Toolbar.");
      return;
    }

  if (!CLUTTER_ACTOR_IS_MAPPED (toolbar))
    {
      /*
       * We need to show the toolbar first, and only when it is visible
       * to show this panel.
       */
      g_signal_connect (toolbar, "show-completed",
                        G_CALLBACK (mnb_toolbar_show_completed_cb),
                        actor);

      mnb_toolbar_show ((MnbToolbar*)toolbar, MNB_SHOW_HIDE_BY_PANEL);
      return;
    }

  g_signal_emit_by_name (actor, "show-begin");

  CLUTTER_ACTOR_CLASS (mnb_drop_down_parent_class)->show (actor);

  clutter_actor_get_position (actor, &x, &y);
  clutter_actor_get_size (actor, &width, &height);

  /* save the size/position so we can clip while we are moving */
  priv->x = x;
  priv->y = y;

  clutter_actor_set_position (actor, x, -height);

  priv->in_show_animation = TRUE;

  g_object_ref (actor);

  animation = clutter_actor_animate (actor, CLUTTER_EASE_IN_SINE,
                                     SLIDE_DURATION,
                                     "x", x,
                                     "y", y,
                                     NULL);

  priv->show_completed_id =
    g_signal_connect_after (animation,
                            "completed",
                            G_CALLBACK (mnb_drop_down_show_completed_cb),
                            actor);
  priv->show_anim = animation;
}

static void
mnb_drop_down_hide_completed_cb (ClutterAnimation *anim, ClutterActor *actor)
{
  MnbDropDownPrivate *priv = MNB_DROP_DOWN (actor)->priv;

  priv->hide_anim = NULL;
  priv->hide_completed_id = 0;

  /* the hide animation has finished, so now really hide the actor */
  CLUTTER_ACTOR_CLASS (mnb_drop_down_parent_class)->hide (actor);

  /* now that it's hidden we can put it back to where it is suppoed to be */
  clutter_actor_set_position (actor, priv->x, priv->y);

  priv->in_hide_animation = FALSE;
  g_signal_emit_by_name (actor, "hide-completed");
  g_object_unref (actor);
}

static void
mnb_drop_down_hide (ClutterActor *actor)
{
  MnbDropDownPrivate *priv = MNB_DROP_DOWN (actor)->priv;
  ClutterAnimation   *animation;

  if (priv->in_hide_animation)
    {
      g_signal_stop_emission_by_name (actor, "hide");
      return;
    }

  priv->in_hide_animation = TRUE;

  if (priv->show_completed_id)
    {
      g_signal_handler_disconnect (priv->show_anim, priv->show_completed_id);
      priv->show_anim = NULL;
      priv->show_completed_id = 0;
      priv->in_show_animation = FALSE;

      if (priv->button)
        {
          if (mx_button_get_checked (priv->button))
            mx_button_set_checked (priv->button, FALSE);
        }
    }

  g_signal_emit_by_name (actor, "hide-begin");

  /* de-activate the button */
  if (priv->button)
    {
      /* hide is hooked into the notify::checked signal from the button, so
       * make sure we don't get into a loop by checking checked first
       */
      if (mx_button_get_checked (priv->button))
        mx_button_set_checked (priv->button, FALSE);
    }

  if (!priv->child)
    {
      CLUTTER_ACTOR_CLASS (mnb_drop_down_parent_class)->hide (actor);
      return;
    }

  g_object_ref (actor);

  animation = clutter_actor_animate (actor, CLUTTER_EASE_IN_SINE,
                                     SLIDE_DURATION,
                                     "y", -clutter_actor_get_height (actor),
                                     NULL);

  priv->hide_completed_id =
    g_signal_connect_after (animation,
                            "completed",
                            G_CALLBACK (mnb_drop_down_hide_completed_cb),
                            actor);

  priv->hide_anim = animation;
}

static void
mnb_drop_down_paint (ClutterActor *actor)
{
  MnbDropDownPrivate *priv = MNB_DROP_DOWN (actor)->priv;
  ClutterGeometry geom;

  clutter_actor_get_allocation_geometry (actor, &geom);

#if CLUTTER_CHECK_VERSION (1, 1, 3)
      cogl_clip_push_rectangle (priv->x - geom.x, priv->y - geom.y,
                                priv->x - geom.x + geom.width,
                                priv->y - geom.y + geom.height);
#else
  cogl_clip_push (priv->x - geom.x,
                  priv->y - geom.y,
                  geom.width,
                  geom.height);
#endif

  CLUTTER_ACTOR_CLASS (mnb_drop_down_parent_class)->paint (actor);
  cogl_clip_pop ();
}

static gboolean
mnb_button_event_capture (ClutterActor *actor, ClutterButtonEvent *event)
{
  /* prevent the event from moving up the scene graph, since we don't want
   * any events to accidently fall onto application windows below the
   * drop down.
   */
  return TRUE;
}

static void
mnb_drop_down_allocate (ClutterActor          *actor,
                        const ClutterActorBox *box,
                        ClutterAllocationFlags flags)
{
#if 0
  MnbDropDownPrivate *priv = MNB_DROP_DOWN (actor)->priv;
#endif
  ClutterActorClass  *parent_class;

  /*
   * The show and hide animations trigger allocations with origin_changed
   * set to TRUE; if we call the parent class allocation in this case, it
   * will force relayout, which we do not want. Instead, we call directly the
   * ClutterActor implementation of allocate(); this ensures our actor box is
   * correct, which is all we call about during the animations.
   *
   * If the drop down is not visible, we just return; this insures that the
   * needs_allocation flag in ClutterActor remains set, and the actor will get
   * reallocated when we show it.
   */
  if (!CLUTTER_ACTOR_IS_VISIBLE (actor))
    return;

#if 0
  /*
   * This is not currently reliable, e.g., the Switcher is animating empty.
   * Once we have the binary flags in Clutter 1.0 to differentiate between
   * allocations that are purely due to change of position and the rest, we
   * need to disable this.
   */
  if (priv->in_show_animation || priv->in_hide_animation)
    {
      ClutterActorClass  *actor_class;

      actor_class = g_type_class_peek (CLUTTER_TYPE_ACTOR);

      if (actor_class)
        actor_class->allocate (actor, box, origin_changed);

      return;
    }
#endif

  parent_class = CLUTTER_ACTOR_CLASS (mnb_drop_down_parent_class);
  parent_class->allocate (actor, box, flags);
}

static void
mnb_drop_down_constructed (GObject *object)
{
#if 0
  MnbDropDownPrivate *priv = MNB_DROP_DOWN (object)->priv;
  ClutterActor       *footer;

  /* footer with "up" button */
  footer = mx_button_new ();
  mx_stylable_set_style_class (MX_STYLABLE (footer), "drop-down-footer");
  mx_table_add_actor (MX_TABLE (object), CLUTTER_ACTOR (footer), 1, 0);
  g_signal_connect_swapped (footer, "clicked",
                            G_CALLBACK (mnb_panel_hide_with_toolbar), object);

  priv->footer = CLUTTER_ACTOR (footer);
#endif
}

static void
mnb_drop_down_class_init (MnbDropDownClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *clutter_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbDropDownPrivate));

  object_class->get_property = mnb_drop_down_get_property;
  object_class->set_property = mnb_drop_down_set_property;
  object_class->dispose      = mnb_drop_down_dispose;
  object_class->finalize     = mnb_drop_down_finalize;
  object_class->constructed  = mnb_drop_down_constructed;

  clutter_class->show = mnb_drop_down_show;
  clutter_class->hide = mnb_drop_down_hide;
  clutter_class->paint = mnb_drop_down_paint;
  clutter_class->allocate = mnb_drop_down_allocate;
  clutter_class->button_press_event = mnb_button_event_capture;
  clutter_class->button_release_event = mnb_button_event_capture;
  clutter_class->allocate = mnb_drop_down_allocate;

  g_object_class_install_property (object_class,
                                   PROP_MUTTER_PLUGIN,
                                   g_param_spec_object ("mutter-plugin",
                                                      "Mutter Plugin",
                                                      "Mutter Plugin",
                                                      MUTTER_TYPE_PLUGIN,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY));
}

static void
mnb_drop_down_init (MnbDropDown *self)
{
  self->priv = GET_PRIVATE (self);

  g_object_set (self,
                "show-on-set-parent", FALSE,
                "reactive", TRUE,
                NULL);
}

MxWidget*
mnb_drop_down_new (MutterPlugin *plugin)
{
  return g_object_new (MNB_TYPE_DROP_DOWN,
                       "mutter-plugin", plugin,
                       NULL);
}

static void
mnb_drop_down_reparent_cb (ClutterActor *child,
                           ClutterActor *old_parent,
                           gpointer      data)
{
  MnbDropDownPrivate *priv = MNB_DROP_DOWN (data)->priv;

  if (clutter_actor_get_parent (child) != data)
    {
      if (priv->reparent_cb)
        {
          g_signal_handler_disconnect (priv->child, priv->reparent_cb);
          priv->reparent_cb = 0;
        }

      priv->child = NULL;
    }
}

void
mnb_drop_down_set_child (MnbDropDown *drop_down,
                         ClutterActor *child)
{
  MnbDropDownPrivate *priv;

  g_return_if_fail (MNB_IS_DROP_DOWN (drop_down));
  g_return_if_fail (child == NULL || CLUTTER_IS_ACTOR (child));

  priv = drop_down->priv;

  if (priv->child)
    {
      if (priv->reparent_cb)
        {
          g_signal_handler_disconnect (priv->child, priv->reparent_cb);
          priv->reparent_cb = 0;
        }

      clutter_container_remove_actor (CLUTTER_CONTAINER (drop_down),
                                      priv->child);
    }

  if (child)
    {
      priv->reparent_cb =
        g_signal_connect (child, "parent-set",
                          G_CALLBACK (mnb_drop_down_reparent_cb),
                          drop_down);
      mx_table_add_actor (MX_TABLE (drop_down), child, 0, 0);
    }

  priv->child = child;
}

ClutterActor*
mnb_drop_down_get_child (MnbDropDown *drop_down)
{
  g_return_val_if_fail (MNB_DROP_DOWN (drop_down), NULL);

  return drop_down->priv->child;
}

static void
mnb_drop_down_button_weak_unref_cb (MnbDropDown *drop_down, GObject *button)
{
  drop_down->priv->button = NULL;
}

static void
mnb_drop_down_panel_set_button (MnbPanel *panel, MxButton *button)
{
  MnbDropDown *drop_down = MNB_DROP_DOWN (panel);
  MxButton  *old_button;

  g_return_if_fail (MNB_IS_DROP_DOWN (drop_down));
  g_return_if_fail (!button || MX_IS_BUTTON (button));

  old_button = drop_down->priv->button;
  drop_down->priv->button = button;

  if (old_button)
    {
      g_object_weak_unref (G_OBJECT (old_button),
                           (GWeakNotify) mnb_drop_down_button_weak_unref_cb,
                           drop_down);
    }

  if (button)
    {
      g_object_weak_ref (G_OBJECT (button),
                         (GWeakNotify) mnb_drop_down_button_weak_unref_cb,
                         drop_down);
    }
}

/*
 * Returns untransformed geometry of the footer relative to the drop down.
 */
void
mnb_drop_down_get_footer_geometry (MnbDropDown *self,
                                   gfloat      *x,
                                   gfloat      *y,
                                   gfloat      *width,
                                   gfloat      *height)
{
  MnbDropDownPrivate *priv = self->priv;

  g_return_if_fail (x && y && width && height);

  /*
   * ??? Something borked here; if I query coords of the footer I am getting
   * what looks like the unexpanded size of the cell, relative to its column
   * and row position, not to the parent. Work around it.
   */
  *x      = clutter_actor_get_x (CLUTTER_ACTOR (self));
  *y      = clutter_actor_get_height (priv->child);
  *width  = clutter_actor_get_width  (CLUTTER_ACTOR (self));
  *height = /*clutter_actor_get_height (priv->footer)*/ 0;
}

static void
mnb_drop_down_panel_show (MnbPanel *panel)
{
  clutter_actor_show (CLUTTER_ACTOR (panel));
}

static void
mnb_drop_down_panel_hide (MnbPanel *panel)
{
  clutter_actor_hide (CLUTTER_ACTOR (panel));
}

static void
mnb_drop_down_panel_set_size (MnbPanel *panel, guint width, guint height)
{
  clutter_actor_set_size (CLUTTER_ACTOR (panel), width, height);
}

static void
mnb_drop_down_panel_get_size (MnbPanel *panel, guint *width, guint *height)
{
  MnbDropDownPrivate *priv = MNB_DROP_DOWN (panel)->priv;
  gfloat w, h;
  gfloat wc = 0.0, hc = 0.0;

  /*
   * FIXME -- need to find out why the height of the panel is reported as
   * 0 and fix that; this is a very ugly cludge.
   */
  clutter_actor_get_size (CLUTTER_ACTOR (panel), &w, &h);

  if (priv->child)
    clutter_actor_get_size (priv->child, &wc, &hc);

  if (width)
    {
      if (w)
        *width = w;
      else if (wc)
        *width = wc + TOOLBAR_X_PADDING * 2;
      else
        *width = 0;
    }

  if (height)
    {
      if (h)
        *height = h;
      else if (hc)
        *height = hc + TOOLBAR_HEIGHT / 2 + 4;
      else
        *height = 0;
    }
}

static void
mnb_drop_down_panel_set_position (MnbPanel *panel, gint x, gint y)
{
  clutter_actor_set_position (CLUTTER_ACTOR (panel), x, y);
}

static void
mnb_drop_down_panel_get_position (MnbPanel *panel, gint *x, gint *y)
{
  gfloat xf, yf;

  clutter_actor_get_position (CLUTTER_ACTOR (panel), &xf, &yf);

  if (x)
    *x = xf;

  if (y)
    *y = yf;
}

static void
mnb_panel_iface_init (MnbPanelIface *iface)
{
  iface->show             = mnb_drop_down_panel_show;
  iface->hide             = mnb_drop_down_panel_hide;

  iface->set_size         = mnb_drop_down_panel_set_size;
  iface->get_size         = mnb_drop_down_panel_get_size;
  iface->set_position     = mnb_drop_down_panel_set_position;
  iface->get_position     = mnb_drop_down_panel_get_position;

  iface->set_button       = mnb_drop_down_panel_set_button;
}

