#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(testutil_read_writer_helper
    message_read_writer_helper.cpp
    message_read_writer_helper.hpp
    )
target_link_libraries(testutil_read_writer_helper
    p2p::p2p_uvarint
    )
