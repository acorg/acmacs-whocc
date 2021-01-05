import sys, os, re, json, datetime, time, subprocess, pprint, traceback
from pathlib import Path
import logging; module_logger = logging.getLogger(__name__)

import acmacs

# ======================================================================

class Error (RuntimeError): pass
class StepFailed (Error): pass

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
        return f"{table_no + 1:03d}.{type}.{table_date}"

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
        if self.table_no == 0:
            raise Error(f"""Cannot use step type {self._type!r} for table {self.table_no}""")
        elif self.table_no == 1:
            self.depends.append(self.make_step_id(table_no=self.table_no-1, type="s", table_date=table_dates[self.table_no-1]))
        else:
            if setup["scratch"]:
                self.depends.append(self.make_step_id(table_no=self.table_no-1, type="s", table_date=table_dates[self.table_no-1]))
            if setup["incremental"]:
                self.depends.append(self.make_step_id(table_no=self.table_no-1, type="i", table_date=table_dates[self.table_no-1]))
        self.src = [None, source_tables[self.table_no]]

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

    def is_completed(self, chain_state):
        return super().is_completed(chain_state) and hasattr(self, "stress")

    def make(self, source_tables, table_dates, steps, setup):
        if self.table_no == 0:
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

    def is_completed(self, chain_state):
        return super().is_completed(chain_state) and hasattr(self, "stress")

    def make(self, source_tables, table_dates, steps, setup):
        if self.table_no == 0:
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

    def __init__(self, chain_data): # source_tables, param, processor):
        self.state = {"setup": {"number_of_optimizations": 0}, "steps": {}}
        self.state_file = Path(chain_data.state_filename())
        self.output_dir = Path(chain_data.output_dir())
        self.htcondor_dir = Path(chain_data.htcondor_dir())
        self.email = chain_data.email()
        self.threads = chain_data.threads()
        self.source_tables = chain_data.source_tables()
        self.load(chain_data)
        self.processor = chain_data.processor()

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

    def load(self, chain_data):
        self.steps = {}
        if self.state_file.exists():
            self.state = json.load(self.state_file.open())
            for step_id, step_data in self.state["steps"].items():
                self.steps[step_id] = step_factory(step_data["_type"], read_from=step_data)
            self.state.pop("steps")
        if self.update(chain_data):
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

    def update(self, chain_data):
        updated = False
        for changeable_value in ["number_of_optimizations", "projections_to_keep", "number_of_optimizations_per_run", "incremental", "scratch"]:
            updated = self.update_value(changeable_value, getattr(chain_data, changeable_value)(), raise_if_changed=False)
        for readonly_value in ["number_of_dimensions", "minimum_column_basis"]:
            updated = self.update_value(readonly_value, getattr(chain_data, readonly_value)(), raise_if_changed=True) or updated
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
        table_dates = []
        for table_no, table in enumerate((Path(tab) for tab in self.source_tables)):
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
            if table_no == 0:
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

    def __init__(self, step, chart=None):
        self.step = step
        self.chart = chart

    def __enter__(self):
        self.step.start = datetime.datetime.now()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.step.finish = datetime.datetime.now()
        self.step.runtime = str(self.step.finish - self.step.start)
        if self.chart:
            nm = self.chart.make_name()
        else:
            nm = ""
        module_logger.info(f"{self.step.step_id()} {self.step.type_desc()} {nm}\n{'':30s}<{self.step.runtime}>")

# ----------------------------------------------------------------------

class Processor:

    def __init__(self, chain_data):
        self.chain_data_ = chain_data

    def merge_incremental(self, chain_state, step):
        with ProcessorTimer(step) as pt:
            master = self.load_chart(step.src[0])
            pt.chart = master
            to_merge = self.load_chart(step.src[1])
            merge, report = acmacs.merge(master, to_merge, type="incremental")
            pt.chart = merge
            self.export(chart=merge, out=Path(step.out[0]))

    def export(self, chart, out :Path):
        out.parent.mkdir(parents=True, exist_ok=True)
        chart.export(str(out), sys.argv[0])

    def load_chart(self, filename):
        chart = self.chain_data_.chart_loaded(acmacs.Chart(str(filename)))
        return chart

# ----------------------------------------------------------------------

class ProcessorBuiltIn (Processor):

    def relax_from_scratch(self, chain_state, step):
        with ProcessorTimer(step) as pt:
            chart = self.load_chart(step.src[0])
            pt.chart = chart
            chart.relax(number_of_dimensions=chain_state.setup()["number_of_dimensions"], number_of_optimizations=chain_state.setup()["number_of_optimizations"], minimum_column_basis=chain_state.setup()["minimum_column_basis"])
            chart.keep_projections(chain_state.setup()["projections_to_keep"])
            self.export(chart=chart, out=Path(step.out[0]))
            step.stress = chart.projection().stress()

    def relax_incremental(self, chain_state, step):
        chart = self.load_chart(step.src[0])
        with ProcessorTimer(step, chart) as pt:
            chart.relax_incremental(number_of_optimizations=chain_state.setup()["number_of_optimizations"])
            chart.keep_projections(chain_state.setup()["projections_to_keep"])
            self.export(chart=chart, out=Path(step.out[0]))
            step.stress = chart.projection().stress()

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
            chart = self.load_chart(step.out[0])
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
# 2021
# ----------------------------------------------------------------------

class IncrementalChain:

    def __init__(self):
        from socket import gethostname
        self.hostname_ = gethostname()
        self.email_subject_ = f"""{self.hostname_} {Path.cwd().parents[0].name}"""

    def run(self):
        "returns exit code: 0 - success, 1,2,3 - failure"
        from acmacs_base import email
        try:
            self.setup_logging()
            email_body = f"""{Path.cwd()}\n/scp:{self.hostname_}:{self.log_filename_.resolve()}\n/scp:{self.hostname_}:{Path("state.json").resolve()}"""
            if self.process():
                email.send(to=self.email(), subject=f"""chain FAILED {self.email_subject_}""", body=f"""chain FAILED\n{email_body}""")
                return 1
            else:
                email.send(to=self.email(), subject=f"""chain completed {self.email_subject_}""", body=f"""chain completed\n{email_body}""")
                return 0
        except KeyboardInterrupt:
            print("KeyboardInterrupt", file=sys.stderr)
            return 0
        except ConnectionRefusedError as err:
            print(f">> {err} - cannot send email?", file=sys.stderr)
            return 0
        except Error as err:
            module_logger.error(f"{err}")
            email.send(to=self.email(), subject=f"""chain EXCEPTION {self.email_subject_}""", body=f"""chain EXCEPTION\n{email_body}\n\n{traceback.format_exc()}""")
            return 2
        except Exception as err:
            module_logger.error(f"{err}", exc_info=True)
            email.send(to=self.email(), subject=f"""chain EXCEPTION {self.email_subject_}""", body=f"""chain EXCEPTION\n{email_body}\n\n{traceback.format_exc()}""")
            return 3

    def process(self):
        state = State(chain_data=self)
        self.report_tables(state)
        while state.has_todo():
            state.check_running()
            if not state.run_ready():
                # module_logger.info(f"""nothing ready, sleeping for {param["sleep_interval_when_not_ready"]} seconds""")
                time.sleep(self.sleep_interval_when_not_ready())
        return state.is_failed()

    sReTableExtract = re.compile(r"[\.\-](\d{8}(?:[\-_]\d+)?)")

    def report_tables(self, state):

        def table_date(stem):
            m = self.sReTableExtract.search(stem)
            if m:
                return m.group(1)
            else:
                return stem

        tables = [table_date(Path(getattr(st.src[0], "name", st.src[0])).stem) for id, st in state.steps.items() if ".s." in id]
        tables_s = "\n   ".join(f"{no:3d} {tab}" for no, tab in enumerate(tables, start=1))
        print(f"Tables: {len(tables)}\n   {tables_s}")

    def chart_loaded(self, chart):
        """hook to preprocess chart on loading"""
        return chart

    def source_tables(self):
        "returns [Path]"
        return []

    def number_of_optimizations(self):
        return 100

    def number_of_dimensions(self):
        return 2

    def minimum_column_basis(self):
        return "none"

    def projections_to_keep(self):
        return 10

    def incremental(self):
        "returns if making incremetal merge map at each step requested"
        return True

    def scratch(self):
        "returns if making map from scratch at each step requested"
        return True

    def state_filename(self):
        return Path("state.json")

    def output_dir(self):
        return Path("out")

    def htcondor_dir(self):
        return Path("htcondor")

    def email(self):
        return "eu@antigenic-cartography.org"

    def threads(self):
        return 16

    def number_of_optimizations_per_run(self):
        return 56

    def sleep_interval_when_not_ready(self):
        "in seconds (htcondor only)"
        return 10 # in seconds

    def processor(self):
        "returns processor instance to use"
        try:
            subprocess.check_output(["condor_version"])
            return ProcessorHTCondor(self)
        except (subprocess.SubprocessError, FileNotFoundError):
            return ProcessorBuiltIn(self)

    def setup_logging(self, log_dir=Path("log"), level=logging.DEBUG, format="%(levelname)s %(asctime)s: %(message)s @@ %(pathname)s:%(lineno)d"):
        "format: https://docs.python.org/3.9/library/logging.html#logrecord-attributes"
        log_dir.mkdir(parents=True, exist_ok=True)
        self.log_filename_ = log_dir.joinpath(f"{datetime.datetime.now().strftime('%Y-%m%d-%H%M')}.log")
        logging.basicConfig(level=level, format=format)
        lh = logging.FileHandler(self.log_filename_)
        lh.setLevel(level)
        lh.setFormatter(logging.Formatter(format))
        logging.getLogger().addHandler(lh)

# ----------------------------------------------------------------------

def make(target_dir):
    target_dir = Path(target_dir)
    from socket import gethostname
    hostname = gethostname()
    if re.match(r"^i\d", hostname):
        number_of_optimizations = 1000
        threads = 16
    else:
        number_of_optimizations = 100
        threads = 4
    if re.match(r"some h1", target_dir.parent.name):
        minimum_column_basis = "1280"
    else:
        minimum_column_basis = "none"

    if "bvic" in target_dir.parent.name:
        wrong_lineage = "yamagata"
    elif "byam" in target_dir.parent.name:
        wrong_lineage = "victoria"
    else:
        wrong_lineage = None
    if wrong_lineage:
        chart_loaded_comment = ""
    else:
        chart_loaded_comment = "# "

    target_dir.mkdir(parents=True, exist_ok=True)
    run_script = target_dir.joinpath("run")
    if not run_script.exists():
        run_script.open("w").write(f"""#! /usr/bin/env python3

import sys, os, re
from pathlib import Path
from acmacs_whocc.incremental_chain import IncrementalChain
# import acmacs

class ThisIncrementalChain (IncrementalChain):

    sReInclude = re.compile(r"-(20)", re.I)
    sReExclude = re.compile(r"exclude", re.I)

    def source_tables(self):
        "returns [Path]"
        whocc_tables = Path(os.environ["HOME"], "ac", "whocc-tables")
        source_dir = whocc_tables.joinpath("{target_dir.parent.name}")
        return sorted(pathname for pathname in source_dir.iterdir()
                      if pathname.suffix == ".ace"
                      and self.sReInclude.search(pathname.name)
                      and not self.sReExclude.search(pathname.name))

    def number_of_optimizations(self):
        return {number_of_optimizations}

    def number_of_dimensions(self):
        return 2

    def minimum_column_basis(self):
        return "{minimum_column_basis}"

    def incremental(self):
        "returns if making incremetal merge map at each step requested"
        return True

    def scratch(self):
        "returns if making map from scratch at each step requested"
        return True

    {chart_loaded_comment}def chart_loaded(self, chart):
    {chart_loaded_comment}    "hook to preprocess chart on loading, e.g. remove antigens/sera of wrong lineage"
    {chart_loaded_comment}    if chart.number_of_projections() == 0:
    {chart_loaded_comment}        wrong_lineage = {wrong_lineage!r}
    {chart_loaded_comment}        chart.remove_antigens_sera(antigens=chart.antigen_indexes().filter_lineage(wrong_lineage), sera=chart.serum_indexes().filter_lineage(wrong_lineage))
    {chart_loaded_comment}    return chart

    # def threads(self):
    #     return {threads}

    # def number_of_optimizations_per_run(self):
    #     return 56

    # def email(self):
    #     return "eu@antigenic-cartography.org"

    # def projections_to_keep(self):
    #     return 10

    # def state_filename(self):
    #     return Path("state.json")

    # def output_dir(self):
    #     return Path("out")

    # def htcondor_dir(self):
    #     return Path("htcondor")

    # def sleep_interval_when_not_ready(self):
    #     "in seconds (htcondor only)"
    #     return 10 # in seconds

# ----------------------------------------------------------------------

exit(ThisIncrementalChain().run())
"""
                                   )
    else:
        print(f">> WARNING: {run_script} already exists", file=sys.stderr)
    run_script.chmod(0o755)

# ======================================================================
# 2020 legacy
# ----------------------------------------------------------------------

class IncrementalChain2020 (IncrementalChain):

    def __init__(self, source_tables, param):
        super().__init__()
        self.source_tables_ = source_tables
        self.param_ = param

    def source_tables(self):
        return self.source_tables_

    def number_of_optimizations(self):
        return self.param_.get("number_of_optimizations", super().number_of_optimizations())

    def number_of_dimensions(self):
        return self.param_.get("number_of_dimensions", super().number_of_dimensions())

    def minimum_column_basis(self):
        return self.param_.get("minimum_column_basis", super().minimum_column_basis())

    def projections_to_keep(self):
        return self.param_.get("projections_to_keep", super().projections_to_keep())

    def incremental(self):
        "returns if making incremetal merge map at each step requested"
        return self.param_.get("incremental", super().incremental())

    def scratch(self):
        "returns if making map from scratch at each step requested"
        return self.param_.get("scratch", super().scratch())

    def state_filename(self):
        return self.param_.get("state_filename", super().state_filename())

    def output_dir(self):
        return self.param_.get("output_dir", super().output_dir())

    def htcondor_dir(self):
        return self.param_.get("htcondor_dir", super().htcondor_dir())

    def email(self):
        return self.param_.get("email", super().email())

    def threads(self):
        return self.param_.get("threads", super().threads())

    def number_of_optimizations_per_run(self):
        return self.param_.get("number_of_optimizations_per_run", super().number_of_optimizations_per_run())

    def sleep_interval_when_not_ready(self):
        "in seconds (htcondor only)"
        return self.param_.get("sleep_interval_when_not_ready", super().sleep_interval_when_not_ready())

    def setup_logging(self):
        param_log = {"dir": Path("log"), "level": logging.DEBUG, "format": "%(levelname)s %(asctime)s: %(message)s @@ %(pathname)s:%(lineno)d", **self.param_.get("log", {})}
        super().setup_logging(log_dir=param_log["dir"], level=param_log["level"], format=param_log["format"])

# ----------------------------------------------------------------------

def main(source_tables, param):
    chain = IncrementalChain2020(source_tables, param)
    return chain.run()

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
