#  GNUmakefile
################################################################
#  For GNU 'make', this file takes priority over other makefiles.
#  Other versions of 'make' ignore this file.  Therefore features
#  that are specific to GNU make should go here.  Currently this
#  file only exists to run an optional 'version_hook' to
#  regenerate version_src.h on every invocation of 'make'

include Makefile

semi=;
version_src.h: $(shell                              \
                                                    \
  [ -x version_hook ] && {                          \
    MAKEOVERRIDES='$(subst ',$$Q,$(MAKEOVERRIDES))' \
    ./version_hook>/dev/null${semi}                 \
  } )#'

