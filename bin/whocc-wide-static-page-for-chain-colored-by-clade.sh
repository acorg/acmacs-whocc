#! /bin/bash

#PROG=$(readlink -f $0)
PROG=$(basename $0)
LOG=/syn/eu/log/whocc-wide-static-page-for-chain.ad.log

export ACMACSD_ROOT="${HOME}/AD"
export PATH="${ACMACSD_ROOT}/bin:${PATH}"
export LD_LIBRARY_PATH="${ACMACSD_ROOT}/lib:${LD_LIBRARY_PATH}"

# ======================================================================

on_error()
{
    (echo "$PROG FAILED"; echo "LOG: /scp:$HOSTNAME:$LOG") | mail -s "$(hostname) $PROG FAILED" eu@antigenic-cartography.org
    exit 1
}

trap on_error ERR

exec > "$LOG"
exec 2>&1

# ======================================================================

date
cd /syn/WebSites/Protected/who/wide-pages-for-chains-colored-by-clade
whocc-wide-static-page-for-chain-colored-by-clade
date

(echo "$PROG completed"; echo "LOG: /scp:$HOSTNAME:$LOG"; echo "https://notebooks.antigenic-cartography.org/who/wide-pages-for-chains-colored-by-clade/") | mail -s "$(hostname) $PROG completed" eu@antigenic-cartography.org

# ======================================================================
