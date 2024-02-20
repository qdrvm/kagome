
add_library(runtime_wasm_edge module_factory_impl.cpp memory_impl.cpp)
target_link_libraries(runtime_wasm_edge
    memory_allocator
    memory_error
    WasmEdge::WasmEdge
    )
kagome_install(runtime_wasm_edge)
