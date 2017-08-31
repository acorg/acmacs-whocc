# -*- Makefile -*-
# Eugene Skepner 2017

# ----------------------------------------------------------------------

MAKEFLAGS = -w

# ----------------------------------------------------------------------

PROGRAMS = whocc-reference-panel-plots whocc-scan-titers whocc-histogram-of-titers

# ----------------------------------------------------------------------

TARGET_ROOT=$(shell if [ -f /Volumes/rdisk/ramdisk-id ]; then echo /Volumes/rdisk/AD; else echo $(ACMACSD_ROOT); fi)
include $(TARGET_ROOT)/share/Makefile.g++
include $(TARGET_ROOT)/share/Makefile.dist-build.vars

OPTIMIZATION = -O3 #-fvisibility=hidden -flto
PROFILE = # -pg
CXXFLAGS = -g -MMD $(OPTIMIZATION) $(PROFILE) -fPIC -std=$(STD) $(WEVERYTHING) $(WARNINGS) -Icc -I$(AD_INCLUDE) $(PKG_INCLUDES)
LDFLAGS = $(OPTIMIZATION) $(PROFILE)

PKG_INCLUDES = $(shell pkg-config --cflags cairo) $(shell pkg-config --cflags liblzma)

# ----------------------------------------------------------------------

all: check-acmacsd-root $(patsubst %,$(DIST)/%,$(PROGRAMS))

install: all
	ln -sf $(DIST)/* $(AD_BIN)
	ln -sf $(abspath bin)/* $(AD_BIN)

test: install
	@#test/test

# ----------------------------------------------------------------------

-include $(BUILD)/*.d

# ----------------------------------------------------------------------

$(DIST)/whocc-reference-panel-plots: $(BUILD)/whocc-reference-panel-plots.o $(BUILD)/whocc-reference-panel-plot-colors.o | $(DIST)
	@echo "LINK       " $@ # '<--' $^
	@$(CXX) $(LDFLAGS) -o $@ $^ -L$(AD_LIB) -lacmacsbase -lacmacschart  -llocationdb -lacmacsdraw -lboost_program_options -lboost_system $(shell pkg-config --libs cairo) $(shell pkg-config --libs liblzma)

$(DIST)/whocc-scan-titers: $(BUILD)/whocc-scan-titers.o | $(DIST)
	@echo "LINK       " $@ # '<--' $^
	@$(CXX) $(LDFLAGS) -o $@ $^ -L$(AD_LIB) -lacmacsbase -lacmacschart -llocationdb -lboost_program_options -lboost_system $(shell pkg-config --libs liblzma)

$(DIST)/whocc-histogram-of-titers: $(BUILD)/whocc-histogram-of-titers.o | $(DIST)
	@echo "LINK       " $@ # '<--' $^
	@$(CXX) $(LDFLAGS) -o $@ $^ -L$(AD_LIB) -lacmacsbase -lacmacschart -llocationdb -lacmacsdraw -lboost_program_options -lboost_system $(shell pkg-config --libs cairo) $(shell pkg-config --libs liblzma)

include $(AD_SHARE)/Makefile.rtags

# ----------------------------------------------------------------------

$(BUILD)/%.o: cc/%.cc | $(BUILD)
	@echo $(CXX_NAME) $<
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

# ----------------------------------------------------------------------

include $(AD_SHARE)/Makefile.dist-build.rules

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
