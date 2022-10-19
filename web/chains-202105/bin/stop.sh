#! /usr/bin/env bash
set -o errexit -o errtrace -o pipefail -o nounset
cd $(dirname $0)/..
pkill -F gunicorn.pid
sleep 3
