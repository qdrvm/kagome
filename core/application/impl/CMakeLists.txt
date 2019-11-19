add_library(config_reader_error
    config_reader/error.cpp
    config_reader/error.hpp
    )
target_link_libraries(config_reader_error
    outcome
    )

add_library(pt_util
    config_reader/pt_util.hpp
    config_reader/pt_util.cpp
    )
target_link_libraries(pt_util
    Boost::boost
    config_reader_error
    p2p::p2p_peer_id
    )

add_library(json_configuration_reader
    config_reader/json_configuration_reader.hpp
    config_reader/json_configuration_reader.cpp
    )
target_link_libraries(json_configuration_reader
    pt_util
    config_reader_error
    p2p::p2p_multiaddress
    )

add_library(key_storage_error
    key_storage_error.cpp
    key_storage_error.hpp
    )
target_link_libraries(key_storage_error
    outcome
    )

add_library(crypto_key_reader
    crypto_key_reader/crypto_key_reader_util.cpp
    crypto_key_reader/crypto_key_reader_util.hpp
    )
target_link_libraries(crypto_key_reader
    OpenSSL::Crypto
    key_storage_error
    )
