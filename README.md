# shi

`shi` is a small, Git-inspired content-addressable file store written in
C++23. It reads files as binary data, builds blob objects with a SHA-256
content hash, compresses them with zlib, and stores them in a local `.shi`
repository.

## Status

`shi` is an experimental project. The current executable supports initializing
a repository, adding individual files, inspecting the staging file, and
synchronizing staged paths with a remote server over SSH/rsync. Commit and
tree history support is not implemented yet.

## Requirements

- C++23 compiler
- CMake 3.10 or newer
- OpenSSL (`libcrypto`) for SHA-256
- zlib for compression
- Boost.Process and Boost.Filesystem for synchronization
- `rsync`, SSH access, and a configured remote destination for `sync`

On macOS with Homebrew, the project expects dependencies under `/opt/homebrew`.

## Build

Using CMake:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/opt/homebrew
cmake --build build
```

The executable is created at `build/bin/shi`.

There is also a local Python build helper:

```sh
python3 compile.py
```

It creates `bin/shi` and uses the Homebrew library paths listed in
`compile.py`. Adjust those paths if your compiler or dependencies are
installed elsewhere.

## Usage

Initialize a repository in the current directory:

```sh
./build/bin/shi init
```

Add one regular file:

```sh
./build/bin/shi add path/to/file
```

Paths containing spaces are supported when quoted:

```sh
./build/bin/shi add "path/to/my file.txt"
```

There is also recursive add using the "." after `add` to add your entire workspace:

```sh
shi add .
```

If you want to ignore files and folders, use a .ignore file. It will automatically be read. By default, the `.shi/` folder will be ignored.

Display the current staging records:

```sh
./build/bin/shi cat-stage
```

Synchronize staged file paths:

```sh
./build/bin/shi sync shi
```

The `sync` command currently sends files to:

```text
steven@100.98.23.73:projects/<project_name>/
```

This destination is hard-coded in `include/sync.h` and should be changed for
another server or project layout.

## SSH authentication

Configure SSH public-key authentication so `rsync` does not require the
account password on every run. For example:

```sh
ssh-keygen -t ed25519 -C "shi-sync"
ssh-copy-id -i ~/.ssh/id_ed25519.pub steven@100.98.23.73
ssh steven@100.98.23.73
```

On macOS, `ssh-add --apple-use-keychain ~/.ssh/id_ed25519` can keep a
passphrase-protected key available through the Keychain.

## Repository layout

After initialization, the repository contains:

```text
.shi/
├── objects/
│   └── <2-byte SHA-256 prefix>/
│       └── <remaining SHA-256 digest>
├── index/
│   └── staging.bin
└── _shi_tree.txt
```

Blob data is represented as:

```text
<type><file-size>\0<raw-file-content>
```

The representation is hashed with SHA-256. The compressed blob is stored under
`.shi/objects`, using the first two hexadecimal digest characters as a
directory prefix. Adding identical content reuses the existing object path.

The staging file records file metadata, the binary blob hash, and the file
path. Its format is currently an internal implementation detail and may change.

## Source tree

```text
main.cc                         Command-line entry point
include/shi.h                   Repository, blob, staging, and sync orchestration
include/sync.h                  Boost.Process rsync wrapper
include/argument_handler/       Argument parsing helpers
include/common/                 Shared utilities and logging
CMakeLists.txt                  CMake build configuration
compile.py                      macOS/Clang build helper
```

## Roadmap

- finish tree objects and commits;
- add history, checkout/read, compare, pull, and push commands;
- improve staging-file portability and version its format; and
- add automated tests for hashing, compression, staging, and synchronization.
