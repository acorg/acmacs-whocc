import sys, re, json, pprint
from pathlib import Path
from aiohttp import web
from web_chains_202105 import utils

# ======================================================================

sChainPage = """<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8" />
    {stylesheets}
    {remote_scripts}
    {inline_scripts}
    <title>{chain_name}</title>
  </head>
  <body>
    <h2 id="title"></h2>
{body}
  </body>
</html>
"""

# ======================================================================

def chain_page(request, subtype_id, chain_id):
    global sChainPage

    remote_scripts = [
        "js/jquery.js",
        "js/chain-page.js",
        ]
    stylesheets = [
        "js/maps.css",
        "js/chain-page.css",
        ]

    data = collect_chain_data(request=request, subtype_id=subtype_id, chain_id=chain_id)

    inline_scripts = [
        f"chain_page_data =\n{json.dumps(data, separators=(',', ':'))};",
        ]

    return web.Response(
        text=sChainPage.format(
            remote_scripts="\n    ".join(f'<script src="{script}" type="module"></script>' for script in remote_scripts),
            stylesheets="\n    ".join(f'<link rel="stylesheet" href="{stylesheet}">' for stylesheet in stylesheets),
            inline_scripts="\n    ".join(f'<script>\n{code}\n    </script>' for code in inline_scripts),
            chain_name=f"{subtype_id} {chain_id}",
            body="", # f"<pre>\n{pprint.pformat(data)}</pre>",
    ),
    content_type='text/html')

# ----------------------------------------------------------------------

sReAceName = re.compile(r"^(?P<step_no>\d+)\.(?:\d+(?:\.\d+)?-)?(?P<date>\d+(?:\.\d+)?)\.(?P<type>[a-z]+)\.ace$")

def collect_chain_data(request, subtype_id, chain_id):

    def find_individual_dir():
        individual_dir = Path(subtype_id, "i-" + chain_id.split("-")[-1])
        if individual_dir.exists():
            return individual_dir
        else:
            return None

    def collect_chain_data_part():
        parts_by_step = {}
        for ace_file in sorted(Path(subtype_id, chain_id).glob("*.ace"), reverse=True):
            if mm := sReAceName.match(ace_file.name):
                parts_by_step.setdefault(mm["step_no"], {})[mm["type"]] = {"ace": str(ace_file)}
                parts_by_step[mm["step_no"]]["date"] = mm["date"]
                parts_by_step[mm["step_no"]]["step"] = int(mm["step_no"])
        parts = [update_with_individual(parts_by_step[step]) for step in sorted(parts_by_step, reverse=True)]
        return parts

    individual_dir = find_individual_dir();

    def update_with_individual(part):
        if individual_files := list(individual_dir.glob(f"*-{part['date']}.ace")):
            part["individual"] = {"ace": str(individual_files[0])}
        return part

    def find_chain_setup():
        setup_file = Path(subtype_id, chain_id + ".setup.json")
        if setup_file.exists():
            return json.load(setup_file.open())
        else:
            return {}

    chain_data = {
        "subtype_id": subtype_id,
        "chain_id": chain_id,
        "parts": collect_chain_data_part(),
        # "type": "Chain" if chain_id[0] == "f" else "Backward chain",
        "type": "" if chain_id[0] == "f" else " backward",
        **utils.format_subtype(request=request, subtype_id=subtype_id)
    }

    for setup_key, setup_value in find_chain_setup().items():
        if setup_key and setup_key[0] != "?" and setup_value and (not isinstance(setup_value, str) or setup_value[0] != "?"):
            chain_data[setup_key] = setup_value
    print(f">>>> collect_chain_data chain_data: {chain_data}", file=sys.stderr)

    return chain_data

# ======================================================================
