#! /bin/bash
cd $(dirname $0)/..
exec gunicorn -c ./gunicorn/gunicorn.conf.py application:app
