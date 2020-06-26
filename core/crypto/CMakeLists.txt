#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(hasher
    hasher.hpp
    hasher/hasher_impl.hpp
    hasher/hasher_impl.cpp
    )
target_link_libraries(hasher
    blake2
    twox
    buffer
    sha
    keccak
    )
kagome_install(hasher)

add_library(sr25519_types
    sr25519_types.cpp
    sr25519_types.hpp
    )
target_link_libraries(sr25519_types
    mp_utils
    blob
    sr25519::sr25519
    )
kagome_install(sr25519_types)

add_library(ed25519_types
    ed25519_types.cpp
    ed25519_types.hpp
    )
target_link_libraries(ed25519_types
    blob
    iroha::ed25519
    )
kagome_install(ed25519_types)

add_library(vrf_provider
    vrf_provider.hpp
    vrf/vrf_provider_impl.cpp
    vrf/vrf_provider_impl.hpp
    )
target_link_libraries(vrf_provider
    mp_utils
    sr25519_types
    buffer
    blob
    p2p::p2p_random_generator # generator from libp2p
    )

add_library(sr25519_provider
    sr25519_provider.hpp
    sr25519/sr25519_provider_impl.hpp
    sr25519/sr25519_provider_impl.cpp
    )
target_link_libraries(sr25519_provider
    p2p::p2p_random_generator # generator from libp2p
    sr25519_types
    )
kagome_install(sr25519_provider)

add_library(ed25519_provider
    ed25519_provider.hpp
    ed25519/ed25519_provider_impl.hpp
    ed25519/ed25519_provider_impl.cpp
    )
target_link_libraries(ed25519_provider
    p2p::p2p_random_generator # generator from libp2p
    ed25519_types
    )
kagome_install(ed25519_provider)

add_library(secp256k1_provider
    secp256k1/secp256k1_provider_impl.cpp
    )
target_link_libraries(secp256k1_provider PUBLIC
    OpenSSL::Crypto
    PUBLIC OpenSSL::SSL
    blob
    buffer
    secp256k1::secp256k1
    scale
    )
kagome_install(
    secp256k1_provider
)

add_library(pbkdf2_provider
    pbkdf2/impl/pbkdf2_provider_impl.cpp
    )
target_link_libraries(pbkdf2_provider
    PUBLIC OpenSSL::SSL
    OpenSSL::Crypto
    buffer
    blob
    )
kagome_install(pbkdf2_provider)

add_subdirectory(bip39)
add_subdirectory(blake2)
add_subdirectory(keccak)
add_subdirectory(sha)
add_subdirectory(twox)
