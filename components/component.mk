#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)


COMPONENT_SRCDIRS := . lib
COMPONENT_ADD_INCLUDEDIRS := ./include lib/include
COMPONENT_EMBED_TXTFILES := server_root_cert.pem
