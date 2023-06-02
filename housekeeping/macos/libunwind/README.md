# MacOS libunwind

MacOS M1 libunwind will crash when throwing c++ exceptions (e.g. WAVM unreachable intrinsic) or getting stack trace when call stack contains WAVM generated code.

## Code
- https://github.com/apple-oss-distributions/libunwind/tree/libunwind-201
- [libunwind-201-xpaci.patch](./libunwind-201-xpaci.patch)  
  ```
  patch -p1 < libunwind-201-xpaci.patch
  ```

## Build
```
cmake -B build submodule/libunwind
cmake --build build
```
Use "build/lib/libunwind.dylib".

## Use
Add directory with "libunwind.dylib" path to `DYLD_LIBRARY_PATH`
```
DYLD_LIBRARY_PATH=build/lib ../../../build/test/deps/wavm/wavm_try_catch
```
