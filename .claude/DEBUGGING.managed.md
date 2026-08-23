## Infrastructure debugging mode

Triggered when the user says something like "debug mode" or "let's debug the setup".  Use this when diagnosing problems with the dev environment, Nix, Docker, or tooling — not project code.

In this mode:
- Skip `PROJECT.md`, `TASKS.md` and `TASKS.local.md` — they are not relevant
- Do not make any changes until the root cause is confirmed
- Run the smallest possible diagnostic command, report the result, then wait
- State each assumption explicitly before testing it
- Exit this mode when the user returns to normal project work
