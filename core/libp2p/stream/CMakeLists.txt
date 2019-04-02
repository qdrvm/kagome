#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(yamux_stream
    yamux_stream.cpp
    yamux_stream.hpp
    )
target_link_libraries(yamux_stream
    yamux
    )
