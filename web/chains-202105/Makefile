# -*- Makefile -*-
# ----------------------------------------------------------------------

ifeq ($(CHAINS_ROOT),)
  $(error CHAINS_ROOT not defined, use make install-chains-202105 CHAINS_ROOT=<dir>)
endif

install:
	rsync -av py js $(CHAINS_ROOT)

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
