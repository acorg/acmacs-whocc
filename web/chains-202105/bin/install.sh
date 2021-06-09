#! /bin/bash
cd $(dirname $0)/..

sudo apt install gunicorn python3-aiohttp
# make virt env, install gunicorn aiohttp
# pip install aiohttp
# aiohttp-send -- https://pypi.org/project/aiohttp-send/0.0.3/

curl https://code.jquery.com/jquery-3.6.0.min.js >js/jquery.js
