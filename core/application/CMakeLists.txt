add_subdirectory(impl)

add_library(configuration_storage
    configuration_storage.hpp
    impl/configuration_storage_impl.hpp
    impl/configuration_storage_impl.cpp
    )
target_link_libraries(configuration_storage
    primitives
    )

add_library(key_storage
    key_storage.hpp
    impl/local_key_storage.cpp
    impl/local_key_storage.hpp
    )
target_link_libraries(key_storage
    primitives
    crypto_key_reader
    )
