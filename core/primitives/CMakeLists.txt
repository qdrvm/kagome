#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(primitives
    author_api_primitives.hpp
    block.hpp
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
    transaction_validity.hpp
    transaction_validity.cpp
    version.hpp
    session_key.hpp)

target_link_libraries(primitives
    buffer
    scale
    blob
    )
kagome_install(primitives)

add_library(base58_codec
    base58_codec.cpp
    )
target_link_libraries(base58_codec
    buffer
    outcome
    )
kagome_install(base58_codec)

add_library(ss58_codec
    ss58_codec.cpp
    )
target_link_libraries(ss58_codec
    buffer
    outcome
    base58_codec
    )
kagome_install(ss58_codec)
