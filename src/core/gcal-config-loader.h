/* gcal-config-loader.h
 *
 * Copyright 2025 The GNOME Calendar contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <glib.h>
#include <libedataserver/libedataserver.h>

G_BEGIN_DECLS

/**
 * GcalAccountConfig:
 * @display_name: Display name for the calendar
 * @server_url: CalDAV server URL
 * @username: CalDAV username (optional; can be derived from URL)
 * @password: CalDAV password (read from file)
 * @color: Calendar color in hex format
 * @enabled: Whether this account is enabled
 * @trust_self_signed_cert: Whether to trust self-signed SSL certificates
 *
 * Configuration for a declarative CalDAV account.
 */
typedef struct
{
  gchar    *display_name;
  gchar    *server_url;
  gchar    *username;
  gchar    *password;
  gchar    *color;
  gboolean  enabled;
  gboolean  trust_self_signed_cert;
} GcalAccountConfig;

gchar*              gcal_config_loader_get_config_path      (void);

gboolean            gcal_config_loader_has_config           (void);

GcalAccountConfig*  gcal_config_loader_load_account         (GError **error);

void                gcal_account_config_free                (GcalAccountConfig *config);

ESource*            gcal_config_loader_create_caldav_source (GcalAccountConfig  *config,
                                                             ESourceRegistry    *registry,
                                                             GError            **error);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (GcalAccountConfig, gcal_account_config_free)

G_END_DECLS
