/* gcal-config-loader.c
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

#define G_LOG_DOMAIN "GcalConfigLoader"

#include "gcal-config-loader.h"
#include "gcal-debug.h"

#include <glib/gstdio.h>
#include <libsoup/soup.h>
#include <string.h>

#define CONFIG_GROUP "CalDAV"

/**
 * gcal_config_loader_get_config_path:
 *
 * Get the path to the configuration file.
 *
 * Returns: (transfer full): Path to config file
 */
gchar*
gcal_config_loader_get_config_path (void)
{
  const gchar *env_config;

  GCAL_ENTRY;

  /* Check environment variable first */
  env_config = g_getenv ("GNOME_CALENDAR_CONFIG");
  if (env_config && *env_config)
    GCAL_RETURN (g_strdup (env_config));

  /* Fall back to XDG config directory */
  GCAL_RETURN (g_build_filename (g_get_user_config_dir (),
                                 "gnome-calendar",
                                 "account.conf",
                                 NULL));
}

/**
 * gcal_config_loader_has_config:
 *
 * Check if a configuration file exists.
 *
 * Returns: %TRUE if config file exists
 */
gboolean
gcal_config_loader_has_config (void)
{
  g_autofree gchar *config_path = NULL;

  GCAL_ENTRY;

  config_path = gcal_config_loader_get_config_path ();
  GCAL_RETURN (g_file_test (config_path, G_FILE_TEST_EXISTS));
}

/**
 * read_password_from_file:
 * @password_file: Path to password file
 * @error: Return location for error
 *
 * Read password from a file (e.g., from agenix).
 *
 * Returns: (transfer full): Password string, or %NULL on error
 */
static gchar*
read_password_from_file (const gchar  *password_file,
                         GError      **error)
{
  g_autofree gchar *contents = NULL;
  gsize length;

  GCAL_ENTRY;

  if (!g_file_get_contents (password_file, &contents, &length, error))
    GCAL_RETURN (NULL);

  /* Remove trailing newlines/whitespace */
  g_strchomp (contents);

  if (length == 0)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_FAILED,
                   "Password file is empty: %s",
                   password_file);
      GCAL_RETURN (NULL);
    }

  GCAL_RETURN (g_steal_pointer (&contents));
}

/**
 * gcal_config_loader_load_account:
 * @error: Return location for error
 *
 * Load CalDAV account configuration from file.
 *
 * Returns: (transfer full): Account configuration, or %NULL on error
 */
GcalAccountConfig*
gcal_config_loader_load_account (GError **error)
{
  g_autoptr (GKeyFile) key_file = NULL;
  g_autofree gchar *config_path = NULL;
  g_autofree gchar *password_file = NULL;
  GcalAccountConfig *config;

  GCAL_ENTRY;

  config_path = gcal_config_loader_get_config_path ();

  if (!g_file_test (config_path, G_FILE_TEST_EXISTS))
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_NOT_FOUND,
                   "Configuration file not found: %s",
                   config_path);
      GCAL_RETURN (NULL);
    }

  key_file = g_key_file_new ();

  if (!g_key_file_load_from_file (key_file, config_path, G_KEY_FILE_NONE, error))
    GCAL_RETURN (NULL);

  config = g_new0 (GcalAccountConfig, 1);

  /* Load configuration values */
  config->enabled = g_key_file_get_boolean (key_file, CONFIG_GROUP, "Enabled", NULL);

  if (!config->enabled)
    {
      g_debug ("Account is disabled in configuration");
      GCAL_RETURN (config);
    }

  config->display_name = g_key_file_get_string (key_file, CONFIG_GROUP, "DisplayName", NULL);
  config->server_url = g_key_file_get_string (key_file, CONFIG_GROUP, "ServerURL", NULL);
  config->username = g_key_file_get_string (key_file, CONFIG_GROUP, "Username", NULL);
  config->color = g_key_file_get_string (key_file, CONFIG_GROUP, "Color", NULL);
  config->trust_self_signed_cert = g_key_file_get_boolean (key_file, CONFIG_GROUP, "TrustSelfSignedCert", NULL);

  /* Validate required fields */
  if (!config->display_name || !config->server_url)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_FAILED,
                   "Missing required fields (DisplayName or ServerURL) in configuration");
      gcal_account_config_free (config);
      GCAL_RETURN (NULL);
    }

  /* Read password from file */
  password_file = g_key_file_get_string (key_file, CONFIG_GROUP, "PasswordFile", NULL);

  if (!password_file)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_FAILED,
                   "PasswordFile not specified in configuration");
      gcal_account_config_free (config);
      GCAL_RETURN (NULL);
    }

  config->password = read_password_from_file (password_file, error);

  if (!config->password)
    {
      gcal_account_config_free (config);
      GCAL_RETURN (NULL);
    }

  /* Set default color if not specified */
  if (!config->color)
    config->color = g_strdup ("#3584e4");

  GCAL_RETURN (config);
}

/**
 * gcal_account_config_free:
 * @config: Config to free
 *
 * Free account configuration.
 */
void
gcal_account_config_free (GcalAccountConfig *config)
{
  if (!config)
    return;

  g_clear_pointer (&config->display_name, g_free);
  g_clear_pointer (&config->server_url, g_free);
  g_clear_pointer (&config->username, g_free);
  g_clear_pointer (&config->color, g_free);

  /* Securely clear password from memory */
  if (config->password)
    {
      memset (config->password, 0, strlen (config->password));
      g_clear_pointer (&config->password, g_free);
    }

  g_free (config);
}

/**
 * gcal_config_loader_create_caldav_source:
 * @config: Account configuration
 * @registry: ESourceRegistry
 * @error: Return location for error
 *
 * Create a CalDAV ESource from configuration.
 *
 * Returns: (transfer full): New ESource, or %NULL on error
 */
ESource*
gcal_config_loader_create_caldav_source (GcalAccountConfig  *config,
                                         ESourceRegistry    *registry,
                                         GError            **error)
{
  g_autoptr (GUri) uri = NULL;
  g_autoptr (ESource) source = NULL;
  ESourceAuthentication *auth;
  ESourceWebdav *webdav;
  ESourceSecurity *security;
  ESourceOffline *offline;
  ESourceRefresh *refresh;
  ESourceCalendar *calendar;
  const gchar *host;
  const gchar *path;
  const gchar *scheme;
  const gchar *user;
  const gchar *userinfo;
  gint port;
  g_autofree gchar *user_from_uri = NULL;

  GCAL_ENTRY;

  g_return_val_if_fail (config != NULL, NULL);
  g_return_val_if_fail (E_IS_SOURCE_REGISTRY (registry), NULL);

  /* Parse server URL */
  uri = g_uri_parse (config->server_url, G_URI_FLAGS_PARSE_RELAXED, error);

  if (!uri)
    GCAL_RETURN (NULL);

  scheme = g_uri_get_scheme (uri);
  host = g_uri_get_host (uri);
  port = g_uri_get_port (uri);
  path = g_uri_get_path (uri);
  user = config->username;
  userinfo = g_uri_get_userinfo (uri);

  if ((!user || !*user) && userinfo && *userinfo)
    {
      user_from_uri = g_strdup (userinfo);
      if (user_from_uri)
        {
          gchar *colon = strchr (user_from_uri, ':');
          if (colon)
            *colon = '\0';
        }
      user = user_from_uri;
    }

  if (!host || !*host)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_FAILED,
                   "Invalid server URL: missing host");
      GCAL_RETURN (NULL);
    }

  /* Default port based on scheme */
  if (port == -1)
    {
      if (g_strcmp0 (scheme, "https") == 0)
        port = 443;
      else if (g_strcmp0 (scheme, "http") == 0)
        port = 80;
      else
        port = 443; /* Default to HTTPS */
    }

  /* Create source */
  source = e_source_new (NULL, NULL, error);

  if (!source)
    GCAL_RETURN (NULL);

  /* Set basic properties */
  e_source_set_display_name (source, config->display_name);
  e_source_set_parent (source, "caldav-stub");

  /* Configure Calendar extension */
  calendar = e_source_get_extension (source, E_SOURCE_EXTENSION_CALENDAR);
  e_source_backend_set_backend_name (E_SOURCE_BACKEND (calendar), "caldav");
  e_source_selectable_set_color (E_SOURCE_SELECTABLE (calendar), config->color);

  /* Configure Authentication extension */
  auth = e_source_get_extension (source, E_SOURCE_EXTENSION_AUTHENTICATION);
  e_source_authentication_set_host (auth, host);
  e_source_authentication_set_port (auth, port);
  if (user && *user)
    e_source_authentication_set_user (auth, user);
  e_source_authentication_set_method (auth, "plain/password");

  /* Configure WebDAV extension */
  webdav = e_source_get_extension (source, E_SOURCE_EXTENSION_WEBDAV_BACKEND);
  e_source_webdav_set_display_name (webdav, config->display_name);
  e_source_webdav_set_resource_path (webdav, path ? path : "/");

  /* Note: SSL trust handling simplified - trust all if requested */
  if (config->trust_self_signed_cert)
    {
      /* Set SSL trust to allow self-signed certificates */
      /* In newer versions, this might need a different approach */
      g_warning ("Self-signed certificate trust requested but may need manual setup");
    }

  /* Configure Security extension */
  if (g_strcmp0 (scheme, "https") == 0)
    {
      security = e_source_get_extension (source, E_SOURCE_EXTENSION_SECURITY);
      e_source_security_set_method (security, "tls");
    }

  /* Configure Offline extension (enable sync) */
  offline = e_source_get_extension (source, E_SOURCE_EXTENSION_OFFLINE);
  e_source_offline_set_stay_synchronized (offline, TRUE);

  /* Configure Refresh extension (sync every 30 minutes) */
  refresh = e_source_get_extension (source, E_SOURCE_EXTENSION_REFRESH);
  e_source_refresh_set_enabled (refresh, TRUE);
  e_source_refresh_set_interval_minutes (refresh, 30);

  g_debug ("Created CalDAV source: %s @ %s:%d%s",
           user && *user ? user : "(none)", host, port, path ? path : "/");

  GCAL_RETURN (g_steal_pointer (&source));
}
