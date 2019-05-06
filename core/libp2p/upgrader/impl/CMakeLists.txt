#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(connection_upgrader
    connection_upgrader_impl.hpp
    connection_upgrader_impl.cpp
    )
target_link_libraries(connection_upgrader
    yamux
    outcome
    logger
    )
