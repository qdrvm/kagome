
add_library(compartment_wrapper compartment_wrapper.cpp)
target_link_libraries(compartment_wrapper
    WAVM::libWAVM)
kagome_install(compartment_wrapper)

llvm_map_components_to_libnames(LLVM_LIBS
	  core
    )

add_library(wavm_memory memory_impl.cpp)
target_link_libraries(wavm_memory
    ${LLVM_LIBS}
    WAVM::libWAVM
    memory_allocator
    )
kagome_install(wavm_memory)

add_library(runtime_wavm
    core_api_factory_impl.cpp
    intrinsics/intrinsic_functions.cpp
    intrinsics/intrinsic_module.cpp
    intrinsics/intrinsic_module_instance.cpp
    intrinsics/intrinsic_resolver_impl.cpp
    instance_environment_factory.cpp
    module.cpp
    module_cache.cpp
    module_params.cpp
    module_factory_impl.cpp
    module_instance.cpp
    wavm_external_memory_provider.cpp
    wavm_internal_memory_provider.cpp
    )
target_link_libraries(runtime_wavm
    wavm_memory
    constant_code_provider
    Boost::boost
    compartment_wrapper
    trie_storage_provider
    )
kagome_install(runtime_wavm)
