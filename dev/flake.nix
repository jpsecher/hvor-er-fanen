{
  description = "ESP8266 RGB matrix firmware";
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
          # The default arduino-cli is wrapped in bubblewrap to fake an FHS layout,
          # which needs CAP_SYS_ADMIN that the dev container does not have.  The
          # container is Debian, so the real FHS paths the toolchain wants already
          # exist and the unwrapped binary is the one that works here.
          arduino-cli.pureGoPkg
          # The esp8266 core ships a stub python3 tool and calls the system one
          # from elf2bin.py and signing.py during every build.
          python3
          nodejs_22
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
          # Keep the Arduino cores, tools and libraries inside dev/ instead of $HOME,
          # so the toolchain is per-project and removable with one directory.
          dev_dir="$(git rev-parse --show-toplevel 2>/dev/null || pwd)/dev"
          export ARDUINO_DIRECTORIES_DATA="$dev_dir/.arduino/data"
          export ARDUINO_DIRECTORIES_DOWNLOADS="$dev_dir/.arduino/downloads"
          export ARDUINO_DIRECTORIES_USER="$dev_dir/.arduino/user"
          export ARDUINO_BOARD_MANAGER_ADDITIONAL_URLS="https://arduino.esp8266.com/stable/package_esp8266com_index.json"
        '';
      };
    });
}
