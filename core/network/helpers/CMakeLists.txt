#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(scale_message_read_writer
    scale_message_read_writer.hpp
    scale_message_read_writer.cpp
    )
target_link_libraries(scale_message_read_writer
    p2p::p2p_message_read_writer
    scale::scale
    )
