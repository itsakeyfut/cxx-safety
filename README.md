# C/C++ Safety Analyzer

A research project bringing Rust-level memory and thread safety to C/C++
through a unified static analysis model built on Clang.

## Supported versions

**LLVM/Clang 22 only.** Other versions will not work. Clang's analysis APIs
change frequently between releases, so a single version is pinned on purpose.

## Build

```console
# LLVM 22 is not in the Ubuntu archive, so add the upstream apt repository.
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 22 all

cmake -B build \
  -DLLVM_DIR=$(llvm-config-22 --cmakedir) \
  -DClang_DIR=/usr/lib/llvm-22/lib/cmake/clang
cmake --build build -j
./build/src/driver/cxx-safety
```

Passing `Clang_DIR` explicitly matters when several LLVM versions are installed
side by side: `find_package(Clang)` may otherwise pick a different one than
`LLVM_DIR` points to.

## License

Apache License 2.0 with LLVM Exception, matching LLVM itself so that parts of
this work can be proposed upstream later.

Copyright (c) 2026 itsakeyfut. See [LICENSE](LICENSE) for details.
