import sys, os, subprocess
from pathlib import Path
import logging; module_logger = logging.getLogger(__name__)

# ----------------------------------------------------------------------

def h1_overlay_relax(sources : [Path], target : Path, log_file=None):
    overlay = target.parent.joinpath(target.stem + ".overlay" + target.suffix)
    h1_overlay(sources=sources, target=overlay, log_file=log_file)
    h1_relax(source=overlay, target=target, log_file=log_file)

# ----------------------------------------------------------------------

def h1_overlay(sources : [Path], target : Path, log_file=None):
    module_logger.info(f"Log file: {log_file}")
    cmd = [os.path.expandvars("${ACMACSD_ROOT}/bin/chart-merge"),
           "-o", target,
           "-m", "overlay",
           "--duplicates-distinct",
           ] + sources
    cmd = [str(field) for field in cmd]
    subprocess.check_call(cmd, stdout=_log(log_file), stderr=subprocess.STDOUT)

# ----------------------------------------------------------------------

def h1_relax(source : Path, target : Path, log_file=None):
    cmd = [os.path.expandvars("${ACMACSD_ROOT}/bin/chart-relax-existing"),
           "--rough",
           source,
           target
           ]
    subprocess.check_call([str(field) for field in cmd], stdout=_log(log_file), stderr=subprocess.STDOUT)

# ----------------------------------------------------------------------

def _log(log_file):
    if log_file:
        return log_file.open("a")
    else:
        return None

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
