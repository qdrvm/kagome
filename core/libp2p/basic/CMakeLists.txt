#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(message_read_writer
    message_read_writer.cpp
    message_read_writer.hpp
    )
target_link_libraries(message_read_writer
    uvarint
    outcome
    )
