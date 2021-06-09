import os, re, json, pprint
from pathlib import Path
from aiohttp import web

routes = web.RouteTableDef()

async def app():
    app = web.Application()
    app.add_routes(routes)
    # app.router.add_get('/', index)
    app.router.add_static("/js/", path="js", name="js")
    return app

# ----------------------------------------------------------------------

sINDEX = """<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8" />
    {stylesheets}
    {remote_scripts}
    {inline_scripts}
    <title>Chains 202105</title>
  </head>
  <body>
    <h2>Chains 202105</h2>
{body}
  </body>
</html>
"""

@routes.get('/')
async def index(request):
    global sINDEX
    remote_scripts = [
        "js/jquery.js",
        "js/directories.js",
        "js/chain-index.js",
        ]
    stylesheets = [
        "js/chain-index.css",
        ]
    inline_scripts = [
        f"index_subtypes =\n{json.dumps(collect_index_subtypes(), indent=1)};",
        ]
    return web.Response(
        text=sINDEX.format(
            remote_scripts="\n    ".join(f'<script src="{script}"></script>' for script in remote_scripts),
            inline_scripts="\n    ".join(f'<script>\n{code}\n    </script>' for code in inline_scripts),
            stylesheets="\n    ".join(f'<link rel="stylesheet" href="{stylesheet}">' for stylesheet in stylesheets),
            # body=f"<pre>{pprint.pformat(vars(request))}</pre>"
            body=""
        ),
        content_type='text/html')

# ======================================================================

sReSubtypeDirName = re.compile(r"(?P<subtype>h1pdm|h3|bvic|byam)-(?P<assay>hi|hint|fra|prn|mn)(?:-(?P<rbc>guinea-pig|turkey|chicken))?-(?P<lab>cdc|cnic|crick|niid|vidrl)")

def collect_index_subtypes():
    return [{"id": fn.name, **fn_m.groupdict()} for fn in Path(".").glob("*") if fn.is_dir() and (fn_m := sReSubtypeDirName.match(fn.name))]

# ======================================================================

# import os, urllib.parse, wsgiref.util, pprint

# def app(environ, start_response):
#     query = urllib.parse.parse_qs(environ['QUERY_STRING']) # dict "?key1=value1&key2=value2" -> {'key1': ['value1'], 'key2': ['value2']}
#     # 'HTTP_ACCEPT_ENCODING': 'gzip, deflate, br'
#     # 'PATH_INFO': '/subdir/name',
#     # pprint.pformat(environ)
#     data = [
#         f"urllib.parse.parse_qs: {pprint.pformat(query_string)}",
#         f"urllib.parse.urlparse: {urllib.parse.urlparse(environ['RAW_URI'])}",
#         f"",
#         f"",
#         # f"request_uri: {wsgiref.util.request_uri(environ)}",
#         # f"application_uri: {wsgiref.util.application_uri(environ)}",
#         # f"shift_path_info: {wsgiref.util.shift_path_info(environ)}",
#         # f"shift_path_info: {wsgiref.util.shift_path_info(environ)}",
#         # f"shift_path_info: {wsgiref.util.shift_path_info(environ)}",
#         # f"shift_path_info: {wsgiref.util.shift_path_info(environ)}",
#     ]
#     data_b = "\n".join(data).encode("utf-8")
#     status = '200 OK'
#     response_headers = [
#         ('Content-type', 'text/plain'),
#         ('Content-Length', str(len(data_b)))
#     ]
#     start_response(status, response_headers)
#     return iter([data_b])
