import sys, os, re, json, datetime, time
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

    
    # state.save(to=sys.stderr)
    # exit(1)
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

#  type: m(erge) i(ncremental) s(cratch)
#  path: str
#  depends: [step_id]
#  src: [filename]
#  out: [filename]
#  stress: float

class Step:

    def __init__(self, read_from=None, output_dir=None, table_dates=None, source_tables=None, steps=None, **args):
        if read_from:
            for key, val in read_from.items():
                setattr(self, key, val)
        else:
            for key, val in args.items():
                setattr(self, key, val)
            self.table_date = table_dates[self.table_no]
            self.make_output_filenames(output_dir)
            try:
                make_type = getattr(self, f"make_{self.type}")
            except AttributeError:
                raise Error(f"""Unsupported step type {self.type!r}""")
            make_type(source_tables=source_tables, table_dates=table_dates, steps=steps)

    def serialize(self):
        return vars(self)

    def state(self, chain_state):
        if self.is_completed():
            return "completed"
        if self.is_failed():
            return "FAILED"
        if self._is_running():
            return "running"
        if self._is_ready(chain_state):
            return "ready"      # can be run
        return "not-ready"

    def is_completed(self):
        if not self.out:
            module_logger.warning(f"step {self.step_id()} has not output, it is always completed")
            return True
        try:
            now = time.time()
            return all((Path(out).stat().st_mtime - now) > 2 for out in self.out) # out was modified more than 2 seconds ago
        except FileNotFoundError:
            return False

    def is_failed(self):
        return False

    def is_ready(self, chain_state):
        return self.state(chain_state) == "ready"

    def _is_running(self):
        return False

    def _is_ready(self, chain_state):
        return not self.depends or all(chain_state.is_completed(dep) for dep in self.depends)

    def make_output_filenames(self, output_dir):
        self.out = [output_dir.joinpath(self.step_id() + ".ace")]

    def step_id(self):
        return self.make_step_id(table_no=self.table_no, type=self.type, table_date=self.table_date)

    @classmethod
    def make_step_id(cls, table_no, type, table_date):
        return f"{table_no:03d}.{type}.{table_date}"

    def make_m(self, source_tables, table_dates, steps):
        if self.table_no == 1:
            raise Error(f"""Cannot use step type {self.type!r} for table {self.table_no}""")
        elif self.table_no == 2:
            self.depends = [self.make_step_id(table_no=self.table_no-1, type="s", table_date=table_dates[self.table_no-1])]
        else:
            self.depends = [self.make_step_id(table_no=self.table_no-1, type=st, table_date=table_dates[self.table_no-1]) for st in ["s", "i"]]

    def make_s(self, source_tables, table_dates, steps):
        if self.table_no == 1:
            self.src = [source_tables[self.table_no]]
        else:
            prev_id = self.make_step_id(table_no=self.table_no, type="m", table_date=self.table_date)
            self.depends = [prev_id]
            self.src = steps[prev_id].out

    def make_i(self, source_tables, table_dates, steps):
        if self.table_no == 1:
            raise Error(f"""Cannot use step type {self.type!r} for table {self.table_no}""")
        prev_id = self.make_step_id(table_no=self.table_no, type="m", table_date=self.table_date)
        self.depends = [prev_id]
        self.src = steps[prev_id].out

# ----------------------------------------------------------------------

class State:

    def __init__(self, source_tables, param):
        self.state = {"setup": {"number_of_optimizations": 0}, "steps": {}}
        self.state_file = Path(param["state_filename"])
        self.output_dir = Path(param["output_dir"])
        self.load(source_tables, param)

    def is_completed(self, step_id):
        return self.steps[step_id].is_completed()

    def ready(self):            # returns list of step_id's in ready state
        return [step_id for step_id, step in self.steps.items() if step.is_ready(self)]

    # ----------------------------------------------------------------------

    def load(self, source_tables, param):
        self.steps = {}
        if self.state_file.exists():
            self.state = json.load(self.state_file.open())
            for step_id, step_data in self.state["steps"].items():
                self.steps[step_id] = Step(read_from=step_data)
            self.state.pop("steps")
        if self.update(source_tables, param):
            self.save()

    def save(self, to=None):
        if to is None:
            if self.state_file.exists():
                self.state_file.rename(str(self.state_file) + "~")
            to = self.state_file.open("w")
        def serialize(obj):
            if hasattr(obj, "serialize"):
                return obj.serialize()
            if isinstance(obj, Path):
                return str(obj)
            raise TypeError()
        self.state["steps"] = self.steps
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
        table_dates = [None]
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
                step = Step(output_dir=self.output_dir, path=step_path, table_no=table_no, type=substep, table_dates=table_dates, source_tables=source_tables, steps=self.steps)
                step_id = step.step_id()
                if step_id not in self.steps:
                    self.steps[step_id] = step
                    updated = True
                elif self.steps[step_id].path != step_path:
                    raise Error(f"""{step_id!r} already in steps has different path {self.steps[step_id].path!r} vs {step_path!r}""")
        return updated

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
    logging.basicConfig(level=param["log"]["level"], format=param["log"]["format"])
    lh = logging.FileHandler(log_filename)
    lh.setLevel(param["log"]["level"])
    lh.setFormatter(logging.Formatter(param["log"]["format"]))
    logging.getLogger().addHandler(lh)

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
