#! /usr/bin/env python3

import sys, os, cgi, cgitb
cgitb.enable()
# cgi.test()

# ----------------------------------------------------------------------

def main():
    data = cgi.parse()
    print("Content-Type: text/html")
    print()
    print(data)

# ----------------------------------------------------------------------

main()

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
