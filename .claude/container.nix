{ pkgs }:
{
  # Nix packages to install in the development container
  packages = [
    # "net-tools"
    # "python3"
  ];
  # Port ranges to expose from the container
  ports = [
    # "3000"
    # "8080:8081"
  ];
  # Host secrets to mount read-only and export as environment variables
  secrets = [
    # { name = "github_token"; hostPath = "$HOME/.config/github/token"; envVar = "GITHUB_TOKEN"; }
  ];
}
