import re
from pathlib import Path
import acmacs

# ======================================================================

def get_chart(request, filename):
    charts = request.app["charts"]
    chart = charts.get(filename)
    if not chart:
        chart = charts[filename] = acmacs.Chart(filename)
    return chart

# ======================================================================

def make_map(request, ace, coloring, size):
    ace = Path(ace)
    output_filename = png_dir(ace).joinpath(f"{ace.stem}.{encode_for_filename(coloring)}.{size}.png")
    chart = get_chart(request, ace)

    drw = acmacs.ChartDraw(chart)
    # reset(drw)
    # clades(drw, data=clades_data)
    drw.title(lines=["{stress}"])
    drw.calculate_viewport()
    drw.draw(output_filename)

    return output_filename.open("rb").read()

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
