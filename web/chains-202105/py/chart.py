import re, pprint
from pathlib import Path
import acmacs
from acmacs_py import utils

# ======================================================================

def get_chart(request, filename :Path):
    filename_s = str(filename)
    charts = request.app["charts"]
    chart = charts.get(filename_s)
    if not chart:
        chart = charts[filename_s] = acmacs.Chart(filename_s)
    return chart

# ======================================================================

def get_map(request, ace, coloring, size):
    ace_filename = Path(ace)
    reorient_master_filename = find_reorient_master(ace_filename.parent)
    output_filename = png_dir(ace_filename).joinpath(f"{ace_filename.stem}.{encode_for_filename(coloring)}.{size}.png")
    if utils.older_than(output_filename, ace_filename, reorient_master_filename):
        make_map(request, output=output_filename, ace_filename=ace_filename, coloring=coloring, size=int(size), reorient_master_filename=reorient_master_filename)
    if output_filename.exists():
        return output_filename.open("rb").read()
    else:
        return b""

# ======================================================================

sGrey = "#D0D0D0"
sTestAantigenSize = 10
sReferenceAntigenSize = sTestAantigenSize * 1.5
sSerumSize = sReferenceAntigenSize

# ----------------------------------------------------------------------

def make_map(request, output :Path, ace_filename :Path, coloring :str, size :int, reorient_master_filename :Path = None):
    try:
        chart = get_chart(request, ace_filename)
        if reorient_master_filename:
            chart.orient_to(master=get_chart(request, reorient_master_filename))
        chart.populate_from_seqdb()

        drw = acmacs.ChartDraw(chart)
        draw_reset(drw)
        draw_color(request, drw, coloring)
        drw.title(lines=["{stress}"])
        drw.legend(offset=[-10, -10], label_size=-1, point_size=-1, title=[])
        drw.calculate_viewport()
        drw.draw(output, size=size, open=False)
    except Exception as err:
        print(f"> ERROR: chart::make_map failed: {err}")

# ----------------------------------------------------------------------

def draw_color(request, drw, coloring):
    draw_color_by(drw, request.app["clade_data"].entry(coloring))

# ----------------------------------------------------------------------

def draw_color_by(drw, data):
    # print("draw_color_by", pprint.pformat(data))
    for en in data:
        if en.get("N") == "antigens":
            args = {
                "fill": en.get("fill", "").replace("{clade-pale}", ""),
                "outline": en.get("outline", "").replace("{clade-pale}", ""),
                "outline_width": en.get("outline_width"),
                "order": en.get("order"),
                "legend": en.get("legend") and acmacs.PointLegend(format=en["legend"].get("label"), show_if_none_selected=en["legend"].get("show_if_none_selected")),
            }

            selector = en["select"]

            def clade_match(clade, cldes):
                if clade[0] != "!":
                    return clade in cldes
                else:
                    return clade[1:] not in cldes

            def sel(ag):
                good = True
                if good and selector.get("sequenced"):
                    good = ag.antigen.sequenced()
                if good and (clade := selector.get("clade")):
                    good = clade_match(clade, ag.antigen.clades())
                if good and (clade_all := selector.get("clade-all")):
                    good = all(clade_match(clade, ag.antigen.clades()) for clade in clade_all)
                if good and (aas := selector.get("amino-acid")):
                    good = ag.antigen.sequence_aa().matches_all(aas)
                return good
            selected = drw.chart().select_antigens(sel)
            # print(f"===== {selected.size()} {selector} {args}")
            drw.modify(selected, **{k: v for k, v in args.items() if v})

# ----------------------------------------------------------------------

def draw_reset(drw):
    chart = drw.chart()
    drw.modify(chart.select_antigens(lambda ag: ag.antigen.reference()), fill="transparent", outline=sGrey, size=sReferenceAntigenSize)
    drw.modify(chart.select_antigens(lambda ag: not ag.antigen.reference()), fill=sGrey, outline=sGrey, size=sTestAantigenSize)
    drw.modify(chart.select_antigens(lambda ag: ag.passage.is_egg()), shape="egg")
    drw.modify(chart.select_antigens(lambda ag: bool(ag.reassortant)), rotation=0.5)
    drw.modify(chart.select_all_sera(), fill="transparent", outline=sGrey, size=sSerumSize)
    drw.modify(chart.select_sera(lambda sr: sr.passage.is_egg()), shape="uglyegg")

# ======================================================================

def png_dir(ace):
    pd = ace.parent.joinpath("png")
    pd.mkdir(exist_ok=True)
    return pd

# ----------------------------------------------------------------------

sReEncoder = re.compile(r"[\(\)\[\]/\"']")

def encode_for_filename(name):
    return sReEncoder.sub("-", name)

# ----------------------------------------------------------------------

sReorientMasterName = "reorient-master.ace"

def find_reorient_master(ace_dir):
    reorient_master_filename = ace_dir.joinpath(sReorientMasterName)
    if not reorient_master_filename.exists():
        reorient_master_filename = ace_dir.parent.joinpath(sReorientMasterName)
        if not reorient_master_filename.exists():
            reorient_master_filename = None
    return reorient_master_filename

# ----------------------------------------------------------------------
