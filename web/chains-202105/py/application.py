import sys, os, re, json, pprint, traceback
from pathlib import Path
from aiohttp import web

routes = web.RouteTableDef()

async def app():
    from web_chains_202105 import directories
    app = web.Application(middlewares=[exception_middleware])
    app.add_routes(routes)
    app.router.add_static("/js/", path="js", name="js")
    directories.load(app)

    app["charts"] = {}          # loaded charts: {ace-path: acmacs.Chart}, loading is done on demand
    sys.path[:0] = ["lib"] # to be abel to import acmacs module (acmacs_py) by web_chains_202105.chart

    return app

# ======================================================================

# https://docs.aiohttp.org/en/stable/web_advanced.html
@web.middleware
async def exception_middleware(request, handler):
    try:
        return await handler(request)
    except Exception as err:
        import cgitb
        context = 10
        if "/api" in request.path:
            return web.json_response({"ERROR": str(err), "tb": cgitb.text(sys.exc_info(), context=context)})
        else:
            return web.Response(text=cgitb.html(sys.exc_info(), context=context), content_type='text/html')

# ======================================================================
# pages
# ======================================================================

@routes.get("/")
async def index(request):
    from web_chains_202105.index_page import index_page
    return index_page(request=request)

# ----------------------------------------------------------------------

@routes.get("/table")
async def table_data(request):
    from web_chains_202105.table_page import table_page
    return table_page(request=request, subtype_id=request.query["subtype_id"], table_date=request.query["date"])

@routes.get("/chain")
async def chain_data(request):
    from web_chains_202105.chain_page import chain_page
    return chain_page(request=request, subtype_id=request.query["subtype_id"], chain_id=request.query["chain_id"])

# ======================================================================
# images
# ======================================================================

def bool_from_str(src):
    return src.lower() in ["true", "yes", "1"]

sMimeType = {
    "png": "image/png",
    "pdf": "application/pdf",
    "ace": "application/octet-stream",
}

def image(request, image_type):
    if request.query["type"] == "map":
        from web_chains_202105.chart import get_map
        args = {kk: v for kk, v in ((k, t(request.query.get(k))) for k,t in [["ace", str], ["coloring", str], ["size", int], ["save_chart", bool_from_str]]) if v is not None}
        headers = {
            "pid": str(os.getpid()),
            "Content-Disposition": f'inline; filename="{Path(args["ace"]).with_suffix("." + image_type).name}"',
        }
        return web.Response(body=get_map(request=request, image_type=image_type, **args), content_type=sMimeType[image_type], headers=headers)
    else:
        print(f">> WARNING: unsupported {image_type}:", request.query)
        return web.Response(text=str(request.query), status=418, headers={"Error": f"unsupported {image_type}"})

@routes.get("/png")
async def png(request):
    return image(request=request, image_type="png")

@routes.get("/pdf")
async def pdf(request):
    return image(request=request, image_type="pdf")

# ======================================================================
# ace
# ======================================================================

@routes.get("/ace")
async def ace(request):
    args = {kk: v for kk, v in ((k, t(request.query.get(k))) for k,t in [["ace", Path]]) if v is not None}
    headers = {
        "pid": str(os.getpid()),
        "Content-Disposition": f'inline; filename="{args["ace"].name}"',
    }
    return web.Response(body=args["ace"].open("rb").read(), content_type=sMimeType["ace"], headers=headers)


# ======================================================================
# api
# ======================================================================

@routes.get("/api/subtype-data/")
async def subtype_data(request):
    from web_chains_202105.index_page import collect_tables_of_subtype
    subtype_id = request.query["subtype_id"]
    return web.json_response({
        "tables": collect_tables_of_subtype(subtype_id),
        "subtype_id": subtype_id,
    })

# ======================================================================
