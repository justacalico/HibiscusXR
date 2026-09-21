# Rules for AI agents

## Testability

Every new line of C++ must be 100% testable. Logic belongs in the pure
modules (math/, panels/layout, text/utf8, text/glyphs, render/scene_geo,
input/keys) that compile and run on the host. Android- and GL-facing
code stays thin: glue only, no decisions. If a new function cannot be
reached by `make test`, the design is wrong - move the logic somewhere
that can.

## Tests

`make test` must pass before a change is called done. No exceptions, no
"should work". New logic ships with new coverage in tests/.

## File size

No god files. If a file is turning into a dumping ground, split it by
responsibility before adding more. Refactor toward an A+ codebase rather
than layering hacks on a working mess.
