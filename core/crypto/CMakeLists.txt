#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(crypto
        hasher.hpp
        hasher/hasher_impl.hpp
        hasher/hasher_impl.cpp
        )

add_subdirectory(blake2)
add_subdirectory(twox)
add_subdirectory(sha)

target_link_libraries(crypto
        blake2
        twox
        buffer
        sha
        )
