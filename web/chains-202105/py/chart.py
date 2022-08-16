import sys, os, threading, re, datetime, time, pprint, subprocess
from pathlib import Path
import acmacs
from acmacs_py import utils, mapi_utils

# ======================================================================

def get_chart(request, filename :Path, populate_seqdb: bool = True):
    filename_s = str(filename)
    return acmacs.Chart(filename_s)

    # charts = request.app["charts"]
    # chart = charts.get(filename_s)
    # if not chart:
    #     # use /syn/eu/ac/results/chains-202105/bin/populate.sh *.ace to re-populate using parallel
    #     #
    #     # if populate_seqdb:
    #     #     lock_filename = filename.with_suffix(".lock")
    #     #     try:
    #     #         for loop_count in range(1000):
    #     #             if not lock_filename.exists():
    #     #                 break;
    #     #             time.sleep(0.2)
    #     #         if lock_filename.exists():
    #     #             print(f">> {filename} still locked with {lock_filename} ", file=sys.stderr)
    #     #         lock_filename.touch(exist_ok=True)
    #     #         start = datetime.datetime.now()
    #     #         subprocess.call([str(Path(os.environ["AE_ROOT"], "bin", "seqdb-chart-populate")), filename_s])
    #     #         print(f">>>> [{os.getpid()}] {filename_s}: populating with seqdb4 <{datetime.datetime.now() - start}>", file=sys.stderr)
    #     #     except Exception as err:
    #     #         print(f"> {filename}: cannot populate with seqdb4: {err}", file=sys.stderr)
    #     #     finally:
    #     #         lock_filename.unlink(missing_ok=True)
    #     chart = charts[filename_s] = acmacs.Chart(filename_s)
    # return chart

# ----------------------------------------------------------------------

def export_chart(request, filename :Path, chart :acmacs.Chart):
    filename_s = str(filename)
    charts = request.app["charts"]
    charts[filename_s] = chart
    # to avoid race of writing and reading the file by multiple processes, save it into another filename and then rename
    export_filename = filename.with_suffix(f".{os.getpid()}.ace")
    chart.export(export_filename)
    os.rename(str(export_filename), filename_s)
    # print(f">>> [{os.getpid()}.{threading.get_native_id()}] {filename} re-written")

# ======================================================================

def get_map(request, coloring: str, size: int, ace: str = None, ace1: str = None, ace2: str = None, image_type: str = "png", save_chart: bool = False, type: str = "map"):
    if type == "map":
        ace_filename = Path(ace)
        reorient_master_filename = find_reorient_master(ace_filename.parent)
        output_filename = png_dir(ace_filename).joinpath(f"{ace_filename.stem}.{encode_for_filename(coloring)}.{size}.{image_type}")
        if utils.older_than(output_filename, ace_filename, reorient_master_filename):
            make_map(request, output=output_filename, ace_filename=ace_filename, coloring=coloring, size=int(size), reorient_master_filename=reorient_master_filename, save_chart=save_chart)
    elif type == "pc":
        ace1_filename = Path(ace1)
        ace2_filename = Path(ace2)
        output_filename = png_dir(ace1_filename).joinpath(f"pc-{ace1_filename.stem}-vs-{ace2_filename.stem}.{encode_for_filename(coloring)}.{size}.{image_type}")
        if utils.older_than(output_filename, ace1_filename, ace2_filename):
            make_pc(request, output=output_filename, ace1_filename=ace1_filename, ace2_filename=ace2_filename, coloring=coloring, size=int(size))
    else:
        output_filename = Path("/does-not-exist")
    if output_filename.exists():
        return output_filename.open("rb").read()
    else:
        return b""

# ======================================================================

sGrey = "#D0D0D0"
sTestAntigenSize = 10
sReferenceAntigenSize = sTestAntigenSize * 1.5
sSerumSize = sReferenceAntigenSize

# ----------------------------------------------------------------------

def make_map(request, output :Path, ace_filename :Path, coloring :str, size :int, reorient_master_filename :Path = None, save_chart :bool = False):
    try:
        # print(f">>> [{os.getpid()}.{threading.get_native_id()}] make_map {coloring} {ace_filename} save:{save_chart}")
        chart = get_chart(request, ace_filename)
        if reorient_master_filename:
            chart.orient_to(master=get_chart(request, reorient_master_filename, populate_seqdb=False))
        # chart.populate_from_seqdb()

        drw = acmacs.ChartDraw(chart)
        request.app["clade_data"].chart_draw_reset(drw=drw, grey=sGrey, test_antigen_size=sTestAntigenSize, reference_antigen_size=sReferenceAntigenSize, serum_size=sSerumSize)
        request.app["clade_data"].chart_draw_modify(drw=drw, mapi_key=coloring)
        if drw.chart().titers().number_of_layers() > 1:
            mark_recent_layer(drw=drw)
        drw.title(lines=["{stress}"])
        drw.legend(offset=[-10, -10], label_size=-1, point_size=-1, title=[])
        drw.calculate_viewport()
        print(f">>> drawing {output}", file=sys.stderr)
        drw.draw(output, size=size, open=False)
        if save_chart:
            # print(f">>> [{os.getpid()}.{threading.get_native_id()}] exporting chart {coloring} {ace_filename} save:{save_chart} {type(save_chart)} {save_chart == True}")
            export_chart(request, ace_filename, drw.chart())
    except Exception as err:
        print(f"> ERROR: chart::make_map failed: {err}", file=sys.stderr)

# ----------------------------------------------------------------------

def make_pc(request, output: Path, ace1_filename: Path, ace2_filename: Path, coloring: str, size: int):
    try:
        chart1 = get_chart(request, ace1_filename)
        # chart1.populate_from_seqdb()
        chart2 = get_chart(request, ace2_filename)

        drw = acmacs.ChartDraw(chart1)
        request.app["clade_data"].chart_draw_reset(drw=drw, grey=sGrey, test_antigen_size=sTestAntigenSize, reference_antigen_size=sReferenceAntigenSize, serum_size=sSerumSize)
        request.app["clade_data"].chart_draw_modify(drw=drw, mapi_key=coloring)
        arrow_sizes, procrustes_data = drw.procrustes_arrows(common=acmacs.CommonAntigensSera(chart1, chart2), secondary_chart=chart2, threshold=0.3)
        drw.title(lines=[f"RMS: {procrustes_data.rms:.4f}"], remove_all_lines=True)
        drw.legend(offset=[-10, -10], label_size=-1, point_size=-1, title=[])
        drw.calculate_viewport()
        drw.draw(output, size=size, open=False)
    except Exception as err:
        print(f"> ERROR: chart::make_pc failed: {err}")

# ----------------------------------------------------------------------

def mark_recent_layer(drw: acmacs.ChartDraw):
    selected = drw.chart().select_antigens(lambda ag: False)
    # print(f"===== {selected.size()} {selector} {args}")
    drw.modify(selected, outline_width=3, order="raise")

# ----------------------------------------------------------------------

# def draw_color(request, drw, coloring):
#     draw_color_by(drw, request.app["clade_data"].entry(coloring))

# ----------------------------------------------------------------------

# def draw_color_by(drw, data):
#     # print("draw_color_by", pprint.pformat(data))
#     for en in data:
#         if en.get("N") == "antigens":
#             args = {
#                 "fill": en.get("fill", "").replace("{clade-pale}", ""),
#                 "outline": en.get("outline", "").replace("{clade-pale}", ""),
#                 "outline_width": en.get("outline_width"),
#                 "order": en.get("order"),
#                 "legend": en.get("legend") and acmacs.PointLegend(format=en["legend"].get("label"), show_if_none_selected=en["legend"].get("show_if_none_selected")),
#             }

#             selector = en["select"]

#             def clade_match(clade, cldes):
#                 if clade[0] != "!":
#                     return clade in cldes
#                 else:
#                     return clade[1:] not in cldes

#             def sel(ag):
#                 good = True
#                 if good and selector.get("sequenced"):
#                     good = ag.antigen.sequenced()
#                 if good and (clade := selector.get("clade")):
#                     good = clade_match(clade, ag.antigen.clades())
#                 if good and (clade_all := selector.get("clade-all") or selector.get("clade_all")):
#                     good = all(clade_match(clade, ag.antigen.clades()) for clade in clade_all)
#                 if good and (aas := selector.get("amino-acid") or selector.get("amino_acid")):
#                     good = ag.antigen.sequence_aa().matches_all(aas)
#                 return good
#             selected = drw.chart().select_antigens(sel)
#             # print(f"===== {selected.size()} {selector} {args}")
#             drw.modify(selected, **{k: v for k, v in args.items() if v})

# ----------------------------------------------------------------------

# def draw_reset(drw):
#     chart = drw.chart()
#     drw.modify(chart.select_antigens(lambda ag: ag.antigen.reference()), fill="transparent", outline=sGrey, size=sReferenceAntigenSize)
#     drw.modify(chart.select_antigens(lambda ag: not ag.antigen.reference()), fill=sGrey, outline=sGrey, size=sTestAntigenSize)
#     drw.modify(chart.select_antigens(lambda ag: ag.passage.is_egg()), shape="egg")
#     drw.modify(chart.select_antigens(lambda ag: bool(ag.reassortant)), rotation=0.5)
#     drw.modify(chart.select_all_sera(), fill="transparent", outline=sGrey, size=sSerumSize)
#     drw.modify(chart.select_sera(lambda sr: sr.passage.is_egg()), shape="uglyegg")

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
