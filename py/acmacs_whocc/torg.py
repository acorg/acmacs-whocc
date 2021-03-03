class TorgGenerator:

    class Error (RuntimeError): pass

    AntigenFieldOrder = ["name", "date", "reassortant", "annotations", "passage", "lab_id", "clade"]
    SerumFieldOrder = ["name", "serum_id", "reassortant", "annotations", "passage", "species", "clade"]

    def __init__(self):
        # self.name = None
        self.lab = None
        self.date = None
        self.subtype = None
        self.assay = None
        self.rbc = None
        self.lineage = None
        self.antigens = []      # {"name":, "passage":, "date":, "reassortant": "annotations":, "lab_id":, "clade":}
        self.sera = []          # {"name":, "passage":, "serum_id":, "reassortant": "annotations":, "species":, "clade":}
        self.titers = []

    def make(self):
        self.check_fields()
        self.make_fields()
        header = [
            ["Lab", self.lab],
            ["Date", self.date],
            ["Subtype", self.subtype],
            ["Assay", self.assay],
            ["Rbc", self.rbc],
            ["Lineage", self.lineage],
        ]

        torg = "# -*- Org -*-\n\n" + "\n".join(f"- {k}: {v}" for k, v in header if v) + "\n\n"
        first_column_width = 10
        torg += f"| {' ':<{first_column_width}s} | " + " | ".join(f"{agf:<{agfw}s}" for agf, agfw in self.antigen_fields) + " | " + " | ".join(f"{' ':<{self.serum_width}s}" for sr in self.sera) + " |\n"
        for sr_field, srfw in self.serum_fields:
            torg += f"| {sr_field:<{first_column_width}s} | " + " | ".join(f"{' ':<{agfw}s}" for agf, agfw in self.antigen_fields) + " | " + " | ".join(f"{sr.get(sr_field, ''):<{self.serum_width}s}" for sr in self.sera) + " |\n"
        for ag_no, ag in enumerate(self.antigens):
            torg += f"| {' ':<{first_column_width}s} | " + " | ".join(f"{ag.get(agf, ''):<{agfw}s}" for agf, agfw in self.antigen_fields) + " | " + " | ".join(f"{self.titers[ag_no][sr_no]:>{self.serum_width}s}" for sr_no in range(len(self.sera))) + " |\n"
        torg += "\n* -------------------- local vars --------------------\n:PROPERTIES:\n:VISIBILITY: folded\n:END:\n\n#+STARTUP: showall indent\n"
        return torg

    def make_fields(self):
        self.antigen_fields = [en for en in ([field, max(len(antigen.get(field, "")) for antigen in self.antigens)] for field in self.AntigenFieldOrder) if en[1]]
        self.serum_fields = [en for en in ([field, max(len(serum.get(field, "")) for serum in self.sera)] for field in self.SerumFieldOrder) if en[1]]
        self.serum_width = max(sfw for sf, sfw in self.serum_fields)

    def check_fields(self):
        if not self.antigens:
            raise self.Error("No antigens defined")
        if not self.sera:
            raise self.Error("No sera defined")
        if not self.titers:
            raise self.Error("No titers defined")
        if len(self.titers) != len(self.antigens):
            raise self.Error(f"Number of rows of titers ({len(self.titers)}) doest not correspond to the number of antigens ({len(self.antigens)})")
        if not all(len(row) == len(self.sera) for row in self.titers):
            raise self.Error(f"Number of columns of titers doest not correspond to the number of sera ({len(self.sera)})")

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
