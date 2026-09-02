# C/C++ Safety Analyzer

A research project bringing Rust-level memory and thread safety to C/C++
through a unified static analysis model built on Clang.

> Keep C/C++. Make the compiler smarter.

**Status:** Phase 0. Nothing is analyzed yet.

## Design

See [docs/agenda.md](docs/agenda.md).

## Supported versions

**LLVM/Clang 22 only.** Other versions will not work. Clang's analysis APIs
change frequently between releases, so a single version is pinned on purpose.

## Build

```console
# LLVM 22 is not in the Ubuntu archive; add the upstream apt repository first.
sudo apt install llvm-22-dev libclang-22-dev clang-22

cmake -B build \
  -DLLVM_DIR=$(llvm-config-22 --cmakedir) \
  -DClang_DIR=/usr/lib/llvm-22/lib/cmake/clang
cmake --build build -j
./build/src/driver/cxx-safety
```

Passing `Clang_DIR` explicitly matters when several LLVM versions are installed
side by side: `find_package(Clang)` may otherwise pick a different one than
`LLVM_DIR` points to.

`Clang_DIR` is usually found automatically from `LLVM_DIR`. If not, pass it
explicitly:

```console
cmake -B build \
  -DLLVM_DIR=/usr/lib/llvm-22/lib/cmake/llvm \
  -DClang_DIR=/usr/lib/llvm-22/lib/cmake/clang
```

## License

Apache License 2.0 with LLVM Exception, matching LLVM itself so that parts of
this work can be proposed upstream later.

Copyright (c) 2026 itsakeyfut. See [LICENSE](LICENSE) for details.
