#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(primitives
    block.hpp
    block.cpp
    block_header.hpp
    block_header.cpp
    digest.hpp
    extrinsic.hpp
    extrinsic.cpp
    version.hpp
    version.cpp
    inherent_data.cpp
    inherent_data.hpp
    transaction_validity.hpp
    block_id.hpp
    common.hpp
    parachain_host.hpp
    scheduled_change.hpp
    scheduled_change.cpp
    parachain_host.cpp transaction_validity.cpp)

target_link_libraries(primitives
    buffer
    outcome
    scale
    blob
    )

add_library(scale_codec
    scale_codec.hpp
    impl/scale_codec_impl.hpp
    impl/scale_codec_impl.cpp
    )
target_link_libraries(
    scale_codec
    scale_buffer_codec
    primitives
)
