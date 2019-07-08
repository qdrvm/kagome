#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(message_read_writer_error
    message_read_writer_error.cpp
    message_read_writer_error.hpp
    )
target_link_libraries(message_read_writer_error
    outcome
    )

add_library(message_read_writer
    message_read_writer.hpp
    message_read_writer.cpp
    )
target_link_libraries(message_read_writer
    uvarint
    )
