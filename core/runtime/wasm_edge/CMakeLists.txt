
add_library(runtime_wasm_edge module_factory_impl.cpp memory_impl.cpp)
target_link_libraries(runtime_wasm_edge
    memory_allocator
    runtime_common
    zstd::libzstd_static
    WasmEdge::WasmEdge
    )
target_compile_definitions(runtime_wasm_edge PRIVATE WASMEDGE_ID="${WASMEDGE_ID}")
kagome_install(runtime_wasm_edge)
