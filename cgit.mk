# This Makefile is run in the "git" directory in order to re-use Git's
# build variables and operating system detection.  Hence all files in
# CGit's directory must be prefixed with "../".
include Makefile

CGIT_PREFIX = ../

-include $(CGIT_PREFIX)cgit.conf

# The CGIT_* variables are inherited when this file is called from the
# main Makefile - they are defined there.

$(CGIT_PREFIX)VERSION: force-version
	@cd $(CGIT_PREFIX) && '$(SHELL_PATH_SQ)' ./gen-version.sh "$(CGIT_VERSION)"
-include $(CGIT_PREFIX)VERSION
.PHONY: force-version

# CGIT_CFLAGS is a separate variable so that we can track it separately
# and avoid rebuilding all of Git when these variables change.
CGIT_CFLAGS += -DCGIT_CONFIG='"$(CGIT_CONFIG)"'
CGIT_CFLAGS += -DCGIT_SCRIPT_NAME='"$(CGIT_SCRIPT_NAME)"'
CGIT_CFLAGS += -DCGIT_CACHE_ROOT='"$(CACHE_ROOT)"'

ifdef NO_C99_FORMAT
	CFLAGS += -DNO_C99_FORMAT
endif

CGIT_OBJ_NAMES += cgit.o
CGIT_OBJ_NAMES += cache.o
CGIT_OBJ_NAMES += cmd.o
CGIT_OBJ_NAMES += configfile.o
CGIT_OBJ_NAMES += html.o
CGIT_OBJ_NAMES += parsing.o
CGIT_OBJ_NAMES += scan-tree.o
CGIT_OBJ_NAMES += shared.o
CGIT_OBJ_NAMES += ui-atom.o
CGIT_OBJ_NAMES += ui-blob.o
CGIT_OBJ_NAMES += ui-clone.o
CGIT_OBJ_NAMES += ui-commit.o
CGIT_OBJ_NAMES += ui-diff.o
CGIT_OBJ_NAMES += ui-log.o
CGIT_OBJ_NAMES += ui-patch.o
CGIT_OBJ_NAMES += ui-plain.o
CGIT_OBJ_NAMES += ui-refs.o
CGIT_OBJ_NAMES += ui-repolist.o
CGIT_OBJ_NAMES += ui-shared.o
CGIT_OBJ_NAMES += ui-snapshot.o
CGIT_OBJ_NAMES += ui-ssdiff.o
CGIT_OBJ_NAMES += ui-stats.o
CGIT_OBJ_NAMES += ui-summary.o
CGIT_OBJ_NAMES += ui-tag.o
CGIT_OBJ_NAMES += ui-tree.o

CGIT_OBJS := $(addprefix $(CGIT_PREFIX),$(CGIT_OBJ_NAMES))

# Only cgit.c reference CGIT_VERSION so we only rebuild its objects when the
# version changes.
CGIT_VERSION_OBJS := $(addprefix $(CGIT_PREFIX),cgit.o)
$(CGIT_VERSION_OBJS): $(CGIT_PREFIX)VERSION
$(CGIT_VERSION_OBJS): EXTRA_CPPFLAGS = \
	-DCGIT_VERSION='"$(CGIT_VERSION)"'


# Git handles dependencies using ":=" so dependencies in CGIT_OBJ are not
# handled by that and we must handle them ourselves.
cgit_dep_files := $(foreach f,$(CGIT_OBJS),$(dir $f).depend/$(notdir $f).d)
cgit_dep_files_present := $(wildcard $(cgit_dep_files))
ifneq ($(cgit_dep_files_present),)
include $(cgit_dep_files_present)
endif

ifeq ($(wildcard $(CGIT_PREFIX).depend),)
missing_dep_dirs += $(CGIT_PREFIX).depend
endif

$(CGIT_PREFIX).depend:
	@mkdir -p $@

$(CGIT_PREFIX)CGIT-CFLAGS: FORCE
	@FLAGS='$(subst ','\'',$(CGIT_CFLAGS))'; \
	    if test x"$$FLAGS" != x"`cat ../CGIT-CFLAGS 2>/dev/null`" ; then \
		echo 1>&2 "    * new CGit build flags"; \
		echo "$$FLAGS" >$(CGIT_PREFIX)CGIT-CFLAGS; \
            fi

$(CGIT_OBJS): %.o: %.c GIT-CFLAGS $(CGIT_PREFIX)CGIT-CFLAGS $(missing_dep_dirs)
	$(QUIET_CC)$(CC) -o $*.o -c $(dep_args) $(ALL_CFLAGS) $(EXTRA_CPPFLAGS) $(CGIT_CFLAGS) $<

$(CGIT_PREFIX)cgit: $(CGIT_OBJS) GIT-LDFLAGS $(GITLIBS)
	$(QUIET_LINK)$(CC) $(ALL_CFLAGS) -o $@ $(ALL_LDFLAGS) $(filter %.o,$^) $(LIBS)
