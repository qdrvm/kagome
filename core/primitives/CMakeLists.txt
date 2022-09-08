#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(primitives
    arithmetic_error.hpp
    arithmetic_error.cpp
    author_api_primitives.hpp
    block.hpp
    block_header.cpp
    block_header.hpp
    block_id.hpp
    common.hpp
    check_inherents_result.hpp
    digest.hpp
    extrinsic.hpp
    inherent_data.cpp
    inherent_data.hpp
    parachain_host.hpp
    scheduled_change.hpp
    token_error.hpp
    token_error.cpp
    transaction_validity.hpp
    transaction_validity.cpp
    version.hpp
    version.cpp
    session_key.hpp)

target_link_libraries(primitives
    buffer
    scale::scale
    blob
    hasher
    )
kagome_install(primitives)

add_library(ss58_codec
    ss58_codec.cpp
    )
target_link_libraries(ss58_codec
    buffer
    outcome
    p2p::p2p_multibase_codec
    )
kagome_install(ss58_codec)
