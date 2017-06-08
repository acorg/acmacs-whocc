#! /usr/bin/env python3

import sys, os, cgi, cgitb, pprint, subprocess
cgitb.enable()
from pathlib import Path
# cgi.test()

# ----------------------------------------------------------------------

def main():
    try:
        data = cgi.parse()
        if not data:
            index()
        elif data.get("f"):
            make_plot(data["f"][0])
        else:
            raise RuntimeError("Unrecognized data:\n" + pprint.pformat(data))
    except Exception:
        print("Content-Type: text/html")
        print()
        raise

# ----------------------------------------------------------------------

sIndexTemplate = """<html>
<head>
  <style>
   h1 { margin-left: 1em; }
   ul { margin-left: 3em; }
   div.desc { margin: 3em 3em; width: 40em; }
   div.proc { margin: 3em 3em; width: 40em; font-size: 0.8em; }
  </style>
</head>
<body>
  <h1>WHO CCs tables. Column basis plots for chain merges.</h1>
  <ul>
%(links)s
  </ul>
  <div class="desc">
    <p>
    Sarah: For each chain, for all the sera, plot the column basis on
    the y axis against table number in chain on the x axis.  Line plot.
    A single plot for each serum in the chain.  Also all the sera on the
    same plot (it will look busy, I know, but might be informative for
    the smaller chains.)
    </p>
    <p>Rationale: to see if there is an upward drift in column basis with time, that perhaps contributes to the maps degrading.</p>
  </div>
  <div class="proc">
   Processing:
    <ol>
      <li>Scan WHO CC chains and save column basis data into json files:
        <pre>
cd /syn/WebSites/Protected/eu/who/column-basis-plots-for-merges
run-and-email aw whocc-column-basis-for-sera-for-chain.py --all
        </pre>
      </li>
      <li>
        Generate index page and pdfs via CGI:
        <pre>
~/AD/bin/whocc-column-basis-plots-for-merges.cgi
        </pre>
      </li>
      <li>
        Generate pdf (called by cgi above):
        <pre>
~/AD/bin/whocc-column-bases-plot-for-chain
        </pre>
      </li>
    </ol>
  </div>
</body>
</html>
"""

def index():
    print("Content-Type: text/html")
    print()
    sources = sorted(Path(".").glob("*.json"))
    fields = {
        "links": "\n".join("<li><a href=\"?f={f}\">{n}</a></li>".format(n=source.name, f=str(source)) for source in sources)
        }
    print(sIndexTemplate % fields)

# ----------------------------------------------------------------------

def make_plot(name):
    pdf = Path(name).stem + ".pdf"
    subprocess.check_call("(echo '===' `date` $REMOTE_ADDR '=== {name}' ; env ACMACSD_ROOT=/home/eu/AD LD_LIBRARY_PATH=/home/eu/AD/lib /home/eu/AD/bin/whocc-column-bases-plot-for-chain -s '{name}' -o '{pdf}') >>plot.log 2>&1".format(name=name, pdf=pdf), shell=True)
    print("Content-Type: application/pdf")
    print()
    print(Path(pdf).open().read())

# ----------------------------------------------------------------------

main()

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
