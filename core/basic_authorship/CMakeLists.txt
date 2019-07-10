#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(block_builder
    block_builder.hpp
    impl/block_builder_impl.cpp
    impl/block_builder_impl.hpp
    )

target_link_libraries(block_builder
    block_builder_api
    )
