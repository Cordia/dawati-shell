/* nutter-grid.h: Reflowing grid layout container for clutter.
 *
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
 * Written by: Øyvind Kolås <pippin@linux.intel.com>
 */

/* TODO:
 *
 * - Better names for properties.
 * - Caching layouted positions? (perhaps needed for huge collections)
 * - More comments / overall concept on how the layouting is done.
 * - Allow more layout directions than just row major / column major.
 */

#include <clutter/clutter-actor.h>
#include <clutter/clutter-container.h>
#include <string.h>

#include "nutter-grid.h"

typedef struct _NutterGridActorData NutterGridActorData;

static void nutter_grid_dispose             (GObject *object);
static void nutter_grid_finalize            (GObject *object);

static void nutter_grid_finalize            (GObject *object);

static void nutter_grid_set_property        (GObject      *object,
                                           guint         prop_id,
                                           const GValue *value,
                                           GParamSpec   *pspec);
static void nutter_grid_get_property        (GObject      *object,
                                           guint         prop_id,
                                           GValue       *value,
                                           GParamSpec   *pspec);

static void clutter_container_iface_init  (ClutterContainerIface *iface);

static void nutter_grid_real_add            (ClutterContainer *container,
                                           ClutterActor     *actor);
static void nutter_grid_real_remove         (ClutterContainer *container,
                                           ClutterActor     *actor);
static void nutter_grid_real_foreach        (ClutterContainer *container,
                                           ClutterCallback   callback,
                                           gpointer          user_data);
static void nutter_grid_real_raise          (ClutterContainer *container,
                                           ClutterActor     *actor,
                                           ClutterActor     *sibling);
static void nutter_grid_real_lower          (ClutterContainer *container,
                                           ClutterActor     *actor,
                                           ClutterActor     *sibling);
static void
nutter_grid_real_sort_depth_order (ClutterContainer *container);

static void
nutter_grid_free_actor_data (gpointer data);

static void nutter_grid_paint (ClutterActor *actor);

static void nutter_grid_pick (ClutterActor *actor,
                                       const ClutterColor *color);

static void
nutter_grid_get_preferred_width (ClutterActor *self,
                                         ClutterUnit for_height,
                                         ClutterUnit *min_width_p,
                                         ClutterUnit *natural_width_p);

static void
nutter_grid_get_preferred_height (ClutterActor *self,
                                           ClutterUnit for_width,
                                           ClutterUnit *min_height_p,
                                           ClutterUnit *natural_height_p);

static void nutter_grid_allocate (ClutterActor *self,
                                           const ClutterActorBox *box,
                                           gboolean absolute_origin_changed);

G_DEFINE_TYPE_WITH_CODE (NutterGrid, nutter_grid,
                         CLUTTER_TYPE_ACTOR,
                         G_IMPLEMENT_INTERFACE (CLUTTER_TYPE_CONTAINER,
                                                clutter_container_iface_init));

#define NUTTER_GRID_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NUTTER_TYPE_GRID, \
                                NutterGridPrivate))

struct _NutterGridPrivate
{
  ClutterUnit for_height,  for_width;
  ClutterUnit pref_width,  pref_height;
  ClutterUnit alloc_width, alloc_height;

  gboolean    absolute_origin_changed;
  GHashTable *hash_table;
  GList      *list;

  gboolean    homogenous_rows;
  gboolean    homogenous_columns;
  gboolean    end_align;
  ClutterUnit column_gap, row_gap;
  gdouble     valign, halign;

  gboolean    column_major;

  gboolean    first_of_batch;
  ClutterUnit a_current_sum, a_wrap;
  ClutterUnit max_extent_a;
  ClutterUnit max_extent_b;
};

enum
{
  PROP_0,
  PROP_HOMOGENOUS_ROWS,
  PROP_HOMOGENOUS_COLUMNS,
  PROP_ROW_GAP,
  PROP_COLUMN_GAP,
  PROP_VALIGN,
  PROP_HALIGN,
  PROP_END_ALIGN,
  PROP_COLUMN_MAJOR,
};

struct _NutterGridActorData
{
  gboolean    xpos_set,   ypos_set;
  ClutterUnit xpos,       ypos;
  ClutterUnit pref_width, pref_height;
};

static void
nutter_grid_class_init (NutterGridClass *klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  ClutterActorClass *actor_class = (ClutterActorClass *) klass;

  gobject_class->dispose = nutter_grid_dispose;
  gobject_class->finalize = nutter_grid_finalize;

  gobject_class->set_property = nutter_grid_set_property;
  gobject_class->get_property = nutter_grid_get_property;

  actor_class->paint                = nutter_grid_paint;
  actor_class->pick                 = nutter_grid_pick;
  actor_class->get_preferred_width  = nutter_grid_get_preferred_width;
  actor_class->get_preferred_height = nutter_grid_get_preferred_height;
  actor_class->allocate             = nutter_grid_allocate;

  g_type_class_add_private (klass, sizeof (NutterGridPrivate));


  g_object_class_install_property
                   (gobject_class,
                    PROP_ROW_GAP,
                    clutter_param_spec_unit ("row-gap",
                                             "Row gap",
                                             "gap between rows in the layout",
                                             0, CLUTTER_MAXUNIT,
                                             0,
                                             G_PARAM_READWRITE|G_PARAM_CONSTRUCT));

  g_object_class_install_property
                   (gobject_class,
                    PROP_COLUMN_GAP,
                    clutter_param_spec_unit ("column-gap",
                                             "Column gap",
                                             "gap between columns in the layout",
                                             0, CLUTTER_MAXUNIT,
                                             0,
                                             G_PARAM_READWRITE|G_PARAM_CONSTRUCT));


  g_object_class_install_property
                   (gobject_class,
                    PROP_HOMOGENOUS_ROWS,
                    g_param_spec_boolean ("homogenous-rows",
                                          "homogenous rows",
                                          "Should all rows have the same height?",
                                          FALSE,
                                          G_PARAM_READWRITE|G_PARAM_CONSTRUCT));

  g_object_class_install_property
                   (gobject_class,
                    PROP_HOMOGENOUS_COLUMNS,
                    g_param_spec_boolean ("homogenous-columns",
                                          "homogenous columns",
                                          "Should all columns have the same height?",
                                          FALSE,
                                          G_PARAM_READWRITE|G_PARAM_CONSTRUCT));

  g_object_class_install_property
                   (gobject_class,
                    PROP_COLUMN_MAJOR,
                    g_param_spec_boolean ("column-major",
                                          "column-major",
                                          "Do a column filling first instead of row filling first",
                                          FALSE,
                                          G_PARAM_READWRITE|G_PARAM_CONSTRUCT));

  g_object_class_install_property
                   (gobject_class,
                    PROP_END_ALIGN,
                    g_param_spec_boolean ("end-align",
                                          "end-align",
                                          "Right/bottom aligned rows/columns",
                                          FALSE,
                                          G_PARAM_READWRITE|G_PARAM_CONSTRUCT));

  g_object_class_install_property
                   (gobject_class,
                    PROP_VALIGN,
                    g_param_spec_double ("valign",
                                         "Vertical align",
                                         "Vertical alignment of items within cells",
                                          0.0, 1.0, 0.0,
                                          G_PARAM_READWRITE|G_PARAM_CONSTRUCT));

  g_object_class_install_property
                   (gobject_class,
                    PROP_HALIGN,
                    g_param_spec_double ("halign",
                                         "Horizontal align",
                                         "Horizontal alignment of items within cells",
                                          0.0, 1.0, 0.0,
                                          G_PARAM_READWRITE|G_PARAM_CONSTRUCT));

}

static void
clutter_container_iface_init (ClutterContainerIface *iface)
{
  iface->add              = nutter_grid_real_add;
  iface->remove           = nutter_grid_real_remove;
  iface->foreach          = nutter_grid_real_foreach;
  iface->raise            = nutter_grid_real_raise;
  iface->lower            = nutter_grid_real_lower;
  iface->sort_depth_order = nutter_grid_real_sort_depth_order;
}

static void
nutter_grid_init (NutterGrid *self)
{
  NutterGridPrivate *priv;

  self->priv = priv = NUTTER_GRID_GET_PRIVATE (self);

  priv->hash_table
    = g_hash_table_new_full (g_direct_hash,
                             g_direct_equal,
                             /*g_object_unref*/NULL,
                             nutter_grid_free_actor_data);
}

static void
nutter_grid_dispose (GObject *object)
{
  NutterGrid *self = (NutterGrid *) object;
  NutterGridPrivate *priv;

  priv = self->priv;

  /* Destroy all of the children. This will cause them to be removed
     from the container and unparented */
  clutter_container_foreach (CLUTTER_CONTAINER (object),
                             (ClutterCallback) clutter_actor_destroy,
                             NULL);

  G_OBJECT_CLASS (nutter_grid_parent_class)->dispose (object);
}

static void
nutter_grid_finalize (GObject *object)
{
  NutterGrid *self = (NutterGrid *) object;
  NutterGridPrivate *priv = self->priv;

  g_hash_table_destroy (priv->hash_table);

  G_OBJECT_CLASS (nutter_grid_parent_class)->finalize (object);
}


void
nutter_grid_set_end_align (NutterGrid *self,
                         gboolean  value)
{
  NutterGridPrivate *priv = NUTTER_GRID_GET_PRIVATE (self);
  priv->end_align = value;
  clutter_actor_queue_relayout (CLUTTER_ACTOR (self));
}

gboolean
nutter_grid_get_end_align (NutterGrid *self)
{
  NutterGridPrivate *priv = NUTTER_GRID_GET_PRIVATE (self);
  return priv->end_align;
}

void
nutter_grid_set_homogenous_rows (NutterGrid *self,
                               gboolean  value)
{
  NutterGridPrivate *priv = NUTTER_GRID_GET_PRIVATE (self);
  priv->homogenous_rows = value;
  clutter_actor_queue_relayout (CLUTTER_ACTOR (self));
}

gboolean
nutter_grid_get_homogenous_rows (NutterGrid *self)
{
  NutterGridPrivate *priv = NUTTER_GRID_GET_PRIVATE (self);
  return priv->homogenous_rows;
}


void
nutter_grid_set_homogenous_columns (NutterGrid *self,
                                  gboolean  value)
{
  NutterGridPrivate *priv = NUTTER_GRID_GET_PRIVATE (self);
  priv->homogenous_columns = value;
  clutter_actor_queue_relayout (CLUTTER_ACTOR (self));
}


gboolean
nutter_grid_get_homogenous_columns (NutterGrid *self)
{
  NutterGridPrivate *priv = NUTTER_GRID_GET_PRIVATE (self);
  return priv->homogenous_columns;
}


void
nutter_grid_set_column_major (NutterGrid *self,
                            gboolean  value)
{
  NutterGridPrivate *priv = NUTTER_GRID_GET_PRIVATE (self);
  priv->column_major = value;
  clutter_actor_queue_relayout (CLUTTER_ACTOR (self));
}

gboolean
nutter_grid_get_column_major (NutterGrid *self)
{
  NutterGridPrivate *priv = NUTTER_GRID_GET_PRIVATE (self);
  return priv->column_major;
}

void
nutter_grid_set_column_gap (NutterGrid    *self,
                          ClutterUnit  value)
{
  NutterGridPrivate *priv = NUTTER_GRID_GET_PRIVATE (self);
  priv->column_gap = value;
  clutter_actor_queue_relayout (CLUTTER_ACTOR (self));
}

ClutterUnit
nutter_grid_get_column_gap (NutterGrid *self)
{
  NutterGridPrivate *priv = NUTTER_GRID_GET_PRIVATE (self);
  return priv->column_gap;
}



void
nutter_grid_set_row_gap (NutterGrid    *self,
                       ClutterUnit  value)
{
  NutterGridPrivate *priv = NUTTER_GRID_GET_PRIVATE (self);
  priv->row_gap = value;
  clutter_actor_queue_relayout (CLUTTER_ACTOR (self));
}

ClutterUnit
nutter_grid_get_row_gap (NutterGrid *self)
{
  NutterGridPrivate *priv = NUTTER_GRID_GET_PRIVATE (self);
  return priv->row_gap;
}


void
nutter_grid_set_valign (NutterGrid *self,
                      gdouble   value)
{
  NutterGridPrivate *priv = NUTTER_GRID_GET_PRIVATE (self);
  priv->valign = value;
  clutter_actor_queue_relayout (CLUTTER_ACTOR (self));
}

gdouble
nutter_grid_get_valign (NutterGrid *self)
{
  NutterGridPrivate *priv = NUTTER_GRID_GET_PRIVATE (self);
  return priv->valign;
}



void
nutter_grid_set_halign (NutterGrid *self,
                      gdouble   value)

{
  NutterGridPrivate *priv = NUTTER_GRID_GET_PRIVATE (self);
  priv->halign = value;
  clutter_actor_queue_relayout (CLUTTER_ACTOR (self));
}

gdouble
nutter_grid_get_halign (NutterGrid *self)
{
  NutterGridPrivate *priv = NUTTER_GRID_GET_PRIVATE (self);
  return priv->halign;
}


static void
nutter_grid_set_property (GObject      *object,
                        guint         prop_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  NutterGrid *grid = NUTTER_GRID (object);

  NutterGridPrivate *priv;

  priv = NUTTER_GRID_GET_PRIVATE (object);

  switch (prop_id)
    {
    case PROP_END_ALIGN:
      nutter_grid_set_end_align (grid, g_value_get_boolean (value));
      break;
    case PROP_HOMOGENOUS_ROWS:
      nutter_grid_set_homogenous_rows (grid, g_value_get_boolean (value));
      break;
    case PROP_HOMOGENOUS_COLUMNS:
      nutter_grid_set_homogenous_columns (grid, g_value_get_boolean (value));
      break;
    case PROP_COLUMN_MAJOR:
      nutter_grid_set_column_major (grid, g_value_get_boolean (value));
      break;
    case PROP_COLUMN_GAP:
      nutter_grid_set_column_gap (grid, clutter_value_get_unit (value));
      break;
    case PROP_ROW_GAP:
      nutter_grid_set_row_gap (grid, clutter_value_get_unit (value));
      break;
    case PROP_VALIGN:
      nutter_grid_set_valign (grid, g_value_get_double (value));
      break;
    case PROP_HALIGN:
      nutter_grid_set_halign (grid, g_value_get_double (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
nutter_grid_get_property (GObject    *object,
                        guint       prop_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  NutterGrid *grid = NUTTER_GRID (object);

  NutterGridPrivate *priv;

  priv = NUTTER_GRID_GET_PRIVATE (object);

  switch (prop_id)
    {
    case PROP_HOMOGENOUS_ROWS:
      g_value_set_boolean (value, nutter_grid_get_homogenous_rows (grid));
      break;
    case PROP_HOMOGENOUS_COLUMNS:
      g_value_set_boolean (value, nutter_grid_get_homogenous_columns (grid));
      break;
    case PROP_END_ALIGN:
      g_value_set_boolean (value, nutter_grid_get_end_align (grid));
      break;
    case PROP_COLUMN_MAJOR:
      g_value_set_boolean (value, nutter_grid_get_column_major (grid));
      break;
    case PROP_COLUMN_GAP:
      clutter_value_set_unit (value, nutter_grid_get_column_gap (grid));
      break;
    case PROP_ROW_GAP:
      clutter_value_set_unit (value, nutter_grid_get_row_gap (grid));
      break;
    case PROP_VALIGN:
      g_value_set_double (value, nutter_grid_get_valign (grid));
      break;
    case PROP_HALIGN:
      g_value_set_double (value, nutter_grid_get_halign (grid));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
nutter_grid_free_actor_data (gpointer data)
{
  g_slice_free (NutterGridActorData, data);
}

ClutterActor *
nutter_grid_new (void)
{
  ClutterActor *self = g_object_new (NUTTER_TYPE_GRID, NULL);

  return self;
}

static void
nutter_grid_real_add (ClutterContainer *container,
                    ClutterActor     *actor)
{
  NutterGridPrivate *priv;
  NutterGridActorData *data;

  g_return_if_fail (NUTTER_IS_GRID (container));

  g_object_ref (actor);

  priv = NUTTER_GRID (container)->priv;

  clutter_actor_set_parent (actor, CLUTTER_ACTOR (container));

  data = g_slice_alloc0 (sizeof (NutterGridActorData));

  priv->list = g_list_append (priv->list, actor);
  g_hash_table_insert (priv->hash_table, actor, data);

  clutter_actor_queue_relayout (CLUTTER_ACTOR (container));

  g_object_unref (actor);
}

static void
nutter_grid_real_remove (ClutterContainer *container,
                       ClutterActor     *actor)
{
  NutterGrid *layout = NUTTER_GRID (container);
  NutterGridPrivate *priv = layout->priv;

  g_object_ref (actor);

  if (g_hash_table_remove (priv->hash_table, actor))
    {
      clutter_actor_unparent (actor);

      clutter_actor_queue_relayout (CLUTTER_ACTOR (layout));

      g_signal_emit_by_name (container, "actor-removed", actor);

      if (CLUTTER_ACTOR_IS_VISIBLE (CLUTTER_ACTOR (layout)))
        clutter_actor_queue_redraw (CLUTTER_ACTOR (layout));
    }
  priv->list = g_list_remove (priv->list, actor);

  g_object_unref (actor);
}

static void
nutter_grid_real_foreach (ClutterContainer *container,
                                   ClutterCallback callback,
                                   gpointer user_data)
{
  NutterGrid *layout = NUTTER_GRID (container);
  NutterGridPrivate *priv = layout->priv;

  g_list_foreach (priv->list, (GFunc) callback, user_data);
}

static void
nutter_grid_real_raise (ClutterContainer *container,
                                 ClutterActor *actor,
                                 ClutterActor *sibling)
{
  /* STUB */
}

static void
nutter_grid_real_lower (ClutterContainer *container,
                                 ClutterActor *actor,
                                 ClutterActor *sibling)
{
  /* STUB */
}

static void
nutter_grid_real_sort_depth_order (ClutterContainer *container)
{
  /* STUB */
}

static void
nutter_grid_paint (ClutterActor *actor)
{
  NutterGrid *layout = (NutterGrid *) actor;
  NutterGridPrivate *priv = layout->priv;
  GList *child_item;

  for (child_item = priv->list;
       child_item != NULL;
       child_item = child_item->next)
    {
      ClutterActor *child = child_item->data;

      g_assert (child != NULL);

      if (CLUTTER_ACTOR_IS_VISIBLE (child))
        clutter_actor_paint (child);
    }

}

static void
nutter_grid_pick (ClutterActor *actor,
                           const ClutterColor *color)
{
  /* Chain up so we get a bounding box pained (if we are reactive) */
  CLUTTER_ACTOR_CLASS (nutter_grid_parent_class)->pick (actor, color);

  /* Just forward to the paint call which in turn will trigger
   * the child actors also getting 'picked'.
   */
  if (CLUTTER_ACTOR_IS_VISIBLE (actor))
   nutter_grid_paint (actor);
}

static void
nutter_grid_get_preferred_width (ClutterActor *self,
                                          ClutterUnit for_height,
                                          ClutterUnit *min_width_p,
                                          ClutterUnit *natural_width_p)
{
  NutterGrid *layout = (NutterGrid *) self;
  NutterGridPrivate *priv = layout->priv;
  ClutterUnit natural_width;

  natural_width = CLUTTER_UNITS_FROM_INT (200);
  if (min_width_p)
    *min_width_p = natural_width;
  if (natural_width_p)
    *natural_width_p = natural_width;

  priv->pref_width = natural_width;
}

static void
nutter_grid_get_preferred_height (ClutterActor *self,
                                ClutterUnit for_width,
                                ClutterUnit *min_height_p,
                                ClutterUnit *natural_height_p)
{
  NutterGrid *layout = (NutterGrid *) self;
  NutterGridPrivate *priv = layout->priv;
  ClutterUnit natural_height;

  natural_height = CLUTTER_UNITS_FROM_INT (200);

  priv->for_width = for_width;
  priv->pref_height = natural_height;

  if (min_height_p)
    *min_height_p = natural_height;
  if (natural_height_p)
    *natural_height_p = natural_height;
}

static ClutterUnit
compute_row_height (GList                    *siblings,
                    ClutterUnit               best_yet,
                    ClutterUnit               current_a,
                    NutterGridPrivate *priv)
{
  GList *l;

  gboolean homogenous_a;
  gboolean homogenous_b;
  ClutterUnit gap;

  if (priv->column_major)
    {
      homogenous_b = priv->homogenous_columns;
      homogenous_a = priv->homogenous_rows;
      gap          = priv->row_gap;
    }
  else
    {
      homogenous_a = priv->homogenous_columns;
      homogenous_b = priv->homogenous_rows;
      gap          = priv->column_gap;
    }

  for (l = siblings; l != NULL; l = l->next)
    {
      ClutterActor *child = l->data;
      ClutterUnit natural_width, natural_height;
      gdouble scale_x, scale_y;

      /* each child will get as much space as they require */
      clutter_actor_get_preferred_size (CLUTTER_ACTOR (child),
                                        NULL, NULL,
                                        &natural_width, &natural_height);

      clutter_actor_get_scale (child, &scale_x, &scale_y);

      natural_width  = (ClutterUnit)((gdouble)natural_width  * scale_x);
      natural_height = (ClutterUnit)((gdouble)natural_height * scale_y);

      if (priv->column_major)
        {
          ClutterUnit temp = natural_height;
          natural_height = natural_width;
          natural_width = temp;
        }

      /* if the primary axis is homogenous, each additional item is the same
       * width */
      if (homogenous_a)
        natural_width = priv->max_extent_a;

      if (natural_height > best_yet)
        best_yet = natural_height;

      /* if the child is overflowing, we wrap to next line */
      if (current_a + natural_width + gap > priv->a_wrap)
        {
          return best_yet;
        }
      current_a += natural_width + gap;
    }
  return best_yet;
}




static ClutterUnit
compute_row_start (GList           *siblings,
                   ClutterUnit      start_x,
                   NutterGridPrivate *priv)
{
  ClutterUnit current_a = start_x;
  GList *l;

  gboolean homogenous_a;
  gboolean homogenous_b;
  ClutterUnit gap;

  if (priv->column_major)
    {
      homogenous_b = priv->homogenous_columns;
      homogenous_a = priv->homogenous_rows;
      gap          = priv->row_gap;
    }
  else
    {
      homogenous_a = priv->homogenous_columns;
      homogenous_b = priv->homogenous_rows;
      gap          = priv->column_gap;
    }

  for (l = siblings; l != NULL; l = l->next)
    {
      ClutterActor *child = l->data;
      ClutterUnit natural_width, natural_height;
      gdouble scale_x, scale_y;

      /* each child will get as much space as they require */
      clutter_actor_get_preferred_size (CLUTTER_ACTOR (child),
                                        NULL, NULL,
                                        &natural_width, &natural_height);


      clutter_actor_get_scale (child, &scale_x, &scale_y);

      natural_width  = (ClutterUnit)((gdouble)natural_width  * scale_x);
      natural_height = (ClutterUnit)((gdouble)natural_height * scale_y);

      if (priv->column_major)
        natural_width = natural_height;

      /* if the primary axis is homogenous, each additional item is the same width */
      if (homogenous_a)
        natural_width = priv->max_extent_a;

      /* if the child is overflowing, we wrap to next line */
      if (current_a + natural_width + gap > priv->a_wrap)
        {
          if (current_a == start_x)
            return start_x;
          return (priv->a_wrap - current_a);
        }
      current_a += natural_width + gap;
    }
  return (priv->a_wrap - current_a);
}

static void
nutter_grid_allocate (ClutterActor          *self,
                              const ClutterActorBox *box,
                              gboolean               absolute_origin_changed)
{
  NutterGrid *layout = (NutterGrid *) self;
  NutterGridPrivate *priv = layout->priv;

  ClutterUnit current_a;
  ClutterUnit current_b;
  ClutterUnit next_b;
  ClutterUnit agap;
  ClutterUnit bgap;

  gboolean homogenous_a;
  gboolean homogenous_b;
  gdouble  aalign;
  gdouble  balign;

  current_a = current_b = next_b = 0;

  GList *iter;

  /* chain up to set actor->allocation */
  CLUTTER_ACTOR_CLASS (nutter_grid_parent_class)
    ->allocate (self, box, absolute_origin_changed);

  priv->alloc_width = box->x2 - box->x1;
  priv->alloc_height = box->y2 - box->y1;
  priv->absolute_origin_changed = absolute_origin_changed;

  /* Make sure we have calculated the preferred size */
  /* what does this do? */
  clutter_actor_get_preferred_size (self, NULL, NULL, NULL, NULL);


  if (priv->column_major)
    {
      priv->a_wrap = priv->alloc_height;
      homogenous_b = priv->homogenous_columns;
      homogenous_a = priv->homogenous_rows;
      aalign = priv->valign;
      balign = priv->halign;
      agap          = priv->row_gap;
      bgap          = priv->column_gap;
    }
  else
    {
      priv->a_wrap = priv->alloc_width;
      homogenous_a = priv->homogenous_columns;
      homogenous_b = priv->homogenous_rows;
      aalign = priv->halign;
      balign = priv->valign;
      agap          = priv->column_gap;
      bgap          = priv->row_gap;
    }

  priv->max_extent_a = 0;
  priv->max_extent_b = 0;

  priv->first_of_batch = TRUE;

  if (homogenous_a ||
      homogenous_b)
    {
      for (iter = priv->list; iter; iter = iter->next)
        {
          ClutterActor *child = iter->data;
          ClutterUnit natural_width;
          ClutterUnit natural_height;
	  gdouble     scale_x, scale_y;

          /* each child will get as much space as they require */
          clutter_actor_get_preferred_size (CLUTTER_ACTOR (child),
                                            NULL, NULL,
                                            &natural_width, &natural_height);

	  /* We want scale taken into account for nutter use */
	  clutter_actor_get_scale (child, &scale_x, &scale_y);

	  natural_width  = (ClutterUnit)((gdouble)natural_width  * scale_x);
	  natural_height = (ClutterUnit)((gdouble)natural_height * scale_y);

          if (natural_width > priv->max_extent_a)
            priv->max_extent_a = natural_width;
          if (natural_height > priv->max_extent_b)
            priv->max_extent_b = natural_width;
        }
    }

  if (priv->column_major)
    {
      ClutterUnit temp = priv->max_extent_a;
      priv->max_extent_a = priv->max_extent_b;
      priv->max_extent_b = temp;
    }

  for (iter = priv->list; iter; iter=iter->next)
    {
      ClutterActor *child = iter->data;
      ClutterUnit natural_a;
      ClutterUnit natural_b;
      ClutterUnit real_a;
      ClutterUnit real_b;
      gdouble scale_x, scale_y;

      /*
       * each child will get as much space as they require
       *
       * For placing the children, we take into account their scale, that is
       * what the natural_a, _b values are for. However, we need to give the
       * child back an allocation that is unscalled, hence the real_a, real_b
       * values.
       */
      clutter_actor_get_preferred_size (CLUTTER_ACTOR (child),
                                        NULL, NULL,
                                        &real_a, &real_b);

      clutter_actor_get_scale (child, &scale_x, &scale_y);

      natural_a = (ClutterUnit)((gdouble)real_a  * scale_x);
      natural_b = (ClutterUnit)((gdouble)real_b * scale_y);

      if (priv->column_major) /* swap axes around if column is major */
        {
          ClutterUnit temp = natural_a;
          natural_a = natural_b;
          natural_b = temp;
        }

      /* if the child is overflowing, we wrap to next line */
      if (current_a + natural_a > priv->a_wrap ||
          (homogenous_a && current_a + priv->max_extent_a > priv->a_wrap))
        {
          current_b = next_b + bgap;
          current_a = 0;
          next_b = current_b + bgap;
          priv->first_of_batch = TRUE;
        }

      if (priv->end_align &&
          priv->first_of_batch)
        {
          current_a = compute_row_start (iter, current_a, priv);
          priv->first_of_batch = FALSE;
        }

      if (next_b-current_b < natural_b)
          next_b = current_b + natural_b;

        {
          ClutterUnit     row_height;
          ClutterActorBox child_box;

          if (homogenous_b)
            {
              row_height = priv->max_extent_b;
            }
          else
            {
              row_height = compute_row_height (iter, next_b-current_b,
                                               current_a, priv);
            }

          if (homogenous_a)
            {
              child_box.x1 = current_a + (priv->max_extent_a-natural_a) * aalign;
              child_box.x2 = child_box.x1 + real_a;

            }
          else
            {
              child_box.x1 = current_a;
              child_box.x2 = child_box.x1 + real_a;
            }

          child_box.y1 = current_b + (row_height-natural_b) * balign;
          child_box.y2 = child_box.y1 + real_b;


          if (priv->column_major)
            {
              ClutterUnit temp = child_box.x1;
              child_box.x1 = child_box.y1;
              child_box.y1 = temp;

              temp = child_box.x2;
              child_box.x2 = child_box.y2;
              child_box.y2 = temp;
            }

          /* update the allocation */
          clutter_actor_allocate (CLUTTER_ACTOR (child),
                                  &child_box,
                                  absolute_origin_changed);

          if (homogenous_a)
            {
              current_a += priv->max_extent_a + agap;
            }
          else
            {
              current_a += natural_a + agap;
            }
        }
    }
}
