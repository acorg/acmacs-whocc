#! /usr/bin/env bash
set -o errexit -o errtrace -o pipefail -o nounset
cd $(dirname $0)/..
./bin/stop.sh
./bin/update.sh >>./gunicorn/log/update.log 2>&1
./bin/start.sh
