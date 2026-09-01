# shi

`shi` is a small, Git-inspired content-addressable file store written in C++23.
It is an experimental project for learning how a version-control-style object
database can represent files as compressed blobs addressed by their SHA-256
content hash.

## Project status

This repository is a prototype, not yet a usable version-control system. The
implemented path is currently:

1. initialize `.shi/objects`;
2. read a regular file;
3. create a `blob <size>\0<content>` object representation;
4. hash it with SHA-256; and
5. zlib-compress and write the blob beneath `.shi/objects/<2-char-prefix>/`.

Tree creation, metadata, recursive directory adds, commits, history, and most
of the declared commands are still placeholders. The command-line dispatcher
is also not implemented yet; the current executable always initializes the
current directory and treats the third argument as the file to add.

## Object layout

Initializing a project is intended to produce this structure:

```text
.shi/
├── objects/
│   └── <sha256-prefix>/
│       └── <remaining-sha256>
├── _shi_tree.txt       # planned; not implemented
└── ...                 # planned metadata
```

Blob IDs are SHA-256 values of the blob representation, including its header.
The first two hexadecimal characters select the objects subdirectory and the
remaining characters are the object filename. Existing blobs are left intact,
so adding identical content is naturally deduplicated.

## Requirements

- macOS with Homebrew paths, or equivalent development libraries
- C++23 compiler
- OpenSSL (SHA-256)
- zlib (compression)
- Boost.Process and Boost.System (the sync prototype)
- `rsync` and SSH, only for the unfinished sync functionality

## Building

The checked-in `compile.py` is intended to compile with Clang and Homebrew's
OpenSSL installation, but it currently needs correction (`os.subprocess` is not
a Python API). The command it is trying to run can be invoked directly after
adjusting library/include paths for the local machine:

```sh
clang++ -std=c++23 -Wall -Wextra main.cc \
  -I/opt/homebrew/opt/openssl@3/include \
  -L/opt/homebrew/opt/openssl@3/lib \
  -lcrypto -lz -lssh -lboost_system -o shi
```

`CMakeLists.txt` currently declares the executable but does not yet configure
the required dependency include paths or libraries, so the direct compiler
command is the more accurate description of the current build setup.

## Current usage

Once the prototype builds, pass a command placeholder followed by a file path:

```sh
./shi add path/to/file
```

The program initializes `.shi/objects` in the current directory and attempts
to add `path/to/file`. A successful add writes a compressed blob and prints a
success message. The file must exist and be a regular file.

## Source tree

```text
main.cc          Minimal current executable entry point
include/shi.h    Initialization, hashing, compression, and blob storage
include/sync.h   Experimental rsync-over-SSH helpers
include/logger.h Small console logger
include/arguments.h  Planned command/flag lexer
CMakeLists.txt   Initial CMake target
compile.py       Draft local compile helper
```

## Roadmap

- implement safe command-line parsing and dispatch;
- finish metadata and tree objects;
- add commit, log, checkout/read, compare, pull, and push operations;
- support recursive adds while excluding `.shi`;
- make paths repository-relative and configurable;
- configure dependencies correctly in CMake; and
- add tests for hashing, compression, object layout, and error handling.

## License

No license has been specified yet.

## Note
This README file is written and maintained by AI. The code is by human brain, eyes, and sweaty hands.