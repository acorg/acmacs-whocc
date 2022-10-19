#! /bin/bash
cd $(dirname $0)/..
exec gunicorn -c ./gunicorn/gunicorn.conf.py --daemon --pid gunicorn.pid application:app
