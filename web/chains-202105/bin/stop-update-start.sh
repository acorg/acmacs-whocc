#! /usr/bin/env bash
set -o errexit -o errtrace -o pipefail -o nounset
cd $(dirname $0)/..
export ACMACSD_ROOT=${ACMACSD_ROOT=/syn/eu/AD}
export ACMACS_DATA=${ACMACS_DATA=/syn/eu/acmacs-data}
export LOCATIONDB_V2=${LOCATIONDB_V2=${ACMACS_DATA}/locationdb.json.xz}
export LOCDB_V2=${LOCDB_V2=$LOCATIONDB_V2}
export HIDB_V5=${HIDB_V5=${ACMACS_DATA}}
export SEQDB_V3=${SEQDB_V3=${ACMACS_DATA}}
export SEQDB_V4=${SEQDB_V4=${ACMACS_DATA}}
./bin/stop.sh || true
./bin/update.sh >>./gunicorn/log/update.log 2>&1
./bin/start.sh
