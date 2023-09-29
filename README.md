# Sloop

## A server written in plain C

This server program allows inbound connections from clients and can interact
with them in any conceivable way.  It is typically used as a web server.  I
have written it in plain C and stripped it down as far as possible.  It does
use realloc at one point in buffer.c.

This code distills my experience with writing servers in various languages,
including Perl and Fexl (which uses C).  My goal is to make the code as simple
as possible, and any remaining intricacy should be there only by proven
necessity.

### Running the demo

There is a `world` server which demonstrates serving a single static web page.

To start the server:
```
cd sloop/src
./world start
```

(Note that if the server is already running, that will automatically stop the
old one for you.)

To connect to the server, point your browser to:
```
http://127.0.0.1:9722
```

To stop the server:
```
./world stop
```

To see a usage message:
```
./world
```

The server uses a `run` directory:
```
cd sloop/run
ls -l
```

There you may see an `error_log` and a `pid` file.  The `error_log` captures
any stderr from a client program.  The `pid` stores the process ID of the main
server program which is used to stop the server.

### Utilities

The `world` script automatically builds the code and runs it, but you can also
run the build script directly if you like, e.g.:
```
./build         # Build anything out of date
./build clean   # Force a full build
./build erase   # Erase build output files
```

There is also a `show` utility that shows you the status of the server,
listing all running `world` processes.

```
./show
```
