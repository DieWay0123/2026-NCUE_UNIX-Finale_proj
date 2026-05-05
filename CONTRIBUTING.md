# Contributing

Use feature branches and keep changes small enough to review.

## Development Flow

1. Build a standalone C prototype.
2. Verify functionality.
3. Move reusable logic into `libdiag`.
4. Integrate as a BusyBox applet.
5. Add tests, benchmarks, and documentation.

## Commit Messages

Examples:

```sh
git commit -m "feat(bbtop): parse process stat from procfs"
git commit -m "feat(bbfscheck): add statvfs filesystem summary"
git commit -m "feat(bbnetmon): parse tcp state from proc net tcp"
```
