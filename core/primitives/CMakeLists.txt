#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(primitives
    block.hpp
    block.cpp
    block_header.hpp
    block_header.cpp
    extrinsic.hpp
    extrinsic.cpp
    version.hpp
    version.cpp
    transaction_validity.hpp
    block_id.hpp
    common.hpp
    )
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
    primitives
)
