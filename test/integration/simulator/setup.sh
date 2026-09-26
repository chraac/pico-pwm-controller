#!/usr/bin/env bash
# Offline simulator setup: extract the vendored npm tarballs straight into
# node_modules/ -- no npm, no lockfile, no registry, fully deterministic.
# Both packages (rp2040js, uf2) are dependency-free prebuilt CJS bundles,
# so plain extraction is all npm would do anyway.
set -euo pipefail

_sim_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$_sim_dir"

mkdir -p node_modules
for tgz in vendor/*.tgz; do
    name=$(tar -xzOf "$tgz" package/package.json | sed -n 's/.*"name": *"\([^"]*\)".*/\1/p' | head -1)
    if [[ -z "$name" ]]; then
        echo "error: cannot read package name from $tgz" >&2
        exit 1
    fi
    rm -rf "node_modules/$name" node_modules/package
    tar -xzf "$tgz" -C node_modules
    mv node_modules/package "node_modules/$name"
    echo "installed $name from $tgz"
done
