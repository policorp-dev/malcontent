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

#pragma once

#include <adwaita.h>


G_BEGIN_DECLS

#define MCT_TYPE_CAROUSEL_ITEM (mct_carousel_item_get_type ())
G_DECLARE_FINAL_TYPE (MctCarouselItem, mct_carousel_item, MCT, CAROUSEL_ITEM, GtkButton)

#define MCT_TYPE_CAROUSEL (mct_carousel_get_type ())
G_DECLARE_FINAL_TYPE (MctCarousel, mct_carousel, MCT, CAROUSEL, AdwBin)

GtkWidget       *mct_carousel_item_new       (void);

void             mct_carousel_item_set_child (MctCarouselItem *self,
                                              GtkWidget       *child);

MctCarousel     *mct_carousel_new            (void);

void             mct_carousel_purge_items    (MctCarousel     *self);

MctCarouselItem *mct_carousel_find_item      (MctCarousel     *self,
                                              gconstpointer   data,
                                              GCompareFunc    func);

void             mct_carousel_select_item    (MctCarousel     *self,
                                              MctCarouselItem *item);

guint            mct_carousel_get_item_count (MctCarousel  *self);

void             mct_carousel_add            (MctCarousel     *self,
                                              MctCarouselItem *item);

void             mct_carousel_set_revealed   (MctCarousel     *self,
                                              gboolean         revealed);

G_END_DECLS
