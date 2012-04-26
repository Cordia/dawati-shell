/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
#include "st-private.h"

/* Utility function to modify a child allocation box with respect to the
 * x/y-fill child properties. Expects childbox to contain the available
 * allocation space.
 */
void
_st_allocate_fill (ClutterActor    *child,
                   ClutterActorBox *childbox,
                   StAlign          x_alignment,
                   StAlign          y_alignment,
                   gboolean         x_fill,
                   gboolean         y_fill)
{
  gfloat natural_width, natural_height;
  gfloat min_width, min_height;
  gfloat child_width, child_height;
  gfloat available_width, available_height;
  ClutterRequestMode request;
  ClutterActorBox allocation = { 0, };
  gdouble x_align, y_align;

  if (x_alignment == ST_ALIGN_START)
    x_align = 0.0;
  else if (x_alignment == ST_ALIGN_MIDDLE)
    x_align = 0.5;
  else
    x_align = 1.0;

  if (y_alignment == ST_ALIGN_START)
    y_align = 0.0;
  else if (y_alignment == ST_ALIGN_MIDDLE)
    y_align = 0.5;
  else
    y_align = 1.0;

  available_width  = childbox->x2 - childbox->x1;
  available_height = childbox->y2 - childbox->y1;

  if (available_width < 0)
    available_width = 0;

  if (available_height < 0)
    available_height = 0;

  if (x_fill)
    {
      allocation.x1 = childbox->x1;
      allocation.x2 = (int)(allocation.x1 + available_width);
    }

  if (y_fill)
    {
      allocation.y1 = childbox->y1;
      allocation.y2 = (int)(allocation.y1 + available_height);
    }

  /* if we are filling horizontally and vertically then we're done */
  if (x_fill && y_fill)
    {
      *childbox = allocation;
      return;
    }

  request = CLUTTER_REQUEST_HEIGHT_FOR_WIDTH;
  g_object_get (G_OBJECT (child), "request-mode", &request, NULL);

  if (request == CLUTTER_REQUEST_HEIGHT_FOR_WIDTH)
    {
      clutter_actor_get_preferred_width (child, available_height,
                                         &min_width,
                                         &natural_width);

      child_width = CLAMP (natural_width, min_width, available_width);

      clutter_actor_get_preferred_height (child, child_width,
                                          &min_height,
                                          &natural_height);

      child_height = CLAMP (natural_height, min_height, available_height);
    }
  else
    {
      clutter_actor_get_preferred_height (child, available_width,
                                          &min_height,
                                          &natural_height);

      child_height = CLAMP (natural_height, min_height, available_height);

      clutter_actor_get_preferred_width (child, child_height,
                                         &min_width,
                                         &natural_width);

      child_width = CLAMP (natural_width, min_width, available_width);
    }

  if (!x_fill)
    {
      allocation.x1 = childbox->x1 + (int)((available_width - child_width) * x_align);
      allocation.x2 = allocation.x1 + (int) child_width;
    }

  if (!y_fill)
    {
      allocation.y1 = childbox->y1 + (int)((available_height - child_height) * y_align);
      allocation.y2 = allocation.y1 + (int) child_height;
    }

  *childbox = allocation;

}

/**
 * _st_set_text_from_style:
 * @text: Target #ClutterText
 * @theme_node: Source #StThemeNode
 *
 * Set various GObject properties of the @text object using
 * CSS information from @theme_node.
 */
void
_st_set_text_from_style (ClutterText *text,
                         StThemeNode *theme_node)
{

  ClutterColor color;
  StTextDecoration decoration;
  PangoAttrList *attribs;
  const PangoFontDescription *font;
  gchar *font_string;

  st_theme_node_get_foreground_color (theme_node, &color);
  clutter_text_set_color (text, &color);

  font = st_theme_node_get_font (theme_node);
  font_string = pango_font_description_to_string (font);
  clutter_text_set_font_name (text, font_string);
  g_free (font_string);

  attribs = pango_attr_list_new ();

  decoration = st_theme_node_get_text_decoration (theme_node);
  if (decoration & ST_TEXT_DECORATION_UNDERLINE)
    {
      PangoAttribute *underline = pango_attr_underline_new (PANGO_UNDERLINE_SINGLE);
      pango_attr_list_insert (attribs, underline);
    }
  if (decoration & ST_TEXT_DECORATION_LINE_THROUGH)
    {
      PangoAttribute *strikethrough = pango_attr_strikethrough_new (TRUE);
      pango_attr_list_insert (attribs, strikethrough);
    }
  /* Pango doesn't have an equivalent attribute for _OVERLINE, and we deliberately
   * skip BLINK (for now...)
   */

  clutter_text_set_attributes (text, attribs);

  pango_attr_list_unref (attribs);
}
