import json, pprint
from pathlib import Path
from aiohttp import web

# ======================================================================

sTablePage = """<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8" />
    {stylesheets}
    {remote_scripts}
    {inline_scripts}
    <title>{table_name}</title>
  </head>
  <body>
    <h2>{table_name}</h2>
{body}
  </body>
</html>
"""

# ======================================================================

def table_page(subtype_id, table_date):
    global sTablePage

    remote_scripts = [
        "js/jquery.js",
        "js/table-page.js",
        ]
    stylesheets = [
        "js/table-page.css",
        ]

    data = collect_table_data(subtype_id=subtype_id, table_date=table_date)

    inline_scripts = [
        f"table_page_data =\n{json.dumps(data, separators=(',', ':'))};",
        ]

    return web.Response(
        text=sTablePage.format(
            remote_scripts="\n    ".join(f'<script src="{script}"></script>' for script in remote_scripts),
            stylesheets="\n    ".join(f'<link rel="stylesheet" href="{stylesheet}">' for stylesheet in stylesheets),
            inline_scripts="\n    ".join(f'<script>\n{code}\n    </script>' for code in inline_scripts),
            table_name=f"{subtype_id} {table_date}",
            body=f"<pre>\n{pprint.pformat(data)}</pre>"
    ),
    content_type='text/html')

# ----------------------------------------------------------------------

def collect_table_data(subtype_id, table_date):

    def collect_table_data_part():
        for patt in ["i-none", "i-1280", "f-*", "b-*"]:
            for subdir in Path(subtype_id).glob(patt):
                yield {
                    "type": "individual" if patt[0] == "i" else "chain",
                    **dict(make_entry(en) for en in subdir.glob(f"*-{table_date}.*"))
                    }

    def make_entry(filename):
        suffx = filename.suffixes
        if suffx[-1] == ".ace":
            if len(suffx) == 2:
                return (suffx[0][1:], str(filename))
            else:
                return ("scratch", str(filename))
        elif suffx[-1] == ".json":
            if len(suffx) == 3:
                return (f"{suffx[1][1:]}_{suffx[0][1:]}", str(filename))
            else:
                return (f"scratch_{suffx[0][1:]}", str(filename))
        else:
            return (f"unknown-{''.join(suffx)}", str(filename))

    return {
        "subtype_id": subtype_id,
        "table_date": table_date,
        "parts": [part_data for part_data in collect_table_data_part() if part_data],
        **format_subtype(subtype_id=subtype_id)
    }

# ----------------------------------------------------------------------

# sSubtypeDisplay = {"h1pdm": "A(H1N1)", "h3": "A(H3N2)", "bvic": "B/Vic", "byam": "B/Yam"}

def format_subtype(subtype_id):
    subtype, assay, *fields = subtype_id.split("-")
    if assay == "hi":
        rbc = "-".join(fields[:-1])
    else:
        rbc = None
    return {
        "subtype": subtype,
        "assay": assay,
        "rbc": rbc,
        "lab": fields[-1],
        }

# ======================================================================
