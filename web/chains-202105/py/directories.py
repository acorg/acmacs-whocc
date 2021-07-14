import json
from pathlib import Path

# ======================================================================

class CladeData:

    sSubtypeToCladePrefix = {"h1pdm": "clades-A(H1N1)PDM09", "h3": "clades-A(H3N2)", "bvic": "clades-BVICTORIA", "byam": "clades-BYAMAGATA"}

    def __init__(self):
        self.data = json.load(Path("clades.mapi").open())

    def entry_names_for_subtype(self, subtype):
        subtype_prefix = self.sSubtypeToCladePrefix[subtype]
        names = [name for name in self.data if name.startswith(subtype_prefix)]
        return names

    def entry(self, name):
        return self.data.get(name, [])

# ======================================================================

def load(app):
    app["clade_data"] = CladeData()

# ======================================================================
