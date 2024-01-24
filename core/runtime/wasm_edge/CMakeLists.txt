
add_library(runtime_wasm_edge module_factory_impl.cpp memory_impl.cpp)
target_link_libraries(runtime_wasm_edge WasmEdge::WasmEdge memory_allocator)
target_include_directories(runtime_wasm_edge PRIVATE "${WASMEDGE_ROOT}/include")
kagome_install(runtime_wasm_edge)
