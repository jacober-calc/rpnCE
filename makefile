# ----------------------------
# RPNCE makefile (CE C/C++ toolchain)
# ----------------------------

NAME = RPNCE
DESCRIPTION = "RPN calculator inspired by RPN83P"
COMPRESSED = YES
ARCHIVED = NO

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# Sources in subfolders of src/ are picked up automatically.

# ----------------------------

include $(shell cedev-config --makefile)
