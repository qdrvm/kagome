#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(yamux
    yamux.cpp
    yamux.hpp
    yamuxed_connection.cpp
    yamuxed_connection.hpp
    yamux_config.hpp
    yamux_frame.cpp
    yamux_frame.hpp
    yamux_stream.cpp
    yamux_stream.hpp
    )
target_link_libraries(yamux
    buffer
    outcome
    logger
    )
