import sys, os, re, datetime, json
from pathlib import Path
import logging; module_logger = logging.getLogger(__name__)

sys.path[:0] = [str(Path(os.environ["ACMACSD_ROOT"]).resolve().joinpath("lib"))]
import acmacs

# ======================================================================

sDefaultParameters = {
    "number_of_optimizations": 100,
    "number_of_dimensions": 2,
    "minimum_column_basis": "none",

    "log": {
        "dir": Path("log"),
        "level": logging.DEBUG,
        "format": "%(levelname)s %(asctime)s: %(message)s",
    },

    "state_filename": Path("state.json"),
    "output_dir": Path("out"),
}

class Error (RuntimeError): pass

# sState = {"setup": {"number_of_optimizations": 0}, "steps": {}}
# sStateFile = Path("state.json")
# sOutputDir = Path("out")
# sOutputDir.mkdir(parents=True, exist_ok=True)

# ======================================================================

def main(source_tables, param):
    try:
        param = {**sDefaultParameters, **param}
        setup_logging(param)
        chain(source_tables, param)
    except Error as err:
        module_logger.error(f"{err}")
        exit(1)

# ----------------------------------------------------------------------

def chain(source_tables, param):
    state = State(source_tables, param)
    state.save(to=sys.stderr)
    exit(1)
    # mrg = acmacs.Chart(str(source_tables[0]))
    # relax(mrg, 0, param)
    # for step, c1_name in enumerate(source_tables[1:], start=1):
    #     mrg_incremental, report = acmacs.merge(mrg, acmacs.Chart(str(c1_name)), type="incremental")
    #     mrg_scratch = mrg_incremental.clone("plot_spec")
    #     relax_incremental(mrg_incremental, step, param)
    #     relax(mrg_scratch, step, param)
    #     if mrg_incremental.projection().stress() < mrg_scratch.projection().stress():
    #         mrg = mrg_incremental
    #         module_logger.info(f"{step:3d}: --> incremental {mrg.projection().stress():9.4f}")
    #     else:
    #         mrg = mrg_scratch
    #         module_logger.info(f"{step:3d}: --> scratch {mrg.projection().stress():9.4f}")

# ======================================================================

# self.state["steps"]:
#   type: m(erge) i(ncremental) s(cratch)
#   path: str
#   depends: [step_id]
#   src: [filename]
#   out: [filename]
#   stress: float

class State:

    def __init__(self, source_tables, param):
        self.state = {"setup": {"number_of_optimizations": 0}, "steps": {}}
        self.state_file = Path(param["state_filename"])
        self.output_dir = Path(param["output_dir"])
        self.load(source_tables, param)

    def load(self, source_tables, param):
        if self.state_file.exists():
            self.state = json.load(self.state_file.open())
        if self.update(source_tables, param):
            self.save()

    def save(self, to=None):
        if to is None:
            if self.state_file.exists():
                self.state_file.rename(str(self.state_file) + "~")
            to = self.state_file.open("w")
        def serialize(obj):
            if isinstance(obj, Path):
                return str(obj)
            raise TypeError()
        json.dump(self.state, to, indent=2, sort_keys=True, default=serialize)

    def update(self, source_tables, param):
        updated = False
        for changeable_value in ["number_of_optimizations"]:
            updated = self.update_value(changeable_value, param[changeable_value], raise_if_changed=False)
        for readonly_value in ["number_of_dimensions", "minimum_column_basis"]:
            updated = self.update_value(readonly_value, param[readonly_value], raise_if_changed=True) or updated
        updated = self.update_steps(source_tables) or updated
        return updated

    def update_value(self, name, value, raise_if_changed):
        updated = False
        setup = self.state.setdefault("setup", {})
        if not setup.get(name):
            module_logger.info(f"""{name} set to {value!r}""")
            setup[name] = value
            updated = True
        elif setup[name] != value:
            if raise_if_changed:
                raise Error(f"""cannot change {name} from {setup[name]!r} to {value!r}""")
            else:
                module_logger.info(f"""{name} changed from {setup[name]!r} to {value!r}""")
                setup[name] = value
                updated = True
        return updated

    def update_steps(self, source_tables):
        updated = False
        step_path = ""
        table_dates = []
        for table_no, table in enumerate((Path(tab) for tab in source_tables), start=1):
            if not table.exists():
                raise Error(f"""Table {table} does not exist""")
            table_date = re.search(r"-(\d+)\.", table.name, re.I).group(1)
            if table_date in table_dates:
                raise Error(f"""{table_date} {table} already in the chain?""")
            table_dates.append(table_date)
            if table_no == 1:
                substeps = ["s"]
                step_path = table_date
            else:
                substeps = ["m", "i", "s"]
                step_path += f":{table_date}"
            for substep in substeps:
                step_id = self.make_step_id(table_no=table_no, step_type=substep, table_date=table_date)
                if step_id not in self.state["steps"]:
                    self.state["steps"][step_id] = self.make_step({"type": substep, "path": step_path}, table_no=table_no, source_tables=source_tables, table_dates=table_dates)
                    updated = True
                elif self.state["steps"][step_id]["path"] != step_path:
                    raise Error(f"""{step_id!r} already in steps has different path {self.state["steps"][step_id]["path"]!r} vs {step_path!r}""")
            # print(table_no, table_date, table)
        return updated

    def make_step_id(self, table_no, step_type, table_date):
        return f"{table_no:03d}.{step_type}.{table_date}"

    def make_step(self, data, table_no, source_tables, table_dates):
        table_date = table_dates[table_no - 1]
        data["out"] = self.make_output_filenames(table_no=table_no, step_type=data["type"], table_date=table_date)
        if data["type"] == "m":
            if table_no == 1:
                raise Error(f"""Cannot use step type {data["type"]!r} for table {table_no}""")
            elif table_no == 2:
                data["depends"] = [self.make_step_id(table_no=table_no-1, step_type="s", table_date=table_dates[table_no-2])]
            else:
                data["depends"] = [self.make_step_id(table_no=table_no-1, step_type=st, table_date=table_dates[table_no-2]) for st in ["s", "i"]]
        elif data["type"] == "s":
            if table_no == 1:
                data["src"] = [source_tables[table_no - 1]]
            else:
                prev_id = self.make_step_id(table_no=table_no, step_type="m", table_date=table_date)
                data["depends"] = [prev_id]
                data["src"] = self.state["steps"][prev_id]["out"]
        elif data["type"] == "i":
            if table_no == 1:
                raise Error(f"""Cannot use step type {data["type"]!r} for table {table_no}""")
            prev_id = self.make_step_id(table_no=table_no, step_type="m", table_date=table_date)
            data["depends"] = [prev_id]
            data["src"] = self.state["steps"][prev_id]["out"]
        else:
            raise Error(f"""Unsupported step type {data["type"]!r}""")
        return data

    def make_output_filenames(self, table_no, step_type, table_date):
        return [self.output_dir.joinpath(self.make_step_id(table_no=table_no, step_type=step_type, table_date=table_date) + ".ace")]

# ----------------------------------------------------------------------

# def relax(chart, step, param):
#     start = datetime.datetime.now()
#     chart.relax(number_of_dimensions=param["number_of_dimensions"], number_of_optimizations=param["number_of_optimizations"], minimum_column_basis=param["minimum_column_basis"])
#     chart.keep_projections(10)
#     module_logger.info(f"{step:3d}:s: {chart.projection().stress():9.4f}  {chart.make_name():70s} [{datetime.datetime.now() - start}]")
#     export(chart, step=step, type="s", param=param)

# # ----------------------------------------------------------------------

# def relax_incremental(chart, step, param):
#     start = datetime.datetime.now()
#     chart.relax_incremental(number_of_optimizations=param["number_of_optimizations"])
#     chart.keep_projections(10)
#     module_logger.info(f"{step:3d}:i: {chart.projection().stress():9.4f}  {chart.make_name():70s} [{datetime.datetime.now() - start}]")
#     export(chart, step=step, type="i", param=param)

# # ----------------------------------------------------------------------

# def export(chart, step, type, param):
#     chart.export(str(sOutputDir.joinpath(f"{step:03d}.{type}.{chart.lab().lower()}-{chart.subtype_short().lower()}-{chart.assay_hi_or_neut()}-{chart.date()}.ace")), sys.argv[0])

# ----------------------------------------------------------------------

def setup_logging(param):
    log_dir = Path(param["log"]["dir"])
    log_dir.mkdir(parents=True, exist_ok=True)
    log_filename = log_dir.joinpath(f"{datetime.datetime.now().strftime('%Y-%m%d-%H%M')}.log")
    # lh = logging.FileHandler(log_filename)
    # lh.setLevel(param["loglevel"])
    # lh.setFormatter(logging.Formatter(param["logformat"]))
    # logging.getLogger().addHandler(lh)
    logging.basicConfig(filename=log_filename, level=param["log"]["level"], format=param["log"]["format"])

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
