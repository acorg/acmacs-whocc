# -*- Makefile -*-
# ----------------------------------------------------------------------

TARGETS = \
  $(DIST)/whocc-reference-panel-plots \
  $(DIST)/whocc-scan-titers \
  $(DIST)/whocc-histogram-of-titers \
  $(DIST)/whocc-chart-hidb-seqdb-report \
  $(DIST)/whocc-melb-serum-id \
  $(DIST)/whocc-sera-of-chart

# ----------------------------------------------------------------------

SRC_DIR = $(abspath $(ACMACSD_ROOT)/sources)

all: install

CONFIGURE_CAIRO = 1
include $(ACMACSD_ROOT)/share/Makefile.config

LDLIBS = \
  $(AD_LIB)/$(call shared_lib_name,libacmacsbase,1,0) \
  $(AD_LIB)/$(call shared_lib_name,liblocationdb,1,0) \
  $(AD_LIB)/$(call shared_lib_name,libacmacsvirus,1,0) \
  $(AD_LIB)/$(call shared_lib_name,libacmacschart,2,0) \
  $(AD_LIB)/$(call shared_lib_name,libhidb,5,0) \
  $(AD_LIB)/$(call shared_lib_name,libseqdb,3,0) \
  $(AD_LIB)/$(call shared_lib_name,libacmacsdraw,1,0) \
  $(CAIRO_LIBS) $(XZ_LIBS) $(CXX_LIBS)

# ----------------------------------------------------------------------

install: $(TARGETS)
	ln -sf $(DIST)/* $(AD_BIN)
	ln -sf $(abspath bin)/* $(AD_BIN)
	$(call symbolic_link,$(abspath py)/acmacs_whocc,$(AD_PY)/acmacs_whocc)
	mkdir -p $(AD_SHARE)/js/who; ln -sf $(SRC_DIR)/acmacs-whocc/js/* $(AD_SHARE)/js/who
	if [ ! -f $(AD_SHARE)/conf/clades.json ]; then mkdir -p $(AD_SHARE)/conf; ln -sf $(abspath conf/clades.json) $(AD_SHARE)/conf; fi

test: install
	@#test/test
.PHONY: test

# ----------------------------------------------------------------------

$(DIST)/whocc-reference-panel-plots: $(BUILD)/whocc-reference-panel-plots.o $(BUILD)/whocc-reference-panel-plot-colors.o | $(DIST)
	$(call echo_link_exe,$@)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS) $(AD_RPATH)

$(DIST)/%: $(BUILD)/%.o | $(DIST)
	$(call echo_link_exe,$@)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS) $(AD_RPATH)

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
