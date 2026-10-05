#!/bin/sh
# Build zenonia2_nx.nro (the launcher) with the runtime's launcher build
# (devkitPro's 64-bit toolchain container). Build the wrapper first
# (../build.sh): the NRO carries ../zenonia2_nx.nsp and ../zenonia2_nx.build.
HERE="$(cd "$(dirname "$0")" && pwd)"
LAUNCHER_DIR="$HERE" PAYLOAD=zenonia2_nx exec "$HERE/../runtime/launcher/build.sh" "$@"
