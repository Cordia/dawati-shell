/*
 * Copyright (c) 2011 Intel Corp.
 *
 * Author: Danielle Madeley <danielle.madeley@collabora.co.uk>
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
 */

#include <glib/gi18n.h>

#include "mnb-home-plugins-engine.h"
#include "mnb-home-widget.h"
#include "utils.h"

#include "dawati-home-plugins-app.h"

#define DEBUG g_message
#define STR_EMPTY(s) ((s) == NULL || *(s) == '\0')

#define GSETTINGS_PLUGIN_PATH_PREFIX "/org/dawati/shell/home/plugin/"

G_DEFINE_TYPE (MnbHomeWidget, mnb_home_widget, MX_TYPE_FRAME);

enum /* properties */
{
  PROP_0,
  PROP_ROW,
  PROP_COLUMN,
  PROP_EDIT_MODE,
  PROP_LAST
};

struct _MnbHomeWidgetPrivate
{
  GSettings *settings;
  MnbHomePluginsEngine *engine;
  DawatiHomePluginsApp *app;

  guint row;
  guint column;
  gboolean edit_mode;
};

static void
mnb_home_widget_get_property (GObject *self,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec)
{
  MnbHomeWidgetPrivate *priv = MNB_HOME_WIDGET (self)->priv;

  switch (prop_id)
    {
      case PROP_ROW:
        g_value_set_uint (value, priv->row);
        break;

      case PROP_COLUMN:
        g_value_set_uint (value, priv->column);
        break;

      case PROP_EDIT_MODE:
        g_value_set_boolean (value, priv->edit_mode);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
        break;
    }
}

static void
mnb_home_widget_set_property (GObject *self,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec)
{
  MnbHomeWidgetPrivate *priv = MNB_HOME_WIDGET (self)->priv;

  switch (prop_id)
    {
      case PROP_ROW:
        priv->row = g_value_get_uint (value);
        break;

      case PROP_COLUMN:
        priv->column = g_value_get_uint (value);
        break;

      case PROP_EDIT_MODE:
        mnb_home_widget_set_edit_mode (MNB_HOME_WIDGET (self),
            g_value_get_boolean (value));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
        break;
    }
}

static void
home_widget_module_changed (GSettings *settings,
    char *key,
    MnbHomeWidget *self)
{
  char *module = g_settings_get_string (settings, key);

  g_clear_object (&self->priv->app);

  if (STR_EMPTY (module))
    {
      DEBUG ("no module");
    }
  else
    {
      char *path;

      DEBUG ("module = '%s'", module);

      self->priv->engine = mnb_home_plugins_engine_dup ();
      path = g_strdup_printf (GSETTINGS_PLUGIN_PATH_PREFIX "%u_%u/settings/",
          self->priv->row, self->priv->column);

      self->priv->app = mnb_home_plugins_engine_create_app (self->priv->engine,
          module, path);

      g_free (path);
    }

  g_free (module);

  /* reload the widget */
  mnb_home_widget_set_edit_mode (self, self->priv->edit_mode);
}

static void
mnb_home_widget_constructed (GObject *self)
{
  MnbHomeWidgetPrivate *priv = MNB_HOME_WIDGET (self)->priv;
  char *path;

  path = g_strdup_printf (GSETTINGS_PLUGIN_PATH_PREFIX "%u_%u/",
      priv->row, priv->column);
  DEBUG ("widget path = '%s'", path);

  priv->settings = g_settings_new_with_path ("org.dawati.shell.home.plugin",
      path);

  g_signal_connect (priv->settings, "changed::module",
      G_CALLBACK (home_widget_module_changed), self);
  home_widget_module_changed (priv->settings, "module", MNB_HOME_WIDGET (self));

  g_free (path);
}

static void
mnb_home_widget_dispose (GObject *self)
{
  MnbHomeWidgetPrivate *priv = MNB_HOME_WIDGET (self)->priv;

  g_object_unref (priv->settings);
  g_object_unref (priv->engine);
  g_object_unref (priv->app);

  G_OBJECT_CLASS (mnb_home_widget_parent_class)->dispose (self);
}

static void
mnb_home_widget_class_init (MnbHomeWidgetClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->get_property = mnb_home_widget_get_property;
  gobject_class->set_property = mnb_home_widget_set_property;
  gobject_class->constructed = mnb_home_widget_constructed;
  gobject_class->dispose = mnb_home_widget_dispose;

  g_type_class_add_private (gobject_class, sizeof (MnbHomeWidgetPrivate));

  g_object_class_install_property (gobject_class, PROP_ROW,
      g_param_spec_uint ("row",
        "Row",
        "",
        0, G_MAXUINT, 0,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_COLUMN,
      g_param_spec_uint ("column",
        "Column",
        "",
        0, G_MAXUINT, 0,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_EDIT_MODE,
      g_param_spec_boolean ("edit-mode",
        "Edit Mode",
        "%TRUE if we are editing the settings for this widget",
        FALSE,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
mnb_home_widget_init (MnbHomeWidget *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, MNB_TYPE_HOME_WIDGET,
      MnbHomeWidgetPrivate);

  g_object_set (self,
      "x-fill", TRUE,
      "y-fill", TRUE,
      NULL);
}

ClutterActor *
mnb_home_widget_new (guint row,
    guint column)
{
  return g_object_new (MNB_TYPE_HOME_WIDGET,
      "row", row,
      "column", column,
      NULL);
}

static void
home_widget_add_module (MxButton *button,
    MnbHomeWidget *self)
{
  /* FIXME: this is where we offer some widgets */
  g_settings_set_string (self->priv->settings, "module", "example");
}

static void
home_widget_remove_module (MxButton *button,
    MnbHomeWidget *self)
{
  char *path;

  path = g_strdup_printf (GSETTINGS_PLUGIN_PATH_PREFIX "%u_%u/",
      self->priv->row, self->priv->column);
  dconf_recursive_unset (path, NULL);

  g_free (path);
}

void
mnb_home_widget_set_edit_mode (MnbHomeWidget *self,
    gboolean edit_mode)
{
  self->priv->edit_mode = edit_mode;

  g_object_notify (G_OBJECT (self), "edit-mode");

  /* FIXME: should hold refs to the actors rather than destroy/recreate them? */
  mx_bin_set_child (MX_BIN (self), NULL);

  /* FIXME: animate this */
  if (edit_mode)
    {
      ClutterActor *table;

      table = mx_table_new ();
      mx_bin_set_child (MX_BIN (self), table);

      if (self->priv->app != NULL)
        {
          ClutterActor *config, *remove;

          remove = mx_button_new_with_label ("x");
          mx_table_add_actor (MX_TABLE (table), remove, 0, 1);
          mx_table_child_set_x_fill (MX_TABLE (table), remove, FALSE);
          mx_table_child_set_y_fill (MX_TABLE (table), remove, FALSE);

          g_signal_connect (remove, "clicked",
              G_CALLBACK (home_widget_remove_module), self);

          config = dawati_home_plugins_app_get_configuration (self->priv->app);

          if (CLUTTER_IS_ACTOR (config))
            mx_table_add_actor (MX_TABLE (table), config, 1, 0);
        }
      else
        {
          ClutterActor *button;

          button = mx_button_new_with_label ("+");
          mx_table_add_actor (MX_TABLE (table), button, 0, 0);

          g_signal_connect (button, "clicked",
              G_CALLBACK (home_widget_add_module), self);
        }

      clutter_actor_show_all (table);
    }
  else
    {
      if (self->priv->app != NULL)
        {
          ClutterActor *widget;

          widget = dawati_home_plugins_app_get_widget (self->priv->app);

          if (!CLUTTER_IS_ACTOR (widget))
            /* FIXME: make this better */
            widget = mx_label_new_with_text (_("Broken plugin"));

          mx_bin_set_child (MX_BIN (self), widget);
        }
    }
}
