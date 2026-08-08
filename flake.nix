{
  description = "My Project";
  inputs = {
    nixpkgs.url = "https://channels.nixos.org/nixpkgs-unstable/nixexprs.tar.xz";
    flake-utils.url = "github:numtide/flake-utils";
    claude-contained.url = "git+https://gitoro.com/jpsecher/claude-contained.git";
    claude-contained.inputs.nixpkgs.follows = "nixpkgs";
    claude-contained.inputs.flake-utils.follows = "flake-utils";
  };
  outputs = { self, nixpkgs, flake-utils, claude-contained }:
    flake-utils.lib.eachDefaultSystem (system:
    let
      pkgs = nixpkgs.legacyPackages.${system};
      projectConfig = import ./.claude/container.nix { inherit pkgs; };
    in {
      devShells.default = claude-contained.lib.mkDevEnv { inherit pkgs; } {
        inherit system;
        projectName = "hvor-er-fanen";
        inherit (projectConfig) packages ports secrets;
      };
    });
}
