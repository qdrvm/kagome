#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(key_store
    key_store_impl.cpp
    session_keys.cpp
    key_type.cpp
    key_file_storage.cpp
    )

target_link_libraries(key_store
    ecdsa_provider
    ed25519_provider
    sr25519_provider
    p2p::p2p_random_generator # generator from libp2p
    bip39_provider
    )
kagome_install(key_store)
