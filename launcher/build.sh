#!/bin/sh
# Build zenonia3_nx.nro (the launcher) with the runtime's launcher build
# (devkitPro's 64-bit toolchain container). Build the wrapper first
# (../build.sh): the NRO carries ../zenonia3_nx.nsp and ../zenonia3_nx.build.
HERE="$(cd "$(dirname "$0")" && pwd)"
LAUNCHER_DIR="$HERE" PAYLOAD=zenonia3_nx exec "$HERE/../runtime/launcher/build.sh" "$@"
