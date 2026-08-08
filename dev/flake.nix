{
  description = "dev";
  nixConfig.warn-dirty = false;  # Silence warnings
  inputs = {
    nixpkgs.url = "https://channels.nixos.org/nixos-unstable-small/nixexprs.tar.xz";
    flake-utils.url = "github:numtide/flake-utils";
  };
  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
    let
      pkgs = nixpkgs.legacyPackages.${system};
    in {
      devShells.default = pkgs.mkShell {
        packages = with pkgs; [
          just
          # bat
          # net-tools
        ];
        shellHook = ''
          if [ ! -f /.dockerenv ]; then
            echo "Error: run 'dev-shell' from the outer nix shell instead of 'nix develop' here directly."
            exit 1
          fi
          PS1="[inner]\w\$ "
          export PS1
          # Silence Nix dirty-tree warning (repo always has uncommitted changes).
          mkdir -p "$HOME/.config/nix"
          grep -qF 'warn-dirty' "$HOME/.config/nix/nix.conf" 2>/dev/null || echo 'warn-dirty = false' >> "$HOME/.config/nix/nix.conf"
        '';
      };
    });
}
