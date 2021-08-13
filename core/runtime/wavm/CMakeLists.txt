
add_library(compartment_wrapper compartment_wrapper.cpp)
target_link_libraries(compartment_wrapper WAVM::libWAVM buffer)
kagome_install(compartment_wrapper)

add_library(wavm_memory memory_impl.cpp)
target_link_libraries(wavm_memory WAVM::libWAVM buffer memory_allocator)
kagome_install(wavm_memory)

add_library(runtime_wavm
    executor_factory.cpp
    intrinsics/intrinsic_functions.cpp
    intrinsics/intrinsic_module.cpp
    intrinsics/intrinsic_module_instance.cpp
    intrinsics/intrinsic_resolver_impl.cpp
    instance_environment_factory.cpp
    module.cpp
    module_factory_impl.cpp
    module_instance.cpp
    wavm_memory_provider.cpp
    )
target_link_libraries(runtime_wavm
    WAVM::libWAVM
    wavm_memory
    constant_code_provider
    Boost::boost
    compartment_wrapper
    trie_storage_provider
    )
kagome_install(runtime_wavm)
