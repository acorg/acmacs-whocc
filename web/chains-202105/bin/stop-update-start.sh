#! /usr/bin/env bash
set -o errexit -o errtrace -o pipefail -o nounset
cd $(dirname $0)/..
export ACMACSD_ROOT=${ACMACSD_ROOT=/syn/eu/AD}
export LOCATIONDB_V2=${LOCATIONDB_V2=/syn/eu/acmacs-data/locationdb.json.xz}
export LOCDB_V2=${LOCDB_V2=$LOCATIONDB_V2}
./bin/stop.sh || true
./bin/update.sh >>./gunicorn/log/update.log 2>&1
./bin/start.sh
