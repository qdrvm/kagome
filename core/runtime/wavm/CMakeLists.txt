
add_library(compartment_wrapper compartment_wrapper.cpp)
target_link_libraries(compartment_wrapper WAVM::libWAVM buffer)
kagome_install(compartment_wrapper)

add_library(wavm_memory memory.cpp)
target_link_libraries(wavm_memory WAVM::libWAVM buffer)
kagome_install(wavm_memory)

add_library(wavm_executor
    module.cpp
    module_instance.cpp
    intrinsics/intrinsic_resolver_impl.cpp
    core_api_factory.cpp
    intrinsics/intrinsic_functions.cpp
    wavm_memory_provider.cpp
    module_factory_impl.cpp
    intrinsics/intrinsic_module_instance.cpp
    intrinsics/intrinsic_module.cpp
    )
target_link_libraries(wavm_executor
    WAVM::libWAVM
    wavm_memory
    constant_code_provider
    Boost::boost
    compartment_wrapper
    )
kagome_install(wavm_executor)
