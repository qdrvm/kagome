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

add_library(crypto_types
    crypto_types.cpp
    crypto_types.hpp
    )
target_link_libraries(crypto_types
    blob
    sr25519
    )

add_library(crypto_util
    util.cpp
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
    crypto_types
    buffer
    blob
    random_generator # generator from libp2p
    )

add_subdirectory(blake2)
add_subdirectory(twox)
add_subdirectory(sha)
