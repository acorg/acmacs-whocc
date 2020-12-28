import sys, os, re, json, datetime, time, subprocess, pprint, socket, traceback
from pathlib import Path
import logging; module_logger = logging.getLogger(__name__)

# sys.path[:0] = [str(Path(os.environ["ACMACSD_ROOT"]).resolve().joinpath("lib"))]
import acmacs
from acmacs_base import email

# ======================================================================

def detect_processor():
    try:
        subprocess.check_output(["condor_version"])
        return "htcondor"
    except (subprocess.SubprocessError, FileNotFoundError):
        return "built-in"

sDefaultParameters = {
    "number_of_optimizations": 100,
    "number_of_dimensions": 2,
    "minimum_column_basis": "none",
    "projections_to_keep": 10,
    "incremental": True,        # make incremetal merge map at each step
    "scratch": True,            # make map from scratch at each step

    "log": {
        "dir": Path("log"),
        "level": logging.DEBUG,
        "format": "%(levelname)s %(asctime)s: %(message)s @@ %(pathname)s:%(lineno)d", # https://docs.python.org/3.9/library/logging.html#logrecord-attributes
    },

    "state_filename": Path("state.json"),
    "output_dir": Path("out"),
    "htcondor_dir": Path("htcondor"),
    "email": "eu@antigenic-cartography.org",
    "threads": 16,
    "number_of_optimizations_per_run": 100,
    "sleep_interval_when_not_ready": 10, # in seconds

    "processor": detect_processor(),
}

class Error (RuntimeError): pass
class StepFailed (Error): pass

# ======================================================================

def main(source_tables, param):
    exit_code = 0
    try:
        param = {**sDefaultParameters, **param}
        log_filename = setup_logging(param)
        if chain(source_tables, param):
            exit_code = 1
            email.send(to=param["email"], subject=f"""chain FAILED {socket.gethostname()} {os.getcwd()}""", body=f"""chain FAILED\n{socket.gethostname()}:{log_filename}""")
        else:
            email.send(to=param["email"], subject=f"""chain completed {socket.gethostname()} {os.getcwd()}""", body=f"""chain completed\n{socket.gethostname()}:{log_filename}""")
    except KeyboardInterrupt:
        print("KeyboardInterrupt", file=sys.stderr)
    except Error as err:
        module_logger.error(f"{err}")
        email.send(to=param["email"], subject=f"""chain EXCEPTION {socket.gethostname()} {os.getcwd()}""", body=f"""chain EXCEPTION\n{socket.gethostname()}:{log_filename}\n\n{traceback.format_exc()}""")
        exit_code = 2
    except Exception as err:
        module_logger.error(f"{err}", exc_info=True)
        email.send(to=param["email"], subject=f"""chain EXCEPTION {socket.gethostname()} {os.getcwd()}""", body=f"""chain EXCEPTION\n{socket.gethostname()}:{log_filename}\n\n{traceback.format_exc()}""")
        exit_code = 3
    exit(exit_code)

# ----------------------------------------------------------------------

def chain(source_tables, param):
    state = State(source_tables=source_tables, param=param, processor=processor_factory(param["processor"]))
    while state.has_todo():
        state.check_running()
        if not state.run_ready():
            # module_logger.info(f"""nothing ready, sleeping for {param["sleep_interval_when_not_ready"]} seconds""")
            time.sleep(param["sleep_interval_when_not_ready"])
    return state.is_failed()

# ======================================================================

#  _type: m(erge) i(ncremental) s(cratch)
#  path: str
#  depends: [step_id]
#  src: [filename]
#  out: [filename]
#  stress: float

class Step:

    sDateTimeFormat = "%Y-%m-%d %H:%M:%S"

    def __init__(self, type=None, read_from=None, output_dir=None, table_dates=None, source_tables=None, steps=None, setup=None, **args):
        self._type = type
        if read_from:
            for key, val in read_from.items():
                if key in ["start", "finish"]:
                    setattr(self, key, datetime.datetime.strptime(val, self.sDateTimeFormat))
                else:
                    setattr(self, key, val)
            getattr(self, "htcondor", {}).pop("check_reported", None) # temp value, must not be really saved
        else:
            for key, val in args.items():
                setattr(self, key, val)
            self.table_date = table_dates[self.table_no]
            self.make_output_filenames(output_dir)
            self.make(source_tables=source_tables, table_dates=table_dates, steps=steps, setup=setup)

    def serialize(self):
        return vars(self)

    def state(self, chain_state):
        if self.is_completed(chain_state):
            return "completed"
        if self.is_failed():
            return "FAILED"
        if self.is_running(chain_state):
            return "running"
        if self._is_ready(chain_state):
            return "ready"      # can be run
        return "not-ready"

    def is_completed(self, chain_state):
        if not self.out:
            module_logger.warning(f"step {self.step_id()} has not output, it is always completed")
            return True
        return all(Path(out).exists() for out in self.out)

    def is_failed(self):
        return getattr(self, "FAILED", False)

    def is_ready(self, chain_state):
        return self.state(chain_state) == "ready"

    def is_running(self, chain_state):
        return chain_state.processor.is_running(chain_state, self)

    def _is_ready(self, chain_state):
        return not getattr(self, "depends", None) or all(chain_state.is_step_completed(dep) for dep in self.depends)

    def make_output_filenames(self, output_dir):
        self.out = [output_dir.joinpath(self.step_id() + ".ace")]

    def step_id(self):
        return self.make_step_id(table_no=self.table_no, type=self._type, table_date=self.table_date)

    @classmethod
    def make_step_id(cls, table_no, type, table_date):
        return f"{table_no:03d}.{type}.{table_date}"

    # returns if step state changed
    def check(self, chain_state):
        try:
            if chain_state.processor.check(chain_state, self):
                chain_state.processor.merge_results(chain_state, self)
                return True
            else:
                return False
        except StepFailed:
            return True

# ----------------------------------------------------------------------

class StepMergeIncremental (Step):

    def type_desc(self):
        return "incremental merge"

    def make(self, source_tables, table_dates, steps, setup):
        self.depends = []
        if self.table_no == 1:
            raise Error(f"""Cannot use step type {self._type!r} for table {self.table_no}""")
        elif self.table_no == 2:
            self.depends.append(self.make_step_id(table_no=self.table_no-1, type="s", table_date=table_dates[self.table_no-1]))
        else:
            if setup["scratch"]:
                self.depends.append(self.make_step_id(table_no=self.table_no-1, type="s", table_date=table_dates[self.table_no-1]))
            if setup["incremental"]:
                self.depends.append(self.make_step_id(table_no=self.table_no-1, type="i", table_date=table_dates[self.table_no-1]))
        self.src = [None, source_tables[self.table_no-1]]

    def run(self, chain_state):
        # module_logger.debug(f"{self.step_id()} deps: {self.depends}")
        candidates = [chain_state.step(dep) for dep in self.depends]
        if len(candidates) == 2:
            if candidates[0].stress < candidates[1].stress:
                self.src[0] = candidates[0].out[0]
            else:
                self.src[0] = candidates[1].out[0]
            module_logger.info(f"choosing master for merge: {self.src[0]} <-- {self.depends[0]} {candidates[0].stress:.4f} vs. {self.depends[1]} {candidates[1].stress:.4f}")
        elif len(candidates) == 1:
            self.src[0] = candidates[0].out[0]
        else:
            raise RuntimeError(f"""{self.__class__}: unsupported "depends": {self.depends}""")
        chain_state.processor.merge_incremental(chain_state, self)

# ----------------------------------------------------------------------

class StepIncremental (Step):

    def type_desc(self):
        return "relax incremental"

    def make(self, source_tables, table_dates, steps, setup):
        if self.table_no == 1:
            raise Error(f"""Cannot use step type {self._type!r} for table {self.table_no}""")
        prev_id = self.make_step_id(table_no=self.table_no, type="m", table_date=self.table_date)
        self.depends = [prev_id]
        self.src = steps[prev_id].out

    def run(self, chain_state):
        chain_state.processor.relax_incremental(chain_state, self)

# ----------------------------------------------------------------------

class StepScratch (Step):

    def type_desc(self):
        return "relax from scratch"

    def make(self, source_tables, table_dates, steps, setup):
        if self.table_no == 1:
            self.src = [source_tables[self.table_no]]
        else:
            prev_id = self.make_step_id(table_no=self.table_no, type="m", table_date=self.table_date)
            self.depends = [prev_id]
            self.src = steps[prev_id].out

    def run(self, chain_state):
        chain_state.processor.relax_from_scratch(chain_state, self)

# ----------------------------------------------------------------------

def step_factory(type, **args):
    if type == "m":
        return StepMergeIncremental(type=type, **args)
    elif type == "i":
        return StepIncremental(type=type, **args)
    elif type == "s":
        return StepScratch(type=type, **args)
    else:
        raise RuntimeError(f"""Unsupported step type {type!r}""")

# ----------------------------------------------------------------------

class State:

    def __init__(self, source_tables, param, processor):
        self.state = {"setup": {"number_of_optimizations": 0}, "steps": {}}
        self.state_file = Path(param["state_filename"])
        self.output_dir = Path(param["output_dir"])
        self.htcondor_dir = Path(param["htcondor_dir"])
        self.email = param["email"]
        self.threads = param["threads"]
        self.source_tables = source_tables
        self.load(param)
        self.processor = processor

    def is_step_completed(self, step_id):
        return self.steps[step_id].is_completed(self)

    def is_failed(self):
        return any(step.is_failed() for step in self.steps.values())

    def ready(self):            # returns list of step_id's in ready state
        return [step_id for step_id, step in self.steps.items() if step.is_ready(self)]

    def has_todo(self):
        running = sum(1 if step.is_running(self) else 0 for step in self.steps.values())
        completed = sum(1 if step.is_completed(self) else 0 for step in self.steps.values())
        total = len(self.steps)
        report = (total, completed)
        if getattr(self, "has_todo_report", None) != report:
            module_logger.info(f"Completed {completed} of {total} ({int(completed / total * 100.0)}%) tables: {len(self.source_tables)}  currently running: {running}")
            self.has_todo_report = report
        if running:
            return True
        if self.is_failed():
            return False
        # nothing running and nothing failed
        return completed < total

    def check_running(self):
        for step in self.steps.values():
            if step.is_running(self):
                if step.check(self):
                    self.save()

    def run_ready(self):        # returns if anyone was ready
        run = False
        for step in self.steps.values():
            if step.is_ready(self):
                # module_logger.debug(f"running {step.step_id()}")
                step.run(self)
                self.save()
                run = True
        return run

    def step(self, step_id):
        return self.steps[step_id]

    # ----------------------------------------------------------------------

    def load(self, param):
        self.steps = {}
        if self.state_file.exists():
            self.state = json.load(self.state_file.open())
            for step_id, step_data in self.state["steps"].items():
                self.steps[step_id] = step_factory(step_data["_type"], read_from=step_data)
            self.state.pop("steps")
        if self.update(param):
            self.save()

    def save(self, to=None):

        def serialize(obj):
            if hasattr(obj, "serialize"):
                return obj.serialize()
            if isinstance(obj, Path):
                return str(obj)
            if isinstance(obj, datetime.datetime):
                return obj.strftime(Step.sDateTimeFormat)
            raise TypeError(type(obj))

        self.state["steps"] = self.steps
        js = json.dumps(self.state, indent=2, sort_keys=True, default=serialize)
        if to is None:
            if self.state_file.exists():
                self.state_file.rename(str(self.state_file) + "~")
            to = self.state_file.open("w")
        to.write(js)

    def setup(self):
        return self.state["setup"]

    def update(self, param):
        updated = False
        for changeable_value in ["number_of_optimizations", "projections_to_keep", "number_of_optimizations_per_run", "incremental", "scratch"]:
            updated = self.update_value(changeable_value, param[changeable_value], raise_if_changed=False)
        for readonly_value in ["number_of_dimensions", "minimum_column_basis"]:
            updated = self.update_value(readonly_value, param[readonly_value], raise_if_changed=True) or updated
        updated = self.update_steps() or updated
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

    def update_steps(self):
        updated = False
        step_path = ""
        substeps_for_table_2 = ["m"]
        if self.setup()["incremental"]:
            substeps_for_table_2.append("i")
        if self.setup()["scratch"]:
            substeps_for_table_2.append("s")
        if len(substeps_for_table_2) == 1:
            raise Error(f"""Both "incremental" and "scratch" are False in the settings""")
        table_dates = [None]
        for table_no, table in enumerate((Path(tab) for tab in self.source_tables), start=1):
            if not table.exists():
                raise Error(f"""Table {table} does not exist""")
            try:
                table_date = re.search(r"-([\d_]+)\.", table.name, re.I).group(1)
            except:
                module_logger.error(f"table.name: {table.name}")
                raise
            if table_date in table_dates:
                raise Error(f"""{table_date} {table} already in the chain?""")
            table_dates.append(table_date)
            if table_no == 1:
                substeps = ["s"]
                step_path = table_date
            else:
                substeps = substeps_for_table_2
                step_path += f":{table_date}"
            for substep in substeps:
                step = step_factory(substep, output_dir=self.output_dir, path=step_path, table_no=table_no, table_dates=table_dates, source_tables=self.source_tables, steps=self.steps, setup=self.setup())
                step_id = step.step_id()
                if step_id not in self.steps:
                    self.steps[step_id] = step
                    updated = True
                elif self.steps[step_id].path != step_path:
                    raise Error(f"""{step_id!r} already in steps has different path {self.steps[step_id].path!r} vs {step_path!r}""")
        return updated

# ======================================================================

class ProcessorTimer:

    def __init__(self, step):
        self.step = step

    def __enter__(self):
        self.step.start = datetime.datetime.now()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.step.finish = datetime.datetime.now()
        self.step.runtime = str(self.step.finish - self.step.start)
        module_logger.info(f"{self.step.step_id()} {self.step.type_desc()} {self.chart.make_name()}\n{'':30s}<{self.step.runtime}>")

def processor_factory(processor_type):
    if processor_type == "built-in":
        return ProcessorBuiltIn()
    elif processor_type == "htcondor":
        return ProcessorHTCondor()
    else:
        raise Error(f"""unknown processor type {processor_type!r}""")

# ----------------------------------------------------------------------

class Processor:

    def merge_incremental(self, chain_state, step):
        with ProcessorTimer(step) as pt:
            merge, report = acmacs.merge(acmacs.Chart(str(step.src[0])), acmacs.Chart(str(step.src[1])), type="incremental")
            self.export(chart=merge, out=Path(step.out[0]))
            pt.chart = merge

    def export(self, chart, out :Path):
        out.parent.mkdir(parents=True, exist_ok=True)
        chart.export(str(out), sys.argv[0])

# ----------------------------------------------------------------------

class ProcessorBuiltIn (Processor):

    def relax_from_scratch(self, chain_state, step):
        with ProcessorTimer(step) as pt:
            chart = acmacs.Chart(str(step.src[0]))
            chart.relax(number_of_dimensions=chain_state.setup()["number_of_dimensions"], number_of_optimizations=chain_state.setup()["number_of_optimizations"], minimum_column_basis=chain_state.setup()["minimum_column_basis"])
            chart.keep_projections(chain_state.setup()["projections_to_keep"])
            self.export(chart=chart, out=Path(step.out[0]))
            step.stress = chart.projection().stress()
            pt.chart = chart

    def relax_incremental(self, chain_state, step):
        with ProcessorTimer(step) as pt:
            chart = acmacs.Chart(str(step.src[0]))
            chart.relax_incremental(number_of_optimizations=chain_state.setup()["number_of_optimizations"])
            chart.keep_projections(chain_state.setup()["projections_to_keep"])
            self.export(chart=chart, out=Path(step.out[0]))
            step.stress = chart.projection().stress()
            pt.chart = chart

    def is_running(self, chain_state, step):
        return False

# ----------------------------------------------------------------------

class ProcessorHTCondor (Processor):

    def processing_dir(self, chain_state, step):
        pd = chain_state.htcondor_dir.joinpath(f"{step.step_id()}.{datetime.datetime.now().strftime('%Y-%m%d-%H%M%S')}")
        # module_logger.debug(f"{step.step_id()} htcondor processing_dir {pd}", stack_info=True)
        pd.mkdir(parents=True, exist_ok=False)
        return pd

    def relax_from_scratch(self, chain_state, step):
        from acmacs_base import htcondor
        number_of_optimizations = chain_state.setup()["number_of_optimizations_per_run"]
        queue_size = int(chain_state.setup()["number_of_optimizations"] / number_of_optimizations) + (1 if (chain_state.setup()["number_of_optimizations"] % number_of_optimizations) > 0 else 0)
        common_args = [
            "-d", chain_state.setup()["number_of_dimensions"],
            "-n", number_of_optimizations,
            "-m", chain_state.setup()["minimum_column_basis"],
            "--keep-projections", chain_state.setup()["projections_to_keep"],
            "--remove-original-projections",
            "--threads", chain_state.threads,
            "--grid",
            "--export-pre-grid",
        ]
        step.htcondor = {"dir": self.processing_dir(chain_state, step), "out": [f"{run_no:04d}.ace" for run_no in range(queue_size)]}
        program_args = [common_args + [str(Path(step.src[0]).resolve()), out] for out in step.htcondor["out"]]
        desc_filename, step.htcondor["log"] = htcondor.prepare_submission(
            program=Path(os.environ["ACMACSD_ROOT"], "bin", "chart-relax"),
            environment={"ACMACSD_ROOT": os.environ["ACMACSD_ROOT"]},
            program_args=program_args,
            description=f"chart-relax grid {step.step_id()}",
            current_dir=step.htcondor["dir"], capture_stdout=True, email=chain_state.email, notification="Error", request_cpus=chain_state.threads)
        step.htcondor["cluster"] = htcondor.submit(desc_filename)
        step.start = datetime.datetime.now()
        module_logger.info(f"""{step.step_id()} {step.type_desc()} htcondor submitted: {step.htcondor["cluster"]}""")

    def relax_incremental(self, chain_state, step):
        from acmacs_base import htcondor
        number_of_optimizations = chain_state.setup()["number_of_optimizations_per_run"]
        queue_size = int(chain_state.setup()["number_of_optimizations"] / number_of_optimizations) + (1 if (chain_state.setup()["number_of_optimizations"] % number_of_optimizations) > 0 else 0)
        common_args = [
            "--incremental",
            "-n", number_of_optimizations,
            "--keep-projections", chain_state.setup()["projections_to_keep"],
            "--remove-original-projections",
            "--threads", chain_state.threads,
            "--grid",
            "--export-pre-grid",
        ]
        step.htcondor = {"dir": self.processing_dir(chain_state, step), "out": [f"{run_no:04d}.ace" for run_no in range(queue_size)]}
        program_args = [common_args + [str(Path(step.src[0]).resolve()), out] for out in step.htcondor["out"]]
        desc_filename, step.htcondor["log"] = htcondor.prepare_submission(
            program=Path(os.environ["ACMACSD_ROOT"], "bin", "chart-relax"),
            environment={"ACMACSD_ROOT": os.environ["ACMACSD_ROOT"]},
            program_args=program_args,
            description=f"chart-relax incremental grid {step.step_id()}",
            current_dir=step.htcondor["dir"], capture_stdout=True, email=chain_state.email, notification="Error", request_cpus=chain_state.threads)
        step.htcondor["cluster"] = htcondor.submit(desc_filename)
        step.start = datetime.datetime.now()
        module_logger.info(f"""{step.step_id()} {step.type_desc()} htcondor submitted: {step.htcondor["cluster"]}""")

    def merge_results(self, chain_state, step):
        if step._type in ["s", "i"]:
            Path(step.out[0]).parent.mkdir(parents=True, exist_ok=True)
            cmd = ["chart-combine-projections", "-k", chain_state.setup()["projections_to_keep"], "-o", step.out[0]] + [Path(step.htcondor["dir"], fn) for fn in step.htcondor["out"]]
            subprocess.check_call([str(en) for en in cmd])
            chart = acmacs.Chart(str(step.out[0]))
            step.stress = chart.projection().stress()
            step.htcondor.pop("out", None)
            module_logger.info(f"{step.step_id()} {step.type_desc()} {chart.make_name()}\n{'':30s}<{step.runtime}>")
        else:
            raise RuntimeError(f"""ProcessorHTCondor.merge_results for step {step._type!r}: not implemented""")

    def is_running(self, chain_state, step):
        return bool(getattr(step, "htcondor", {}).get("cluster", None))

    # returns if job completed
    # raises StepFailed if job failed
    def check(self, chain_state, step):
        from acmacs_base import htcondor
        now = datetime.datetime.now()
        job = htcondor.Job(step.htcondor["cluster"], step.htcondor["log"])
        state = job.state()
        if state["FAILED"]:
            step.FAILED = True
            module_logger.error(f"""{step.step_id()} htcondor jobs {step.htcondor["cluster"]} FAILED""")
            self._done(step, now)
            raise StepFailed()
        elif state["DONE"]:
            module_logger.info(f"""{step.step_id()} htcondor jobs {step.htcondor["cluster"]} done""")
            self._done(step, now)
            step.runtime = str(step.finish - step.start)
            return True
        elif (now - step.htcondor.get("check_reported", step.start)).seconds > 300:
            step.htcondor["check_reported"] = now
            module_logger.info(f"""{step.step_id()}   {state["PERCENT"]}%          """)
        return False

    def _done(self, step, now):
        step.finish = now
        for key in ["cluster", "check_reported", "log"]:
            step.htcondor.pop(key, None)

# ======================================================================

def setup_logging(param):
    log_dir = Path(param["log"]["dir"])
    log_dir.mkdir(parents=True, exist_ok=True)
    log_filename = log_dir.joinpath(f"{datetime.datetime.now().strftime('%Y-%m%d-%H%M')}.log")
    logging.basicConfig(level=param["log"]["level"], format=param["log"]["format"])
    lh = logging.FileHandler(log_filename)
    lh.setLevel(param["log"]["level"])
    lh.setFormatter(logging.Formatter(param["log"]["format"]))
    logging.getLogger().addHandler(lh)
    return log_filename

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
