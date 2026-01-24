# Qore ZeroMQ Module

## Overview

The Qore ZeroMQ module provides an API for socket operations based on the [ZeroMQ](http://zeromq.org) library.

## Features

- Full ZeroMQ socket support (PUSH/PULL, PUB/SUB, REQ/REP, DEALER/ROUTER, etc.)
- Thread-safe CLIENT/SERVER sockets (draft API)
- CURVE encryption support
- Automatic frame serialization for CLIENT/SERVER sockets

## Requirements

- Qore 0.9+
- CMake 3.14+
- C++11 compiler

### ZeroMQ Libraries (Optional)

If you have system-installed ZeroMQ libraries with draft API support:
- libzmq 4.2+ (built with `--enable-drafts` or `-DENABLE_DRAFTS=ON`)
- czmq 4.0.2+

If system libraries are not available or don't have draft API support, the module will automatically download and build libzmq and czmq from source.

## Building

### Basic Build (Recommended)

```bash
cd module-zmq
mkdir build
cd build
cmake ..
make
make install
```

By default, CMake will:
1. Check if system ZMQ/CZMQ libraries have draft API support
2. If yes, use them; if no, build from source with draft API enabled

### Build Options

#### USE_SYSTEM_ZMQ

Controls whether to use system libraries or build from source:

| Value | Behavior |
|-------|----------|
| `AUTO` (default) | Try system libraries, fall back to building from source |
| `ON` | Force system libraries (fails if draft API not available) |
| `OFF` | Force building from source |

Examples:

```bash
# Force use of system libraries (fails if no draft API)
cmake -DUSE_SYSTEM_ZMQ=ON ..

# Force building from source
cmake -DUSE_SYSTEM_ZMQ=OFF ..
```

#### Custom Library Locations

If ZMQ/CZMQ are installed in non-standard locations:

```bash
export ZMQ_DIR=/path/to/zmq
export CZMQ_DIR=/path/to/czmq
cmake -DUSE_SYSTEM_ZMQ=ON ..
```

## Thread-Safe Sockets (Draft API)

This module includes support for ZeroMQ's thread-safe CLIENT/SERVER sockets:

- `ZSocketClient` - Thread-safe client socket
- `ZSocketServer` - Thread-safe server socket with routing_id support

These sockets can be safely accessed from multiple threads without external locking.

### Example

```qore
#!/usr/bin/env qore
%new-style
%requires zmq

# Create a SERVER socket
ZContext ctx();
ZSocketServer server(ctx, "tcp://127.0.0.1:*");
printf("Server bound to: %s\n", server.endpoint());

# Create a CLIENT socket
ZSocketClient client(ctx, server.endpoint());

# Send from client (thread-safe)
client.send("Hello", "World");

# Receive on server
ZMsg msg = server.recvMsg();
int routing_id = server.getRoutingId();

# Send response to specific client
server.setRoutingId(routing_id);
server.send("Response");
```

### Feature Detection

Use the `HAVE_ZMQ_DRAFT_APIS` constant to check for draft API availability:

```qore
if (HAVE_ZMQ_DRAFT_APIS) {
    # Use CLIENT/SERVER sockets
} else {
    # Fall back to DEALER/ROUTER
}
```

## Running Tests

```bash
cd build
QORE_MODULE_DIR=. qore ../test/zmq.qtest -v
QORE_MODULE_DIR=. qore ../test/zmq-client-server.qtest -v
```

## Documentation

Build the API documentation:

```bash
cd build
make docs
```

Documentation will be generated in the `docs/` directory.

## License

LGPL 2.1 - see LICENSE file for details.
