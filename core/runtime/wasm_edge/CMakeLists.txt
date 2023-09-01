
add_library(runtime_wasm_edge module_factory_impl.cpp)
target_link_libraries(runtime_wasm_edge "${WASM_EDGE_LIBRARY}")
target_include_directories(runtime_wasm_edge PRIVATE "${WASMEDGE_ROOT}/include")
