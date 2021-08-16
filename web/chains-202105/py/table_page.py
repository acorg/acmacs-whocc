import json, pprint
from pathlib import Path
from aiohttp import web
from web_chains_202105 import utils

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
    <h2 id="title"></h2>
{body}
  </body>
</html>
"""

# ======================================================================

def table_page(request, subtype_id, table_date):
    global sTablePage

    remote_scripts = [
        "js/jquery.js",
        "js/table-page.js",
        ]
    stylesheets = [
        "js/maps.css",
        "js/table-page.css",
        ]

    data = collect_table_data(request=request, subtype_id=subtype_id, table_date=table_date)

    inline_scripts = [
        f"table_page_data =\n{json.dumps(data, separators=(',', ':'))};",
        ]

    return web.Response(
        text=sTablePage.format(
            remote_scripts="\n    ".join(f'<script src="{script}" type="module"></script>' for script in remote_scripts),
            stylesheets="\n    ".join(f'<link rel="stylesheet" href="{stylesheet}">' for stylesheet in stylesheets),
            inline_scripts="\n    ".join(f'<script>\n{code}\n    </script>' for code in inline_scripts),
            table_name=f"{subtype_id} {table_date}",
            body="", # f"<pre>\n{pprint.pformat(data)}</pre>"
    ),
    content_type='text/html')

# ----------------------------------------------------------------------

def collect_table_data(request, subtype_id, table_date):
    from web_chains_202105.chart import get_chart

    def collect_table_data_part():
        for patt in ["i-*", "f-*", "b-*"]:
            for subdir in sorted(Path(subtype_id).glob(patt), reverse=True):
                entries = make_entries(subdir, patt[0] == "i")
                if entries:
                    yield {
                        "type": "individual" if patt[0] == "i" else "chain",
                        "chain_id": subdir.name,
                        **entries
                        }

    def make_entries(subdir, individual):
        entries = {}
        filenames = list(subdir.glob(f"*{table_date}.*"))
        if not filenames and subdir.name[0] == "b": # workaround for the backward chain step naming problem
            filenames = list(subdir.glob(f"*.{table_date}-*"))
        for filename in filenames:
            key1, key2 = keys_for_filename(filename)
            if individual and key1 == "scratch":
                key1 = "individual"
            entries.setdefault(key1, {})[key2] = str(filename)
            if key2 == "ace":
                chart = get_chart(request=request, filename=filename)
                chart_date = chart.date()
                entries[key1]["date"] = chart_date
                entries.setdefault("date", chart_date)
        return entries

    def keys_for_filename(filename):
        suffx = filename.suffixes
        if suffx[-1] == ".ace":
            if len(suffx) >= 2: # 123.20160113-20210625.incremental.ace
                return (suffx[-2][1:], "ace")
            else:
                return ("scratch", "ace")
        elif suffx[-1] == ".json":
            if len(suffx) >= 3: # 123.20160113-20210625.scratch.grid.json
                return (suffx[-3][1:], suffx[-2][1:])
            else:
                return ("scratch", suffx[-2][1:])
        else:
            return ("unknown", "".join(suffx))

    return {
        "subtype_id": subtype_id,
        "table_date": table_date,
        "parts": [part_data for part_data in collect_table_data_part() if part_data],
        **utils.format_subtype(request=request, subtype_id=subtype_id)
    }

# ----------------------------------------------------------------------


# ======================================================================
