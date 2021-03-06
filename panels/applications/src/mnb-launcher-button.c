 /*
 * Copyright (C) 2008 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Written by: Robert Staudinger <robsta@o-hand.com>.
 * Based on nbtk-label.c by Thomas Wood <thomas@linux.intel.com>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <glib/gi18n.h>
#include <clutter/clutter.h>
#include <mx/mx.h>
#include "mnb-launcher-button.h"

static void mx_focusable_iface_init (MxFocusableIface *iface);

G_DEFINE_TYPE_WITH_CODE (MnbLauncherButton, mnb_launcher_button, MX_TYPE_TABLE,
                         G_IMPLEMENT_INTERFACE (MX_TYPE_FOCUSABLE,
                                                mx_focusable_iface_init))

#define MNB_LAUNCHER_BUTTON_GET_PRIVATE(obj)    \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MNB_TYPE_LAUNCHER_BUTTON, MnbLauncherButtonPrivate))

/* Distance between icon and text. */
#define ROW_SPACING 5
#define COL_SPACING 10
#define FAV_TOGGLE_SIZE 24
#define FAV_TOGGLE_X_OFFSET 7
#define FAV_TOGGLE_Y_OFFSET 5

enum
{
  ACTIVATED,
  FAV_TOGGLED,

  LAST_SIGNAL
};

struct _MnbLauncherButtonPrivate
{
  ClutterActor  *icon;
  MxLabel       *title;
  ClutterActor  *fav_toggle;

  char          *description;
  char          *category;
  char          *executable;
  char          *desktop_file_path;
  char          *icon_name;
  char          *icon_file;
  gint           icon_size;

  guint is_pressed  : 1;

  /* Cached for matching. */
  char          *category_key;
  char          *title_key;
  char          *description_key;
  char          *comment_key;
  char          *executable_key;

  /* Those are mutually exclusive.
   * fav_sibling:   sibling in the fav pane.
   * plain_sibling: sibling in the expander. */
  MnbLauncherButton *fav_sibling;
  MnbLauncherButton *plain_sibling;
};

static guint _signals[LAST_SIGNAL] = { 0, };

static MxFocusable *
mnb_launcher_button_accept_focus (MxFocusable *focusable,
                                  MxFocusHint hint)
{
  clutter_ungrab_pointer ();
  clutter_actor_grab_key_focus (CLUTTER_ACTOR (focusable));
  mx_stylable_set_style_pseudo_class (MX_STYLABLE (focusable), "hover");

  return focusable;
}

static void
finalize (GObject *object)
{
  MnbLauncherButton *self = MNB_LAUNCHER_BUTTON (object);

  /* Child actors are managed by clutter. */

  g_free (self->priv->description);
  g_free (self->priv->category);
  g_free (self->priv->executable);
  g_free (self->priv->desktop_file_path);
  g_free (self->priv->icon_name);
  g_free (self->priv->icon_file);

  g_free (self->priv->category_key);
  g_free (self->priv->title_key);
  g_free (self->priv->description_key);
  g_free (self->priv->executable_key);

  G_OBJECT_CLASS (mnb_launcher_button_parent_class)->finalize (object);
}

static gboolean
mnb_launcher_button_button_press_event (ClutterActor       *actor,
                                        ClutterButtonEvent *event)
{
  if (event->button == 1)
    {
      MnbLauncherButton *self = MNB_LAUNCHER_BUTTON (actor);

      self->priv->is_pressed = TRUE;
      clutter_grab_pointer (actor);
      return TRUE;
    }

  return FALSE;
}

static gboolean
mnb_launcher_button_button_release_event (ClutterActor       *actor,
                                          ClutterButtonEvent *event)
{
  if (event->button == 1)
    {
      MnbLauncherButton *self = MNB_LAUNCHER_BUTTON (actor);

      if (!self->priv->is_pressed)
        return FALSE;

      clutter_ungrab_pointer ();
      self->priv->is_pressed = FALSE;
      g_signal_emit (self, _signals[ACTIVATED], 0);

      mx_stylable_set_style_pseudo_class (MX_STYLABLE (self), NULL);
      mx_widget_hide_tooltip (MX_WIDGET (self));

      return TRUE;
    }

  return FALSE;
}

/* probably not really needed but making sure the is_pressed state is
 * consistent
 */
static gboolean
mnb_launcher_button_key_press_event (ClutterActor       *actor,
                                     ClutterKeyEvent *event)
{
  if (event->keyval == CLUTTER_KEY_Return ||
      event->keyval == CLUTTER_KEY_KP_Enter ||
      event->keyval == CLUTTER_KEY_ISO_Enter)
    {
      MnbLauncherButton *self = MNB_LAUNCHER_BUTTON (actor);

      self->priv->is_pressed = TRUE;
      clutter_actor_grab_key_focus (actor);
      return TRUE;
    }

  return FALSE;
}

static gboolean
mnb_launcher_button_key_release_event (ClutterActor       *actor,
                                       ClutterKeyEvent    *event)
{
  if (event->keyval == CLUTTER_KEY_Return ||
      event->keyval == CLUTTER_KEY_KP_Enter ||
      event->keyval == CLUTTER_KEY_ISO_Enter)
    {
      MnbLauncherButton *self = MNB_LAUNCHER_BUTTON (actor);

      if (!self->priv->is_pressed)
        return FALSE;

      clutter_ungrab_keyboard ();
      self->priv->is_pressed = FALSE;
      g_signal_emit (self, _signals[ACTIVATED], 0);

      mx_stylable_set_style_pseudo_class (MX_STYLABLE (self), NULL);
      mx_widget_hide_tooltip (MX_WIDGET (self));

      return TRUE;
    }

  return FALSE;
}

/* Accept focus handles setting hover pseudo style class and leave events
 * handle unsetting the hover pseudo style class
 */
static gboolean
_leave_event_cb (ClutterActor         *actor,
                 ClutterCrossingEvent *event,
                 gpointer              data)
{
  MnbLauncherButton *self = MNB_LAUNCHER_BUTTON (actor);

  mx_stylable_set_style_pseudo_class (MX_STYLABLE (self), NULL);

  if (self->priv->is_pressed)
    {
      clutter_ungrab_pointer ();
      clutter_ungrab_keyboard ();
      self->priv->is_pressed = FALSE;
    }

  return FALSE;
}

static gboolean
_enter_event_cb (ClutterActor         *actor,
                 ClutterCrossingEvent *event,
                 gpointer              data)
{
  MxFocusable *focusable;
  MxFocusManager *f_manager;

  f_manager =
    mx_focus_manager_get_for_stage (CLUTTER_STAGE (clutter_actor_get_stage (actor)));

  focusable = mx_focus_manager_get_focused (f_manager);

  /* hide the hover state on a button that has accepted focus via the default
   * focus as we're navigating by mouse and not keys we don't want to move
   * the focus as we may still want it in the place e.g. search bar
   */
  if ((focusable) &&
      (MNB_IS_LAUNCHER_BUTTON (focusable)) &&
      (CLUTTER_ACTOR (focusable) != actor))
    mx_stylable_set_style_pseudo_class (MX_STYLABLE (focusable), NULL);

  mx_stylable_set_style_pseudo_class (MX_STYLABLE (actor), "hover");

  return FALSE;
}

static void
mnb_launcher_button_allocate (ClutterActor          *actor,
                              const ClutterActorBox *box,
                              ClutterAllocationFlags flags)
{
  MnbLauncherButton *self = MNB_LAUNCHER_BUTTON (actor);
  ClutterActorBox    pin_box;

  CLUTTER_ACTOR_CLASS (mnb_launcher_button_parent_class)
    ->allocate (actor, box, flags);

  /* Pin location hardcoded, designers want this to fit perfectly. */
  pin_box.x1 = (int) (box->x2 - box->x1 - FAV_TOGGLE_SIZE - FAV_TOGGLE_X_OFFSET);
  pin_box.x2 = (int) (pin_box.x1 + FAV_TOGGLE_SIZE);
  pin_box.y1 = (int) (FAV_TOGGLE_Y_OFFSET);
  pin_box.y2 = (int) (pin_box.y1 + FAV_TOGGLE_SIZE);
  clutter_actor_allocate (CLUTTER_ACTOR (self->priv->fav_toggle), &pin_box, flags);
}

static void
mnb_launcher_button_paint (ClutterActor *actor)
{
  CLUTTER_ACTOR_CLASS (mnb_launcher_button_parent_class)->paint (actor);

  clutter_actor_paint ((ClutterActor *) ((MnbLauncherButton *) actor)->priv->fav_toggle);
}

static void
mnb_launcher_button_pick (ClutterActor       *actor,
                          const ClutterColor *color)
{
  MnbLauncherButtonPrivate *priv = MNB_LAUNCHER_BUTTON (actor)->priv;
  ClutterGeometry geom;

  /* draw a rectangle to conver the entire actor */

  clutter_actor_get_allocation_geometry (actor, &geom);

  cogl_set_source_color4ub (color->red, color->green, color->blue, color->alpha);
  cogl_rectangle (0, 0, geom.width, geom.height);

  if (priv->fav_toggle)
    clutter_actor_paint (priv->fav_toggle);
}

static void
mx_focusable_iface_init (MxFocusableIface *iface)
{
  iface->accept_focus = mnb_launcher_button_accept_focus;
}

static void
fav_button_notify_toggled_cb (MxButton          *button,
                              GParamSpec        *pspec,
                              MnbLauncherButton *self)
{
  g_signal_emit (self, _signals[FAV_TOGGLED], 0);
}

static void
mnb_launcher_button_class_init (MnbLauncherButtonClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbLauncherButtonPrivate));

  object_class->finalize = finalize;

  actor_class->button_press_event = mnb_launcher_button_button_press_event;
  actor_class->button_release_event = mnb_launcher_button_button_release_event;
  actor_class->key_press_event = mnb_launcher_button_key_press_event;
  actor_class->key_release_event = mnb_launcher_button_key_release_event;
  actor_class->allocate = mnb_launcher_button_allocate;
  actor_class->allocate = mnb_launcher_button_allocate;
  actor_class->pick = mnb_launcher_button_pick;
  actor_class->paint = mnb_launcher_button_paint;


  _signals[ACTIVATED] = g_signal_new ("activated",
                                    G_TYPE_FROM_CLASS (klass),
                                    G_SIGNAL_RUN_LAST,
                                    G_STRUCT_OFFSET (MnbLauncherButtonClass, activated),
                                    NULL, NULL,
                                    g_cclosure_marshal_VOID__VOID,
                                    G_TYPE_NONE, 0);

  _signals[FAV_TOGGLED] = g_signal_new ("fav-toggled",
                                    G_TYPE_FROM_CLASS (klass),
                                    G_SIGNAL_RUN_LAST,
                                    G_STRUCT_OFFSET (MnbLauncherButtonClass, fav_toggled),
                                    NULL, NULL,
                                    g_cclosure_marshal_VOID__VOID,
                                    G_TYPE_NONE, 0);
}

static void
mnb_launcher_button_init (MnbLauncherButton *self)
{
  ClutterActor *label;

  self->priv = MNB_LAUNCHER_BUTTON_GET_PRIVATE (self);

  g_signal_connect (self, "leave-event",
                    G_CALLBACK (_leave_event_cb), NULL);
  g_signal_connect (self, "key-focus-out",
                    G_CALLBACK (_leave_event_cb), NULL);
  g_signal_connect (self, "enter-event",
                    G_CALLBACK (_enter_event_cb), NULL);

  self->priv->icon = NULL;

  mx_stylable_set_style_class (MX_STYLABLE (self), "contentTile");

  /* "app" label */
  self->priv->title = (MxLabel *) mx_label_new ();
  mx_label_set_x_align (MX_LABEL (self->priv->title), MX_ALIGN_MIDDLE);

  clutter_actor_set_reactive (CLUTTER_ACTOR (self->priv->title), FALSE);
  mx_table_insert_actor_with_properties (MX_TABLE (self),
                                         CLUTTER_ACTOR (self->priv->title),
                                         1, 0,
                                         "x-align", MX_ALIGN_MIDDLE,
                                         "x-expand", TRUE,
                                         "x-fill", TRUE,
                                         "y-align", MX_ALIGN_MIDDLE,
                                         "y-expand", TRUE,
                                         "y-fill", TRUE,
                                         NULL);

  label = mx_label_get_clutter_text (self->priv->title);
  clutter_text_set_ellipsize (CLUTTER_TEXT (label), PANGO_ELLIPSIZE_END);
  clutter_text_set_line_alignment (CLUTTER_TEXT (label), PANGO_ALIGN_CENTER);
  clutter_text_set_line_wrap (CLUTTER_TEXT (label), TRUE);
  clutter_text_set_line_wrap_mode (CLUTTER_TEXT (label), PANGO_WRAP_WORD_CHAR);

  /* "fav app" toggle */
  self->priv->fav_toggle = g_object_ref_sink (CLUTTER_ACTOR (mx_button_new ()));
  mx_button_set_is_toggle (MX_BUTTON (self->priv->fav_toggle), TRUE);
  clutter_actor_set_name (CLUTTER_ACTOR (self->priv->fav_toggle),
                          "mnb-launcher-button-fav-toggle");
  clutter_actor_set_size (self->priv->fav_toggle, FAV_TOGGLE_SIZE, FAV_TOGGLE_SIZE);
  mx_table_insert_actor (MX_TABLE (self),
                         CLUTTER_ACTOR (self->priv->fav_toggle),
                         0, 0);


  g_signal_connect (self->priv->fav_toggle, "notify::toggled",
                    G_CALLBACK (fav_button_notify_toggled_cb), self);

  clutter_actor_set_reactive ((ClutterActor *) self, TRUE);
}

MxWidget *
mnb_launcher_button_new (const gchar *icon_name,
                         const gchar *icon_file,
                         gint         icon_size,
                         const gchar *title,
                         const gchar *category,
                         const gchar *description,
                         const gchar *executable,
                         const gchar *desktop_file_path)
{
  MnbLauncherButton *self;

  self = g_object_new (MNB_TYPE_LAUNCHER_BUTTON, NULL);

  self->priv->icon_name = g_strdup (icon_name);
  mnb_launcher_button_set_icon (self, icon_file, icon_size);

  if (title)
    mx_label_set_text (self->priv->title, title);

  if (category)
    self->priv->category = g_strdup (category);

  if (description)
  {
    self->priv->description = g_strdup (description);
    /* NO tooltip due to ugly ness and length of description
     * mx_widget_set_tooltip_text (MX_WIDGET (self), self->priv->description);
     */
  }

  if (executable)
    self->priv->executable = g_strdup (executable);

  if (desktop_file_path)
    self->priv->desktop_file_path = g_strdup (desktop_file_path);

  return MX_WIDGET (self);
}

const char *
mnb_launcher_button_get_title (MnbLauncherButton *self)
{
  g_return_val_if_fail (self, NULL);

  if (self->priv->title)
    return mx_label_get_text (self->priv->title);
  else
    return NULL;
}

const char *
mnb_launcher_button_get_category (MnbLauncherButton *self)
{
  g_return_val_if_fail (self, NULL);

  return self->priv->category;
}

const char *
mnb_launcher_button_get_description (MnbLauncherButton *self)
{
  g_return_val_if_fail (self, NULL);

  return self->priv->description;
}

const char *
mnb_launcher_button_get_executable (MnbLauncherButton *self)
{
  g_return_val_if_fail (self, NULL);

  return self->priv->executable;
}

const char *
mnb_launcher_button_get_desktop_file_path (MnbLauncherButton *self)
{
  g_return_val_if_fail (self, NULL);

  return self->priv->desktop_file_path;
}

gboolean
mnb_launcher_button_get_favorite (MnbLauncherButton *self)
{
  g_return_val_if_fail (self, FALSE);

  return mx_button_get_toggled (MX_BUTTON (self->priv->fav_toggle));
}

void
mnb_launcher_button_set_favorite (MnbLauncherButton *self,
                                  gboolean           is_favorite)
{
  g_return_if_fail (self);

  mx_button_set_toggled (MX_BUTTON (self->priv->fav_toggle), is_favorite);
}

const gchar *
mnb_launcher_button_get_icon_name (MnbLauncherButton *self)
{
  g_return_val_if_fail (self, NULL);

  return self->priv->icon_name;
}

void
mnb_launcher_button_set_icon (MnbLauncherButton  *self,
                              const gchar        *icon_file,
                              gint                icon_size)
{
  MxTextureCache *texture_cache;
  GError         *error = NULL;

  if (self->priv->icon_file)
    {
      g_free (self->priv->icon_file);
      self->priv->icon_file = NULL;
    }

  if (self->priv->icon)
    {
      clutter_actor_destroy (self->priv->icon);
      self->priv->icon = NULL;
    }

  self->priv->icon_file = g_strdup (icon_file);
  self->priv->icon_size = icon_size;

  error = NULL;
  texture_cache = mx_texture_cache_get_default ();
  self->priv->icon = mx_texture_cache_get_actor (texture_cache,
                                                   self->priv->icon_file);

  if (error) {
    g_warning (G_STRLOC "%s", error->message);
    g_clear_error (&error);
  }

  if (self->priv->icon) {

    if (self->priv->icon_size > -1) {
      clutter_actor_set_size (self->priv->icon,
                              self->priv->icon_size,
                              self->priv->icon_size);
    }

    mx_table_insert_actor_with_properties (MX_TABLE (self),
                                           CLUTTER_ACTOR (self->priv->icon),
                                           0, 0,
                                           "x-align", MX_ALIGN_MIDDLE,
                                           "x-expand", FALSE,
                                           "x-fill", FALSE,
                                           "y-align", MX_ALIGN_MIDDLE,
                                           "y-expand", FALSE,
                                           "y-fill", FALSE,
                                           NULL);
  }
}

gint
mnb_launcher_button_compare (MnbLauncherButton *self,
                             MnbLauncherButton *other)
{
  g_return_val_if_fail (self, 0);
  g_return_val_if_fail (other, 0);

  return g_utf8_collate (mx_label_get_text (MX_LABEL (self->priv->title)),
                         mx_label_get_text (MX_LABEL (other->priv->title)));
}

gboolean
mnb_launcher_button_match (MnbLauncherButton *self,
                           const gchar       *lcase_needle)
{
  g_return_val_if_fail (self, 0);

  /* Empty key matches. */
  if (g_utf8_strlen (lcase_needle, -1) == 0)
    return TRUE;

  /* Category */
  if (!self->priv->category_key)
    self->priv->category_key = g_utf8_strdown (self->priv->category, -1);

  if (self->priv->category_key &&
      NULL != strstr (self->priv->category_key, lcase_needle))
    {
      return TRUE;
    }

  /* Title. */
  if (!self->priv->title_key)
    self->priv->title_key =
      g_utf8_strdown (mx_label_get_text (MX_LABEL (self->priv->title)),
                      -1);

  if (self->priv->title_key &&
      NULL != strstr (self->priv->title_key, lcase_needle))
    {
      return TRUE;
    }

  /* Description. */
  if (self->priv->description && !self->priv->description_key)
    self->priv->description_key = g_utf8_strdown (self->priv->description, -1);

  if (self->priv->description_key &&
      NULL != strstr (self->priv->description_key, lcase_needle))
    {
      return TRUE;
    }

  /* Executable. */
  if (self->priv->executable && !self->priv->executable_key)
    self->priv->executable_key = g_utf8_strdown (self->priv->executable, -1);

  if (self->priv->executable_key &&
      NULL != strstr (self->priv->executable_key, lcase_needle))
    {
      return TRUE;
    }

  return FALSE;
}
