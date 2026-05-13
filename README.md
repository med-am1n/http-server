# HTTP Server in C

A lightweight, multi-process HTTP server written in C.

## Project Structure

```
http-server/
├── src/
│   ├── http/              # HTTP protocol implementation
│   │   ├── http.c
│   │   └── http.h
│   └── main.c             # Server implementation
├── include/               # Public headers
│   └── http.h
├── bin/                   # Compiled binaries
│   └── server
├── public/                # Static files (HTML, CSS, etc.)
│   └── index.html
├── build/                 # Build artifacts (object files)
├── Makefile
└── README.md
```

## Building

```bash
make
```

This will compile the server binary into the `bin/` directory.

### Clean Build

```bash
make clean
make
```

## Running

### Start the Server

```bash
./bin/server
```

The server listens on port 3490 and serves files from the `public/` directory.

## Features

- **Multi-process architecture**: Each client connection is handled by a forked child process
- **HTTP/1.1 support**: Basic GET request handling
- **Static file serving**: Serves `public/index.html`
- **Error handling**: 404 and 500 status codes
- **Signal handling**: Proper cleanup of child processes to prevent zombies

## Implementation Details

- **src/http/**: Core HTTP parsing and response serialization
- **src/main.c**: Server socket management and request handling

## Requirements

- GCC compiler
- POSIX-compatible system (Linux, macOS, BSD)
