empty:=
space:= $(empty) $(empty)
defbody=${firstword ${subst =,$(space),$(1)}},"${subst $(space),=,${wordlist 2,100000000,${subst =,$(space),$(1)}}}"
_MAKEOV2:=${foreach X,\
            ${subst \ ,_S,  \
            ${subst \\,_B,  \
            ${subst ",_D,   \
            ${subst ',_Q,   \
            ${subst $$$$,_C,\
            ${subst _,_U,$(MAKEOVERRIDES)}}}}}},\
           MDEF(${call defbody,$(X)})$\
}#  keep emacs happy: '}"}}}}

version_make.h: MFORCE
	@if test ! -e $(@:.h=.t) || ! { echo '$(_MAKEOV2)' | diff -q - $(@:.h=.t) >/dev/null 2>&1; }; then \
	  echo '$(_MAKEOV2)' > $(@:.h=.t); \
	  echo '#define VERSION_MAKEVARS(MDEF) \' >$@; \
	  sed \
	    -e 's/_S/ /g'     \
	    -e 's/_B/\\\\/g'  \
	    -e 's/_D/\\"/g'   \
	    -e 's/_Q/'"'"'/g' \
	    -e 's/_C/\$$/g'   \
	    -e 's/_U/_/g'     \
            < $(@:.h=.t) >> $@; \
	fi

MFORCE:
