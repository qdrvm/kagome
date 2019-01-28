This is the folder for HEADER ONLY dependencies.

To add new dependency, add new submodule:
```
cd deps
git submodule add $GIT_URL
```

Then, update root CMakeLists.txt:
```
include_directories(
  # add path to header only deps here
+  deps/spdlog/include
)
```
