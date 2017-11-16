# -*- Makefile -*-
# Eugene Skepner 2017

# ----------------------------------------------------------------------

MAKEFLAGS = -w

# ----------------------------------------------------------------------

TARGETS = \
	$(DIST)/whocc-reference-panel-plots \
	$(DIST)/whocc-scan-titers \
	$(DIST)/whocc-histogram-of-titers

# ----------------------------------------------------------------------

include $(ACMACSD_ROOT)/share/makefiles/Makefile.g++
include $(ACMACSD_ROOT)/share/makefiles/Makefile.dist-build.vars

CXXFLAGS = -g -MMD $(OPTIMIZATION) $(PROFILE) -fPIC -std=$(STD) $(WARNINGS) -Icc -I$(AD_INCLUDE) $(PKG_INCLUDES)
LDFLAGS = $(OPTIMIZATION) $(PROFILE)

PKG_INCLUDES = $(shell pkg-config --cflags cairo) $(shell pkg-config --cflags liblzma)

LDLIBS = \
	$(AD_LIB)/$(call shared_lib_name,libacmacsbase,1,0) \
	$(AD_LIB)/$(call shared_lib_name,libacmacschart,1,0) \
	$(AD_LIB)/$(call shared_lib_name,liblocationdb,1,0) \
	-L$(AD_LIB) -lacmacsdraw -lboost_program_options $(shell pkg-config --libs cairo) $(shell pkg-config --libs liblzma)

# ----------------------------------------------------------------------

all: check-acmacsd-root $(TARGETS)

install: $(TARGETS)
	ln -sf $(DIST)/* $(AD_BIN)
	ln -sf $(abspath bin)/* $(AD_BIN)

test: install
	@#test/test

# ----------------------------------------------------------------------

-include $(BUILD)/*.d
include $(ACMACSD_ROOT)/share/makefiles/Makefile.dist-build.rules
include $(ACMACSD_ROOT)/share/makefiles/Makefile.rtags

# ----------------------------------------------------------------------

$(DIST)/whocc-reference-panel-plots: $(BUILD)/whocc-reference-panel-plots.o $(BUILD)/whocc-reference-panel-plot-colors.o | $(DIST)
	@printf "%-16s %s\n" "LINK" $@
	@$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(DIST)/%: $(BUILD)/%.o | $(DIST)
	@printf "%-16s %s\n" "LINK" $@
	@$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
