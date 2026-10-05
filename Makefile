#---------------------------------------------------------------------------------
# ZENONIA 2 -- Nintendo Switch wrapper (32-bit / AArch32)
#
# Ships NO game code and NO game assets: the game's own APK (the user's copy,
# any name) is read at run time; its library is unpacked from it on the first
# launch (source/zen_main.c, the setup plan), and the pictures, data and sounds
# are read from the APK itself (source/zen_apk.c).
#
# The build is the android32 runtime's (runtime/runtime.mk: devkitARM +
# libnx32 + mesa32 from portlibs32/); ./build.sh runs it in the toolchain
# container. Output: zenonia2_nx.nsp -- an ExeFS NSP (main + main.npdm,
# 32-bit), which the launcher NRO carries (launcher/). A 32-bit program
# cannot be an NRO: hbloader is 64-bit.
#---------------------------------------------------------------------------------
TARGET               := zenonia2_nx
PORT_NPDM_PROGRAM_ID := 0x0100000000001021
include runtime/runtime.mk

.PHONY: check
check:
	@echo "run on the host: python3 runtime/tools/gen_imports.py --check"
