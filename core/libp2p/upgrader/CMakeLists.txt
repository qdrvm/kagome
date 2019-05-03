#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(connection_upgrader
    connection_upgrader.hpp
    connection_upgrader.cpp
    )
target_link_libraries(yamux
    buffer
    outcome
    logger
    )
