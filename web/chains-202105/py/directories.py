import sys, os, json
from pathlib import Path
from acmacs_py.mapi_utils import MapiSettings

# ======================================================================

class CladeData:

    sSubtypeToCladePrefix = {"h1pdm": "clades-A(H1N1)2009pdm", "h3": "clades-A(H3N2)", "bvic": "clades-B/Vic", "byam": "clades-B/Yam"}

    def __init__(self):
        self.mapi_settings = MapiSettings("clades.mapi")

    def entry_names_for_subtype(self, subtype, date_range: list[str]):
        subtype_prefix = self.sSubtypeToCladePrefix[subtype]
        names = sorted(name for name in self.mapi_settings.names() if name.startswith(subtype_prefix))
        if date_range[1] < "2018":
            # use old clades only
            names = list(filter(lambda cn: "-v" not in cn, names))
        elif date_range[0] >= "2018":
            # use new clades only
            names = list(filter(lambda cn: "-v" in cn, names))
        return names

    def chart_draw_modify(self, *args, **kw):
        self.mapi_settings.chart_draw_modify(*args, **kw)

    def chart_draw_reset(self, *args, **kw):
        self.mapi_settings.chart_draw_reset(*args, **kw)

# ======================================================================

def load(app):
    app["clade_data"] = CladeData()

# ======================================================================
