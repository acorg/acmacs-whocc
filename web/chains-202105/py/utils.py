# ======================================================================

# sSubtypeDisplay = {"h1pdm": "A(H1N1)", "h3": "A(H3N2)", "bvic": "B/Vic", "byam": "B/Yam"}

def format_subtype(request, subtype_id, date_range: list[str]):
    subtype, assay, *fields = subtype_id.split("-")
    if assay == "hi":
        rbc = "-".join(fields[:-1])
    else:
        rbc = None
    return {
        "subtype": subtype,
        "assay": assay,
        "rbc": rbc,
        "lab": fields[-1],
        "coloring": request.app["clade_data"].entry_names_for_subtype(subtype, date_range)
        }

# ======================================================================
