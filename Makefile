# -*- Makefile -*-
# ----------------------------------------------------------------------

TARGETS = \
  $(DIST)/whocc-reference-panel-plots \
  $(DIST)/whocc-scan-titers \
  $(DIST)/whocc-histogram-of-titers \
  $(DIST)/whocc-chart-hidb-seqdb-report \
  $(DIST)/whocc-melb-serum-id \
  $(DIST)/whocc-sera-of-chart \
  $(DIST)/whocc-xlsx-to-torg \
  $(DIST)/whocc-check-new-tables \
  $(DIST)/chart-vaccines \
  $(DIST)/chart-update-vaccines \
  $(DIST)/chart-table-compare \
  $(DIST)/chart-table-map-compare \
  $(DIST)/xlsx2csv \
  $(DIST)/xlsx2html \
  $(DIST)/xlsx-read-test

# $(DIST)/guile-test

SHEET_SOURCES = \
  sheet-extractor.cc \
  sheet-to-torg.cc \
  sheet.cc \
  data-fix.cc

CSV_SOURCES = csv-parser.cc

WHOCC_XLSX_TO_TORG_SOURCES = \
  $(SHEET_SOURCES) \
  whocc-xlsx-to-torg-py.cc

# data-fix-guile.cc

# ACMACS_PY_SOURCES = \
#   py.cc

# ACMACS_PY_LIB_MAJOR = 1
# ACMACS_PY_LIB_MINOR = 0
# ACMACS_PY_LIB_NAME = acmacs
# ACMACS_PY_LIB = $(DIST)/$(ACMACS_PY_LIB_NAME)$(PYTHON_MODULE_SUFFIX)

# ----------------------------------------------------------------------

SRC_DIR = $(abspath $(ACMACSD_ROOT)/sources)

all: install

CONFIGURE_CAIRO = 1
CONFIGURE_PYTHON = 1
include $(ACMACSD_ROOT)/share/Makefile.config

LDLIBS = \
  $(AD_LIB)/$(call shared_lib_name,libacmacsbase,1,0) \
  $(AD_LIB)/$(call shared_lib_name,liblocationdb,1,0) \
  $(AD_LIB)/$(call shared_lib_name,libacmacsvirus,1,0) \
  $(AD_LIB)/$(call shared_lib_name,libacmacswhoccdata,1,0) \
  $(AD_LIB)/$(call shared_lib_name,libacmacschart,2,0) \
  $(AD_LIB)/$(call shared_lib_name,libhidb,5,0) \
  $(AD_LIB)/$(call shared_lib_name,libseqdb,3,0) \
  $(AD_LIB)/$(call shared_lib_name,libacmacsdraw,1,0) \
  $(CAIRO_LIBS) $(XZ_LIBS) $(CXX_LIBS)

# $(GUILE_LIBS)

XLSX_LIBS = $(XLNT_LIBS)

# ----------------------------------------------------------------------

install: make-installation-dirs $(TARGETS)
	$(call install_all,$(AD_PACKAGE_NAME))
	$(call install_py_all)
	$(call install_wildcard,$(abspath js)/*,$(AD_SHARE)/js/who)

install-chains-202105:
	$(MAKE) -C web/chains-202105 install

test: install
	@#test/test
.PHONY: test

# ----------------------------------------------------------------------

# $(ACMACS_PY_LIB): $(patsubst %.cc,$(BUILD)/%.o,$(ACMACS_PY_SOURCES)) | $(DIST)
# 	$(call echo_shared_lib,$@)
# 	$(call make_shared_lib,$(ACMACS_PY_LIB_NAME),$(ACMACS_PY_LIB_MAJOR),$(ACMACS_PY_LIB_MINOR)) $(LDFLAGS) -o $@ $^ $(LDLIBS) $(PYTHON_LIBS)

$(DIST)/whocc-reference-panel-plots: $(BUILD)/whocc-reference-panel-plots.o $(BUILD)/whocc-reference-panel-plot-colors.o | $(DIST)
	$(call echo_link_exe,$@)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS) $(AD_RPATH)

$(DIST)/%: $(BUILD)/%.o $(patsubst %.cc,$(BUILD)/%.o,$(CSV_SOURCES)) | $(DIST)
	$(call echo_link_exe,$@)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS) $(AD_RPATH) $$(if echo "$@" | grep xls >/dev/null 2>&1; then echo "$(XLSX_LIBS)"; fi)

$(DIST)/whocc-xlsx-to-torg: $(BUILD)/whocc-xlsx-to-torg.o $(patsubst %.cc,$(BUILD)/%.o,$(WHOCC_XLSX_TO_TORG_SOURCES)) $(patsubst %.cc,$(BUILD)/%.o,$(CSV_SOURCES)) | $(DIST)
	$(call echo_link_exe,$@)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS) $(PYTHON_LIBS) $(AD_RPATH) $$(if echo "$@" | grep xls >/dev/null 2>&1; then echo "$(XLSX_LIBS)"; fi)

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
