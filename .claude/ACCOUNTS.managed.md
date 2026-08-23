# ACCOUNTS.md — Claude account and settings isolation

A project that should use a different Claude account, a company account say, needs its own Claude Code config directory.  The mechanism differs by environment, but the effect is the same: separate account, settings and plugin store.

One directory can serve both environments — `claudeDir` and `CLAUDE_CONFIG_DIR` may name the same path.  They disagree only on where the config file sits, so such a directory ends up with one shared settings and plugin store behind two separate logins.

## Containerised

- Set `claudeDir` in `flake.nix` next to `projectName`.  The container mounts that host directory as its `~/.claude`, together with the sibling `${claudeDir}.json`
- The two must move together, or only part of the account is switched
- Do not run a host-native and a containerised session at once against the same `claudeDir` — both write the same `.claude.json` and it can be corrupted

## Host-native

- Export `CLAUDE_CONFIG_DIR` before launching `claude`, for instance `export CLAUDE_CONFIG_DIR="$HOME/.claude-company"` in the project's `.envrc`.  `claudeDir` does not apply here — it is a `dev-env.nix` parameter and nothing outside the container reads it
- With devenv, set it in `devenv.local.nix` — `enterShell = ''export CLAUDE_CONFIG_DIR="$HOME/.claude-company"'';` — and not in the committed `devenv.nix`, which would impose one developer's directory on everyone.  Add `use devenv` to `.envrc` as well, or the variable exists only inside `devenv shell`
- The layout is not the containerised one: the config file is `$CLAUDE_CONFIG_DIR/.claude.json`, inside the directory, rather than the sibling the container mounts
- Credentials follow the directory — `$CLAUDE_CONFIG_DIR/.credentials.json` on Linux, and on macOS a distinct Keychain item whose service name carries a hash of the directory path, so the isolation holds natively too
- The plugin store is per config directory, so the marketplace registration and the `--scope user` install are repeated in the new one — see `PLUGINS.md`
- Keep the variable exported for the whole session, not just the launch, so subprocesses agree with the parent about it
