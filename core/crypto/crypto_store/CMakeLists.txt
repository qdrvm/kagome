#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(crypto_store_key_type
    key_type.cpp
    )
target_link_libraries(crypto_store_key_type
    Boost::boost
    )
kagome_install(crypto_store_key_type)

add_library(key_file_storage
    key_file_storage.cpp
    )
target_link_libraries(key_file_storage
    Boost::filesystem
    crypto_store_key_type
    )
kagome_install(key_file_storage)

add_library(crypto_store
    crypto_store_impl.cpp
    crypto_suites.hpp
    key_cache.hpp
    session_keys.cpp
    session_keys.hpp)

target_link_libraries(crypto_store
    key_file_storage
    ecdsa_provider
    ed25519_provider
    sr25519_provider
    p2p::p2p_random_generator # generator from libp2p
    bip39_provider
    crypto_store_key_type
    )
kagome_install(crypto_store)
