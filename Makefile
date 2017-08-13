# -*- Makefile -*-
# Eugene Skepner 2017

# ----------------------------------------------------------------------

MAKEFLAGS = -w

# ----------------------------------------------------------------------

PROGRAMS = whocc-reference-panel-plots whocc-scan-titers whocc-histogram-of-titers

LIB_DIR = $(ACMACSD_ROOT)/lib

# ----------------------------------------------------------------------

include $(ACMACSD_ROOT)/share/Makefile.g++
include $(ACMACSD_ROOT)/share/Makefile.dist-build.vars

OPTIMIZATION = -O3 #-fvisibility=hidden -flto
PROFILE = # -pg
CXXFLAGS = -g -MMD $(OPTIMIZATION) $(PROFILE) -fPIC -std=$(STD) $(WEVERYTHING) $(WARNINGS) -Icc -I$(BUILD)/include -I$(ACMACSD_ROOT)/include $(PKG_INCLUDES)
LDFLAGS = $(OPTIMIZATION) $(PROFILE)

PKG_INCLUDES = $(shell pkg-config --cflags cairo) $(shell pkg-config --cflags liblzma)

# ----------------------------------------------------------------------

all: check-acmacsd-root $(patsubst %,$(DIST)/%,$(PROGRAMS))

install: all
	ln -sf $(DIST)/* $(ACMACSD_ROOT)/bin
	ln -sf $(abspath bin)/* $(ACMACSD_ROOT)/bin

test: install
	@#test/test

# ----------------------------------------------------------------------

-include $(BUILD)/*.d

# ----------------------------------------------------------------------

$(DIST)/whocc-reference-panel-plots: $(BUILD)/whocc-reference-panel-plots.o $(BUILD)/whocc-reference-panel-plot-colors.o | $(DIST)
	$(CXX) $(LDFLAGS) -o $@ $^ -L$(LIB_DIR) -lacmacsbase -lacmacschart  -llocationdb -lacmacsdraw -lboost_program_options -lboost_filesystem -lboost_system $(shell pkg-config --libs cairo) $(shell pkg-config --libs liblzma)

$(DIST)/whocc-scan-titers: $(BUILD)/whocc-scan-titers.o | $(DIST)
	$(CXX) $(LDFLAGS) -o $@ $^ -L$(LIB_DIR) -lacmacsbase -lacmacschart -llocationdb -lboost_program_options -lboost_filesystem -lboost_system $(shell pkg-config --libs liblzma)

$(DIST)/whocc-histogram-of-titers: $(BUILD)/whocc-histogram-of-titers.o | $(DIST)
	$(CXX) $(LDFLAGS) -o $@ $^ -L$(LIB_DIR) -lacmacsbase -lacmacschart -llocationdb -lacmacsdraw -lboost_program_options -lboost_filesystem -lboost_system $(shell pkg-config --libs cairo) $(shell pkg-config --libs liblzma)

include $(ACMACSD_ROOT)/share/Makefile.rtags

# ----------------------------------------------------------------------

$(BUILD)/%.o: cc/%.cc | $(BUILD)
	@echo $<
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

# ----------------------------------------------------------------------

check-acmacsd-root:
ifndef ACMACSD_ROOT
	$(error ACMACSD_ROOT is not set)
endif

include $(ACMACSD_ROOT)/share/Makefile.dist-build.rules

.PHONY: check-acmacsd-root

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
