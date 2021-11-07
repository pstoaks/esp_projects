#
# Paul S. Downloaded the uasync code from  from here: https://forum.micropython.org/viewtopic.php?t=4828
#
# Simple HTTP server based on the uasyncio example script
# by J.G. Wezensky (joewez@gmail.com)
#
# Upgraded by Paul Stoaks for uasyncio v3
#

import uasyncio as asyncio
import uos
# import pkg_resources

ServerPaths = {}  # This global is set by the start() function

def get_req_from_response(response):
    """Returns the path and query strings from an HTTP response string.
       For a response of: "Content = b'GET /leds?series=winter&bright=30 HTTP/1.1\r\nHost: 192.168.0.122\r\n..."
       Path value is: /leds
       Query is: {"series": "winter", "bright": 30}
    """
    req = response.split("\r\n")[0].split()[1].split("?")
    path = req[0]
    if len(req) > 1:
        query = _parse_query(req[1])
    else:
        query = {}
    return {"path": path, "query": query}
    
    
def _parse_query(query):
    """ Helper function that parses a query string into a dictionary."""
    rtn = {}
    if query:
        sq = query.split('&')
        for x in sq:
            y = x.split('=')
            if len(y) == 2:
                rtn[y[0]] = y[1]
    return rtn


# Looks up the content-type based on the file extension
def get_mime_type(file):
    """ Note that the 'file' is only used to determine the mime type of the return.
        It need not actually be a file.
    """
    if file.endswith(".html"):
        return "text/html", False
    if file.endswith(".css"):
        return "text/css", True
    if file.endswith(".js"):
        return "text/javascript", True
    if file.endswith(".png"):
        return "image/png", True
    if file.endswith(".gif"):
        return "image/gif", True
    if file.endswith(".jpeg") or file.endswith(".jpg"):
        return "image/jpeg", True
    if file.endswith(".json"):
        return "application/json", True
    return "text/plain", False

# Quick check if a file exists
def exists(file):
    try:
        s = uos.stat(file)
        return True
    except:
        return False


async def send_http_response_header(writer, file):
    """ Note that the 'file' is only used to determine the mime type of the return.
        It need not actually be a file.
    """
    mime_type, cacheable = get_mime_type(file)
    await writer.awrite("HTTP/1.0 200 OK\r\n")
    await writer.awrite("Content-Type: {}\r\n".format(mime_type))
    if cacheable:
        await writer.awrite("Cache-Control: max-age=86400\r\n")
    await writer.awrite("\r\n")


async def send_url_not_found(writer):
    await writer.awrite("HTTP/1.0 404 NA\r\n\r\n")


async def send_file(file_path):
    if exists(file_path):
        with open(file_path, 'rb') as fp:
            buffer = fp.read(512)
            while buffer != b'':
                await writer.awrite(buffer)
                buffer = fp.read(512)
    else:
        await send_url_not_found(writer)


async def serve(reader, writer):
    try:
        request = await reader.read(1024)
        print('Content = %s' % request)
        response = ""
        if len(request) > 1:
            req = get_req_from_response(str(request))
            path = req['path']
            if path in ServerPaths:
                response = ServerPaths[path](req['query'])
                await send_http_response_header(writer, "file.html") # Temporary. Determine mime type by response
                await writer.awrite(response)
            else:
                await send_url_not_found(writer)
    except:
        print("Exception in serve()")
        asyncio.get_event_loop().stop()
        raise
    finally:
        await writer.wait_closed()

def start(server_paths):
    global ServerPaths
    ServerPaths = server_paths

    loop = asyncio.get_event_loop()
    loop.create_task(asyncio.start_server(serve, "0.0.0.0", 80, 20))
    try:
        loop.run_forever()
    except KeyboardInterrupt:
        print("User terminated")
    finally:
        print("closing")
        loop.close()
