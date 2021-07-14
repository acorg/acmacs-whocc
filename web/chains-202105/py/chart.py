import re, pprint
from pathlib import Path
import acmacs
from acmacs_py import utils

# ======================================================================

def get_chart(request, filename):
    charts = request.app["charts"]
    chart = charts.get(filename)
    if not chart:
        chart = charts[filename] = acmacs.Chart(filename)
    return chart

# ======================================================================

def get_map(request, ace, coloring, size):
    ace = Path(ace)
    output_filename = png_dir(ace).joinpath(f"{ace.stem}.{encode_for_filename(coloring)}.{size}.png")
    if utils.older_than(output_filename, ace):
        make_map(request, output=output_filename, ace=ace, coloring=coloring, size=int(size))
    return output_filename.open("rb").read()

# ======================================================================

sGrey = "#D0D0D0"
sTestAantigenSize = 10
sReferenceAntigenSize = sTestAantigenSize * 1.5
sSerumSize = sReferenceAntigenSize

# ----------------------------------------------------------------------

def make_map(request, output, ace, coloring, size):
    chart = get_chart(request, ace)
    drw = acmacs.ChartDraw(chart)
    draw_reset(drw)
    draw_color(request, drw, coloring)
    drw.title(lines=["{stress}"])
    drw.legend(offset=[-10, -10], label_size=-1, point_size=-1, title=[])
    drw.calculate_viewport()
    drw.draw(output)

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
