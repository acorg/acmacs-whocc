import acmacs

# ======================================================================

def chart(request, filename):
    charts = request.app["charts"]
    chart = charts.get(filename)
    if not chart:
        chart = charts[filename] = acmacs.Chart(filename)
    return chart

# ======================================================================
