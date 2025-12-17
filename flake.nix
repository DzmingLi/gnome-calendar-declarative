{
  description = "GNOME Calendar with declarative CalDAV configuration";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      supportedSystems = [ "x86_64-linux" "aarch64-linux" ];
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;
      nixpkgsFor = forAllSystems (system: nixpkgs.legacyPackages.${system});
    in
    {
      packages = forAllSystems (system: {
        default = nixpkgsFor.${system}.callPackage ./nix/package.nix { };
        gnome-calendar = self.packages.${system}.default;
      });

      devShells = forAllSystems (system:
        let
          pkgs = nixpkgsFor.${system};
        in
        {
          default = pkgs.mkShell {
            inputsFrom = [ self.packages.${system}.default ];
            packages = with pkgs; [
              meson
              ninja
              pkg-config
            ];
          };
        });

      homeManagerModules.default = import ./nix/home-manager-module.nix;
      homeManagerModules.gnome-calendar = self.homeManagerModules.default;
    };
}
