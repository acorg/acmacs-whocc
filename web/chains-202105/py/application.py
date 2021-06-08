import pprint
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
    <style>
    </style>
    <script src="js/a.js"></script>
    <title>Chains 202105</title>
  </head>
  <body>
    <h2>Chains 202105</h2>
  </body>
</html>
"""

@routes.get('/')
async def index(request):
    global sINDEX
    return web.Response(text=sINDEX, content_type='text/html')

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
