import json, pprint
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

    data = {} # collect_chain_data(request=request, subtype_id=subtype_id, chain_id=chain_id)

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

# ======================================================================
