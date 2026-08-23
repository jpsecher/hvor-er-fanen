# PLUGINS.md — plugins and skills across environments

Claude Code records plugin installs with absolute paths.  A project may run host-natively, in the dev container, or both, and those paths do not agree between them, so a single install cannot serve every environment.  Getting this wrong is why a plugin fails to load with "cannot be found".

## Availability and intent are separate

- **Availability** — whether the plugin's files exist in this environment.  Absolute-path-bound, so it is per-environment: once host-native, once in the container.
- **Intent** — whether this project uses the plugin.  Path-free, committed with the repo, and the same in every environment.

Installing conflates the two only if you install at project scope, which records the project's absolute path.  Keep them apart instead.

## Project-integrated plugins

A plugin the project depends on, such as Allium, belongs in the committed `.claude/settings.json`:

    "enabledPlugins": { "allium@juxt-plugins": true }

That is intent, it travels with the repo, and it names no path.  Anyone cloning the project gets the declaration.

Availability is then a one-off per environment.  Register the marketplace first, because marketplace registrations live in the plugin store and so are per-environment too:

    claude plugin marketplace add juxt/claude-plugins
    claude plugin install allium@juxt-plugins --scope user

Skipping the first line makes the second fail: without the marketplace registered there is nothing for `@juxt-plugins` to resolve against.

Run both from the shell, not from the `/plugin` prompt inside a session.  An in-session install records `scope: project` with the project's path even when `--scope user` is given, which is the binding this whole file exists to avoid.  The shell form reports `(scope: user)` and records no project path.

A project-scope entry cannot be removed with `claude plugin uninstall` while `.claude/settings.json` still enables the plugin — Claude Code refuses to uninstall a declaration shared with the team.  Install the user-scope entry first; the stale project entry is then inert and can be dropped from `installed_plugins.json` by hand.

User scope records no project path, so one install serves every project sharing that Claude directory.  Never install a project-integrated plugin at project scope — that writes an absolute project path, which stops working as soon as the same project is opened in the other environment, and it has to be repeated for every project.

## Personal plugins and skills

Anything only you use is a user-scope install that no project enables.  Same command, same once-per-environment rule, but leave it out of `.claude/settings.json` so it is not shared with anyone else through the repo.

User-local skills live in `~/.claude/skills/<name>/`, which Claude Code loads as a plugin named `<name>@skills-dir`.  `claude plugin init <name>` scaffolds one.

## Why twice

The container mounts the host Claude directory, so credentials, settings and history are shared deliberately.  Plugin state cannot be shared the same way, because the cache paths recorded on one side do not resolve on the other — a host install appears in the container as "failed to load: cache-miss", and a container install is invisible to the host.

Containers therefore keep their own plugin store, shared across every containerised project using the same Claude directory.  So a plugin is installed once for all containers and once on the host: twice in total, never once per project.

The store is a Docker volume applied when the container is created, so a container that predates the volume picks it up only when recreated with `remove-dev` followed by `start-dev`.  The volume starts empty rather than inheriting the host's plugins, so the first containerised session needs the one-off install above.  It is deliberately left alone by `remove-dev-full`, which is per-project — removing it there would wipe the plugins of every other containerised project sharing the Claude directory.
