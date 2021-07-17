import re, json, pprint
from pathlib import Path
from aiohttp import web

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
            body=f"<pre>\n{pprint.pformat(data)}</pre>"
    ),
    content_type='text/html')

# ----------------------------------------------------------------------

sReAceName = re.compile(r"^(?P<step_no>\d+)\.(?:\d+-)(?P<date>\d+)\.(?P<type>[a-z]+)\.ace$")

def collect_chain_data(request, subtype_id, chain_id):

    def collect_chain_data_part():
        parts_by_date = {}
        for ace_file in sorted(Path(subtype_id, chain_id).glob("*.ace"), reverse=True):
            if mm := sReAceName.match(ace_file.name):
                parts_by_date.setdefault(mm["date"], {})[mm["type"]] = str(ace_file)
                parts_by_date[mm["date"]]["step"] = int(mm["step_no"])
        return parts_by_date

    return {
        "subtype_id": subtype_id,
        "chain_id": chain_id,
        "parts": collect_chain_data_part(), # [part_data for part_data in collect_chain_data_part() if part_data],
        # **format_subtype(request=request, subtype_id=subtype_id)
    }

# ======================================================================
