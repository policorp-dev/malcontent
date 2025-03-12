/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 *
 * Copyright © 2016 Red Hat, Inc.
 * Copyright © 2019 Endless Mobile, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *  - Felipe Borges <felipeborges@gnome.org>
 *  - Georges Basile Stavracas Neto <georges@endlessos.org>
 *  - Philip Withnall <withnall@endlessm.com>
 */

#include <glib-object.h>
#include <gtk/gtk.h>

#include "carousel.h"


#define ARROW_SIZE 20

#define MCT_TYPE_CAROUSEL_LAYOUT (mct_carousel_layout_get_type ())
G_DECLARE_FINAL_TYPE (MctCarouselLayout, mct_carousel_layout, MCT, CAROUSEL_LAYOUT, GtkLayoutManager)

struct _MctCarouselItem {
  GtkButton parent;

  gint page;
};

G_DEFINE_TYPE (MctCarouselItem, mct_carousel_item, GTK_TYPE_BUTTON)

GtkWidget *
mct_carousel_item_new (void)
{
  return g_object_new (MCT_TYPE_CAROUSEL_ITEM, NULL);
}

void
mct_carousel_item_set_child (MctCarouselItem *self,
                             GtkWidget       *child)
{
  g_return_if_fail (MCT_IS_CAROUSEL_ITEM (self));

  gtk_button_set_child (GTK_BUTTON (self), child);
}

static void
mct_carousel_item_class_init (MctCarouselItemClass *klass)
{
  gtk_widget_class_set_css_name (GTK_WIDGET_CLASS (klass), "carousel-item");
}

static void
mct_carousel_item_init (MctCarouselItem *self)
{
}

struct _MctCarousel {
  AdwBin parent;

  GtkRevealer *revealer;

  GList *children;
  gint visible_page;
  MctCarouselItem *selected_item;
  GtkWidget *last_box;
  GtkWidget *arrow;

  /* Widgets */
  GtkStack *stack;
  GtkWidget *go_back_button;
  GtkWidget *go_next_button;

  GtkStyleProvider *provider;
};

G_DEFINE_TYPE (MctCarousel, mct_carousel, ADW_TYPE_BIN)

enum {
  ITEM_ACTIVATED,
  NUM_SIGNALS
};

static guint signals[NUM_SIGNALS] = { 0, };

#define ITEMS_PER_PAGE 3

static gint
mct_carousel_item_get_x (MctCarouselItem *item,
                         MctCarousel     *carousel)
{
  GtkWidget *widget, *parent;
  gint width;
  gdouble dest_x;
  graphene_point_t p;

  parent = GTK_WIDGET (carousel->revealer);
  widget = GTK_WIDGET (item);

  width = gtk_widget_get_width (widget);
  if (!gtk_widget_compute_point (widget, parent,
                                 &GRAPHENE_POINT_INIT (width / 2, 0),
                                 &p))
    return 0;

  dest_x = p.x;

  return CLAMP (dest_x - ARROW_SIZE,
                0,
                gtk_widget_get_width (parent));
}

static void
mct_carousel_move_arrow (MctCarousel *self)
{
  gchar *css;
  gint end_x;

  if (!self->selected_item)
    return;

  end_x = mct_carousel_item_get_x (self->selected_item, self);

  if (self->provider)
    gtk_style_context_remove_provider_for_display (gtk_widget_get_display (self->arrow), self->provider);
  g_clear_object (&self->provider);

  css = g_strdup_printf (".carousel-arrow { margin-left: %dpx; }", end_x);
  self->provider = GTK_STYLE_PROVIDER (gtk_css_provider_new ());
  gtk_css_provider_load_from_string (GTK_CSS_PROVIDER (self->provider), css);
  gtk_style_context_add_provider_for_display (gtk_widget_get_display (self->arrow),
                                              self->provider,
                                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  g_free (css);
}

static gint
get_last_page_number (MctCarousel *self)
{
  if (g_list_length (self->children) == 0)
    return 0;

  return ((g_list_length (self->children) - 1) / ITEMS_PER_PAGE);
}

static void
update_buttons_visibility (MctCarousel *self)
{
  gtk_widget_set_visible (self->go_back_button, (self->visible_page > 0));
  gtk_widget_set_visible (self->go_next_button, (self->visible_page < get_last_page_number (self)));
}

/**
 * mct_carousel_find_item:
 * @carousel: an MctCarousel instance
 * @data: user data passed to the comparison function
 * @func: the function to call for each element.
 *      It should return 0 when the desired element is found
 *
 * Finds an MctCarousel item using the supplied function to find the
 * desired element.
 * Ideally useful for matching a model object and its correspondent
 * widget.
 *
 * Returns: the found MctCarouselItem, or %NULL if it is not found
 */
MctCarouselItem *
mct_carousel_find_item (MctCarousel   *self,
                        gconstpointer  data,
                        GCompareFunc   func)
{
  GList *list;

  list = self->children;
  while (list != NULL)
    {
      if (!func (list->data, data))
        return list->data;
      list = list->next;
    }

  return NULL;
}

static void
on_item_toggled (MctCarouselItem *item,
                 GdkEvent        *event,
                 gpointer         user_data)
{
  MctCarousel *self = MCT_CAROUSEL (user_data);

  mct_carousel_select_item (self, item);
}

void
mct_carousel_select_item (MctCarousel     *self,
                          MctCarouselItem *item)
{
  gchar *page_name;
  gboolean page_changed = TRUE;

  /* Select first user if none is specified */
  if (item == NULL)
    {
      if (self->children != NULL)
        item = self->children->data;
      else
        return;
    }

  if (self->selected_item != NULL)
    page_changed = (self->selected_item->page != item->page);

  self->selected_item = item;
  self->visible_page = item->page;
  g_signal_emit (self, signals[ITEM_ACTIVATED], 0, item);

  if (!page_changed)
    {
      mct_carousel_move_arrow (self);
      return;
    }

  page_name = g_strdup_printf ("%d", self->visible_page);
  gtk_stack_set_visible_child_name (self->stack, page_name);

  g_free (page_name);

  update_buttons_visibility (self);

  /* mct_carousel_move_arrow is called from on_transition_running */
}

static void
mct_carousel_select_item_at_index (MctCarousel *self,
                                   gint         index)
{
  GList *l = NULL;

  l = g_list_nth (self->children, index);
  mct_carousel_select_item (self, l->data);
}

static void
mct_carousel_goto_previous_page (GtkWidget *button,
                                 gpointer   user_data)
{
  MctCarousel *self = MCT_CAROUSEL (user_data);

  self->visible_page--;
  if (self->visible_page < 0)
    self->visible_page = 0;

  /* Select first item of the page */
  mct_carousel_select_item_at_index (self, self->visible_page * ITEMS_PER_PAGE);
}

static void
mct_carousel_goto_next_page (GtkWidget *button,
                             gpointer   user_data)
{
  MctCarousel *self = MCT_CAROUSEL (user_data);
  gint last_page;

  last_page = get_last_page_number (self);

  self->visible_page++;
  if (self->visible_page > last_page)
    self->visible_page = last_page;

  /* Select first item of the page */
  mct_carousel_select_item_at_index (self, self->visible_page * ITEMS_PER_PAGE);
}

void
mct_carousel_add (MctCarousel     *self,
                  MctCarouselItem *item)
{
  gboolean last_box_is_full;

  g_return_if_fail (MCT_IS_CAROUSEL (self));
  g_return_if_fail (MCT_IS_CAROUSEL_ITEM (item));

  self->children = g_list_append (self->children, item);
  item->page = get_last_page_number (self);
  g_signal_connect (item, "clicked", G_CALLBACK (on_item_toggled), self);

  last_box_is_full = ((g_list_length (self->children) - 1) % ITEMS_PER_PAGE == 0);
  if (last_box_is_full)
    {
      g_autofree gchar *page = NULL;

      page = g_strdup_printf ("%d", item->page);
      self->last_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 18);
      gtk_widget_set_hexpand (self->last_box, TRUE);
      gtk_widget_set_valign (self->last_box, GTK_ALIGN_CENTER);
      gtk_box_set_homogeneous (GTK_BOX (self->last_box), TRUE);
      gtk_stack_add_named (self->stack, self->last_box, page);
    }

  gtk_box_append (GTK_BOX (self->last_box), GTK_WIDGET (item));

  update_buttons_visibility (self);
}

void
mct_carousel_purge_items (MctCarousel *self)
{
  GtkWidget *child;

  while ((child = gtk_widget_get_first_child (GTK_WIDGET (self->stack))) != NULL)
    gtk_stack_remove (self->stack, child);

  g_list_free (self->children);
  self->children = NULL;
  self->visible_page = 0;
  self->selected_item = NULL;
}

MctCarousel *
mct_carousel_new (void)
{
  return g_object_new (MCT_TYPE_CAROUSEL, NULL);
}

static void
mct_carousel_dispose (GObject *object)
{
  MctCarousel *self = MCT_CAROUSEL (object);

  g_clear_object (&self->provider);
  if (self->children != NULL)
    {
      g_list_free (self->children);
      self->children = NULL;
    }

  G_OBJECT_CLASS (mct_carousel_parent_class)->dispose (object);
}

static void
mct_carousel_class_init (MctCarouselClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wclass = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (wclass,
                                               "/org/freedesktop/MalcontentControl/ui/carousel.ui");

  gtk_widget_class_bind_template_child (wclass, MctCarousel, stack);
  gtk_widget_class_bind_template_child (wclass, MctCarousel, go_back_button);
  gtk_widget_class_bind_template_child (wclass, MctCarousel, go_next_button);
  gtk_widget_class_bind_template_child (wclass, MctCarousel, arrow);
  gtk_widget_class_bind_template_child (wclass, MctCarousel, revealer);

  gtk_widget_class_bind_template_callback (wclass, mct_carousel_goto_previous_page);
  gtk_widget_class_bind_template_callback (wclass, mct_carousel_goto_next_page);

  gtk_widget_class_set_layout_manager_type (wclass, MCT_TYPE_CAROUSEL_LAYOUT);

  object_class->dispose = mct_carousel_dispose;

  signals[ITEM_ACTIVATED] =
      g_signal_new ("item-activated",
                    MCT_TYPE_CAROUSEL,
                    G_SIGNAL_RUN_LAST,
                    0,
                    NULL, NULL,
                    g_cclosure_marshal_VOID__OBJECT,
                    G_TYPE_NONE, 1,
                    MCT_TYPE_CAROUSEL_ITEM);
}

static void
on_transition_running (MctCarousel *self)
{
  if (!gtk_stack_get_transition_running (self->stack))
    mct_carousel_move_arrow (self);
}

static void
mct_carousel_init (MctCarousel *self)
{
  GtkStyleProvider *provider;

  gtk_widget_init_template (GTK_WIDGET (self));

  provider = GTK_STYLE_PROVIDER (gtk_css_provider_new ());
  gtk_css_provider_load_from_resource (GTK_CSS_PROVIDER (provider),
                                       "/org/freedesktop/MalcontentControl/ui/carousel.css");

  gtk_style_context_add_provider_for_display (gdk_display_get_default (),
                                              provider,
                                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION - 1);

  g_object_unref (provider);

  g_signal_connect_swapped (self->stack, "notify::transition-running", G_CALLBACK (on_transition_running), self);
}

guint
mct_carousel_get_item_count (MctCarousel *self)
{
  return g_list_length (self->children);
}

void
mct_carousel_set_revealed (MctCarousel *self,
                           gboolean     revealed)
{
  g_return_if_fail (MCT_IS_CAROUSEL (self));

  gtk_revealer_set_reveal_child (self->revealer, revealed);
}

struct _MctCarouselLayout {
  GtkLayoutManager parent;
};

G_DEFINE_FINAL_TYPE (MctCarouselLayout, mct_carousel_layout, GTK_TYPE_LAYOUT_MANAGER)

static void
mct_carousel_layout_measure (GtkLayoutManager *layout_manager,
                             GtkWidget        *widget,
                             GtkOrientation    orientation,
                             int               for_size,
                             int              *minimum,
                             int              *natural,
                             int              *minimum_baseline,
                             int              *natural_baseline)
{
  MctCarousel *carousel;

  g_assert (MCT_IS_CAROUSEL (widget));

  carousel = MCT_CAROUSEL (widget);

  gtk_widget_measure (GTK_WIDGET (carousel->revealer),
                      orientation, for_size,
                      minimum, natural,
                      minimum_baseline, natural_baseline);
}

static void
mct_carousel_layout_allocate (GtkLayoutManager *layout_manager,
                              GtkWidget        *widget,
                              int               width,
                              int               height,
                              int               baseline)
{
  MctCarousel *carousel;

  g_assert (MCT_IS_CAROUSEL (widget));

  carousel = MCT_CAROUSEL (widget);
  gtk_widget_allocate (GTK_WIDGET (carousel->revealer), width, height, baseline, NULL);

  if (carousel->selected_item == NULL)
    return;

  if (gtk_stack_get_transition_running (carousel->stack))
    return;

  mct_carousel_move_arrow (carousel);
}

static void
mct_carousel_layout_class_init (MctCarouselLayoutClass *klass)
{
  GtkLayoutManagerClass *layout_manager_class = GTK_LAYOUT_MANAGER_CLASS (klass);

  layout_manager_class->measure = mct_carousel_layout_measure;
  layout_manager_class->allocate = mct_carousel_layout_allocate;
}

static void
mct_carousel_layout_init (MctCarouselLayout *self)
{
}
