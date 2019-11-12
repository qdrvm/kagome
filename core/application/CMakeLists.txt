add_subdirectory(impl)

add_library(configuration_storage
    configuration_storage.hpp
    impl/configuration_storage_impl.hpp
    impl/configuration_storage_impl.cpp
    )
target_link_libraries(configuration_storage
    primitives
    )
