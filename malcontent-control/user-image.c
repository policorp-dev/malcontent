/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 *
 * Copyright © 2022 Endless Mobile, Inc.
 * Copyright © 2015 Red Hat, Inc.
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
 *  - Georges Basile Stavracas Neto <georges@endlessos.org>
 *  - Ondrej Holy <oholy@redhat.com>
 */

#include <adwaita.h>
#include <act/act.h>
#include <sys/stat.h>

#include "user-image.h"


struct _MctUserImage
{
  AdwBin parent_instance;

  AdwAvatar *avatar;

  ActUser *user;
};

G_DEFINE_TYPE (MctUserImage, mct_user_image, ADW_TYPE_BIN)

static GdkTexture *
render_user_icon_texture (ActUser *user)
{
  g_autoptr(GdkTexture) texture = NULL;
  g_autoptr(GError) error = NULL;
  const gchar *icon_file;

  g_return_val_if_fail (ACT_IS_USER (user), NULL);

  icon_file = act_user_get_icon_file (user);
  if (icon_file == NULL)
    return NULL;

  texture = gdk_texture_new_from_filename (icon_file, &error);
  if (error != NULL)
    {
      g_warning ("Error loading user icon: %s", error->message);
      return NULL;
    }

  return g_steal_pointer (&texture);
}

static void
render_image (MctUserImage *image)
{
  g_autoptr(GdkTexture) texture = NULL;

  if (image->user == NULL)
    return;

  texture = render_user_icon_texture (image->user);
  adw_avatar_set_custom_image (image->avatar, GDK_PAINTABLE (texture));
  adw_avatar_set_text (image->avatar, act_user_get_real_name (image->user));
}

void
mct_user_image_set_user (MctUserImage *image,
                         ActUser      *user)
{
  g_clear_object (&image->user);
  image->user = g_object_ref (user);

  render_image (image);
}

static void
mct_user_image_finalize (GObject *object)
{
  MctUserImage *image = MCT_USER_IMAGE (object);

  g_clear_object (&image->user);

  G_OBJECT_CLASS (mct_user_image_parent_class)->finalize (object);
}

static void
mct_user_image_class_init (MctUserImageClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = mct_user_image_finalize;
}

static void
mct_user_image_init (MctUserImage *image)
{
  image->avatar = ADW_AVATAR (adw_avatar_new (48, NULL, TRUE));
  adw_bin_set_child (ADW_BIN (image), GTK_WIDGET (image->avatar));
}

GtkWidget *
mct_user_image_new (void)
{
  return g_object_new (MCT_TYPE_USER_IMAGE, NULL);
}
