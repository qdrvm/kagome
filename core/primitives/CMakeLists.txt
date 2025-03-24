#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(primitives
    arithmetic_error.cpp
    author_api_primitives.hpp
    block_header.cpp
    inherent_data.cpp
    token_error.cpp
    transaction_validity.cpp
)

target_link_libraries(primitives
    scale::scale
    blob
    hasher
    )
kagome_install(primitives)

add_library(ss58_codec
    ss58_codec.cpp
    )
target_link_libraries(ss58_codec
    outcome
    p2p::p2p_multibase_codec
    )
kagome_install(ss58_codec)
