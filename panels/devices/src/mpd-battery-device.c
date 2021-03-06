
/*
 * Copyright © 2010 Intel Corp.
 *
 * Authors: Rob Staudinger <robert.staudinger@intel.com>
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

#include <libupower-glib/upower.h>
#include <glib/gi18n.h>

#include "mpd-battery-device.h"
#include "mpd-gobject.h"
#include "config.h"

G_DEFINE_TYPE (MpdBatteryDevice, mpd_battery_device, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_BATTERY_DEVICE, MpdBatteryDevicePrivate))

enum
{
  PROP_0,

  PROP_PERCENTAGE,
  PROP_STATE
};

typedef struct
{
  UpClient              *client;
  UpDevice              *device;
  unsigned int           percentage;
  MpdBatteryDeviceState  state;
} MpdBatteryDevicePrivate;

static void
mpd_battery_device_set_percentage (MpdBatteryDevice       *self,
                                   float                   percentage);
static void
mpd_battery_device_set_state      (MpdBatteryDevice       *self,
                                   MpdBatteryDeviceState   state);

static void
_client_device_added_cb (UpClient         *client,
			 UpDevice         *device,
			 MpdBatteryDevice *self)
{
  MpdBatteryDevicePrivate *priv = GET_PRIVATE (self);
  UpDeviceKind device_kind;

  if (NULL == priv->device)
  {
    g_object_get (device, "kind", &device_kind, NULL);

    if (UP_DEVICE_KIND_BATTERY == device_kind)
    {
      priv->device = g_object_ref (device);

      mpd_battery_device_set_percentage (self,
                                         mpd_battery_device_get_percentage (self));
      mpd_battery_device_set_state (self,
                                    mpd_battery_device_get_state (self));
    }
  }
}

static void
_client_device_removed_cb (UpClient         *client,
			   UpDevice         *device,
			   MpdBatteryDevice *self)
{
  MpdBatteryDevicePrivate *priv = GET_PRIVATE (self);

  if (priv->device &&
      0 == g_strcmp0 (up_device_get_object_path (device),
                      up_device_get_object_path (priv->device)))
  {
    g_object_unref (priv->device);
    priv->device = NULL;

    mpd_battery_device_set_percentage (self, -1);
    mpd_battery_device_set_state (self, MPD_BATTERY_DEVICE_STATE_MISSING);
  }
}

static void
_client_device_changed_cb (UpClient         *client,
			   UpDevice         *device,
			   MpdBatteryDevice *self)
{
  MpdBatteryDevicePrivate *priv = GET_PRIVATE (self);

  if (priv->device &&
      0 == g_strcmp0 (up_device_get_object_path (device),
                      up_device_get_object_path (priv->device)))
  {
    mpd_battery_device_set_percentage (self,
                                       mpd_battery_device_get_percentage (self));
    mpd_battery_device_set_state (self,
                                  mpd_battery_device_get_state (self));
  }
}

static GObject *
_constructor (GType                  type,
              unsigned int           n_properties,
              GObjectConstructParam *properties)
{
  static MpdBatteryDevice *self = NULL;
  MpdBatteryDevicePrivate *priv = NULL;
  GPtrArray     *devices;
  GError        *error = NULL;
  unsigned int   i;

  /* This is a singleton */

  if (self)
  {
    return g_object_ref (self);
  }

  self = (MpdBatteryDevice *)
                  G_OBJECT_CLASS (mpd_battery_device_parent_class)
                    ->constructor (type, n_properties, properties);
  priv = GET_PRIVATE (self);
  g_return_val_if_fail (priv->client, NULL);
  g_object_add_weak_pointer ((GObject *) self, (gpointer) &self);

  /* Look up battery device. */

  up_client_enumerate_devices_sync (priv->client, NULL, &error);
  if (error)
  {
    g_critical ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
    g_object_unref (self);
    return NULL;
  }

  devices = up_client_get_devices (priv->client);

  for (i = 0; i < devices->len; i++)
  {
    UpDevice *device = g_ptr_array_index (devices, i);
    UpDeviceKind device_kind;

    g_object_get (device, "kind", &device_kind, NULL);

    if (UP_DEVICE_KIND_BATTERY == device_kind)
      priv->device = g_object_ref (device);
  }

  g_ptr_array_unref (devices);

  /* Set initial properties. */

  mpd_battery_device_set_percentage (self,
                                      mpd_battery_device_get_percentage (self));
  mpd_battery_device_set_state (self,
                                mpd_battery_device_get_state (self));

  return (GObject *) self;
}

static void
_get_property (GObject      *object,
               unsigned int  property_id,
               GValue       *value,
               GParamSpec   *pspec)
{
  switch (property_id)
  {
  case PROP_PERCENTAGE:
    g_value_set_float (value,
                       mpd_battery_device_get_percentage (
                          MPD_BATTERY_DEVICE (object)));
    break;
  case PROP_STATE:
    g_value_set_uint (value,
                       mpd_battery_device_get_state (
                          MPD_BATTERY_DEVICE (object)));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_set_property (GObject      *object,
               unsigned int  property_id,
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
_dispose (GObject *object)
{
  MpdBatteryDevicePrivate *priv = GET_PRIVATE (object);

  mpd_gobject_detach (object, (GObject **) &priv->client);

  G_OBJECT_CLASS (mpd_battery_device_parent_class)->dispose (object);
}

static void
mpd_battery_device_class_init (MpdBatteryDeviceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamFlags   param_flags;

  g_type_class_add_private (klass, sizeof (MpdBatteryDevicePrivate));

  object_class->constructor = _constructor;
  object_class->dispose = _dispose;
  object_class->get_property = _get_property;
  object_class->set_property = _set_property;

  /* Properties */

  param_flags = G_PARAM_READABLE | G_PARAM_STATIC_STRINGS;

  g_object_class_install_property (object_class,
                                   PROP_PERCENTAGE,
                                   g_param_spec_float ("percentage",
                                                       "Percentage",
                                                       "Battery energy percentage",
                                                       0., 100., 0.,
                                                       param_flags));
  g_object_class_install_property (object_class,
                                   PROP_STATE,
                                   g_param_spec_uint ("state",
                                                      "State",
                                                      "Battery state",
                                                      MPD_BATTERY_DEVICE_STATE_UNKNOWN,
                                                      MPD_BATTERY_DEVICE_STATE_DELIMITER,
                                                      MPD_BATTERY_DEVICE_STATE_UNKNOWN,
                                                      param_flags));
}

static void
mpd_battery_device_init (MpdBatteryDevice *self)
{
  MpdBatteryDevicePrivate *priv = GET_PRIVATE (self);

  priv->client = up_client_new ();
  g_signal_connect (priv->client, "device-added",
                    G_CALLBACK (_client_device_added_cb), self);
  g_signal_connect (priv->client, "device-removed",
                    G_CALLBACK (_client_device_removed_cb), self);
  g_signal_connect (priv->client, "device-changed",
                    G_CALLBACK (_client_device_changed_cb), self);
}

MpdBatteryDevice *
mpd_battery_device_new (void)
{
  return g_object_new (MPD_TYPE_BATTERY_DEVICE, NULL);
}

float
mpd_battery_device_get_percentage (MpdBatteryDevice *self)
{
  MpdBatteryDevicePrivate *priv = GET_PRIVATE (self);
  double energy;
  double energy_full;

  g_return_val_if_fail (MPD_IS_BATTERY_DEVICE (self), -1.);

  /* Have battery? */
  if (NULL == priv->device)
    return -1.;

  g_object_get (priv->device,
                "energy", &energy,
                "energy-full", &energy_full,
                NULL);

  return (float ) (energy / energy_full * 100);
}

static void
mpd_battery_device_set_percentage (MpdBatteryDevice *self,
                                   float             percentage)
{
  MpdBatteryDevicePrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (MPD_IS_BATTERY_DEVICE (self));

  /* Filter out changes after the decimal point. */
  if ((unsigned int) percentage != priv->percentage)
  {
    priv->percentage = percentage;
    g_object_notify (G_OBJECT (self), "percentage");
  }
}

MpdBatteryDeviceState
mpd_battery_device_get_state (MpdBatteryDevice *self)
{
  MpdBatteryDevicePrivate *priv = GET_PRIVATE (self);
  MpdBatteryDeviceState state;
  UpDeviceState         device_state;

  g_return_val_if_fail (MPD_IS_BATTERY_DEVICE (self),
                        MPD_BATTERY_DEVICE_STATE_UNKNOWN);

  if (NULL == priv->device)
  {
    return MPD_BATTERY_DEVICE_STATE_MISSING;
  }

  g_object_get (priv->device, "state", &device_state, NULL);

  switch (device_state)
  {
  case UP_DEVICE_STATE_CHARGING:
    state = MPD_BATTERY_DEVICE_STATE_CHARGING;
    break;
  case UP_DEVICE_STATE_DISCHARGING:
  case UP_DEVICE_STATE_EMPTY:
    state = MPD_BATTERY_DEVICE_STATE_DISCHARGING;
    break;
  case UP_DEVICE_STATE_FULLY_CHARGED:
    state = MPD_BATTERY_DEVICE_STATE_FULLY_CHARGED;
    break;
  default:
    state = MPD_BATTERY_DEVICE_STATE_UNKNOWN;
  }

  return state;
}

static void
mpd_battery_device_set_state (MpdBatteryDevice       *self,
                              MpdBatteryDeviceState   state)
{
  MpdBatteryDevicePrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (MPD_IS_BATTERY_DEVICE (self));

  if (state != priv->state)
  {
    priv->state = state;
    g_object_notify (G_OBJECT (self), "state");
  }
}

char *
mpd_battery_device_get_state_text (MpdBatteryDevice *self)
{
  MpdBatteryDevicePrivate *priv = GET_PRIVATE (self);
  char *description;

  g_return_val_if_fail (MPD_IS_BATTERY_DEVICE (self), NULL);

  switch (priv->state)
  {
  case MPD_BATTERY_DEVICE_STATE_MISSING:
    description = g_strdup_printf (_("Sorry, you don't appear to have a "
                                     "battery installed."));
    break;
  case MPD_BATTERY_DEVICE_STATE_CHARGING:
    description = g_strdup_printf (_("Your battery is charging. "
                                     "It is about %d%% full."),
                                   priv->percentage);
    break;
  case MPD_BATTERY_DEVICE_STATE_DISCHARGING:
    description = g_strdup_printf (_("Your battery is being used. "
                                     "It is about %d%% full."),
                                   priv->percentage);
    break;
  case MPD_BATTERY_DEVICE_STATE_FULLY_CHARGED:
    description = g_strdup_printf (_("Your battery is fully charged and "
                                     "you're ready to go."));
    break;
  default:
    description = g_strdup_printf (_("Sorry, it looks like your battery is "
                                     "broken."));
  }

  return description;
}

void
mpd_battery_device_dump (MpdBatteryDevice *self)
{
  MpdBatteryDevicePrivate *priv = GET_PRIVATE (self);
  double energy;
  double energy_full;

  g_return_if_fail (MPD_IS_BATTERY_DEVICE (self));

  g_object_get (priv->device,
                "energy", &energy,
                "energy-full", &energy_full,
                NULL);

  g_debug ("energy: %.2f, full: %.2f", energy, energy_full);
}

