[//]: # (
Copyright Quadrivium LLC
All Rights Reserved
SPDX-License-Identifier: Apache-2.0
)

# Tooling 

In Kagome we use certain set of tools to assure code quality. Here is a list, and guide how to use them.


## clang-tidy

Set of rules is specified at root [`.clang-tidy`](../../.clang-tidy) file.

### Configure + Build + Run clang-tidy (slow)

1. Ensure clang-tidy is in PATH
2. `mkdir build`
3. `cd build`
4. `cmake .. -DCLANG_TIDY=ON`
5. `make`

Warnings/errors will be reported to stderr, same as compiler warnings/errors.

### Run clang-tidy for changes between your branch and master

1. Ensure `clang-tidy` is in PATH
2. Ensure `clang-tidy-diff.py` is in PATH.
    - on Mac it is usually located at `/usr/local/Cellar/llvm/8.0.0_1/share/clang/clang-tidy-diff.py` (note, 8.0.0_1 is your version, it may be different).
    - on Linux it is usually located at `/usr/lib/llvm-8/share/clang/clang-tidy-diff.py`
3. `mkdir build`
4. `cd build`
5. `cmake ..`
6. `make generated` - this step creates generated headers (protobuf, etc)
7. `cd ..`
8. `housekeeping/clang-tidy-diff.sh`

## Toolchain build

When CMAKE_TOOLCHAIN_FILE is specified, then specific toolchain is used.
Toolchain is a cmake file, which sets specific variables, such as compiler, its flags, build mode, language standard.

Example:
```sh
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain/gcc-10_cxx20.cmake
```

All dependencies will be built with gcc-12 and cxx20 standard.

Default toolchain is `cxx20.cmake`. [List of toolchains.](../../cmake/toolchain)

Also, **sanitizers** can be enabled with use of toolchains, so all dependencies will be built with specified sanitizer.

[List of sanitizers available.](../../cmake/san)


## coverage

Coverage is calculated automatically by Jenkins (CI).

We use [codecov](https://codecov.io/gh/soramitsu/kagome) to display coverage.

