#! /usr/bin/env bash
set -o errexit -o errtrace -o pipefail -o nounset
cd $(dirname $0)/..
if [[ -f  gunicorn.pid ]]; then
    pkill -F gunicorn.pid
    sleep 3
else
    echo No pidfile gunicorn.pid >&2
    exit 1
fi
