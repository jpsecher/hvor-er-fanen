# DEV-ENV.md — Nix/Docker dev container

This project uses the optional `claude-contained` Nix + Docker dev environment.  Skip this file if the project was set up without the Nix harness.

## Dev environment

- The outer dev shell (`flake.nix`) uses the `claude-contained` library from `https://gitoro.com/jpsecher/claude-contained`, specifically `dev-env.nix` and `templates/` — WebFetch them when needed
- Update the container setup with `nix flake update` if the library has changed
- `dev-shell`: starts/enters the container and enters `nix develop dev/` — cwd is `dev/`, user is root
- `init-dev`: scaffolds `dev/flake.nix` and `dev/justfile` from the claude-contained templates
- `status`: shows container health and auth status

## Claude account

- The container mounts the host directory named by `claudeDir` (default `$HOME/.claude`) as its `~/.claude`, together with the sibling `${claudeDir}.json`.  Set `claudeDir` in `flake.nix` next to `projectName` to give a project its own Claude account and settings — see `ACCOUNTS.md`
- A claude.ai login persists in `${claudeDir}/.credentials.json`, a Console login in `${claudeDir}.json`.  Keep one account type per pair, or Claude Code warns about an auth conflict at startup
- On macOS the host-native CLI keeps credentials in the Keychain, so a native session and a containerised one are already separate accounts

## Running commands

- All tools are controlled by the inner flake (`dev/flake.nix`) and `dev/justfile`
- Add needed dependencies to inner flake
- All commands already run in the inner-shell environment — no `nix develop . --command` wrapper is needed.  The container-side `claude` wrapper starts the session inside `nix develop dev/` on any entry path (launcher, resume, IDE, direct `docker exec`), so every command inherits the toolchain and the flake's exported vars
- A non-zero exit code from tests means failures in the code, not infrastructure problems

### Workflow split

- User (host): Performs commits; run `dev-shell`, then `just <cmd>`
- Claude (container): `just <cmd>` — the toolchain resolves directly; the `nixbld` warning is harmless
- When new dependencies are added to `dev/flake.nix`, exit `claude` and relaunch to re-evaluate the dev shell (the environment is captured once at session start)
