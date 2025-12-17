{ lib
, stdenv
, meson
, ninja
, pkg-config
, wrapGAppsHook4
, blueprint-compiler
, desktop-file-utils
, gettext
, evolution-data-server
, glib
, gtk4
, libadwaita
, libical
, libsoup_3
, gsettings-desktop-schemas
, libgweather
, geoclue2
}:

stdenv.mkDerivation rec {
  pname = "gnome-calendar";
  version = "50.alpha-declarative";

  src = lib.cleanSource ../.;

  nativeBuildInputs = [
    meson
    ninja
    pkg-config
    wrapGAppsHook4
    blueprint-compiler
    desktop-file-utils
    gettext
  ];

  buildInputs = [
    evolution-data-server
    evolution-data-server.dev
    glib
    gtk4
    libadwaita
    libical
    libsoup_3
    gsettings-desktop-schemas
    libgweather
    geoclue2
  ];

  mesonFlags = [
    "-Dprofile=default"
  ];

  meta = with lib; {
    description = "GNOME Calendar with declarative CalDAV support (no GOA dependency)";
    homepage = "https://apps.gnome.org/Calendar/";
    license = licenses.gpl3Plus;
    maintainers = [ ];
    platforms = platforms.linux;
    mainProgram = "gnome-calendar";
  };
}
