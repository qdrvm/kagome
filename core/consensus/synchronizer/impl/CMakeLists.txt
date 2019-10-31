#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(synchronizer
    synchronizer_impl.cpp
    synchronizer_impl.hpp
    )
target_link_libraries(synchronizer
    block_header_repository
    block_tree_error
    logger
    buffer
    p2p::p2p_peer_id
    scale_message_read_writer
    )
