{ config, lib, pkgs, ... }:

with lib;

let
  cfg = config.programs.gnome-calendar;

  mkAccountConfig = account: ''
    [CalDAV]
    Enabled=${boolToString account.enable}
    DisplayName=${account.displayName}
    ServerURL=${account.serverUrl}
    ${optionalString (account.username != null) "Username=${account.username}"}
    PasswordFile=${account.passwordFile}
    Color=${account.color}
    ${optionalString account.trustSelfSignedCert "TrustSelfSignedCert=true"}
  '';

in
{
  options.programs.gnome-calendar = {
    enable = mkEnableOption "GNOME Calendar";

    package = mkOption {
      type = types.package;
      default = pkgs.gnome-calendar or pkgs.gnome.gnome-calendar;
      defaultText = literalExpression "pkgs.gnome-calendar";
      description = ''
        The GNOME Calendar package to use.
        By default, uses the system package.
        Set to the flake package for declarative CalDAV support.
      '';
    };

    caldav = mkOption {
      type = types.nullOr (types.submodule {
        options = {
          enable = mkOption {
            type = types.bool;
            default = true;
            description = "Enable this CalDAV account";
          };

          displayName = mkOption {
            type = types.str;
            example = "My Calendar";
            description = "Display name for the calendar";
          };

          serverUrl = mkOption {
            type = types.str;
            example = "https://caldav.example.com";
            description = ''
              CalDAV server URL.
              Examples:
              - Nextcloud: https://cloud.example.com/remote.php/dav
              - Radicale: https://radicale.example.com/user@example.com/
              - Google: https://apidata.googleusercontent.com/caldav/v2/

              If the server requires a username, include it in the URL
              userinfo (e.g. https://user@caldav.example.com/).
            '';
          };

          username = mkOption {
            type = types.nullOr types.str;
            default = null;
            example = "user@example.com";
            description = ''
              Username for CalDAV authentication.

              If set, this will be written as Username= in the
              generated account.conf.
            '';
          };

          passwordFile = mkOption {
            type = types.str;
            example = literalExpression "\${config.age.secrets.caldav-password.path}";
            description = ''
              Path to file containing CalDAV password.

              The file should contain only the password (trailing newlines will be stripped).

              For use with agenix:
                passwordFile = config.age.secrets.caldav-password.path;

              For sops-nix:
                passwordFile = config.sops.secrets.caldav-password.path;
            '';
          };

          color = mkOption {
            type = types.str;
            default = "#3584e4";
            example = "#e01b24";
            description = ''
              Calendar color in hex format.

              Common colors:
              - Blue (default): #3584e4
              - Red: #e01b24
              - Green: #33d17a
              - Yellow: #f6d32d
              - Purple: #9141ac
            '';
          };

          trustSelfSignedCert = mkOption {
            type = types.bool;
            default = false;
            description = ''
              Whether to trust self-signed SSL certificates.

              WARNING: Only enable this for testing or if you know what you're doing.
              It's recommended to use proper SSL certificates (e.g., Let's Encrypt).
            '';
          };
        };
      });
      default = null;
      example = literalExpression ''
        {
          displayName = "Work Calendar";
          serverUrl = "https://cloud.company.com/remote.php/dav";
          passwordFile = config.age.secrets.work-caldav.path;
          color = "#e01b24";
        }
      '';
      description = ''
        CalDAV account configuration.

        This module generates a configuration file at
        ~/.config/gnome-calendar/account.conf that GNOME Calendar
        will read on startup.

        Note: This requires the patched version of GNOME Calendar
        from this flake that removes GOA dependency.
      '';
    };
  };

  config = mkIf cfg.enable {
    home.packages = [ cfg.package ];

    xdg.configFile."gnome-calendar/account.conf" = mkIf (cfg.caldav != null) {
      text = mkAccountConfig cfg.caldav;
    };

    # Ensure the config directory exists
    home.activation.gnomeCalendarConfig = mkIf (cfg.caldav != null) (
      lib.hm.dag.entryAfter [ "writeBoundary" ] ''
        $DRY_RUN_CMD mkdir -p $VERBOSE_ARG ${config.xdg.configHome}/gnome-calendar
      ''
    );
  };
}
