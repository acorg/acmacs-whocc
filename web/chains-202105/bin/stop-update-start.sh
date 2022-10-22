#! /usr/bin/env bash
set -o errexit -o errtrace -o pipefail -o nounset
cd $(dirname $0)/..
export ACMACSD_ROOT=${ACMACSD_ROOT=/syn/eu/AD}
./bin/stop.sh || true
./bin/update.sh >>./gunicorn/log/update.log 2>&1
./bin/start.sh
