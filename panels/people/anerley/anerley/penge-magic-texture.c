/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Authors: Rob Bradford <rob@linux.intel.com>
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

#include <cogl/cogl.h>

#include "penge-magic-texture.h"

G_DEFINE_TYPE (PengeMagicTexture, penge_magic_texture, CLUTTER_TYPE_TEXTURE)

static void
penge_magic_texture_paint (ClutterActor *actor)
{
  ClutterActorBox box;
  CoglHandle tex;
  float bw, bh;
  float aw, ah;
  float v;
  float tx1, tx2, ty1, ty2;
  ClutterColor col = { 0xff, 0xff, 0xff, 0xff };

  clutter_actor_get_allocation_box (actor, &box);
  tex = clutter_texture_get_cogl_texture (CLUTTER_TEXTURE (actor));

  bw = (float) cogl_texture_get_width (tex); /* base texture width */
  bh = (float) cogl_texture_get_height (tex); /* base texture height */

  aw = (float) (box.x2 - box.x1); /* allocation width */
  ah = (float) (box.y2 - box.y1); /* allocation height */

  /* no comment */
  if ((float)bw/bh < (float)aw/ah)
  {
    /* fit width */
    v = (((float)ah * bw) / ((float)aw * bh)) / 2;
    tx1 = 0;
    tx2 = 1;
    ty1 = (0.5 - v);
    ty2 = (0.5 + v);
  } else {
    /* fit height */
    v = (((float)aw * bh) / ((float)ah * bw)) / 2;
    tx1 = (0.5 - v);
    tx2 = (0.5 + v);
    ty1 = 0;
    ty2 = 1;
  }

  col.alpha = clutter_actor_get_paint_opacity (actor);
  cogl_set_source_color4ub (col.red, col.green, col.blue, col.alpha);
  cogl_rectangle (0, 0, aw, ah);
  cogl_set_source_texture (tex);
  cogl_rectangle_with_texture_coords (0, 0,
                                      aw, ah,
                                      tx1, ty1,
                                      tx2, ty2);
}
static void
penge_magic_texture_tile_get_preferred_width (ClutterActor *actor,
                                              gfloat        for_height,
                                              gfloat       *min_width,
                                              gfloat       *pref_width)
{
  if (min_width)
    *min_width = 64;
  if (pref_width)
    *pref_width = 64;
}

static void
penge_magic_texture_tile_get_preferred_height (ClutterActor *actor,
                                               gfloat        for_width,
                                               gfloat       *min_height,
                                               gfloat       *pref_height)
{
  if (min_height)
    *min_height = 64;
  if (pref_height)
    *pref_height = 64;
}


static void
penge_magic_texture_class_init (PengeMagicTextureClass *klass)
{
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  actor_class->paint = penge_magic_texture_paint;
  actor_class->get_preferred_width = penge_magic_texture_tile_get_preferred_width;
  actor_class->get_preferred_height = penge_magic_texture_tile_get_preferred_height;
}

static void
penge_magic_texture_init (PengeMagicTexture *self)
{
}

