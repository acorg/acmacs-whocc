import os, datetime, pprint

def app(environ, start_response):
    data = f"who-is-who\n{datetime.datetime.now()}\nWD: {os.getcwd()}\n\n{pprint.pformat(environ)}\n\n{pprint.pformat(start_response)}"
    data_b = data.encode("utf-8")
    status = '200 OK'
    response_headers = [
        ('Content-type', 'text/plain'),
        ('Content-Length', str(len(data_b)))
    ]
    start_response(status, response_headers)
    return iter([data_b])
