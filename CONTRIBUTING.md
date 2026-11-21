# Development workflow

## Commits

- One commit per meaningful, independently working milestone.
- Tests pass before committing (`scripts/verify_env.sh`). If something is committed in a
  known-broken or untested state, the commit message says so and why.
- [Conventional Commits](https://www.conventionalcommits.org/) format:

  ```
  <type>(<scope>): <imperative summary, ≤72 chars>

  <optional body: what changed and why>
  ```

  Types: `feat`, `fix`, `test`, `docs`, `chore`, `perf`, `refactor`, `build`.
  Scopes: `data`, `models`, `engine` (C++), `bench`, `stream`, `env`.

  Example: `perf(engine): unroll NEON tree traversal for batch size 16`

## Never committed

Datasets, virtual environments, trained model binaries, raw benchmark output, and any
credentials or keys. `.gitignore` enforces this; check `git status` before every commit.

Benchmark *summaries* that support a documented conclusion may be committed under
`docs/`, together with the environment report that produced them.

## History

No force-pushes, history rewrites, or branch deletions on shared branches.

## Documentation

README status table and `docs/decisions.md` are updated in the same commit as the
change they describe.
