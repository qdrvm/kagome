#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(libp2p_yamux
    yamux.cpp
    yamux.hpp
    )
target_link_libraries(libp2p_yamux
    libp2p_yamuxed_connection
    )
