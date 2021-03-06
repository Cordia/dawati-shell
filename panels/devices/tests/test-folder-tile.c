
/*
 * Copyright (c) 2010 Intel Corp.
 *
 * Author: Robert Staudinger <robert.staudinger@intel.com>
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

#include <stdlib.h>
#include <clutter/clutter.h>
#include "mpd-folder-tile.h"

int
main (int     argc,
      char  **argv)
{
  ClutterActor  *stage;
  ClutterActor  *tile;

  if (clutter_init (&argc, &argv) != CLUTTER_INIT_SUCCESS)
    {
      g_critical ("Could not initialize Clutter");
      return EXIT_FAILURE;
    }

  stage = clutter_stage_new ();

  tile = mpd_folder_tile_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), tile);

  clutter_actor_show_all (stage);
  clutter_main ();
  return EXIT_SUCCESS;
}

