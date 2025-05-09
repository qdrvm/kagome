#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(kagome_crypto common.cpp)
target_link_libraries(kagome_crypto
    OpenSSL::Crypto
    logger
    )
kagome_install(kagome_crypto)

add_library(hasher
    hasher/hasher_impl.cpp
)
target_link_libraries(hasher
    blake2
    twox
    sha
    keccak
)
kagome_install(hasher)

add_library(sr25519_types
    sr25519_types.cpp
)
target_link_libraries(sr25519_types
    mp_utils
    blob
    schnorrkel::schnorrkel
    kagome_crypto
)
kagome_install(sr25519_types)

add_library(ed25519_types
    ed25519_types.cpp
    ed25519_types.hpp
)
target_link_libraries(ed25519_types
    blob
    scale::scale
    schnorrkel::schnorrkel
    kagome_crypto
)
kagome_install(ed25519_types)

add_library(vrf_provider
    vrf/vrf_provider_impl.cpp
)
target_link_libraries(vrf_provider
    mp_utils
    sr25519_types
    blob
    p2p::p2p_random_generator # generator from libp2p
    kagome_crypto
)
kagome_install(vrf_provider)

add_library(sr25519_provider
    sr25519/sr25519_provider_impl.cpp
)
target_link_libraries(sr25519_provider
    p2p::p2p_random_generator # generator from libp2p
    sr25519_types
)
kagome_install(sr25519_provider)

add_library(bandersnatch_provider
    bandersnatch/bandersnatch_provider_impl.cpp
)
target_link_libraries(bandersnatch_provider
    bandersnatch_vrfs::bandersnatch_vrfs
)
kagome_install(bandersnatch_provider)

add_library(bandersnatch_vrf
    bandersnatch/vrf.cpp
)
target_link_libraries(bandersnatch_vrf
    bandersnatch_vrfs::bandersnatch_vrfs
)
kagome_install(bandersnatch_vrf)

add_library(ecdsa_provider
    ecdsa/ecdsa_provider_impl.cpp
    ecdsa_types.cpp
)
target_link_libraries(ecdsa_provider
    hasher
    logger
    secp256k1_provider
    kagome_crypto
)
kagome_install(ecdsa_provider)

add_library(ed25519_provider
    ed25519/ed25519_provider_impl.cpp
)
target_link_libraries(ed25519_provider
    ed25519_types
    hasher
    logger
    schnorrkel::schnorrkel
)
kagome_install(ed25519_provider)

add_library(secp256k1_provider
    secp256k1/secp256k1_provider_impl.cpp
)
target_link_libraries(secp256k1_provider
    PUBLIC
    OpenSSL::Crypto
    OpenSSL::SSL
    blob
    libsecp256k1::secp256k1
    scale::scale
)
kagome_install(
    secp256k1_provider
)

add_library(pbkdf2_provider
    pbkdf2/impl/pbkdf2_provider_impl.cpp
)
target_link_libraries(pbkdf2_provider
    PUBLIC
    OpenSSL::SSL
    OpenSSL::Crypto
    blob
)
kagome_install(pbkdf2_provider)

add_library(elliptic_curves
    elliptic_curves/elliptic_curves_impl.cpp
)
target_link_libraries(elliptic_curves
    logger
    arkworks::arkworks
)
kagome_install(elliptic_curves)

add_subdirectory(bip39)
add_subdirectory(blake2)
add_subdirectory(key_store)
add_subdirectory(sha)
add_subdirectory(twox)
