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
    )
kagome_install(hasher)

add_library(sr25519_types
    sr25519_types.cpp
    sr25519_types.hpp
    )
target_link_libraries(sr25519_types
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

add_library(crypto_util
    hash_creator.cpp
    util.hpp
    )
target_link_libraries(crypto_util
    Boost::boost
    )

add_library(vrf_provider
    vrf_provider.hpp
    vrf/vrf_provider_impl.cpp
    vrf/vrf_provider_impl.hpp
    )
target_link_libraries(vrf_provider
    crypto_util
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

add_subdirectory(blake2)
add_subdirectory(sha)
add_subdirectory(twox)
