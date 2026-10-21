# Shared workspace-root resolution for the tools scripts - sourced, not
# run. HIBISCUS_ROOT must point at a build root; see system/dist/README.md
# for what one contains (layout-sources.sh lays the repo sources out into
# it, fetch-inputs.sh the input packages).
HIBISCUS_ROOT="${HIBISCUS_ROOT:?set HIBISCUS_ROOT to a build root - see system/dist/README.md}"
