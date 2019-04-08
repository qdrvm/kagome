#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(yamux
    yamux/yamux.cpp
    yamux/yamux.hpp
    yamux/yamux_config.hpp
    yamux/yamux_frame.hpp
    yamux/yamux_stream.cpp
    yamux/yamux_stream.hpp
    )
target_link_libraries(
    buffer
    )
