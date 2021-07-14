# -*- Makefile -*-
# ----------------------------------------------------------------------

ifeq ($(CHAINS_ROOT),)
  $(error CHAINS_ROOT not defined, use make install-chains-202105 CHAINS_ROOT=<dir>)
endif

install:
	rsync -av --copy-unsafe-links --exclude __pycache__ --exclude '.*' bin js py $(ACMACSD_ROOT)/share/conf/clades.json $(CHAINS_ROOT)
	rsync -av --copy-unsafe-links --exclude __pycache__ --exclude '.*' --exclude chain202105 $(ACMACSD_ROOT)/sources/acmacs-py/py/acmacs_py $(CHAINS_ROOT)/py

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
